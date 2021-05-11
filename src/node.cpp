#include "natalie/node.hpp"

namespace Natalie {

ValuePtr AliasNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "alias"),
            m_new_name->to_ruby(env),
            m_existing_name->to_ruby(env),
        }
    };
}

void AliasNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_new_name);
    visitor.visit(m_existing_name);
}

ValuePtr ArgNode::to_ruby(Env *env) {
    if (m_value) {
        return new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "lasgn"),
                SymbolValue::intern(env, m_name),
                m_value->to_ruby(env),
            }
        };
    } else {
        String name;
        if (m_name)
            name = String(m_name);
        else
            name = String();
        if (m_splat)
            name.prepend_char('*');
        else if (m_block_arg)
            name.prepend_char('&');
        return SymbolValue::intern(env, name.c_str());
    }
}

ValuePtr ArrayNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "array") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr AssignmentNode::to_ruby(Env *env) {
    const char *assignment_type;
    ValuePtr left;
    switch (m_identifier->type()) {
    case Node::Type::MultipleAssignment: {
        auto masgn = static_cast<MultipleAssignmentNode *>(m_identifier);
        auto sexp = masgn->to_ruby_with_array(env);
        auto value = new SexpValue { env, this, { SymbolValue::intern(env, "to_ary") } };
        value->push(m_value->to_ruby(env));
        sexp->push(value);
        return sexp;
    }
    case Node::Type::Identifier: {
        auto sexp = static_cast<IdentifierNode *>(m_identifier)->to_assignment_sexp(env);
        sexp->push(m_value->to_ruby(env));
        return sexp;
    }
    default:
        NAT_UNREACHABLE();
    }
}

ValuePtr AttrAssignNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "attrasgn"),
            m_receiver->to_ruby(env),
            SymbolValue::intern(env, m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

ValuePtr BeginNode::to_ruby(Env *env) {
    assert(m_body);
    auto *sexp = new SexpValue { env, this, { SymbolValue::intern(env, "rescue") } };
    if (!m_body->is_empty())
        sexp->push(m_body->without_unnecessary_nesting()->to_ruby(env));
    for (auto rescue_node : m_rescue_nodes) {
        sexp->push(rescue_node->to_ruby(env));
    }
    if (m_else_body)
        sexp->push(m_else_body->without_unnecessary_nesting()->to_ruby(env));
    if (m_ensure_body) {
        if (m_rescue_nodes.is_empty())
            (*sexp)[0] = SymbolValue::intern(env, "ensure");
        else
            sexp = new SexpValue { env, this, { SymbolValue::intern(env, "ensure"), sexp } };
        sexp->push(m_ensure_body->without_unnecessary_nesting()->to_ruby(env));
    }
    return sexp;
}

void BeginNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_body);
    visitor.visit(m_else_body);
    visitor.visit(m_ensure_body);
    for (auto node : m_rescue_nodes) {
        visitor.visit(node);
    }
}

Node *BeginRescueNode::name_to_node() {
    assert(m_name);
    return new AssignmentNode {
        token(),
        m_name,
        new IdentifierNode {
            new Token { Token::Type::GlobalVariable, "$!", file(), line(), column() },
            false },
    };
}

void BeginRescueNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_name);
    visitor.visit(m_body);
    for (auto node : m_exceptions) {
        visitor.visit(node);
    }
}

ValuePtr BeginRescueNode::to_ruby(Env *env) {
    auto array = new ArrayNode { token() };
    for (auto exception_node : m_exceptions) {
        array->add_node(exception_node);
    }
    if (m_name)
        array->add_node(name_to_node());
    auto *rescue_node = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "resbody"),
            array->to_ruby(env),
        }
    };
    for (auto node : *m_body->nodes()) {
        rescue_node->push(node->to_ruby(env));
    }
    return rescue_node;
}

ValuePtr BlockNode::to_ruby(Env *env) {
    return to_ruby_with_name(env, "block");
}

ValuePtr BlockNode::to_ruby_with_name(Env *env, const char *name) {
    auto *array = new SexpValue { env, this, { SymbolValue::intern(env, name) } };
    for (auto node : m_nodes) {
        array->push(node->to_ruby(env));
    }
    return array;
}

ValuePtr BlockPassNode::to_ruby(Env *env) {
    auto *sexp = new SexpValue { env, this, { SymbolValue::intern(env, "block_pass") } };
    sexp->push(m_node->to_ruby(env));
    return sexp;
}

ValuePtr BreakNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "break") } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

