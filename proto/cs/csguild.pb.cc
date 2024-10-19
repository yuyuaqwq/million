// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: csguild.proto

#include "csguild.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace Cs {
PROTOBUF_CONSTEXPR GuildCreateReq::GuildCreateReq(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.guild_name_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct GuildCreateReqDefaultTypeInternal {
  PROTOBUF_CONSTEXPR GuildCreateReqDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~GuildCreateReqDefaultTypeInternal() {}
  union {
    GuildCreateReq _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 GuildCreateReqDefaultTypeInternal _GuildCreateReq_default_instance_;
}  // namespace Cs
static ::_pb::Metadata file_level_metadata_csguild_2eproto[1];
static const ::_pb::EnumDescriptor* file_level_enum_descriptors_csguild_2eproto[1];
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_csguild_2eproto = nullptr;

const uint32_t TableStruct_csguild_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::Cs::GuildCreateReq, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Cs::GuildCreateReq, _impl_.guild_name_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::Cs::GuildCreateReq)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::Cs::_GuildCreateReq_default_instance_._instance,
};

const char descriptor_table_protodef_csguild_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\rcsguild.proto\022\002Cs\032 google/protobuf/des"
  "criptor.proto\032\rcsmsgid.proto\"*\n\016GuildCre"
  "ateReq\022\022\n\nguild_name\030\001 \001(\t:\004\310\257\005\001*P\n\rSubM"
  "sgIdGuild\022\034\n\030SUB_MSG_ID_GUILD_INVALID\020\000\022"
  "\033\n\027SUB_MSG_ID_GUILD_CREATE\020\001\032\004\210\361\004\001:P\n\023cs"
  "_sub_msg_id_guild\022\037.google.protobuf.Mess"
  "ageOptions\030\371U \001(\0162\021.Cs.SubMsgIdGuildb\006pr"
  "oto3"
  ;
static const ::_pbi::DescriptorTable* const descriptor_table_csguild_2eproto_deps[2] = {
  &::descriptor_table_csmsgid_2eproto,
  &::descriptor_table_google_2fprotobuf_2fdescriptor_2eproto,
};
static ::_pbi::once_flag descriptor_table_csguild_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_csguild_2eproto = {
    false, false, 284, descriptor_table_protodef_csguild_2eproto,
    "csguild.proto",
    &descriptor_table_csguild_2eproto_once, descriptor_table_csguild_2eproto_deps, 2, 1,
    schemas, file_default_instances, TableStruct_csguild_2eproto::offsets,
    file_level_metadata_csguild_2eproto, file_level_enum_descriptors_csguild_2eproto,
    file_level_service_descriptors_csguild_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_csguild_2eproto_getter() {
  return &descriptor_table_csguild_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_csguild_2eproto(&descriptor_table_csguild_2eproto);
namespace Cs {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* SubMsgIdGuild_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_csguild_2eproto);
  return file_level_enum_descriptors_csguild_2eproto[0];
}
bool SubMsgIdGuild_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}


// ===================================================================

class GuildCreateReq::_Internal {
 public:
};

GuildCreateReq::GuildCreateReq(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:Cs.GuildCreateReq)
}
GuildCreateReq::GuildCreateReq(const GuildCreateReq& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  GuildCreateReq* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.guild_name_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.guild_name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.guild_name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_guild_name().empty()) {
    _this->_impl_.guild_name_.Set(from._internal_guild_name(), 
      _this->GetArenaForAllocation());
  }
  // @@protoc_insertion_point(copy_constructor:Cs.GuildCreateReq)
}

inline void GuildCreateReq::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.guild_name_){}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.guild_name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.guild_name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

GuildCreateReq::~GuildCreateReq() {
  // @@protoc_insertion_point(destructor:Cs.GuildCreateReq)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void GuildCreateReq::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.guild_name_.Destroy();
}

void GuildCreateReq::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void GuildCreateReq::Clear() {
// @@protoc_insertion_point(message_clear_start:Cs.GuildCreateReq)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.guild_name_.ClearToEmpty();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* GuildCreateReq::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string guild_name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_guild_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Cs.GuildCreateReq.guild_name"));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* GuildCreateReq::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Cs.GuildCreateReq)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string guild_name = 1;
  if (!this->_internal_guild_name().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_guild_name().data(), static_cast<int>(this->_internal_guild_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Cs.GuildCreateReq.guild_name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_guild_name(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Cs.GuildCreateReq)
  return target;
}

size_t GuildCreateReq::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Cs.GuildCreateReq)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string guild_name = 1;
  if (!this->_internal_guild_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_guild_name());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData GuildCreateReq::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    GuildCreateReq::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GuildCreateReq::GetClassData() const { return &_class_data_; }


void GuildCreateReq::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<GuildCreateReq*>(&to_msg);
  auto& from = static_cast<const GuildCreateReq&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Cs.GuildCreateReq)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_guild_name().empty()) {
    _this->_internal_set_guild_name(from._internal_guild_name());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void GuildCreateReq::CopyFrom(const GuildCreateReq& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Cs.GuildCreateReq)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool GuildCreateReq::IsInitialized() const {
  return true;
}

void GuildCreateReq::InternalSwap(GuildCreateReq* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.guild_name_, lhs_arena,
      &other->_impl_.guild_name_, rhs_arena
  );
}

::PROTOBUF_NAMESPACE_ID::Metadata GuildCreateReq::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_csguild_2eproto_getter, &descriptor_table_csguild_2eproto_once,
      file_level_metadata_csguild_2eproto[0]);
}
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 ::PROTOBUF_NAMESPACE_ID::internal::ExtensionIdentifier< ::PROTOBUF_NAMESPACE_ID::MessageOptions,
    ::PROTOBUF_NAMESPACE_ID::internal::EnumTypeTraits< ::Cs::SubMsgIdGuild, ::Cs::SubMsgIdGuild_IsValid>, 14, false>
  cs_sub_msg_id_guild(kCsSubMsgIdGuildFieldNumber, static_cast< ::Cs::SubMsgIdGuild >(0), nullptr);

// @@protoc_insertion_point(namespace_scope)
}  // namespace Cs
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Cs::GuildCreateReq*
Arena::CreateMaybeMessage< ::Cs::GuildCreateReq >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Cs::GuildCreateReq >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
