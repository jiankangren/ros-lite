#pragma once

#if ROSLITE_TARGET_CLUSTER_ID == 0
  #include "roslite/include/ros/common.h"
  #include "roslite/include/ros/time.h"
  #include "roslite/include/ros/duration.h"
#else
  #include "ros/common.h"
  #include "ros/time.h"
  #include "ros/duration.h"
#endif

#include <cstring>
#include <string>
#include <memory>
#include <vector>

/**
 * \brief Declare your serializer to use an allInOne member instead of requiring 3 different serialization
 * functions.
 *
 * The allinone method has the form:
\verbatim
template<typename Stream, typename T>
inline static void allInOne(Stream& stream, T t)
{
  stream.next(t.a);
  stream.next(t.b);
  ...
}
\endverbatim
 *
 * The only guarantee given is that Stream::next(T) is defined.
 */
#define ROS_DECLARE_ALLINONE_SERIALIZER \
  template<typename Stream, typename T> \
  inline static void write(Stream& stream, const T& t) \
  { \
    allInOne<Stream, const T&>(stream, t); \
  } \
  \
  template<typename Stream, typename T> \
  inline static void read(Stream& stream, T& t) \
  { \
    allInOne<Stream, T&>(stream, t); \
  } \
  \
  template<typename T> \
  inline static uint32_t serializedLength(const T& t) \
  { \
    LStream stream; \
    allInOne<LStream, const T&>(stream, t); \
    return stream.getLength(); \
  }

namespace ROSLITE_NAMESPACE
{

class SerializedMessage
{
public:
  uint8_t* buf;
  size_t num_bytes;
  uint8_t* message_start;

  void* message;
  const std::type_info* type_info;