ValuePtr CallNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "call"),
            m_receiver->to_ruby(env),
            SymbolValue::intern(env, m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

ValuePtr CaseNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "case"),
            m_subject->to_ruby(env),
        }
    };
    for (auto when_node : m_when_nodes) {
        sexp->push(when_node->to_ruby(env));
    }
    if (m_else_node) {
        sexp->push(m_else_node->without_unnecessary_nesting()->to_ruby(env));
    } else {
        sexp->push(env->nil_obj());
    }
    return sexp;
}

ValuePtr CaseWhenNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "when"),
            m_condition->to_ruby(env),
        }
    };
    for (auto node : *m_body->nodes()) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr ClassNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "class"), SymbolValue::intern(env, m_name->name()), m_superclass->to_ruby(env) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

void ClassNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_name);
    visitor.visit(m_superclass);
    visitor.visit(m_body);
}

ValuePtr Colon2Node::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "colon2"),
            m_left->to_ruby(env),
            SymbolValue::intern(env, m_name),
        }
    };
}

ValuePtr Colon3Node::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "colon3"),
            SymbolValue::intern(env, m_name),
        }
    };
}

ValuePtr ConstantNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "const"),
            SymbolValue::intern(env, m_token->literal()),
        }
    };
}

ValuePtr DefinedNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "defined"),
            m_arg->to_ruby(env),
        }
    };
}

ValuePtr DefNode::to_ruby(Env *env) {
    SexpValue *sexp;
    if (m_self_node) {
        sexp = new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "defs"),
                m_self_node->to_ruby(env),
                SymbolValue::intern(env, m_name->name()),
                build_args_sexp(env),
            }
        };
    } else {
        sexp = new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "defn"),
                SymbolValue::intern(env, m_name->name()),
                build_args_sexp(env),
            }
        };
    }
    if (m_body->is_empty()) {
        sexp->push(new SexpValue { env, this, { SymbolValue::intern(env, "nil") } });
    } else {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

SexpValue *DefNode::build_args_sexp(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "args") } };
    if (m_args)
        for (auto arg : *m_args) {
            switch (arg->type()) {
            case Node::Type::Arg:
            case Node::Type::KeywordArg:
            case Node::Type::MultipleAssignment:
                sexp->push(arg->to_ruby(env));
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
    return sexp;
}

void DefNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_self_node);
    visitor.visit(m_name);
    visitor.visit(m_body);
    if (m_args)
        for (auto arg : *m_args) {
            visitor.visit(arg);
        }
}

ValuePtr EvaluateToStringNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "evstr"), m_node->to_ruby(env) } };
}

ValuePtr FalseNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "false") } };
}

ValuePtr HashNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "hash") } };
    for (auto node : m_nodes) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr IdentifierNode::to_ruby(Env *env) {
    switch (token_type()) {
    case Token::Type::BareName:
        if (m_is_lvar) {
            return new SexpValue { env, this, { SymbolValue::intern(env, "lvar"), SymbolValue::intern(env, name()) } };
        } else {
            return new SexpValue { env, this, { SymbolValue::intern(env, "call"), env->nil_obj(), SymbolValue::intern(env, name()) } };
        }
    case Token::Type::ClassVariable:
        return new SexpValue { env, this, { SymbolValue::intern(env, "cvar"), SymbolValue::intern(env, name()) } };
    case Token::Type::Constant:
        return new SexpValue { env, this, { SymbolValue::intern(env, "const"), SymbolValue::intern(env, name()) } };
    case Token::Type::GlobalVariable: {
        auto ref = nth_ref();
        if (ref > 0)
            return new SexpValue { env, this, { SymbolValue::intern(env, "nth_ref"), ValuePtr { env, ref } } };
        else
            return new SexpValue { env, this, { SymbolValue::intern(env, "gvar"), SymbolValue::intern(env, name()) } };
    }
    case Token::Type::InstanceVariable:
        return new SexpValue { env, this, { SymbolValue::intern(env, "ivar"), SymbolValue::intern(env, name()) } };
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

SexpValue *IdentifierNode::to_assignment_sexp(Env *env) {
    return new SexpValue {
        env,
        this,
        { assignment_type(env), SymbolValue::intern(env, name()) }
    };
}

ValuePtr IfNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "if"),
            m_condition->to_ruby(env),
            m_true_expr->to_ruby(env),
            m_false_expr->to_ruby(env),
        }
    };
}

ValuePtr IterNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "iter"),
            m_call->to_ruby(env),
        }
    };
    if (m_args->is_empty())
        sexp->push(ValuePtr { env, 0 });
    else
        sexp->push(build_args_sexp(env));
    if (!m_body->is_empty()) {
        if (m_body->has_one_node())
            sexp->push((*m_body->nodes())[0]->to_ruby(env));
        else
            sexp->push(m_body->to_ruby(env));
    }
    return sexp;
}

