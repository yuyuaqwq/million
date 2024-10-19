#pragma once

#include <cstdint>

#include <meta/macro.hpp>

#include <million/detail/dl_export.h>
#include <million/service_handle.h>
#include <million/msg_def.h>
#include <million/detail/noncopyable.h>

namespace million {

class MILLION_CLASS_EXPORT IMsg : noncopyable {
public:
    IMsg() = default;
    virtual ~IMsg() = default;

    SessionId session_id() const { return session_id_; }
    void set_session_id(SessionId session_id) { session_id_ = session_id; }

    ServiceHandle sender() const { return sender_; }
    void set_sender(ServiceHandle sender) { sender_ = sender; }

    template<typename MsgT>
    MsgT* get() { return static_cast<MsgT*>(this); }

private:
    ServiceHandle sender_;
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
    static inline constexpr TypeT kTypeValue = kType;

    using Base = MsgBaseT<TypeT>;

    MsgT() : Base(kType) {};
    ~MsgT() = default;
};


// 生成一个逗号
// (由COMMON_IF_NOT_END调用)
#define MILLION_COMMON_IF_TRUE ,

// 生成一个逗号, 直到__VA_ARGS__末尾不生成
#define MILLION_COMMON_IF_NOT_END(i, ...) META_NOT_IF(META_INDEX_IS_END(i, __VA_ARGS__), MILLION_COMMON_IF_TRUE)

#define MILLION_FIELD_TO_REF_DECL_I(x) x&&
#define MILLION_FIELD_TO_REF_DECL(x) MILLION_FIELD_TO_REF_DECL_I x

// 生成构造函数的参数声明, 以及默认参数
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
// (CTOR_ARGS_DECL_WITH_DEFAULT的META_FOR循环代码)
#define MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL(i, ...) \
	MILLION_FIELD_TO_REF_DECL(META_INDEX(i, __VA_ARGS__)) = {} MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)

// 字段声明转构造函数字段初始化(第二步)
// 参数 field_name: 字段名称
#define MILLION_FIELD_TO_CTOR_INIT_I(field_name) field_name(std::move(field_name))
// 字段声明转构造函数字段初始化(第一步)
// 参数 field: (TYPE) NAME格式的字段
// (由CTOR_INIT_LIST_IMPL调用)
#define MILLION_FIELD_TO_CTOR_INIT(field) MILLION_FIELD_TO_CTOR_INIT_I(META_EMPTY_PACK(field))

// 生成构造函数的初始化列表
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
// (CTOR_INIT_LIST的META_FOR循环代码)
#define MILLION_CTOR_INIT_LIST_IMPL(i, ...) \
	MILLION_FIELD_TO_CTOR_INIT(META_INDEX(i, __VA_ARGS__)) MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)

// 生成元数据结构体内的字段(第二步)
// 参数 struct_name: 结构体名称
// 参数 i: 类似for的索引
// 参数 field_name: 字段名称
#define MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, field_name) \
	static auto value(struct_name& obj) -> decltype(auto) { return (obj.field_name); } \
	constexpr static const char* name() { return META_TO_STR(field_name); }

// 生成元数据结构体内的字段(第一步)
// 把数据加工一下后传给DEF_META_FIELD_DATA_IMPL_II
// 参数 struct_name: 结构体名称
// 参数 i: 类似for的索引
// 参数 field: (TYPE) NAME格式的字段
// (由DEF_META_FIELD_DATA_IMPL调用)
#define MILLION_DEF_META_FIELD_DATA_IMPL_I(struct_name, i, field) MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, META_EMPTY_PACK(field))

// 生成单个字段的元数据结构体
// META_INDEX(0, __VA_ARGS__)用于获取之前传入的结构体名称
// META_INDEX(META_INC(i), __VA_ARGS__)获取索引对应的字段声明, 注意这里需要把索引+1, 因为第一个是结构体名称
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: 第一个参数是结构体名称, 余下的是(TYPE) NAME格式的字段声明列表
// (META_FIELD_DATAS的META_FOR循环代码)
#define MILLION_DEF_META_FIELD_DATA_IMPL(i, ...) \
	template<> struct MetaFieldData<i> { \
		MILLION_DEF_META_FIELD_DATA_IMPL_I(META_INDEX(0, __VA_ARGS__), i, META_INDEX(META_INC(i), __VA_ARGS__)) \
	};

// 字段声明转字段定义
// (TYPE) NAME -> TYPE NAME;
// (FIELDS_DECL的META_FOR_EACH循环代码)
#define MILLION_FIELDS_TO_DECL(x) META_UNPACK(x);

// 生成构造函数的初始化列表
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
#define MILLION_CTOR_INIT_LIST(...) META_FOR(MILLION_CTOR_INIT_LIST_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
// 生成构造函数的参数声明和默认参数
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
#define MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(...) META_FOR(MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
// 生成成员变量的声明
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
#define MILLION_FIELDS_DECL(...) META_FOR_EACH(MILLION_FIELDS_TO_DECL, __VA_ARGS__)
// 生成字段(成员变量)元数据
// 注意这里把name传入了META_FOR的参数列表
// 参数 name: 结构体名称
// 参数 __VA_ARGS__: (TYPE) NAME格式的字段声明列表
#define MILLION_META_FIELD_DATAS(name, ...) META_FOR(MILLION_DEF_META_FIELD_DATA_IMPL, 0, META_COUNT(__VA_ARGS__), name, __VA_ARGS__)

// 数据定义的主宏
#define MILLION_MSG_DEFINE(name, kType, ...) \
    struct name : public ::million::MsgT<kType> { \
		name(MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(__VA_ARGS__)) \
			: MILLION_CTOR_INIT_LIST(__VA_ARGS__) {} \
		MILLION_FIELDS_DECL(__VA_ARGS__) \
		template<size_t index> struct MetaFieldData; \
		constexpr static inline size_t kMetaFieldCount = META_COUNT(__VA_ARGS__); \
		MILLION_META_FIELD_DATAS(name, __VA_ARGS__) \
	};

template<class T, class Func>
void ForeachMetaFieldData(T&& obj, Func&& cb) {
	using DecayT = std::decay_t<T>;

	[&] <size_t... I>(std::index_sequence<I...>) {
		(cb(typename DecayT::template MetaFieldData<I>{}), ...);
	}(std::make_index_sequence<DecayT::kMetaFieldCount>{});
}

//enum class PKK {
//	kPPS
//};

//MILLION_MSG_DEFINE(SDD, PKK::kPPS, (float)sss, (int)kkp, (char)ssd)

//int main()
//{
//	SDD sss{ 1.0f, 122 };
//	ForeachMetaFieldData(sss, [&](auto field_data) {
//		std::cout << "Name:" << field_data.name() << " - Value:" << field_data.value(sss) << std::endl;
//		});
//}

} // namespace million