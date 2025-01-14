/*
 * Copyright (c) 2023, Dan Klishch <danilklishch@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Parser/CppASTConverter.h"
#include "Function.h"
#include "Parser/SpecParser.h"

namespace JSSpecCompiler {

NonnullRefPtr<FunctionDefinition> CppASTConverter::convert()
{
    StringView name = m_function->name()->full_name();

    Vector<Tree> toplevel_statements;
    for (auto const& statement : m_function->definition()->statements()) {
        auto maybe_tree = as_nullable_tree(statement);
        if (maybe_tree)
            toplevel_statements.append(maybe_tree.release_nonnull());
    }
    auto tree = make_ref_counted<TreeList>(move(toplevel_statements));

    return make_ref_counted<FunctionDefinition>(name, tree);
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::VariableDeclaration const& variable_declaration)
{
    static Tree variable_declaration_present_error
        = make_ref_counted<ErrorNode>("Encountered variable declaration with initial value"sv);

    if (variable_declaration.initial_value() != nullptr)
        return variable_declaration_present_error;
    return nullptr;
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::ReturnStatement const& return_statement)
{
    return make_ref_counted<ReturnNode>(as_tree(return_statement.value()));
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::FunctionCall const& function_call)
{
    Vector<Tree> arguments;
    for (auto const& argument : function_call.arguments())
        arguments.append(as_tree(argument));

    return make_ref_counted<FunctionCall>(as_tree(function_call.callee()), move(arguments));
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::Name const& name)
{
    return make_ref_counted<UnresolvedReference>(name.full_name());
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::IfStatement const& if_statement)
{
    // NOTE: This is so complicated since we probably want to test IfBranchMergingPass, which
    //       expects standalone `IfBranch` and `ElseIfBranch` nodes.

    Vector<Tree> trees;
    Cpp::IfStatement const* current = &if_statement;

    while (true) {
        auto predicate = as_tree(current->predicate());
        auto then_branch = as_possibly_empty_tree(current->then_statement());

        if (trees.is_empty())
            trees.append(make_ref_counted<IfBranch>(predicate, then_branch));
        else
            trees.append(make_ref_counted<ElseIfBranch>(predicate, then_branch));

        auto else_statement = dynamic_cast<Cpp::IfStatement const*>(current->else_statement());
        if (else_statement)
            current = else_statement;
        else
            break;
    }

    auto else_statement = current->else_statement();
    if (else_statement)
        trees.append(make_ref_counted<ElseIfBranch>(
            nullptr, as_possibly_empty_tree(else_statement)));

    return make_ref_counted<TreeList>(move(trees));
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::BlockStatement const& block)
{
    Vector<Tree> statements;
    for (auto const& statement : block.statements()) {
        auto maybe_tree = as_nullable_tree(statement);
        if (maybe_tree)
            statements.append(maybe_tree.release_nonnull());
    }
    return make_ref_counted<TreeList>(move(statements));
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::AssignmentExpression const& assignment)
{
    // NOTE: Later stages of the compilation process basically treat `BinaryOperator::Declaration`
    //       the same as `BinaryOperator::Assignment`, so variable shadowing is impossible. The only
    //       difference in their semantics is that "declarations" define names of local variables.
    //       Since we are effectively ignoring actual C++ variable declarations, we need to define
    //       locals somewhere else. Using "declarations" instead of "assignments" here does this job
    //       cleanly.
    return make_ref_counted<BinaryOperation>(
        BinaryOperator::Declaration, as_tree(assignment.lhs()), as_tree(assignment.rhs()));
}

template<>
NullableTree CppASTConverter::convert_node(Cpp::NumericLiteral const& literal)
{
    // TODO: Numerical literals are not limited to i64.
    return make_ref_counted<MathematicalConstant>(literal.value().to_int<i64>().value());
}

NullableTree CppASTConverter::as_nullable_tree(Cpp::Statement const* statement)
{
    static Tree unknown_ast_node_error
        = make_ref_counted<ErrorNode>("Encountered unknown C++ AST node"sv);

    Optional<NullableTree> result;

    auto dispatch_convert_if_one_of = [&]<typename... Ts> {
        (([&]<typename T> {
            if (result.has_value())
                return;
            auto casted_ptr = dynamic_cast<T const*>(statement);
            if (casted_ptr != nullptr)
                result = convert_node<T>(*casted_ptr);
        }).template operator()<Ts>(),
            ...);
    };

    dispatch_convert_if_one_of.operator()<
        Cpp::VariableDeclaration,
        Cpp::ReturnStatement,
        Cpp::FunctionCall,
        Cpp::Name,
        Cpp::IfStatement,
        Cpp::BlockStatement,
        Cpp::AssignmentExpression,
        Cpp::NumericLiteral>();

    if (result.has_value())
        return *result;
    return unknown_ast_node_error;
}

Tree CppASTConverter::as_tree(Cpp::Statement const* statement)
{
    static Tree empty_tree_error
        = make_ref_counted<ErrorNode>("AST conversion unexpectedly produced empty tree"sv);

    auto result = as_nullable_tree(statement);
    if (result)
        return result.release_nonnull();
    return empty_tree_error;
}

Tree CppASTConverter::as_possibly_empty_tree(Cpp::Statement const* statement)
{
    auto result = as_nullable_tree(statement);
    if (result)
        return result.release_nonnull();
    return make_ref_counted<TreeList>(Vector<Tree> {});
}

}
