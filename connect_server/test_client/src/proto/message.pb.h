// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: message.proto

#ifndef PROTOBUF_message_2eproto__INCLUDED
#define PROTOBUF_message_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2003000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2003000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_reflection.h>
// @@protoc_insertion_point(includes)

namespace gim {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_message_2eproto();
void protobuf_AssignDesc_message_2eproto();
void protobuf_ShutdownFile_message_2eproto();

class Message;

// ===================================================================

class Message : public ::google::protobuf::Message {
 public:
  Message();
  virtual ~Message();
  
  Message(const Message& from);
  
  inline Message& operator=(const Message& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const Message& default_instance();
  
  void Swap(Message* other);
  
  // implements Message ----------------------------------------------
  
  Message* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Message& from);
  void MergeFrom(const Message& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // optional string to = 1;
  inline bool has_to() const;
  inline void clear_to();
  static const int kToFieldNumber = 1;
  inline const ::std::string& to() const;
  inline void set_to(const ::std::string& value);
  inline void set_to(const char* value);
  inline void set_to(const char* value, size_t size);
  inline ::std::string* mutable_to();
  
  // optional int64 id = 2;
  inline bool has_id() const;
  inline void clear_id();
  static const int kIdFieldNumber = 2;
  inline ::google::protobuf::int64 id() const;
  inline void set_id(::google::protobuf::int64 value);
  
  // optional int64 time = 3;
  inline bool has_time() const;
  inline void clear_time();
  static const int kTimeFieldNumber = 3;
  inline ::google::protobuf::int64 time() const;
  inline void set_time(::google::protobuf::int64 value);
  
  // optional string from = 4;
  inline bool has_from() const;
  inline void clear_from();
  static const int kFromFieldNumber = 4;
  inline const ::std::string& from() const;
  inline void set_from(const ::std::string& value);
  inline void set_from(const char* value);
  inline void set_from(const char* value, size_t size);
  inline ::std::string* mutable_from();
  
  // optional int32 type = 5;
  inline bool has_type() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 5;
  inline ::google::protobuf::int32 type() const;
  inline void set_type(::google::protobuf::int32 value);
  
  // optional string sn = 6;
  inline bool has_sn() const;
  inline void clear_sn();
  static const int kSnFieldNumber = 6;
  inline const ::std::string& sn() const;
  inline void set_sn(const ::std::string& value);
  inline void set_sn(const char* value);
  inline void set_sn(const char* value, size_t size);
  inline ::std::string* mutable_sn();
  
  // optional bytes data = 7;
  inline bool has_data() const;
  inline void clear_data();
  static const int kDataFieldNumber = 7;
  inline const ::std::string& data() const;
  inline void set_data(const ::std::string& value);
  inline void set_data(const char* value);
  inline void set_data(const void* value, size_t size);
  inline ::std::string* mutable_data();
  
  // @@protoc_insertion_point(class_scope:gim.Message)
 private:
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  mutable int _cached_size_;
  
  ::std::string* to_;
  static const ::std::string _default_to_;
  ::google::protobuf::int64 id_;
  ::google::protobuf::int64 time_;
  ::std::string* from_;
  static const ::std::string _default_from_;
  ::google::protobuf::int32 type_;
  ::std::string* sn_;
  static const ::std::string _default_sn_;
  ::std::string* data_;
  static const ::std::string _default_data_;
  friend void  protobuf_AddDesc_message_2eproto();
  friend void protobuf_AssignDesc_message_2eproto();
  friend void protobuf_ShutdownFile_message_2eproto();
  
  ::google::protobuf::uint32 _has_bits_[(7 + 31) / 32];
  
  // WHY DOES & HAVE LOWER PRECEDENCE THAN != !?
  inline bool _has_bit(int index) const {
    return (_has_bits_[index / 32] & (1u << (index % 32))) != 0;
  }
  inline void _set_bit(int index) {
    _has_bits_[index / 32] |= (1u << (index % 32));
  }
  inline void _clear_bit(int index) {
    _has_bits_[index / 32] &= ~(1u << (index % 32));
  }
  
  void InitAsDefaultInstance();
  static Message* default_instance_;
};
// ===================================================================


// ===================================================================

// Message

// optional string to = 1;
inline bool Message::has_to() const {
  return _has_bit(0);
}
inline void Message::clear_to() {
  if (to_ != &_default_to_) {
    to_->clear();
  }
  _clear_bit(0);
}
inline const ::std::string& Message::to() const {
  return *to_;
}
inline void Message::set_to(const ::std::string& value) {
  _set_bit(0);
  if (to_ == &_default_to_) {
    to_ = new ::std::string;
  }
  to_->assign(value);
}
inline void Message::set_to(const char* value) {
  _set_bit(0);
  if (to_ == &_default_to_) {
    to_ = new ::std::string;
  }
  to_->assign(value);
}
inline void Message::set_to(const char* value, size_t size) {
  _set_bit(0);
  if (to_ == &_default_to_) {
    to_ = new ::std::string;
  }
  to_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Message::mutable_to() {
  _set_bit(0);
  if (to_ == &_default_to_) {
    to_ = new ::std::string;
  }
  return to_;
}

// optional int64 id = 2;
inline bool Message::has_id() const {
  return _has_bit(1);
}
inline void Message::clear_id() {
  id_ = GOOGLE_LONGLONG(0);
  _clear_bit(1);
}
inline ::google::protobuf::int64 Message::id() const {
  return id_;
}
inline void Message::set_id(::google::protobuf::int64 value) {
  _set_bit(1);
  id_ = value;
}

// optional int64 time = 3;
inline bool Message::has_time() const {
  return _has_bit(2);
}
inline void Message::clear_time() {
  time_ = GOOGLE_LONGLONG(0);
  _clear_bit(2);
}
inline ::google::protobuf::int64 Message::time() const {
  return time_;
}
inline void Message::set_time(::google::protobuf::int64 value) {
  _set_bit(2);
  time_ = value;
}

// optional string from = 4;
inline bool Message::has_from() const {
  return _has_bit(3);
}
inline void Message::clear_from() {
  if (from_ != &_default_from_) {
    from_->clear();
  }
  _clear_bit(3);
}
inline const ::std::string& Message::from() const {
  return *from_;
}
inline void Message::set_from(const ::std::string& value) {
  _set_bit(3);
  if (from_ == &_default_from_) {
    from_ = new ::std::string;
  }
  from_->assign(value);
}
inline void Message::set_from(const char* value) {
  _set_bit(3);
  if (from_ == &_default_from_) {
    from_ = new ::std::string;
  }
  from_->assign(value);
}
inline void Message::set_from(const char* value, size_t size) {
  _set_bit(3);
  if (from_ == &_default_from_) {
    from_ = new ::std::string;
  }
  from_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Message::mutable_from() {
  _set_bit(3);
  if (from_ == &_default_from_) {
    from_ = new ::std::string;
  }
  return from_;
}

// optional int32 type = 5;
inline bool Message::has_type() const {
  return _has_bit(4);
}
inline void Message::clear_type() {
  type_ = 0;
  _clear_bit(4);
}
inline ::google::protobuf::int32 Message::type() const {
  return type_;
}
inline void Message::set_type(::google::protobuf::int32 value) {
  _set_bit(4);
  type_ = value;
}

// optional string sn = 6;
inline bool Message::has_sn() const {
  return _has_bit(5);
}
inline void Message::clear_sn() {
  if (sn_ != &_default_sn_) {
    sn_->clear();
  }
  _clear_bit(5);
}
inline const ::std::string& Message::sn() const {
  return *sn_;
}
inline void Message::set_sn(const ::std::string& value) {
  _set_bit(5);
  if (sn_ == &_default_sn_) {
    sn_ = new ::std::string;
  }
  sn_->assign(value);
}
inline void Message::set_sn(const char* value) {
  _set_bit(5);
  if (sn_ == &_default_sn_) {
    sn_ = new ::std::string;
  }
  sn_->assign(value);
}
inline void Message::set_sn(const char* value, size_t size) {
  _set_bit(5);
  if (sn_ == &_default_sn_) {
    sn_ = new ::std::string;
  }
  sn_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Message::mutable_sn() {
  _set_bit(5);
  if (sn_ == &_default_sn_) {
    sn_ = new ::std::string;
  }
  return sn_;
}

// optional bytes data = 7;
inline bool Message::has_data() const {
  return _has_bit(6);
}
inline void Message::clear_data() {
  if (data_ != &_default_data_) {
    data_->clear();
  }
  _clear_bit(6);
}
inline const ::std::string& Message::data() const {
  return *data_;
}
inline void Message::set_data(const ::std::string& value) {
  _set_bit(6);
  if (data_ == &_default_data_) {
    data_ = new ::std::string;
  }
  data_->assign(value);
}
inline void Message::set_data(const char* value) {
  _set_bit(6);
  if (data_ == &_default_data_) {
    data_ = new ::std::string;
  }
  data_->assign(value);
}
inline void Message::set_data(const void* value, size_t size) {
  _set_bit(6);
  if (data_ == &_default_data_) {
    data_ = new ::std::string;
  }
  data_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Message::mutable_data() {
  _set_bit(6);
  if (data_ == &_default_data_) {
    data_ = new ::std::string;
  }
  return data_;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace gim

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_message_2eproto__INCLUDED