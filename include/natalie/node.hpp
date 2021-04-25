#pragma once

#include <string>

#include "natalie/gc.hpp"
#include "natalie/token.hpp"

namespace Natalie {

struct Node : public Cell {
    enum class Type {
        Alias,
        Arg,
        Array,
        Assignment,
        AttrAssign,
        Begin,
        BeginRescue,
        Block,
        BlockPass,
        Break,
        Call,
        Case,
        CaseWhen,
        Class,
        Colon2,
        Colon3,
        Constant,
        Def,
        Defined,
        EvaluateToString,
        False,
        Hash,
        Identifier,
        If,
        Iter,
        InterpolatedRegexp,
        InterpolatedShell,
        InterpolatedString,
        KeywordArg,
        Literal,
        LogicalAnd,
        LogicalOr,
        Match,
        Module,
        MultipleAssignment,
        Next,
        Nil,
        NilSexp,
        Not,
        OpAssign,
        OpAssignAccessor,
        OpAssignAnd,
        OpAssignOr,
        Range,
        Regexp,
        Return,
        SafeCall,
        Sclass,
        Self,
        Splat,
        SplatAssignment,
        StabbyProc,
        String,
        Super,
        Symbol,
        True,
        Until,
        While,
        Yield,
    };

    Node(Token *token)
        : m_token { token } { }

    virtual ValuePtr to_ruby(Env *env) {
        NAT_UNREACHABLE();
    }

    virtual Type type() { NAT_UNREACHABLE(); }

    virtual bool is_callable() { return false; }

    const char *file() { return m_token->file(); }
    size_t line() { return m_token->line(); }
    size_t column() { return m_token->column(); }

    Token *token() { return m_token; }

protected:
    Token *m_token { nullptr };
};

struct NodeWithArgs : Node {
    NodeWithArgs(Token *token)
        : Node { token } { }

    void add_arg(Node *arg) {
        m_args.push(arg);
    }

    Vector<Node *> *args() { return &m_args; }

protected:
    Vector<Node *> m_args {};
};

struct SymbolNode;

struct AliasNode : Node {
    AliasNode(Token *token, SymbolNode *new_name, SymbolNode *existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } { }

    virtual Type type() override { return Type::Alias; }

    virtual ValuePtr to_ruby(Env *) override;

private:
    SymbolNode *m_new_name { nullptr };
    SymbolNode *m_existing_name { nullptr };
};

struct ArgNode : Node {
    ArgNode(Token *token)
        : Node { token } { }

    ArgNode(Token *token, const char *name)
        : Node { token }
        , m_name { name } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Arg; }

    virtual ValuePtr to_ruby(Env *) override;

    const char *name() { return m_name; }

    bool splat() { return m_splat; }
    void set_splat(bool splat) { m_splat = splat; }

    bool block_arg() { return m_block_arg; }
    void set_block_arg(bool block_arg) { m_block_arg = block_arg; }

    Node *value() { return m_value; }
    void set_value(Node *value) { m_value = value; }

    void add_to_locals(Env *env, Vector<SymbolValue *> *locals) {
        locals->push(SymbolValue::intern(env, m_name));
    }

protected:
    const char *m_name { nullptr };
    bool m_block_arg { false };
    bool m_splat { false };
    Node *m_value { nullptr };
};

struct ArrayNode : Node {
    ArrayNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Array; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> *nodes() { return &m_nodes; }

protected:
    Vector<Node *> m_nodes {};
};

struct BlockPassNode : Node {
    BlockPassNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::BlockPass; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_node { nullptr };
};

struct BreakNode : NodeWithArgs {
    BreakNode(Token *token, Node *arg = nullptr)
        : NodeWithArgs { token }
        , m_arg { arg } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Break; }

protected:
    Node *m_arg { nullptr };
};

struct IdentifierNode;

struct AssignmentNode : Node {
    AssignmentNode(Token *token, Node *identifier, Node *value)
        : Node { token }
        , m_identifier { identifier }
        , m_value { value } {
        assert(m_identifier);
        assert(m_value);
    }

