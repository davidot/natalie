#include "natalie.hpp"
#include <random>

namespace Natalie {

ValuePtr ArrayValue::initialize(Env *env, ValuePtr size, ValuePtr value) {
    if (!size) {
        return this;
    }
    if (size->is_array()) {
        for (auto &item : *size->as_array()) {
            push(item);
        }
        return this;
    }
    size->assert_type(env, Value::Type::Integer, "Integer");
    if (!value) value = env->nil_obj();
    for (nat_int_t i = 0; i < size->as_integer()->to_nat_int_t(); i++) {
        push(value);
    }
    return this;
}

ValuePtr ArrayValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "[" };
    for (size_t i = 0; i < size(); i++) {
        ValuePtr obj = (*this)[i];
        StringValue *repr = obj.send(env, "inspect")->as_string();
        out->append(env, repr);
        if (i < size() - 1) {
            out->append(env, ", ");
        }
    }
    out->append_char(env, ']');
    return out;
}

ValuePtr ArrayValue::ltlt(Env *env, ValuePtr arg) {
    this->assert_not_frozen(env);
    push(arg);
    return this;
}

ValuePtr ArrayValue::add(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { env, *this };
    new_array->concat(*other->as_array());
    return new_array;
}

ValuePtr ArrayValue::sub(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        int found = 0;
        for (auto &compare_item : *other->as_array()) {
            if (item.send(env, "==", 1, &compare_item, nullptr)->is_truthy()) {
                found = 1;
                break;
            }
        }
        if (!found) {
            new_array->push(item);
        }
    }
    return new_array;
}

ValuePtr ArrayValue::ref(Env *env, ValuePtr index_obj, ValuePtr size) {
    if (index_obj->type() == Value::Type::Integer) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();
        if (index < 0) {
            index = this->size() + index;
        }
        if (index < 0) {
            return env->nil_obj();
        }
        size_t sindex = static_cast<size_t>(index);
        if (sindex >= this->size()) {
            return env->nil_obj();
        } else if (!size) {
            return (*this)[index];
        }
        size->assert_type(env, Value::Type::Integer, "Integer");
        size_t end = index + size->as_integer()->to_nat_int_t();
        size_t max = this->size();
        end = end > max ? max : end;
        ArrayValue *result = new ArrayValue { env };
        for (size_t i = index; i < end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        ValuePtr begin_obj = range->begin();
        ValuePtr end_obj = range->end();
        begin_obj->assert_type(env, Value::Type::Integer, "Integer");
        end_obj->assert_type(env, Value::Type::Integer, "Integer");
        nat_int_t begin = begin_obj->as_integer()->to_nat_int_t();
        nat_int_t end = end_obj->as_integer()->to_nat_int_t();
        if (begin < 0) {
            begin = this->size() + begin;
        }
        if (end < 0) {
            end = this->size() + end;
        }
        if (begin < 0 || end < 0) {
            if (begin_obj->as_integer()->is_zero()) {
                // NOTE: not entirely sure about this, but range beginning with 0..
                // seems to be a special case ¯\_(ツ)_/¯
                return new ArrayValue { env };
            }
            return env->nil_obj();
        }
        size_t u_end = static_cast<size_t>(end);
        if (!range->exclude_end()) u_end++;
        size_t max = this->size();
        u_end = u_end > max ? max : u_end;
        ArrayValue *result = new ArrayValue { env };
        for (size_t i = begin; i < u_end; i++) {
            result->push((*this)[i]);
        }
        return result;
    } else {
        env->raise("TypeError", "no implicit conversion of {} into Integer", index_obj->klass()->class_name_or_blank());
    }
}

ValuePtr ArrayValue::refeq(Env *env, ValuePtr index_obj, ValuePtr size, ValuePtr val) {
    this->assert_not_frozen(env);
    index_obj->assert_type(env, Value::Type::Integer, "Integer"); // TODO: accept a range
    nat_int_t index = index_obj->as_integer()->to_nat_int_t();
    assert(index >= 0); // TODO: accept negative index
    size_t u_index = static_cast<size_t>(index);
    if (!val) {
        val = size;
        if (u_index < this->size()) {
            (*this)[u_index] = val;
        } else {
            expand_with_nil(env, u_index);
            push(val);
        }
        return val;
    }
    size->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t length = size->as_integer()->to_nat_int_t();
    assert(length >= 0);
    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayValue *ary2 = new ArrayValue { env };
    // stuff before the new entry/entries
    for (size_t i = 0; i < u_index; i++) {
        if (i >= this->size()) break;
        ary2->push((*this)[i]);
    }
    // extra nils if needed
    ary2->expand_with_nil(env, u_index);
    // the new entry/entries
    if (val->is_array()) {
        for (auto &v : *val->as_array()) {
            ary2->push(v);
        }
    } else {
        ary2->push(val);
    }
    // stuff after the new entry/entries
    for (size_t i = u_index + length; i < this->size(); i++) {
        ary2->push((*this)[i]);
    }
    overwrite(*ary2);
    return val;
}

