#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ProcValue : public Value {
public:
    enum class ProcType {
        Proc,
        Lambda
    };

    ProcValue(Env *env)
        : Value { Value::Type::Proc, env->Object()->const_fetch(env, SymbolValue::intern(env, "Proc"))->as_class() } { }

    ProcValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Proc, klass } { }

    ProcValue(Env *env, Block *block, ProcType type = ProcType::Proc)
        : Value { Value::Type::Proc, env->Object()->const_fetch(env, SymbolValue::intern(env, "Proc"))->as_class() }
        , m_block { block }
        , m_type { type } {
        assert(m_block);
    }

    static ValuePtr from_block_maybe(Env *env, Block *block) {
        if (!block) {
            return env->nil_obj();
        }
        return new ProcValue { env, block };
    }

    ValuePtr initialize(Env *, Block *);

    Block *block() { return m_block; }
    bool is_lambda() { return m_type == ProcType::Lambda; }

    virtual ProcValue *to_proc(Env *) override {
        return this;
    }

    ValuePtr call(Env *, size_t, ValuePtr *, Block *);

    Env *env() { return m_block->env(); }

    int arity() { return m_block ? m_block->arity() : 0; }

    virtual void visit_children(Visitor &visitor) override {
        Value::visit_children(visitor);
        visitor.visit(m_block);
    }

    virtual void gc_print() override {
        fprintf(stderr, "<ProcValue %p>", this);
    }

private:
    Block *m_block { nullptr };
    ProcType m_type { ProcType::Proc };
};

}
