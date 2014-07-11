#include "vast/importer.h"

#include "vast/segment.h"
#include "vast/source/bro.h"
#include "vast/io/serialization.h"

#ifdef VAST_HAVE_BROCCOLI
#include "vast/source/broccoli.h"
#endif

namespace vast {

using namespace cppa;

importer::importer(path dir,
                   actor receiver,
                   size_t max_events_per_chunk,
                   size_t max_segment_size,
                   uint64_t batch_size)
  : dir_{dir / "ingest" / "segments"},
    receiver_{receiver},
    max_events_per_chunk_{max_events_per_chunk},
    max_segment_size_{max_segment_size},
    batch_size_{batch_size}
{
}

partial_function importer::act()
{
  trap_exit(true);

  segmentizer_ = spawn<segmentizer, monitored>(
      this, max_events_per_chunk_, max_segment_size_);

  traverse(
      dir_,
      [&](path const& p) -> bool
      {
        VAST_LOG_ACTOR_INFO("found orphaned segment: " << p.basename());
        orphaned_.insert(p.basename());
        return true;
      });

  attach_functor(
      [=](uint32_t)
      {
        receiver_ = invalid_actor;
        source_ = invalid_actor;
        segmentizer_ = invalid_actor;
      });

  auto on_exit = [=](exit_msg const& e)
  {
    if (source_)
      // Tell the source to exit, it will in turn propagate the exit
      // message to the sink.
      send_exit(source_, exit::stop);
    else
      // If we have no source, we just tell the segmentizer to exit.
      send_exit(segmentizer_, e.reason);
  };

  auto on_ack = [=](uuid const& id)
  {
    VAST_LOG_ACTOR_DEBUG("got ack for segment " << id);

    auto i = orphaned_.find(path{to_string(id)});
    if (i != orphaned_.end())
    {
      VAST_LOG_ACTOR_INFO("submitted orphaned segment " << id);
      rm(dir_ / *i);
      orphaned_.erase(i);
    }
  };

  ready_ =
  {
    on_exit,
    [=](down_msg const&)
    {
      segmentizer_ = invalid_actor;
    },
    on(atom("ack"), arg_match) >> on_ack,
    on(atom("backlog"), arg_match) >> [=](bool backlogged)
    {
      if (backlogged)
      {
        VAST_LOG_ACTOR_DEBUG("pauses segment sending");
        become(paused_);
      }
    },
    [=](segment& s)
    {
      VAST_LOG_ACTOR_DEBUG("delivers segment " << s.id());
      send(receiver_, std::move(s), this);
    },
    after(std::chrono::seconds(0)) >> [=]
    {
      if (! segmentizer_)
        become(terminating_);
    }
  };

  paused_ =
  {
    on_exit,
    on(atom("ack"), arg_match) >> on_ack,
    on(atom("backlog"), arg_match) >> [=](bool backlogged)
    {
      if (! backlogged)
      {
        VAST_LOG_ACTOR_DEBUG("resumes segment sending");
        become(ready_);
      }
    },
  };

  terminating_ =
  {
    [=](segment const& s)
    {
      if (! exists(dir_) && ! mkdir(dir_))
      {
        VAST_LOG_ACTOR_ERROR("failed to create directory " << dir_);
        return;
      }

      auto p = dir_ / path{to_string(s.id())};
      VAST_LOG_ACTOR_INFO("archives segment to " << p);

      auto t = io::archive(p, s);
      if (! t)
        VAST_LOG_ACTOR_ERROR("failed to archive " << p << ": " << t.error());
    },
    after(std::chrono::seconds(0)) >> [=]
    {
      quit(exit::done);
    }
  };

  return
  {
    on_exit,
    [=](down_msg const& d)
    {
      quit(d.reason);
    },
    on(atom("submit")) >> [=]
    {
      for (auto& base : orphaned_)
      {
        auto p = dir_ / base;
        segment s;
        if (! io::unarchive(p, s))
        {
          VAST_LOG_ACTOR_ERROR("failed to load orphaned segment " << base);
          continue;
        }

        // FIXME: enqueue segments in the order they have been received.
        send(this, std::move(s));
      }

      become(ready_);
    },
    on(atom("add"), "bro", arg_match) >> [=](std::string const& file)
    {
      VAST_LOG_ACTOR_INFO("ingests " << file);

      source_ = spawn<source::bro, detached>(segmentizer_, file, -1);
      source_->link_to(segmentizer_);
      send(source_, atom("batch size"), batch_size_);
      send(source_, atom("run"));

      become(ready_);
    },
    on(atom("add"), any_vals) >> [=]
    {
      VAST_LOG_ACTOR_ERROR("got invalid import file type");
      quit(exit::error);
    },
  };
}

std::string importer::describe() const
{
  return "importer";
}

} // namespace vast