SexpValue *IterNode::build_args_sexp(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "args") } };
    if (m_args)
        for (auto arg : *m_args) {
            switch (arg->type()) {
            case Node::Type::Arg:
            case Node::Type::KeywordArg:
            case Node::Type::MultipleAssignment:
                sexp->push(arg->to_ruby(env));
                break;
            default:
                NAT_UNREACHABLE();
            }
        }
    return sexp;
}

ValuePtr InterpolatedRegexpNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "dregx") } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr InterpolatedShellNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "dxstr") } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr InterpolatedStringNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "dstr") } };
    for (size_t i = 0; i < m_nodes.size(); i++) {
        auto node = m_nodes[i];
        if (i == 0 && node->type() == Node::Type::String)
            sexp->push(static_cast<StringNode *>(node)->value());
        else
            sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr KeywordArgNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "kwarg"),
            SymbolValue::intern(env, m_name),
        }
    };
    if (m_value)
        sexp->push(m_value->to_ruby(env));
    return sexp;
}

ValuePtr LiteralNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

void LiteralNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_value);
}

ValuePtr LogicalAndNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "and"),
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

ValuePtr LogicalOrNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "or"),
            m_left->to_ruby(env),
            m_right->to_ruby(env),
        }
    };

    return sexp;
}

ValuePtr MatchNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, m_regexp_on_left ? "match2" : "match3"),
            m_regexp->to_ruby(env),
            m_arg->to_ruby(env),
        }
    };
}

void MatchNode::visit_children(Visitor &visitor) {
    Node::visit_children(visitor);
    visitor.visit(m_regexp);
    visitor.visit(m_arg);
}

ValuePtr ModuleNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "module"), SymbolValue::intern(env, m_name->name()) } };
    if (!m_body->is_empty()) {
        for (auto node : *(m_body->nodes())) {
            sexp->push(node->to_ruby(env));
        }
    }
    return sexp;
}

ValuePtr MultipleAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "masgn") } };
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Arg:
        case Node::Type::MultipleAssignment:
            sexp->push(node->to_ruby(env));
            break;
        default:
            printf("node = %s\n", node->to_ruby(env)->inspect_str(env));
            NAT_NOT_YET_IMPLEMENTED(); // maybe not needed?
        }
    }
    return sexp;
}

void MultipleAssignmentNode::add_locals(Env *env, Vector<SymbolValue *> *locals) {
    for (auto node : m_nodes) {
        switch (node->type()) {
        case Node::Type::Identifier: {
            auto identifier = static_cast<IdentifierNode *>(node);
            identifier->add_to_locals(env, locals);
            break;
        }
        case Node::Type::SplatAssignment: {
            auto splat = static_cast<SplatAssignmentNode *>(node);
            if (splat->node())
                splat->node()->add_to_locals(env, locals);
            break;
        }
        case Node::Type::MultipleAssignment:
            static_cast<MultipleAssignmentNode *>(node)->add_locals(env, locals);
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
}

ArrayValue *MultipleAssignmentNode::to_ruby_with_array(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "masgn") } };
    auto array = new SexpValue { env, this, { SymbolValue::intern(env, "array") } };
    for (auto identifier : m_nodes) {
        switch (identifier->type()) {
        case Node::Type::Identifier:
            array->push(static_cast<IdentifierNode *>(identifier)->to_assignment_sexp(env));
            break;
        case Node::Type::SplatAssignment: {
            array->push(identifier->to_ruby(env));
            break;
        }
        case Node::Type::MultipleAssignment:
            array->push(static_cast<MultipleAssignmentNode *>(identifier)->to_ruby_with_array(env));
            break;
        default:
            NAT_UNREACHABLE();
        }
    }
    sexp->push(array);
    return sexp;
}

ValuePtr NextNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "next") } };
    if (m_arg)
        sexp->push(m_arg->to_ruby(env));
    return sexp;
}

ValuePtr NilNode::to_ruby(Env *env) {
    return env->nil_obj();
}

ValuePtr NilSexpNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "nil") } };
}

ValuePtr NotNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "not"), m_expression->to_ruby(env) } };
}

ValuePtr OpAssignNode::to_ruby(Env *env) {
    auto sexp = m_name->to_assignment_sexp(env);
    assert(m_op);
    auto call = new CallNode { token(), m_name, m_op->c_str() };
    call->add_arg(m_value);
    sexp->push(call->to_ruby(env));
    return sexp;
}