    virtual Type type() override { return Type::Assignment; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_identifier { nullptr };
    Node *m_value { nullptr };
};

struct BlockNode;
struct BeginRescueNode;

struct BeginNode : Node {
    BeginNode(Token *token, BlockNode *body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    virtual Type type() override { return Type::Begin; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }
    bool no_rescue_nodes() { return m_rescue_nodes.size() == 0; }

    bool has_ensure_body() { return m_ensure_body ? true : false; }
    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

    BlockNode *body() { return m_body; }

protected:
    BlockNode *m_body { nullptr };
    BlockNode *m_else_body { nullptr };
    BlockNode *m_ensure_body { nullptr };
    Vector<BeginRescueNode *> m_rescue_nodes {};
};

struct BeginRescueNode : Node {
    BeginRescueNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::BeginRescue; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_exception_node(Node *node) {
        m_exceptions.push(node);
    }

    void set_exception_name(IdentifierNode *name) {
        m_name = name;
    }

    void set_body(BlockNode *body) { m_body = body; }

    Node *name_to_node();

protected:
    IdentifierNode *m_name { nullptr };
    Vector<Node *> m_exceptions {};
    BlockNode *m_body { nullptr };
};

struct BlockNode : Node {
    BlockNode(Token *token)
        : Node { token } { }

    BlockNode(Token *token, Node *single_node)
        : Node { token } {
        add_node(single_node);
    }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual Type type() override { return Type::Block; }

    virtual ValuePtr to_ruby(Env *) override;
    ValuePtr to_ruby_with_name(Env *, const char *);

    Vector<Node *> *nodes() { return &m_nodes; }
    bool is_empty() { return m_nodes.is_empty(); }

    bool has_one_node() { return m_nodes.size() == 1; }

    Node *without_unnecessary_nesting() {
        if (has_one_node())
            return m_nodes[0];
        else
            return this;
    }

protected:
    Vector<Node *> m_nodes {};
};

struct CallNode : NodeWithArgs {
    CallNode(Token *token, Node *receiver, const char *message)
        : NodeWithArgs { token }
        , m_receiver { receiver }
        , m_message { message } {
        assert(m_receiver);
        assert(m_message);
    }

    CallNode(Token *token, CallNode &node)
        : NodeWithArgs { token }
        , m_receiver { node.m_receiver }
        , m_message { node.m_message } {
        for (auto arg : node.m_args) {
            add_arg(arg);
        }
    }

