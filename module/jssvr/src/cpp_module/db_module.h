#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object/cpp_module_object.h>
#include <mjs/object.h>
#include <million/db/db_row.h>

namespace million {
namespace jssvr {

enum class CustomClassId {
    kDBRowObject = mjs::ClassId::kCustom,
    kConfigTableObject,
    kConfigTableWeakObject,
};

class DBModuleObject : public mjs::CppModuleObject {
private:
    DBModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static DBModuleObject* New(mjs::Runtime* runtime) {
        return new DBModuleObject(runtime);
    }
};

class DBRowClassDef : public mjs::ClassDef {
public:
    DBRowClassDef(mjs::Runtime* runtime);
};

class DBRowObject : public mjs::Object {
private:
    DBRowObject(mjs::Context* context, db::DBRow&& db_row);

public:
    static DBRowObject* New(mjs::Context* context, db::DBRow&& db_row) {
        return new DBRowObject(context, std::move(db_row));
    }

    void SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) override;
    bool GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) override;

    const db::DBRow& db_row() const { return db_row_;}
    db::DBRow& db_row() { return db_row_; }

private:
    db::DBRow db_row_;
};

} // namespace jssvr
} // namespace million
