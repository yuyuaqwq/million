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
    using MsgType = TypeT;

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


// ����һ������
// (��COMMON_IF_NOT_END����)
#define MILLION_COMMON_IF_TRUE ,

// ����һ������, ֱ��__VA_ARGS__ĩβ������
#define MILLION_COMMON_IF_NOT_END(i, ...) META_NOT_IF(META_INDEX_IS_END(i, __VA_ARGS__), MILLION_COMMON_IF_TRUE)

// ���ֶ���������ȡ����(�ڶ���)
// ����:
// ����x��([MAYBE] DEFAULT) NAME
// META_IS_PAREN�ж��ǲ�������, ��������ž�ȥ������, ���û��ֱ�ӷ���
#define MILLION_FIELD_EXTRACT_NAME_I(x) META_IF_ELSE(META_IS_PAREN(x), META_EMPTY x, x)

// ���ֶ���������ȡĬ��ֵ(�ڶ���)
// ����:
// ����x��([MAYBE] DEFAULT) NAME
// ���������, ��ͨ��META_EXTRACT_PAREN_UNPACK��ȡ��������, ���û�оͷ���{}
#define MILLION_FIELD_EXTRACT_DEFAULT_I(x) META_IF_ELSE(META_IS_PAREN(x), META_EXTRACT_PAREN_UNPACK(x), {})

// ���ֶ���������ȡ����
// (TYPE)([MAYBE] DEFAULT) NAME -> NAME
#define MILLION_FIELD_EXTRACT_NAME(field) MILLION_FIELD_EXTRACT_NAME_I(META_EMPTY field)
// ���ֶ���������ȡĬ��ֵ
// (TYPE) NAME -> {}
// (TYPE)(DEFAULT) NAME -> DEFAULT
#define MILLION_FIELD_EXTRACT_DEFAULT(field) MILLION_FIELD_EXTRACT_DEFAULT_I(META_EMPTY field)
// ���ֶ���������ȡ����
// (TYPE)([MAYBE] DEFAULT) NAME -> TYPE
#define MILLION_FIELD_EXTRACT_TYPE(field) META_EXTRACT_PAREN_UNPACK(field)

// ��δ�ӹ������ֶ�����, תΪ�ֶ�����
// (TYPE)([MAYBE] DEFAULT) NAME -> TYPE NAME
#define MILLION_FIELD_TO_DECL(field) MILLION_FIELD_EXTRACT_TYPE(field) MILLION_FIELD_EXTRACT_NAME(field)


#define MILLION_FIELD_TO_CONST_REF_DECL_I(x) x&&
// ��������x��(float) kk�����Ժ���չ������ FIELD_TO_CONST_REF_DECL_I (float) kk
// FIELD_TO_CONST_REF_DECL_I (float)�ᱻʶ��ɺ���ã���float��Ϊ��������
// ���Ǿ͵õ���const float&�Ľ��
// Ȼ���ڼ��Ϻ����kk���������const float& kk
#define MILLION_FIELD_TO_CONST_REF_DECL(x) MILLION_FIELD_TO_CONST_REF_DECL_I x

// ���ɹ��캯���Ĳ�������, �Լ�Ĭ�ϲ���
// ���� i: ����for������
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
// (CTOR_ARGS_DECL_WITH_DEFAULT��META_FORѭ������)
#define MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL(i, ...) \
	MILLION_FIELD_TO_CONST_REF_DECL(META_INDEX(i, __VA_ARGS__)) = {} MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)

// �ֶ�����ת���캯���ֶγ�ʼ��(�ڶ���)
// ���� field_name: �ֶ�����
#define MILLION_FIELD_TO_CTOR_INIT_I(field_name) field_name(std::move(field_name))
// �ֶ�����ת���캯���ֶγ�ʼ��(��һ��)
// ���� field: δ�ӹ����ֶ�
// (��CTOR_INIT_LIST_IMPL����)
#define MILLION_FIELD_TO_CTOR_INIT(field) MILLION_FIELD_TO_CTOR_INIT_I(META_EMPTY_PACK(field))