ValuePtr OpAssignAccessorNode::to_ruby(Env *env) {
    if (*m_message == "[]=") {
        auto arg_list = new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "arglist"),
            }
        };
        for (auto arg : m_args) {
            arg_list->push(arg->to_ruby(env));
        }
        return new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "op_asgn1"),
                m_receiver->to_ruby(env),
                arg_list,
                SymbolValue::intern(env, m_op),
                m_value->to_ruby(env),
            }
        };
    } else {
        assert(m_args.is_empty());
        return new SexpValue {
            env,
            this,
            {
                SymbolValue::intern(env, "op_asgn2"),
                m_receiver->to_ruby(env),
                SymbolValue::intern(env, m_message),
                SymbolValue::intern(env, m_op),
                m_value->to_ruby(env),
            }
        };
    }
}

ValuePtr OpAssignAndNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "op_asgn_and"),
            m_name->to_ruby(env),
            (new AssignmentNode { token(), m_name, m_value })->to_ruby(env),
        },
    };
}

ValuePtr OpAssignOrNode::to_ruby(Env *env) {
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "op_asgn_or"),
            m_name->to_ruby(env),
            (new AssignmentNode { token(), m_name, m_value })->to_ruby(env),
        },
    };
}

ValuePtr RangeNode::to_ruby(Env *env) {
    if (m_first->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_first)->value_type() == Value::Type::Integer && m_last->type() == Node::Type::Literal && static_cast<LiteralNode *>(m_last)->value_type() == Value::Type::Integer) {
        return new SexpValue {
            env,
            this,
            { SymbolValue::intern(env, "lit"), new RangeValue { env, static_cast<LiteralNode *>(m_first)->value(), static_cast<LiteralNode *>(m_last)->value(), m_exclude_end } }
        };
    }
    return new SexpValue {
        env,
        this,
        { SymbolValue::intern(env, m_exclude_end ? "dot3" : "dot2"), m_first->to_ruby(env), m_last->to_ruby(env) }
    };
}

ValuePtr RegexpNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

ValuePtr ReturnNode::to_ruby(Env *env) {
    if (m_arg) {
        return new SexpValue { env, this, { SymbolValue::intern(env, "return"), m_arg->to_ruby(env) } };
    }
    return new SexpValue { env, this, { SymbolValue::intern(env, "return") } };
}

ValuePtr SafeCallNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "safe_call"),
            m_receiver->to_ruby(env),
            SymbolValue::intern(env, m_message),
        }
    };

    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

ValuePtr SelfNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "self") } };
}

ValuePtr SclassNode::to_ruby(Env *env) {
    auto sexp = new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "sclass"),
            m_klass->to_ruby(env),
        }
    };
    for (auto node : *m_body->nodes()) {
        sexp->push(node->to_ruby(env));
    }
    return sexp;
}

ValuePtr ShellNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "xstr"), m_value } };
}

ValuePtr SplatAssignmentNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "splat") } };
    if (m_node)
        sexp->push(m_node->to_assignment_sexp(env));
    return sexp;
}

ValuePtr SplatNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "splat") } };
    if (m_node)
        sexp->push(m_node->to_ruby(env));
    return sexp;
}

ValuePtr StabbyProcNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lambda") } };
}

ValuePtr StringNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "str"), m_value } };
}

ValuePtr SymbolNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "lit"), m_value } };
}

ValuePtr TrueNode::to_ruby(Env *env) {
    return new SexpValue { env, this, { SymbolValue::intern(env, "true") } };
}

ValuePtr SuperNode::to_ruby(Env *env) {
    if (empty_parens()) {
        return new SexpValue { env, this, { SymbolValue::intern(env, "super") } };
    } else if (m_args.is_empty()) {
        return new SexpValue { env, this, { SymbolValue::intern(env, "zsuper") } };
    }
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "super") } };
    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

ValuePtr UntilNode::to_ruby(Env *env) {
    auto sexp = WhileNode::to_ruby(env);
    (*sexp->as_array())[0] = SymbolValue::intern(env, "until");
    return sexp;
}

ValuePtr WhileNode::to_ruby(Env *env) {
    ValuePtr is_pre, body;
    if (m_pre)
        is_pre = env->true_obj();
    else
        is_pre = env->false_obj();
    if (m_body->is_empty())
        body = env->nil_obj();
    else
        body = m_body->without_unnecessary_nesting()->to_ruby(env);
    return new SexpValue {
        env,
        this,
        {
            SymbolValue::intern(env, "while"),
            m_condition->to_ruby(env),
            body,
            is_pre,
        }
    };
}

ValuePtr YieldNode::to_ruby(Env *env) {
    auto sexp = new SexpValue { env, this, { SymbolValue::intern(env, "yield") } };
    if (m_args.is_empty())
        return sexp;
    for (auto arg : m_args) {
        sexp->push(arg->to_ruby(env));
    }
    return sexp;
}

}
