#ifndef VAST_IO_SERIALIZATION_H
#define VAST_IO_SERIALIZATION_H

#include <memory>
#include "vast/config.h"
#include "vast/intrusive.h"
#include "vast/traits.h"
#include "vast/type_info.h"
#include "vast/io/coded_stream.h"

namespace vast {

namespace io {

/// Interface for serialization of objects.
class serializer
{
public:
  virtual ~serializer() = default;

  /// Checks whether the serializer writes out type information.
  /// The default implementation uses an typed serializer.
  /// @return `true` if the serializer is typed.
  virtual bool typed() const;

  /// Begins reading an object of a given type.
  /// The default implementation serializes the unique type ID.
  /// @param ti The type information descripting the object instance.
  /// @return `true` on success.
  virtual bool begin_object(stable_type_info const& ti);

  /// Finishes reading an object.
  /// The default implementation does nothing.
  /// @return `true` on success.
  virtual bool end_object();

  /// Begins writing a sequence.
  /// @param size The size of the sequence.
  /// @return `true` on success.
  virtual bool begin_sequence(uint64_t size) = 0;

  /// Finishes writing a sequence.
  /// The default implementation does nothing.
  /// @return `true` on success.
  virtual bool end_sequence();

  /// Writes a value.
  /// @param x The value to write.
  /// @return `true` on success.
  virtual bool write_bool(bool x) = 0;
  virtual bool write_int8(int8_t x) = 0;
  virtual bool write_uint8(uint8_t x) = 0;
  virtual bool write_int16(int16_t x) = 0;
  virtual bool write_uint16(uint16_t x) = 0;
  virtual bool write_int32(int32_t x) = 0;
  virtual bool write_uint32(uint32_t x) = 0;
  virtual bool write_int64(int64_t x) = 0;
  virtual bool write_uint64(uint64_t x) = 0;
  virtual bool write_double(double x) = 0;

  /// Writes raw bytes.
  /// @param data The data to write.
  /// @param size The number of bytes to write.
  /// @return `true` on success.
  virtual bool write_raw(void const* data, size_t size) = 0;

protected:
  serializer() = default;
};

/// Interface for deserialization of objects.
class deserializer
{
public:
  virtual ~deserializer() = default;

  /// Checks whether the deserializer reads in type information.
  /// The default implementation uses an typed deserializer.
  /// @return `true` if the deserializer is typed.
  virtual bool typed() const;

  /// Begins writing an object of a given type.
  /// The default implementation reads the unique type ID and returns the
  /// corresponding type info object..
  /// @param ti The type information descripting the object instance.
  /// @return An engaged option on success.
  virtual stable_type_info const* begin_object();

  /// Finishes writing an object.
  /// The default implementation does nothing.
  /// @return `true` on success.
  virtual bool end_object();

  /// Begins reading a sequence.
  /// @param size The size of the sequence.
  /// @return `true` on success.
  virtual bool begin_sequence(uint64_t& size) = 0;

  /// Finishes reading a sequence.
  /// The default implementation does nothing.
  /// @return `true` on success.
  virtual bool end_sequence();

  /// Reads a value.
  /// @param x The value to read into.
  /// @return `true` on success.
  virtual bool read_bool(bool& x) = 0;
  virtual bool read_int8(int8_t& x) = 0;
  virtual bool read_uint8(uint8_t& x) = 0;
  virtual bool read_int16(int16_t& x) = 0;
  virtual bool read_uint16(uint16_t& x) = 0;
  virtual bool read_int32(int32_t& x) = 0;
  virtual bool read_uint32(uint32_t& x) = 0;
  virtual bool read_int64(int64_t& x) = 0;
  virtual bool read_uint64(uint64_t& x) = 0;
  virtual bool read_double(double& x) = 0;

  /// Reads raw bytes.
  /// @param data The location to read into.
  /// @param size The number of bytes to read.
  /// @return `true` on success.
  virtual bool read_raw(void* data, size_t size) = 0;

protected:
  deserializer() = default;
};

/// Serializes binary objects into an input stream.
class binary_serializer : public serializer
{
public:
  /// Constructs a deserializer with an output stream.
  /// @param source The output stream to write into.
  binary_serializer(output_stream& sink);