// ���ɹ��캯���ĳ�ʼ���б�
// ���� i: ����for������
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
// (CTOR_INIT_LIST��META_FORѭ������)
#define MILLION_CTOR_INIT_LIST_IMPL(i, ...) \
	MILLION_FIELD_TO_CTOR_INIT(META_INDEX(i, __VA_ARGS__)) MILLION_COMMON_IF_NOT_END(i, __VA_ARGS__)

// ����Ԫ���ݽṹ���ڵ��ֶ�(�ڶ���)
// ���� struct_name: �ṹ������
// ���� i: ����for������
// ���� field_name: �ֶ�����
#define MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, field_name) \
	static auto value(struct_name& obj) -> decltype(auto) { return (obj.field_name); } \
	constexpr static const char* name() { return META_TO_STR(field_name); }

// ����Ԫ���ݽṹ���ڵ��ֶ�(��һ��)
// �����ݼӹ�һ�º󴫸�DEF_META_FIELD_DATA_IMPL_II
// ���� struct_name: �ṹ������
// ���� i: ����for������
// ���� field: δ�ӹ����ֶ�
// (��DEF_META_FIELD_DATA_IMPL����)
#define MILLION_DEF_META_FIELD_DATA_IMPL_I(struct_name, i, field) MILLION_DEF_META_FIELD_DATA_IMPL_II(struct_name, i, META_EMPTY_PACK(field))

// ���ɵ����ֶε�Ԫ���ݽṹ��
// META_INDEX(0, __VA_ARGS__)���ڻ�ȡ֮ǰ����Ľṹ������
// META_INDEX(META_INC(i), __VA_ARGS__)��ȡ������Ӧ���ֶ�����, ע��������Ҫ������+1, ��Ϊ��һ���ǽṹ������
// ���� i: ����for������
// ���� __VA_ARGS__: ��һ�������ǽṹ������, ���µ���δ�ӹ����ֶ������б�
// (META_FIELD_DATAS��META_FORѭ������)
#define MILLION_DEF_META_FIELD_DATA_IMPL(i, ...) \
	template<> struct MetaFieldData<i> { \
		MILLION_DEF_META_FIELD_DATA_IMPL_I(META_INDEX(0, __VA_ARGS__), i, META_INDEX(META_INC(i), __VA_ARGS__)) \
	};

// �ֶ�����ת�ֶζ���
// (TYPE) NAME -> TYPE NAME;
// (FIELDS_DECL��META_FOR_EACHѭ������)
#define MILLION_FIELDS_TO_DECL(x) META_UNPACK(x);

// ���ɹ��캯���ĳ�ʼ���б�
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
#define MILLION_CTOR_INIT_LIST(...) META_FOR(MILLION_CTOR_INIT_LIST_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
// ���ɹ��캯���Ĳ���������Ĭ�ϲ���
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
#define MILLION_CTOR_ARGS_DECL_WITH_DEFAULT(...) META_FOR(MILLION_CTOR_ARGS_DECL_WITH_DEFAULT_IMPL, 0, META_COUNT(__VA_ARGS__), __VA_ARGS__)
// ���ɳ�Ա����������
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
#define MILLION_FIELDS_DECL(...) META_FOR_EACH(MILLION_FIELDS_TO_DECL, __VA_ARGS__)
// �����ֶ�(��Ա����)Ԫ����
// ע�������name������META_FOR�Ĳ����б�
// ���� name: �ṹ������
// ���� __VA_ARGS__: δ�ӹ����ֶ������б�
#define MILLION_META_FIELD_DATAS(name, ...) META_FOR(MILLION_DEF_META_FIELD_DATA_IMPL, 0, META_COUNT(__VA_ARGS__), name, __VA_ARGS__)

// ���ݶ��������
#define MILLION_MSG_DEFINE(name, type, ...) \
    struct name : public ::million::MsgT<type> { \
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

} // namespace million