ValuePtr ArrayValue::any(Env *env, size_t argc, ValuePtr *args, Block *block) {
    auto Enumerable = env->Object()->const_fetch(env, SymbolValue::intern(env, "Enumerable"))->as_module();
    auto any_method = Enumerable->find_method(env, SymbolValue::intern(env, "any?"));
    return any_method->call(env, this, argc, args, block);
}

ValuePtr ArrayValue::eq(Env *env, ValuePtr other) {
    if (!other->is_array()) return env->false_obj();
    ArrayValue *other_array = other->as_array();
    if (size() != other_array->size()) return env->false_obj();
    if (size() == 0) return env->true_obj();
    for (size_t i = 0; i < size(); i++) {
        // TODO: could easily be optimized for strings and numbers
        ValuePtr item = (*other_array)[i];
        ValuePtr result = (*this)[i].send(env, "==", 1, &item, nullptr);
        if (result->type() == Value::Type::False) return result;
    }
    return env->true_obj();
}

ValuePtr ArrayValue::eql(Env *env, ValuePtr other) {
    if (other == this)
        return env->true_obj();
    if (!other->is_array())
        return env->false_obj();

    auto other_array = other->as_array();
    if (size() != other_array->size())
        return env->false_obj();

    for (size_t i = 0; i < size(); ++i) {
        ValuePtr item = (*other_array)[i];
        ValuePtr result = (*this)[i].send(env, "eql?", 1, &item, nullptr);
        if (result->type() == Value::Type::False)
            return result;
    }

    return env->true_obj();
}

ValuePtr ArrayValue::each(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    for (auto &obj : *this) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, nullptr);
    }
    return this;
}

ValuePtr ArrayValue::map(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        new_array->push(result);
    }
    return new_array;
}

// TODO: accept integer and return array
ValuePtr ArrayValue::first(Env *env) {
    if (size() > 0) {
        return (*this)[0];
    } else {
        return env->nil_obj();
    }
}

ValuePtr ArrayValue::sample(Env *env) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> random_number(1, size());

    if (size() > 0) {
        return (*this)[random_number(rng) - 1];
    } else {
        return env->nil_obj();
    }
}

// TODO: accept integer and return array
ValuePtr ArrayValue::last(Env *env) {
    if (size() > 0) {
        return (*this)[size() - 1];
    } else {
        return env->nil_obj();
    }
}

ValuePtr ArrayValue::include(Env *env, ValuePtr item) {
    if (size() == 0) {
        return env->false_obj();
    } else {
        for (auto &compare_item : *this) {
            if (item.send(env, "==", 1, &compare_item, nullptr)->is_truthy()) {
                return env->true_obj();
            }
        }
        return env->false_obj();
    }
}

ValuePtr ArrayValue::index(Env *env, ValuePtr object, Block *block) {
    assert(size() <= NAT_INT_MAX);
    auto length = static_cast<nat_int_t>(size());
    if (block) {
        for (nat_int_t i = 0; i < length; i++) {
            auto item = m_vector[i];
            ValuePtr args[] = { item };
            auto result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
            if (result->is_truthy())
                return ValuePtr { env, i };
        }
        return env->nil_obj();
    } else if (object) {
        for (nat_int_t i = 0; i < length; i++) {
            auto item = m_vector[i];
            ValuePtr args[] = { object };
            if (item.send(env, "==", 1, args)->is_truthy())
                return ValuePtr { env, i };
        }
        return env->nil_obj();
    } else {
        // TODO
        env->assert_block_given(block);
        NAT_UNREACHABLE();
    }
}

ValuePtr ArrayValue::shift(Env *env, ValuePtr count) {
    auto has_count = count != nullptr;
    size_t shift_count = 1;
    ValuePtr result = nullptr;
    if (has_count) {
        count->assert_type(env, Value::Type::Integer, "Integer");
        shift_count = count->as_integer()->to_nat_int_t();
        if (shift_count == 0) {
            return new ArrayValue { env };
        }
        result = new ArrayValue { env, m_vector.slice(0, shift_count) };
    } else {
        result = m_vector[0];
    }

    auto shifted = m_vector.slice(shift_count);
    m_vector = std::move(shifted);
    return result;
}

