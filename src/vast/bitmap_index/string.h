#ifndef VAST_BITMAP_INDEX_STRING_H
#define VAST_BITMAP_INDEX_STRING_H

#include "vast/bitmap.h"
#include "vast/bitmap_index.h"
#include "vast/util/dictionary.h"

namespace vast {

/// A bitmap index for strings. It uses a @link dictionary
/// vast::util::dictionary@endlink to map each string to a unique numeric value
/// to be used by the bitmap.
class string_bitmap_index : public bitmap_index
{
  using bitstream_type = null_bitstream; // TODO: Use compressed bitstream.
  using dictionary_codomain = uint64_t;

public:
  virtual bool patch(size_t n) override;

  virtual option<bitstream>
  lookup(relational_operator op, value const& val) const override;

  virtual std::string to_string() const override;

private:
  virtual bool push_back_impl(value const& val) override;

  bitmap<dictionary_codomain, bitstream_type> bitmap_;
  util::map_dictionary<dictionary_codomain> dictionary_;
};

} // namespace vast

#endif