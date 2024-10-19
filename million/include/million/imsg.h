#pragma once

#include <cstdint>

// #include <meta/macro.hpp>

#include <million/detail/dl_export.h>
#include <million/msg_def.h>
#include <million/detail/noncopyable.h>

namespace million {

class MILLION_CLASS_EXPORT IMsg : noncopyable {
public:
    IMsg() = default;
    virtual ~IMsg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

    template<typename MsgT>
    MsgT* get() { return static_cast<MsgT*>(this); }

private:
    SessionId session_id_{ kSessionIdInvalid };
};

template<typename TypeT>
class MsgBaseT : public IMsg {
public:
    MsgBaseT(TypeT type) : type_(type) {}
    ~MsgBaseT() = default;

    using IMsg::get;

    TypeT type() const { return type_; }

private:
    const TypeT type_;
};

template<auto kType>
class MsgT : public MsgBaseT<decltype(kType)> {
public:
    using TypeT = decltype(kType);
    static constexpr TypeT kTypeValue = kType;

    using Base = MsgBaseT<TypeT>;

    MsgT() : Base(kType) {};
    ~MsgT() = default;
};


//
//#define COMMON_IF_TRUE ,
//
//#define COMMON_IF_NOT_END(i, ...) META_NOT_IF(META_INDEX_IS_END(i, __VA_ARGS__), COMMON_IF_TRUE)
//
//#define CTOR_ARGS_DECL_WITH_DEFAULT_IMPL(i, ...) \
//	META_UNPACK(META_INDEX(i, __VA_ARGS__)) = {} COMMON_IF_NOT_END(i, __VA_ARGS__)
//
//
//#define FIELD_TO_CTOR_INIT_I(field_name) field_name(field_name)
//
//#define FIELD_TO_CTOR_INIT(field) FIELD_TO_CTOR_INIT_I(META_EMPTY_PACK(field))
//
//
//#define CTOR_INIT_LIST_IMPL(i, ...) \
//	FIELD_TO_CTOR_INIT(META_INDEX(i, __VA_ARGS__)) COMMON_IF_NOT_END(i, __VA_ARGS__)
//
//#define DEF_META_FIELD_DATA_IMPL_II(name, i, field_name) \
//	static auto GetValue(name& obj) -> decltype(auto) { return (obj.field_name); } \
//	constexpr static const char* GetName() { return META_TO_STR(field_name); }
//
//#define DEF_META_FIELD_DATA_IMPL_I(name, i, field) DEF_META_FIELD_DATA_IMPL_II(name, i, META_EMPTY_PACK(field))
//
//#define DEF_META_FIELD_DATA_IMPL(i, ...) \
//	template<> struct MetaFieldData<i> { \
//		DEF_META_FIELD_DATA_IMPL_I(META_INDEX(0, __VA_ARGS__), i, META_INDEX(META_INC(i), __VA_ARGS__)) \
//	};
//
//#define FIELDS_TO_DECL(x) META_UNPACK(x);
//
//
//#define CTOR_INIT_LIST(...) META_FOR(CTOR_INIT_LIST_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
//
//#define CTOR_ARGS_DECL_WITH_DEFAULT(...) META_FOR(CTOR_ARGS_DECL_WITH_DEFAULT_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
//
//#define FIELDS_DECL(...) META_FOR_EACH(FIELDS_TO_DECL, __VA_ARGS__)
//
//#define META_FIELD_DATAS(name, ...) META_FOR(DEF_META_FIELD_DATA_IMPL, 0, META_COUNT(__VA_ARGS__), name, __VA_ARGS__)
//
//#define MSG_DEFINE(name, kType, ...) \
//    struct name : public Msg<kType> { \
//		name(CTOR_ARGS_DECL_WITH_DEFAULT(__VA_ARGS__)) \
//			: CTOR_INIT_LIST(__VA_ARGS__) {} \
//		FIELDS_DECL(__VA_ARGS__) \
//		template<size_t index> struct MetaFieldData; \
//		constexpr static inline size_t kMetaFieldCount = META_COUNT(__VA_ARGS__); \
//		META_FIELD_DATAS(name, __VA_ARGS__) \
//	};

//enum class PKK {
//	kPPS
//};
//
//template<auto P>
//struct Msg {
//
//};
//
//MSG_DEFINE(SDD, PKK::kPPS, (float)sss, (int)kkp, (char)ssd)
//
//template<class T, class Func>
//void ForeachMeta(T&& obj, Func&& cb) {
//	using DecayT = std::decay_t<T>;
//	[] <size_t... I>(std::index_sequence<I...>) {
//		(cb(typename DecayT::template MetaFieldData<I>{}), ...);
//	};
//}
//
//int main()
//{
//	SDD sss{ 1.0f, 122 };
//	ForeachMeta(sss, [](auto& field_data) {
//
//		});
//}

} // namespace million