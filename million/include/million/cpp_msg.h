#pragma once

#include <cstdint>
#include <stdexcept>
#include <typeinfo>
#include <memory>

#include <meta/macro.hpp>

#include <million/api.h>
#include <million/session_def.h>

namespace million {
	 
class MILLION_API CppMsg {
public:
	CppMsg() = default;
    virtual ~CppMsg() = default;

	virtual const std::type_info& type() const { return type_static(); };
	static const std::type_info& type_static() { return typeid(CppMsg); };

    template<typename MsgT>
    MsgT* get() { return static_cast<MsgT*>(this); }

	virtual CppMsg* Copy() const { return new CppMsg(*this); };
};

using CppMsgUnique = std::unique_ptr<CppMsg>;

// 生成一个逗号
// (由COMMON_IF_NOT_END调用)
#define _MILLION_COMMON_IF_TRUE ,

// 生成一个逗号, 直到__VA_ARGS__末尾不生成
#define _MILLION_COMMON_IF_NOT_END(i, ...) META_NOT_IF(META_INDEX_IS_END(i, __VA_ARGS__), _MILLION_COMMON_IF_TRUE)

#define _MW_SIGNAL_FIELD_TO_DECL(field) META_EXTRACT_PAREN_UNPACK(field) META_EMPTY field

#define _MILLION_FIELD_TO_CONST_REF_DECL_I(x) x //auto&&
// 假设这里x是(float) kk，所以后面展开就是 FIELD_TO_CONST_REF_DECL_I (float) kk
// FIELD_TO_CONST_REF_DECL_I (float)会被识别成宏调用，把float作为参数传入
// 于是就得到了const float&的结果
// 然后在加上后面的kk，就组成了const float& kk
#define _MILLION_FIELD_TO_CONST_REF_DECL(x) _MILLION_FIELD_TO_CONST_REF_DECL_I x


// 生成构造函数的参数声明, 以及默认参数
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: 未加工的字段声明列表
// (CTOR_ARGS_DECL_WITH_DEFAULT的META_FOR循环代码)
#define _MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL(i, ...) \
	_MILLION_FIELD_TO_CONST_REF_DECL(META_INDEX(i, __VA_ARGS__)) _MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)


// 字段声明转构造函数字段初始化(第二步)
// 参数 field_name: 字段名称
#define _MILLION_FIELD_TO_CTOR_INIT_I(field_name) field_name(std::move(field_name)) // std::forward<decltype(field_name)>
// 字段声明转构造函数字段初始化(第一步)
// 参数 field: 未加工的字段
// (由CTOR_INIT_LIST_IMPL调用)
#define _MILLION_FIELD_TO_CTOR_INIT(field) _MILLION_FIELD_TO_CTOR_INIT_I(META_EMPTY_PACK(field))

// 生成构造函数的初始化列表
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: 未加工的字段声明列表
// (CTOR_INIT_LIST的META_FOR循环代码)
#define _MILLION_CTOR_INIT_LIST_IMPL(i, ...) \
	_MILLION_FIELD_TO_CTOR_INIT(META_INDEX(i, __VA_ARGS__)) _MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)

// 生成元数据结构体内的字段(第二步)
// 参数 struct_name: 结构体名称
// 参数 i: 类似for的索引
// 参数 field_name: 字段名称
#define _MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, field_name) \
	static auto value(struct_name& obj) -> decltype(auto) { return (obj.field_name); } \
	constexpr static const char* name() { return META_TO_STR(field_name); }

// 生成元数据结构体内的字段(第一步)
// 把数据加工一下后传给DEF_META_FIELD_DATA_IMPL_II
// 参数 struct_name: 结构体名称
// 参数 i: 类似for的索引
// 参数 field: 未加工的字段
// (由DEF_META_FIELD_DATA_IMPL调用)
#define _MILLION_DEF_META_FIELD_DATA_IMPL_I(struct_name, i, field) _MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, META_EMPTY_PACK(field))

// 生成单个字段的元数据结构体
// META_INDEX(0, __VA_ARGS__)用于获取之前传入的结构体名称
// META_INDEX(META_INC(i), __VA_ARGS__)获取索引对应的字段声明, 注意这里需要把索引+1, 因为第一个是结构体名称
// 参数 i: 类似for的索引
// 参数 __VA_ARGS__: 第一个参数是结构体名称, 余下的是未加工的字段声明列表
// (META_FIELD_DATAS的META_FOR循环代码)
#define _MILLION_DEF_META_FIELD_DATA_IMPL(i, ...) \
	template<> struct MetaFieldData<i> { \
		_MILLION_DEF_META_FIELD_DATA_IMPL_I(META_INDEX(0, __VA_ARGS__), i, META_INDEX(META_INC(i), __VA_ARGS__)) \
	};