  virtual bool typed() const override;
  virtual bool begin_object(stable_type_info const& ti) override;
  virtual bool begin_sequence(uint64_t size) override;
  virtual bool write_bool(bool x) override;
  virtual bool write_int8(int8_t x) override;
  virtual bool write_uint8(uint8_t x) override;
  virtual bool write_int16(int16_t x) override;
  virtual bool write_uint16(uint16_t x) override;
  virtual bool write_int32(int32_t x) override;
  virtual bool write_uint32(uint32_t x) override;
  virtual bool write_int64(int64_t x) override;
  virtual bool write_uint64(uint64_t x) override;
  virtual bool write_double(double x) override;
  virtual bool write_raw(void const* data, size_t size) override;
  virtual size_t bytes() const;

private:
  coded_output_stream sink_;
  size_t bytes_ = 0;
};

/// Deserializes objects from an input stream.
class binary_deserializer : public deserializer
{
public:
  /// Constructs a deserializer with an input stream.
  /// @param source The input stream to read from.
  binary_deserializer(input_stream& source);

  virtual bool typed() const override;
  virtual stable_type_info const* begin_object() override;
  virtual bool begin_sequence(uint64_t& size) override;
  virtual bool read_bool(bool& x) override;
  virtual bool read_int8(int8_t& x) override;
  virtual bool read_uint8(uint8_t& x) override;
  virtual bool read_int16(int16_t& x) override;
  virtual bool read_uint16(uint16_t& x) override;
  virtual bool read_int32(int32_t& x) override;
  virtual bool read_uint32(uint32_t& x) override;
  virtual bool read_int64(int64_t& x) override;
  virtual bool read_uint64(uint64_t& x) override;
  virtual bool read_double(double& x) override;
  virtual bool read_raw(void* data, size_t size) override;
  virtual size_t bytes() const;

private:
  coded_input_stream source_;
  size_t bytes_ = 0;
};


/// Provides clean access of private class internals to the serialization
/// framework.
struct access
{
  template <typename T>
  static inline auto save(serializer& sink, T x, int)
    -> decltype(x.serialize(sink), void())
  {
    x.serialize(sink);
  }

  template <typename T>
  static inline auto save(serializer& sink, T x, long)
    -> decltype(serialize(sink, x), void())
  {
    serialize(sink, x);
  }

  template <typename T>
  static inline auto load(deserializer& source, T& x, int)
    -> decltype(x.deserialize(source), void())
  {
    x.deserialize(source);
  }

  template <typename T>
  static inline auto load(deserializer& source, T& x, long)
    -> decltype(deserialize(source, x), void())
  {
    deserialize(source, x);
  }
};

/// Serializes an instance.
/// @tparam T the type of the instance to serialize.
/// @param sink The serializer to write a `T` into.
/// @param x An instance of type `T`.
template <typename T>
void save(serializer& sink, T const& x)
{
  access::save(sink, x, 0);
}

/// Deserializes an instance.
/// @tparam T the type of the instance to serialize.
/// @param source The deserializer to extract a `T` from.
/// @param x An instance of type `T`.
template <typename T>
void load(deserializer& source, T& x)
{
  access::load(source, x, 0);
}

/// Serializes an object.
/// @tparam T the type of the instance to serialize.
/// @param sink The serializer to write a `T` into.
/// @param x An instance of type `T`.
/// @return A reference to *sink*.
template <typename T>
serializer& operator<<(serializer& sink, T const& x)
{
  auto typed = sink.typed();
  if (typed)
  {
    auto ti = stable_typeid<T>();
    if (! ti)
      throw std::logic_error("lacking type info for deserialization");
    sink.begin_object(*ti);
  }
  save(sink, x);
  if (typed)
    sink.end_object();
  return sink;
}

/// Deserializes an object.
/// @tparam T the type of the instance to serialize.
/// @param source The deserializer to extract a `T` from.
/// @param x An instance of type `T`.
/// @return A reference to *source*.
template <typename T>
deserializer& operator>>(deserializer& source, T& x)
{
  auto typed = source.typed();
  if (typed)
  {
    auto ti = source.begin_object();
    if (! (ti && ti->equals(typeid(T))))
      throw std::logic_error("type mismatch during deserialization");
  }
  load(source, x);
  if (typed)
    source.end_object();
  return source;
}

} // namespace io
} // namespace vast

#include "vast/io/serialization/arithmetic.h"
#include "vast/io/serialization/container.h"
#include "vast/io/serialization/pointer.h"
#include "vast/io/serialization/string.h"
#include "vast/io/serialization/time.h"

#endif