    virtual Type type() override { return Type::Call; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual bool is_callable() override { return true; }

    Node *receiver() { return m_receiver; }

    const char *message() { return m_message; }
    void set_message(const char *message) { m_message = message; }

protected:
    Node *m_receiver { nullptr };
    const char *m_message { nullptr };
};

struct CaseNode : Node {
    CaseNode(Token *token, Node *subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    virtual Type type() override { return Type::Case; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_when_node(Node *node) {
        m_when_nodes.push(node);
    }

    void set_else_node(BlockNode *node) {
        m_else_node = node;
    }

protected:
    Node *m_subject { nullptr };
    Vector<Node *> m_when_nodes {};
    BlockNode *m_else_node { nullptr };
};

struct CaseWhenNode : Node {
    CaseWhenNode(Token *token, Node *condition, BlockNode *body)
        : Node { token }
        , m_condition { condition }
        , m_body { body } {
        assert(m_condition);
        assert(m_body);
    }

    virtual Type type() override { return Type::CaseWhen; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
};

struct AttrAssignNode : CallNode {
    AttrAssignNode(Token *token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    AttrAssignNode(Token *token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::AttrAssign; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct SafeCallNode : CallNode {
    SafeCallNode(Token *token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    SafeCallNode(Token *token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::SafeCall; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct ConstantNode;

struct ClassNode : Node {
    ClassNode(Token *token, ConstantNode *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } { }

    virtual Type type() override { return Type::Class; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    ConstantNode *m_name { nullptr };
    Node *m_superclass { nullptr };
    BlockNode *m_body { nullptr };
};

struct Colon2Node : Node {
    Colon2Node(Token *token, Node *left, const char *name)
        : Node { token }
        , m_left { left }
        , m_name { name } {
        assert(m_left);
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon2; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_left { nullptr };
    const char *m_name { nullptr };
};

struct Colon3Node : Node {
    Colon3Node(Token *token, const char *name)
        : Node { token }
        , m_name { name } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon3; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    const char *m_name { nullptr };
};

struct ConstantNode : Node {
    ConstantNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Constant; }

    virtual ValuePtr to_ruby(Env *) override;

    const char *name() { return m_token->literal(); }
};

struct LiteralNode : Node {
    LiteralNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Literal; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }
    Value::Type value_type() { return m_value->type(); }

protected:
    ValuePtr m_value { nullptr };
};

struct DefinedNode : Node {
    DefinedNode(Token *token, Node *arg)
        : Node { token }
        , m_arg { arg } {
        assert(arg);
    }

    virtual Type type() override { return Type::Defined; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_arg { nullptr };
};

struct DefNode : Node {
    DefNode(Token *token, Node *self_node, IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_self_node { self_node }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    DefNode(Token *token, IdentifierNode *name, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    virtual Type type() override { return Type::Def; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    SexpValue *build_args_sexp(Env *);

    Node *m_self_node { nullptr };
    IdentifierNode *m_name { nullptr };
    Vector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

struct EvaluateToStringNode : Node {
    EvaluateToStringNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::EvaluateToString; }

protected:
    Node *m_node { nullptr };
};

struct FalseNode : Node {
    FalseNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::False; }
};

struct HashNode : Node {
    HashNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Array; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_node(Node *node) {
        m_nodes.push(node);
    }

protected:
    Vector<Node *> m_nodes {};
};

struct IdentifierNode : Node {
    IdentifierNode(Token *token, bool is_lvar)
        : Node { token }
        , m_is_lvar { is_lvar } { }

    virtual Type type() override { return Type::Identifier; }

    virtual ValuePtr to_ruby(Env *) override;

    Token::Type token_type() { return m_token->type(); }

    const char *name() { return m_token->literal(); }

    void append_to_name(char c) {
        m_token->set_literal(std::string(m_token->literal()) + c);
    }

    virtual bool is_callable() override {
        switch (token_type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            return !m_is_lvar;
        default:
            return false;
        }
    }

    bool is_lvar() { return m_is_lvar; }
    void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

    nat_int_t nth_ref() {
        auto str = name();
        size_t len = strlen(str);
        if (strcmp(str, "$0") == 0)
            return 0;
        if (len <= 1)
            return 0;
        int ref = 0;
        for (size_t i = 1; i < len; i++) {
            char c = str[i];
            if (i == 1 && c == '0')
                return 0;
            int num = c - 48;
            if (num < 0 || num > 9)
                return 0;
            ref *= 10;
            ref += num;
        }
        return ref;
    }

    SexpValue *to_assignment_sexp(Env *);

    SymbolValue *assignment_type(Env *env) {
        switch (token_type()) {
        case Token::Type::BareName:
            return SymbolValue::intern(env, "lasgn");
        case Token::Type::ClassVariable:
            return SymbolValue::intern(env, "cvdecl");
        case Token::Type::Constant:
            return SymbolValue::intern(env, "cdecl");
        case Token::Type::GlobalVariable:
            return SymbolValue::intern(env, "gasgn");
        case Token::Type::InstanceVariable:
            return SymbolValue::intern(env, "iasgn");
        default:
            NAT_UNREACHABLE();
        }
    }

    SymbolValue *to_symbol(Env *env) {
        return SymbolValue::intern(env, name());
    }

    void add_to_locals(Env *env, Vector<SymbolValue *> *locals) {
        if (token_type() == Token::Type::BareName)
            locals->push(to_symbol(env));
    }

protected:
    bool m_is_lvar { false };
};

struct IfNode : Node {
    IfNode(Token *token, Node *condition, Node *true_expr, Node *false_expr)
        : Node { token }
        , m_condition { condition }
        , m_true_expr { true_expr }
        , m_false_expr { false_expr } {
        assert(m_condition);
        assert(m_true_expr);
        assert(m_false_expr);
    }

    virtual Type type() override { return Type::If; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_condition { nullptr };
    Node *m_true_expr { nullptr };
    Node *m_false_expr { nullptr };
};

struct IterNode : Node {
    IterNode(Token *token, Node *call, Vector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_call { call }
        , m_args { args }
        , m_body { body } {
        assert(m_call);
        assert(m_args);
        assert(m_body);
    }

    virtual Type type() override { return Type::Iter; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    SexpValue *build_args_sexp(Env *);

    Node *m_call { nullptr };
    Vector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

struct InterpolatedNode : Node {
    InterpolatedNode(Token *token)
        : Node { token } { }

    void add_node(Node *node) { m_nodes.push(node); };

protected:
    Vector<Node *> m_nodes {};
};

struct InterpolatedRegexpNode : InterpolatedNode {
    InterpolatedRegexpNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedRegexp; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct InterpolatedShellNode : InterpolatedNode {
    InterpolatedShellNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedShell; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct InterpolatedStringNode : InterpolatedNode {
    InterpolatedStringNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedString; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct KeywordArgNode : ArgNode {
    KeywordArgNode(Token *token, const char *name)
        : ArgNode { token, name } { }

    virtual Type type() override { return Type::KeywordArg; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct LogicalAndNode : Node {
    LogicalAndNode(Token *token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalAnd; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *left() { return m_left; }
    Node *right() { return m_right; }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

struct LogicalOrNode : Node {
    LogicalOrNode(Token *token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalOr; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *left() { return m_left; }
    Node *right() { return m_right; }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

struct RegexpNode;

struct MatchNode : Node {
    MatchNode(Token *token, RegexpNode *regexp, Node *arg, bool regexp_on_left)
        : Node { token }
        , m_regexp { regexp }
        , m_arg { arg }
        , m_regexp_on_left { regexp_on_left } { }

    virtual Type type() override { return Type::Match; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    RegexpNode *m_regexp { nullptr };
    Node *m_arg { nullptr };
    bool m_regexp_on_left { false };
};

struct ModuleNode : Node {
    ModuleNode(Token *token, ConstantNode *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    virtual Type type() override { return Type::Module; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    ConstantNode *m_name { nullptr };
    BlockNode *m_body { nullptr };
};

struct MultipleAssignmentNode : ArrayNode {
    MultipleAssignmentNode(Token *token)
        : ArrayNode { token } { }

    virtual Type type() override { return Type::MultipleAssignment; }

    virtual ValuePtr to_ruby(Env *) override;
    ArrayValue *to_ruby_with_array(Env *);

    void add_locals(Env *, Vector<SymbolValue *> *);
};

struct NextNode : Node {
    NextNode(Token *token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Next; }

protected:
    Node *m_arg { nullptr };
};

struct NilNode : Node {
    NilNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Nil; }
};

struct NotNode : Node {
    NotNode(Token *token, Node *expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Not; }

protected:
    Node *m_expression { nullptr };
};

struct NilSexpNode : Node {
    NilSexpNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::NilSexp; }
};

struct OpAssignNode : Node {
    OpAssignNode(Token *token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(Token *token, const char *op, IdentifierNode *name, Node *value)
        : Node { token }
        , m_op { op }
        , m_name { name }
        , m_value { value } {
        assert(m_op);
        assert(m_name);
        assert(m_value);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssign; }

protected:
    const char *m_op { nullptr };
    IdentifierNode *m_name { nullptr };
    Node *m_value { nullptr };
};

struct OpAssignAccessorNode : NodeWithArgs {
    OpAssignAccessorNode(Token *token, const char *op, Node *receiver, const char *message, Node *value)
        : NodeWithArgs { token }
        , m_op { op }
        , m_receiver { receiver }
        , m_message { message }
        , m_value { value } {
        assert(m_op);
        assert(m_receiver);
        assert(m_message);
        assert(m_value);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignAccessor; }

protected:
    const char *m_op { nullptr };
    Node *m_receiver { nullptr };
    const char *m_message { nullptr };
    Node *m_value { nullptr };
};

struct OpAssignAndNode : OpAssignNode {
    OpAssignAndNode(Token *token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignAnd; }
};

struct OpAssignOrNode : OpAssignNode {
    OpAssignOrNode(Token *token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignOr; }
};

struct RangeNode : Node {
    RangeNode(Token *token, Node *first, Node *last, bool exclude_end)
        : Node { token }
        , m_first { first }
        , m_last { last }
        , m_exclude_end { exclude_end } {
        assert(m_first);
        assert(m_last);
    }

    virtual Type type() override { return Type::Range; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_first { nullptr };
    Node *m_last { nullptr };
    bool m_exclude_end { false };
};

struct RegexpNode : Node {
    RegexpNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Regexp; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

protected:
    ValuePtr m_value { nullptr };
};

struct ReturnNode : Node {
    ReturnNode(Token *token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual Type type() override { return Type::Return; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_arg { nullptr };
};

struct SclassNode : Node {
    SclassNode(Token *token, Node *klass, BlockNode *body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    virtual Type type() override { return Type::Sclass; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    Node *m_klass { nullptr };
    BlockNode *m_body { nullptr };
};

struct SelfNode : Node {
    SelfNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Self; }

    virtual ValuePtr to_ruby(Env *) override;
};

struct ShellNode : Node {
    ShellNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::String; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

protected:
    ValuePtr m_value { nullptr };
};

struct SplatAssignmentNode : Node {
    SplatAssignmentNode(Token *token)
        : Node { token } { }

    SplatAssignmentNode(Token *token, IdentifierNode *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::SplatAssignment; }

    virtual ValuePtr to_ruby(Env *) override;

    IdentifierNode *node() { return m_node; }

protected:
    IdentifierNode *m_node { nullptr };
};

struct SplatNode : Node {
    SplatNode(Token *token)
        : Node { token } { }

    SplatNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::Splat; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *node() { return m_node; }

protected:
    Node *m_node { nullptr };
};

struct StabbyProcNode : Node {
    StabbyProcNode(Token *token, Vector<Node *> *args)
        : Node { token }
        , m_args { args } {
        assert(m_args);
    }

    virtual Type type() override { return Type::StabbyProc; }

    virtual ValuePtr to_ruby(Env *) override;

    Vector<Node *> *args() { return m_args; };

protected:
    Vector<Node *> *m_args { nullptr };
};

struct StringNode : Node {
    StringNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::String; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

protected:
    ValuePtr m_value { nullptr };
};

struct SymbolNode : Node {
    SymbolNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Symbol; }

    virtual ValuePtr to_ruby(Env *) override;

protected:
    ValuePtr m_value { nullptr };
};

struct TrueNode : Node {
    TrueNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::True; }
};

struct SuperNode : NodeWithArgs {
    SuperNode(Token *token)
        : NodeWithArgs { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Super; }

    bool parens() { return m_parens; }
    void set_parens(bool parens) { m_parens = parens; }

    bool empty_parens() { return m_parens && m_args.is_empty(); }

protected:
    bool m_parens { false };
};

struct WhileNode : Node {
    WhileNode(Token *token, Node *condition, BlockNode *body, bool pre)
        : Node { token }
        , m_condition { condition }
        , m_body { body }
        , m_pre { pre } {
        assert(m_condition);
        assert(m_body);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::While; }

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
    bool m_pre { false };
};

struct UntilNode : WhileNode {
    UntilNode(Token *token, Node *condition, BlockNode *body, bool pre)
        : WhileNode { token, condition, body, pre } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Until; }
};

struct YieldNode : NodeWithArgs {
    YieldNode(Token *token)
        : NodeWithArgs { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Yield; }
};

}