// 字段声明转字段定义
// (TYPE) NAME -> TYPE NAME;
// (FIELDS_DECL的META_FOR_EACH循环代码)
#define _MILLION_FIELDS_TO_DECL(x) _MW_SIGNAL_FIELD_TO_DECL(x);

// 生成构造函数的初始化列表
// 参数 __VA_ARGS__: 未加工的字段声明列表
#define _MILLION_CTOR_INIT_LIST(...) META_FOR(_MILLION_CTOR_INIT_LIST_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
// 生成构造函数的参数声明和默认参数
// 参数 __VA_ARGS__: 未加工的字段声明列表
#define _MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(...) META_FOR(_MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)


// 生成成员变量的声明
// 参数 __VA_ARGS__: 未加工的字段声明列表
#define _MILLION_FIELDS_DECL(...) META_FOR_EACH(_MILLION_FIELDS_TO_DECL, __VA_ARGS__)
// 生成字段(成员变量)元数据
// 注意这里把name传入了META_FOR的参数列表
// 参数 name: 结构体名称
// 参数 __VA_ARGS__: 未加工的字段声明列表
#define _MILLION_META_FIELD_DATAS(name, ...) META_FOR(_MILLION_DEF_META_FIELD_DATA_IMPL, 0, META_COUNT(__VA_ARGS__), name, __VA_ARGS__)

// 数据定义的主宏
#define MILLION_MSG_DEFINE(API_, NAME_, ...) \
    class API_ NAME_ : public ::million::CppMessage { \
	public: \
        NAME_() = delete; \
		NAME_(_MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(__VA_ARGS__)) \
			: _MILLION_CTOR_INIT_LIST(__VA_ARGS__) {} \
		_MILLION_FIELDS_DECL(__VA_ARGS__) \
		virtual const std::type_info& type() const override { return type_static(); } \
		static const std::type_info& type_static() { return typeid(NAME_); } \
		virtual CppMessage* Copy() const override { return new NAME_(*this); } \
		template<size_t index> struct MetaFieldData; \
		constexpr static inline size_t kMetaFieldCount = META_COUNT(__VA_ARGS__); \
		_MILLION_META_FIELD_DATAS(NAME_, __VA_ARGS__) \
	};

#define MILLION_MSG_DEFINE_NONCOPYABLE(API_, NAME_, ...) \
    class API_ NAME_ : public ::million::CppMessage { \
	public: \
        NAME_() = delete; \
		NAME_(_MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(__VA_ARGS__)) \
			: _MILLION_CTOR_INIT_LIST(__VA_ARGS__) {} \
		_MILLION_FIELDS_DECL(__VA_ARGS__) \
		virtual const std::type_info& type() const override { return type_static(); } \
		static const std::type_info& type_static() { return typeid(NAME_); } \
		virtual CppMessage* Copy() const override { throw std::runtime_error("Non copy messages."); } \
		template<size_t index> struct MetaFieldData; \
		constexpr static inline size_t kMetaFieldCount = META_COUNT(__VA_ARGS__); \
		_MILLION_META_FIELD_DATAS(NAME_, __VA_ARGS__) \
	};

#define MILLION_MSG_DEFINE_EMPTY(API_, NAME_) \
    class API_ NAME_ : public ::million::CppMessage { \
	public: \
		virtual const std::type_info& type() const override { return type_static(); } \
		static const std::type_info& type_static() { return typeid(NAME_); } \
	};

template<class T, class Func>
void ForeachMetaFieldData(T&& obj, Func&& cb) {
	using DecayT = std::decay_t<T>;
	[&] <size_t... I>(std::index_sequence<I...>) {
		(cb(typename DecayT::template MetaFieldData<I>{}), ...);
	}(std::make_index_sequence<DecayT::kMetaFieldCount>{});
}

template <typename MsgT, typename... Args>
inline std::unique_ptr<MsgT> make_cpp_msg(Args&&... args) {
    return std::make_unique<MsgT>(std::forward<Args>(args)...);
}

} // namespace million