ValuePtr ArrayValue::sort(Env *env) {
    ArrayValue *copy = new ArrayValue { env, *this };
    copy->sort_in_place(env);
    return copy;
}

ValuePtr ArrayValue::join(Env *env, ValuePtr joiner) {
    if (size() == 0) {
        return new StringValue { env };
    } else if (size() == 1) {
        return (*this)[0].send(env, "to_s");
    } else {
        if (!joiner) joiner = new StringValue { env, "" };
        joiner->assert_type(env, Value::Type::String, "String");
        StringValue *out = (*this)[0].send(env, "to_s")->dup(env)->as_string();
        for (size_t i = 1; i < size(); i++) {
            ValuePtr item = (*this)[i];
            out->append(env, joiner->as_string());
            out->append(env, item.send(env, "to_s")->as_string());
        }
        return out;
    }
}

ValuePtr ArrayValue::cmp(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Array, "Array");
    ArrayValue *other_array = other->as_array();
    for (size_t i = 0; i < size(); i++) {
        if (i >= other_array->size()) {
            return ValuePtr { env, 1 };
        }
        ValuePtr item = (*other_array)[i];
        ValuePtr cmp_obj = (*this)[i].send(env, "<=>", 1, &item, nullptr);
        assert(cmp_obj->type() == Value::Type::Integer);
        nat_int_t cmp = cmp_obj->as_integer()->to_nat_int_t();
        if (cmp < 0) return ValuePtr { env, -1 };
        if (cmp > 0) return ValuePtr { env, 1 };
    }
    if (other_array->size() > size()) {
        return ValuePtr { env, -1 };
    }
    return ValuePtr { env, 0 };
}

ValuePtr ArrayValue::push(Env *env, size_t argc, ValuePtr *args) {
    for (size_t i = 0; i < argc; i++) {
        push(args[i]);
    }
    return this;
}

void ArrayValue::push_splat(Env *env, ValuePtr val) {
    if (!val->is_array() && val->respond_to(env, "to_a")) {
        val = val.send(env, "to_a");
    }
    if (val->is_array()) {
        for (ValuePtr v : *val->as_array()) {
            push(*v);
        }
    } else {
        push(*val);
    }
}

ValuePtr ArrayValue::pop(Env *env) {
    this->assert_not_frozen(env);
    if (size() == 0) return env->nil_obj();
    ValuePtr val = m_vector[m_vector.size() - 1];
    m_vector.set_size(m_vector.size() - 1);
    return val;
}

void ArrayValue::expand_with_nil(Env *env, size_t total) {
    for (size_t i = size(); i < total; i++) {
        push(*env->nil_obj());
    }
}

void ArrayValue::sort_in_place(Env *env) {
    this->assert_not_frozen(env);
    auto cmp = [](void *env, ValuePtr a, ValuePtr b) {
        ValuePtr compare = a.send(static_cast<Env *>(env), "<=>", 1, &b, nullptr);
        return compare->as_integer()->to_nat_int_t() < 0;
    };
    m_vector.sort(Vector<ValuePtr>::SortComparator { env, cmp });
}

ValuePtr ArrayValue::select(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_truthy()) {
            new_array->push(item);
        }
    }
    return new_array;
}

ValuePtr ArrayValue::reject(Env *env, Block *block) {
    env->assert_block_given(block); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *this) {
        ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        if (result->is_falsey()) {
            new_array->push(item);
        }
    }
    return new_array;
}

ValuePtr ArrayValue::max(Env *env) {
    if (m_vector.size() == 0)
        return env->nil_obj();
    ValuePtr max = nullptr;
    for (auto item : *this) {
        if (!max || item.send(env, ">", 1, &max)->is_truthy())
            max = item;
    }
    return max;
}

ValuePtr ArrayValue::min(Env *env) {
    if (m_vector.size() == 0)
        return env->nil_obj();
    ValuePtr min = nullptr;
    for (auto item : *this) {
        if (!min || item.send(env, "<", 1, &min)->is_truthy())
            min = item;
    }
    return min;
}

ValuePtr ArrayValue::compact(Env *env) {
    auto ary = new ArrayValue { env };
    for (auto item : *this) {
        if (item->is_nil()) continue;
        ary->push(item);
    }
    return ary;
}

ValuePtr ArrayValue::uniq(Env *env) {
    auto hash = new HashValue { env };
    auto nil = env->nil_obj();
    for (auto item : *this) {
        hash->put(env, item, nil);
    }
    return hash->keys(env);
}

}