  SerializedMessage()
  : buf(0)
  , num_bytes(0)
  , message_start(0)
  , type_info(0)
  {}

//   SerializedMessage(boost::shared_array<uint8_t> buf, size_t num_bytes)
//   : buf(buf)
//   , num_bytes(num_bytes)
//   , message_start(buf ? buf.get() : 0)
//   , type_info(0)
//   { }
};

namespace serialization
{
    
/**
 * \brief Templated serialization class.  Default implementation provides backwards compatibility with
 * old message types.
 *
 * Specializing the Serializer class is the only thing you need to do to get the ROS serialization system
 * to work with a type.
 */
template<typename T>
struct Serializer //  Maruyama: Avoid Boost, Need defaut Serializer
{
//   /**
//    * \brief Write an object to the stream.  Normally the stream passed in here will be a ros::serialization::OStream
//    */
//   template<typename Stream>
//   inline static void write(Stream& stream, typename boost::call_traits<T>::param_type t)
//   {
//     t.serialize(stream.getData(), 0);
//   }

//   /**
//    * \brief Read an object from the stream.  Normally the stream passed in here will be a ros::serialization::IStream
//    */
//   template<typename Stream>
//   inline static void read(Stream& stream, typename boost::call_traits<T>::reference t)
//   {
//     t.deserialize(stream.getData());
//   }

//   /**
//    * \brief Determine the serialized length of an object.
//    */
//   inline static uint32_t serializedLength(typename boost::call_traits<T>::param_type t)
//   {
//     return t.serializationLength();
//   }
};

/**
 * \brief Serialize an object.  Stream here should normally be a ros::serialization::OStream
 */
template<typename T, typename Stream>
inline void serialize(Stream& stream, const T& t)
{
  Serializer<T>::write(stream, t);
}
    
/**
 * \brief Deserialize an object.  Stream here should normally be a ros::serialization::IStream
 */
template<typename T, typename Stream>
inline void deserialize(Stream& stream, T& t)
{
  Serializer<T>::read(stream, t);
}

/**
 * \brief Determine the serialized length of an object
 */
template<typename T>
inline uint32_t serializationLength(const T& t)
{
  return Serializer<T>::serializedLength(t);
}

#define ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(Type) \
  template<> struct Serializer<Type> \
  { \
    template<typename Stream> inline static void write(Stream& stream, const Type v) \
    { \
      memcpy(stream.advance(sizeof(v)), &v, sizeof(v) ); \
    } \
    \
    template<typename Stream> inline static void read(Stream& stream, Type& v) \
    { \
      memcpy(&v, stream.advance(sizeof(v)), sizeof(v) ); \
    } \
    \
    inline static uint32_t serializedLength(const Type t) \
      { \
      return sizeof(Type); \
    } \
};

ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(uint8_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(int8_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(uint16_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(int16_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(uint32_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(int32_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(uint64_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(int64_t);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(float);
ROS_CREATE_SIMPLE_SERIALIZER_ARM_ROSLITE(double);

/**
 * \brief Serializer specialized for bool (serialized as uint8)
 */
template<> struct Serializer<bool>
{
  template<typename Stream> inline static void write(Stream& stream, const bool v)
  {
    uint8_t b = (uint8_t)v;
#if defined(__arm__) || defined(__arm)
    memcpy(stream.advance(sizeof(1)), &b, 1 );
#else
    *reinterpret_cast<uint8_t*>(stream.advance(1)) = b;
#endif
  }

  template<typename Stream> inline static void read(Stream& stream, bool& v)
  {
    uint8_t b;
#if defined(__arm__) || defined(__arm)
    memcpy(&b, stream.advance(sizeof(1)), 1 );
#else
    b = *reinterpret_cast<uint8_t*>(stream.advance(1));
#endif
    v = (bool)b;
  }

  inline static uint32_t serializedLength(bool)
  {
    return 1;
  }
};

/**
 * \brief  Serializer specialized for std::string
 */
template<class ContainerAllocator>
struct Serializer< std::basic_string<char, std::char_traits<char>, ContainerAllocator> >
{
  typedef std::basic_string<char, std::char_traits<char>, ContainerAllocator> StringType;

  template<typename Stream>
  inline static void write(Stream& stream, const StringType& str)
  {
    const char *cp = (const char *)str.c_str();
	int length = strlen(cp);
	size_t len = length;
  //    size_t len = str.size();
    stream.next((uint32_t)len);
    if (len > 0)
    {
      memcpy(stream.advance((uint32_t)len), str.data(), len);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, StringType& str)
  {
    uint32_t len;
    stream.next(len);
    if (len > 0)
    {
      str = StringType((char*)stream.advance(len), len);
    }
    else
    {
      str.clear();
    }
  }

  inline static uint32_t serializedLength(const StringType& str)
  {
    return 4 + (uint32_t)str.size();
  }
};

/**
 * \brief Serializer specialized for ros::Time
 */
template<>
struct Serializer<ROSLITE_NAMESPACE::Time>
{
  template<typename Stream>
  inline static void write(Stream& stream, const ROSLITE_NAMESPACE::Time& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  template<typename Stream>
  inline static void read(Stream& stream, ROSLITE_NAMESPACE::Time& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  inline static uint32_t serializedLength(const ROSLITE_NAMESPACE::Time&)
  {
    return 8;
  }
};

/**
 * \brief Serializer specialized for ros::Duration
 */
template<>
struct Serializer<ROSLITE_NAMESPACE::Duration>
{
  template<typename Stream>
  inline static void write(Stream& stream, const ROSLITE_NAMESPACE::Duration& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  template<typename Stream>
  inline static void read(Stream& stream, ROSLITE_NAMESPACE::Duration& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  inline static uint32_t serializedLength(const ROSLITE_NAMESPACE::Duration&)
  {
    return 8;
  }
};

// /**
//  * \brief Vector serializer.  Default implementation does nothing
//  */
// // template<typename T, class ContainerAllocator, class Enabled = void>
// template<typename T, class ContainerAllocator>
// struct VectorSerializer
// {};

/**
 * \brief Vector serializer, specialized for non-fixed-size, non-simple types
 */
template<typename T, class ContainerAllocator>
// struct VectorSerializer<T, ContainerAllocator, typename boost::disable_if<mt::IsFixedSize<T> >::type >
struct VectorSerializer
{
    typedef std::vector<T, typename ContainerAllocator::template rebind<T>::other> VecType;
    typedef typename VecType::iterator IteratorType;
    typedef typename VecType::const_iterator ConstIteratorType;

    template<typename Stream>
    inline static void write(Stream& stream, const VecType& v)
    {
      stream.next((uint32_t)v.size());
      ConstIteratorType it = v.begin();
      ConstIteratorType end = v.end();
      for (; it != end; ++it)
      {
        stream.next(*it);
      }
    }

    template<typename Stream>
    inline static void read(Stream& stream, VecType& v)
    {
      uint32_t len;
      stream.next(len);
      v.resize(len);
      IteratorType it = v.begin();
      IteratorType end = v.end();
      for (; it != end; ++it)
      {
        stream.next(*it);
      }
    }

    inline static uint32_t serializedLength(const VecType& v)
    {
      uint32_t size = 4;
      ConstIteratorType it = v.begin();
      ConstIteratorType end = v.end();
      for (; it != end; ++it)
      {
        size += serializationLength(*it);
      }

      return size;
    }
};

/**
 * \brief serialize version for std::vector
 */
template<typename T, class ContainerAllocator, typename Stream>
inline void serialize(Stream& stream, const std::vector<T, ContainerAllocator>& t)
{
  VectorSerializer<T, ContainerAllocator>::write(stream, t);
}

/**
 * \brief deserialize version for std::vector
 */
template<typename T, class ContainerAllocator, typename Stream>
inline void deserialize(Stream& stream, std::vector<T, ContainerAllocator>& t)
{
  VectorSerializer<T, ContainerAllocator>::read(stream, t);
}

/**
 * \brief serializationLength version for std::vector
 */
template<typename T, class ContainerAllocator>
inline uint32_t serializationLength(const std::vector<T, ContainerAllocator>& t)
{
  return VectorSerializer<T, ContainerAllocator>::serializedLength(t);
}

/**
 * \brief Enum
 */
namespace stream_types
{
enum StreamType
{
  Input,
  Output,
  Length
};
}
typedef stream_types::StreamType StreamType;

/**
 * \brief Stream base-class, provides common functionality for IStream and OStream
 */
struct Stream
{
  /*
   * \brief Returns a pointer to the current position of the stream
   */
  inline uint8_t* getData() { return data_; }
  /**
   * \brief Advances the stream, checking bounds, and returns a pointer to the position before it
   * was advanced.
   * \throws StreamOverrunException if len would take this stream past the end of its buffer
   */
  inline uint8_t* advance(uint32_t len)
  {
    uint8_t* old_data = data_;
    data_ += len;
    // if (data_ > end_)
    // {
    //   // Throwing directly here causes a significant speed hit due to the extra code generated
    //   // for the throw statement
    //   throwStreamOverrun();
    // }
    return old_data;
  }

  /**
   * \brief Returns the amount of space left in the stream
   */
  inline uint32_t getLength() { return (uint32_t)(end_ - data_); }

protected:
  Stream(uint8_t* _data, uint32_t _count)
  : data_(_data)
  , end_(_data + _count)
    {}

private:
  uint8_t* data_;
  uint8_t* end_;
};


/**
 * \brief Output stream
 */
struct OStream : public Stream
{
  static const StreamType stream_type = stream_types::Output;

  OStream(uint8_t* data, uint32_t count)
  : Stream(data, count)
  {}

  /**
   * \brief Serialize an item to this output stream
   */
  template<typename T>
  inline void next(const T& t)
  {
    serialize(*this, t);
  }

  template<typename T>
  inline OStream& operator<<(const T& t)
  {
    serialize(*this, t);
    return *this;
  }
};

/**
 * \brief Input stream
 */
struct IStream : public Stream
{
  static const StreamType stream_type = stream_types::Input;

  IStream(uint8_t* data, uint32_t count)
  : Stream(data, count)
  {}

  /**
   * \brief Deserialize an item from this input stream
   */
  template<typename T>
  inline void next(T& t)
  {
    deserialize(*this, t);
  }

  template<typename T>
  inline IStream& operator>>(T& t)
  {
    deserialize(*this, t);
    return *this;
  }
};

/**
 * \brief Length stream
 *
 * LStream is not what you would normally think of as a stream, but it is used in order to support
 * allinone serializers.
 */
struct LStream
{
  static const StreamType stream_type = stream_types::Length;

  LStream()
  : count_(0)
  {}

  /**
   * \brief Add the length of an item to this length stream
   */
  template<typename T>
  inline void next(const T& t)
  {
    count_ += serializationLength(t);
  }

  /**
   * \brief increment the length by len
   */
  inline uint32_t advance(uint32_t len)
  {
    uint32_t old = count_;
    count_ += len;
    return old;
  }

  /**
   * \brief Get the total length of this tream
   */
  inline uint32_t getLength() { return count_; }

private:
  uint32_t count_;
};

/**
 * \brief Serialize a message
 */
template<typename M>
inline SerializedMessage serializeMessage(const M& message)
{
#if 0 // ESOL for evaluation
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
#endif
    SerializedMessage m;
    uint32_t len = serializationLength(message);
    m.num_bytes = len + 4;  // Maruyama: + 4????
    // m.buf.reset(new uint8_t[m.num_bytes]);
    m.buf = (uint8_t*)malloc(m.num_bytes);  
    
    OStream stream(m.buf, (uint32_t)m.num_bytes);
    serialize(stream, (uint32_t)m.num_bytes - 4);
    m.message_start = stream.getData();
    serialize(stream, message);
    #if 0 // ESOL for evaluation
    clock_gettime(CLOCK_REALTIME, &end);
    std::cout <<std::fixed << "ser:"<<(double)start.tv_sec + (double)start.tv_nsec/1000000000L<<","<< (double)end.tv_sec + (double)end.tv_nsec/1000000000L << std::endl;
#endif
  return m;
}

}  // namespace serialization
}  // namespace ROSLITE_NAMESPACE
