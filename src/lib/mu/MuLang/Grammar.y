%{
/* -*- mode: C++ -*-
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0 
// 
*/

#ifdef yyerror
#undef yyerror
#endif
#define yyerror(state, MSG) ParseError(state, MSG)
#define IN_GRAMMAR

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <MuLang/DynamicArrayType.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/FlexLexer.h>
#include <MuLang/MuLangContext.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Class.h>
#include <Mu/Construct.h>
#include <Mu/Context.h>
#include <Mu/Environment.h>
#include <Mu/FreeVariable.h>
#include <Mu/Function.h>
#include <Mu/FunctionType.h>
#include <Mu/Interface.h>
#include <Mu/ListType.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/NodePrinter.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Signature.h>
#include <Mu/StackVariable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/TupleType.h>
#include <Mu/Type.h>
#include <Mu/TypeModifier.h>
#include <Mu/TypePattern.h>
#include <Mu/TypeVariable.h>
#include <Mu/VariantTagType.h>
#include <iostream>
#include <limits>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stl_ext/stl_ext_algo.h>
#include <string.h>

using namespace std;

extern void                 yyParseContext(Mu::NodeAssembler* c);
int                         yylex(void*, void*);
void                        ParseError(void*, const char *,...);
void                        ModuleLocationError(void*, const char *,...);
void                        OpError(void*, const char *,Mu::Node*,Mu::Node*);
void                        ParseWarning(void*, const char *,...);

const Mu::Function*         yyFunction(const Mu::Symbol*);
const Mu::Type*             yyConstantExpression(Mu::Node*);

typedef Mu::NodeAssembler::NodeList         NodeList;
typedef Mu::NodeAssembler::Initializer      Initializer;
typedef Mu::NodeAssembler::InitializerList  InitializerList;

// Name::Ref -> string, for error messages
const char* str(Mu::Name::Ref n) { return Mu::Name(n).c_str(); }

#define yyFlexLexer MUYYFlexLexer

#define AS (reinterpret_cast<yyFlexLexer*>(state)->assembler())
#define CONTEXT (static_cast<Mu::MuLangContext*>(AS->context()))


%}

%union
{
    int                             _token;
    Mu::int64                       _int;
    char                            _char;
    float                           _float;
    char                            _op[4];
    Mu::Object*                     _object;
    Mu::Name::Ref                   _name;
    Mu::Symbol*                     _symbol;
    Mu::Function*                   _function;
    Mu::Node*                       _node;
    Mu::NodeAssembler::NodeList     _nodeList;
    Mu::NodeAssembler::SymbolList   _symbolList;
    Mu::NodeAssembler::NameList     _nameList;
    Mu::NodeAssembler::Pattern*     _pattern;
    Mu::Type*                       _type;
    Mu::TypeModifier*               _typeModifier;
    Mu::Module*                     _module;
    Mu::Class*                      _class;
    Mu::Interface*                  _interface;
    Mu::StackVariable*              _stackVar;
    Mu::MemberVariable*             _memberVar;
    Mu::GlobalVariable*             _globalVar;
    Mu::Variable*                   _var;
    Mu::Construct*                  _construct;
    Mu::SymbolicConstant*           _symConstant;
    Mu::Node*                       _nodeTriple[3];
}

//
//  Expect 11 shift/reduce conflicts produced by:
//
//  1: unresolved function call versus symbolic assignment
//   symbolic_statement  ->  identifier . sym_assign_op symbol_type_or_module ';'
//   symbolic_statement  ->  identifier . sym_assign_op value_expression ';'
//   function_call  ->  identifier . '(' argument_slot_list_opt ')'
//
//  2: unresolved function call versus unresolved variable reference
//   function_call  ->  identifier . '(' argument_slot_list_opt ')'
//   primary_expression  ->  identifier .
//
//  3: prefixed function call versus prefixed variable reference
//   function_call  ->  prefixed_symbol . '(' argument_list_opt ')' 
//   primary_expression  ->  prefixed_symbol .
//
//  4: function call versus variable reference
//   function_call  ->  symbol . '(' argument_slot_list_opt ')'
//   primary_expression  ->  symbol .
//
//  5: function call versus member reference
//   function_call  ->  value_prefix symbol . '(' @1 argument_list_opt ')'
//   member_variable  ->  value_prefix symbol .
//
//  6-11: conditional expression versus conditional statements
//   if_statement  ->  IF '(' expression ')' . statement 
//   if_statement  ->  IF '(' expression ')' . statement ELSE statement
//   primary_expression  ->  '(' expression ')' .
//
//  14: case pattern which accepts an empty type_constructor
//   pattern  ->  type_constructor
//   pattern  ->  type_constructor pattern
//   
//  In once case, this stuff is being caused by disambiguation of a
//  symbol that needs to be referenced versus a function call and in
//  the other its related to an unresolved function call versus an
//  unresolved reference. A fix would mean delaying the recognition of
//  the reference patterns (instead of in primary_expression) but
//  AFAIK would be much much more complex. Obviously shifting is the
//  correct way to resolve the problem anyway. (Just like the if-else
//  problem).
//
//  Note that the if/else shift/reduce conflict is resolved by using
//  %prec in the grammar, but is exacerbated by the conditional expression
//  using the same keywords. 
//
//%expect 11

//
//  Make it reentrant
//
%pure-parser
%parse-param { void * state }
%lex-param { void * state }

%token <_construct>     MU_CONSTRUCT
%token <_float>         MU_FLOATCONST
%token <_int>           MU_BOOLCONST
%token <_int>           MU_CHARCONST
%token <_int>           MU_INTCONST
%token <_memberVar>     MU_MEMBERVARIABLE
%token <_module>        MU_MODULESYMBOL
%token <_name>          MU_IDENTIFIER
%token <_object>        MU_OBJECTCONST
%token <_object>        MU_STRINGCONST
%token <_op>            MU_EQUALS
%token <_op>            MU_GREATEREQUALS
%token <_op>            MU_LESSEQUALS
%token <_op>            MU_LOGICAND
%token <_op>            MU_LOGICOR
%token <_op>            MU_LSHIFT
%token <_op>            MU_NOTEQUALS
%token <_op>            MU_OP_ASSIGN
%token <_op>            MU_EQ
%token <_op>            MU_NEQ
%token <_op>            MU_OP_INC
%token <_op>            MU_OP_SYMASSIGN
%token <_op>            MU_RSHIFT
%token <_symConstant>   MU_SYMBOLICCONST
%token <_symbol>        MU_PREFIXEDSYMBOL
%token <_symbol>        MU_SYMBOL
%token <_symbol>        MU_VARIABLEATTRIBUTE
%token <_token>         MU_KIND_KEY
%token <_token>         MU_TYPE_KEY
%token <_token>         MU_ALIAS
%token <_token>         MU_ARROW
%token <_token>         MU_BREAK
%token <_token>         MU_CASE
%token <_token>         MU_CATCH
%token <_token>         MU_CLASS
%token <_token>         MU_CONST
%token <_token>         MU_CONTINUE
%token <_token>         MU_DO
%token <_token>         MU_DOCUMENTATION
%token <_token>         MU_DOUBLECOLON
%token <_token>         MU_DOUBLEDOT
%token <_token>         MU_ELLIPSIS
%token <_token>         MU_ELSE
%token <_token>         MU_FOR
%token <_token>         MU_FOREACH
%token <_token>         MU_FORINDEX
%token <_token>         MU_FUNCTION
%token <_token>         MU_GLOBAL
%token <_token>         MU_IF
%token <_token>         MU_INTERFACE
%token <_token>         MU_LAMBDA
%token <_token>         MU_LET
%token <_token>         MU_METHOD
%token <_token>         MU_MODULE
%token <_token>         MU_NIL
%token <_token>         MU_OPERATOR
%token <_token>         MU_QUADRUPLEDOT
%token <_token>         MU_DOUBLEARROW
%token <_token>         MU_REPEAT
%token <_token>         MU_REQUIRE
%token <_token>         MU_RETURN
%token <_token>         MU_TEMPLATE
%token <_token>         MU_THEN
%token <_token>         MU_THROW
%token <_token>         MU_TRY
%token <_token>         MU_UNION
%token <_token>         MU_USE
%token <_token>         MU_WHILE
%token <_token>         MU_YIELD
%token <_type>          MU_TYPE
%token <_type>          MU_TYPECONSTRUCTOR
%token <_typeModifier>  MU_TYPEMODIFIER
%token <_variable>      MU_VARIABLE

%nonassoc               MU_CALL
%nonassoc               MU_JUSTIF
%nonassoc               MU_ELSE
%nonassoc               MU_THEN

%type <_function>   function_prolog
%type <_function>   lambda_prolog
%type <_function>   lambda_value_expression
%type <_int>        array_dimension_declaration
%type <_module>     module
%type <_module>     module_prolog
%type <_name>       documentation_identifier
%type <_name>       identifier
%type <_name>       identifier_or_initializer_symbol
%type <_name>       identifier_or_symbol
%type <_name>       identifier_or_symbol_or_module_or_type
%type <_name>       identifier_or_symbol_or_type
%type <_nameList>   identifier_or_initializer_symbol_list
%type <_node>       AND_expression
%type <_node>       additive_expression
%type <_node>       aggregate_initializer
%type <_node>       aggregate_value
%type <_node>       argument_opt
%type <_node>       assignment_expression
%type <_node>       break_or_continue_statement
%type <_node>       case_pattern
%type <_node>       case_pattern_statement
%type <_node>       case_prolog
%type <_node>       case_statement
%type <_node>       cast_expression
%type <_node>       catch_clause
%type <_node>       catch_declaration
%type <_node>       colon_expression
%type <_node>       compilation_unit
%type <_node>       compound_statement
%type <_node>       conditional_expression
%type <_node>       constant
%type <_node>       constant_suffix
%type <_node>       declaration
%type <_node>       declaration_statement
%type <_node>       equality_expression
%type <_node>       exclusive_OR_expression
%type <_node>       expression
%type <_node>       expression_or_true
%type <_node>       expression_statement
%type <_node>       for_init_expression
%type <_node>       if_statement
%type <_node>       inclusive_OR_expression
%type <_node>       initializer
%type <_node>       iteration_statement
%type <_node>       lambda_expression
%type <_node>       let_initializer
%type <_node>       logical_AND_expression
%type <_node>       logical_OR_expression
%type <_node>       module_statement
%type <_node>       multiplicative_expression
%type <_node>       parameter_assignment
%type <_node>       postfix_expression
%type <_node>       postfix_value_expression
%type <_node>       primary_expression
%type <_node>       relational_expression
%type <_node>       return_statement
%type <_node>       shift_expression
%type <_node>       statement
%type <_node>       throw_statement
%type <_node>       try_catch_statement
%type <_node>       type_declaration
%type <_node>       type_or_function_declaration
%type <_node>       type_or_function_declaration_list
%type <_node>       unary_expression
%type <_node>       value_expression
%type <_node>       value_prefix
%type <_nodeList>   aggregate_initializer_list
%type <_nodeList>   aggregate_next_field_type
%type <_nodeList>   argument_list
%type <_nodeList>   argument_list_opt
%type <_nodeList>   argument_slot_list
%type <_nodeList>   argument_slot_list_opt
%type <_nodeList>   case_pattern_statement_list
%type <_nodeList>   catch_clause_list
%type <_nodeList>   for_index_init_expressions
%type <_nodeList>   initializer_list
%type <_nodeList>   let_initializer_list
%type <_nodeList>   list_element_list
%type <_nodeList>   statement_list
%type <_nodeList>   tuple_expression
%type <_nodeTriple> for_each_init_expressions
%type <_op>         assignment_operator
%type <_op>         naked_operator
%type <_pattern>    cons_pattern
%type <_pattern>    pattern
%type <_pattern>    pattern_list
%type <_pattern>    primary_pattern
%type <_symbol>     class_or_interface_declaration
%type <_symbol>     class_or_interface_prolog
%type <_symbol>     function_declaration
%type <_symbol>     initializer_symbol
%type <_symbol>     parameter
%type <_symbol>     prefixed_symbol
%type <_symbol>     require_statement
%type <_symbol>     scope_prefix
%type <_symbol>     symbol
%type <_symbol>     symbol_type_or_module
%type <_symbol>     symbolic_statement
%type <_symbol>     type_or_module
%type <_symbol>     use_statement
%type <_symbolList> class_or_interface_imp_opt
%type <_symbolList> parameter_list
%type <_symbolList> type_list
%type <_token>      opt_comma
%type <_token>      anonymous_scope
%type <_token>      array_multidimension_declaration
%type <_token>      class_or_interface
%type <_token>      documentation_declaration
%type <_token>      documentation_declaration_list
%type <_token>      documentation_statement
%type <_token>      sym_assign_op
%type <_token>      variant_type_declaration
%type <_type>       declaration_type
%type <_type>       postfix_type
%type <_type>       primary_type
%type <_type>       type
%type <_type>       type_constructor
%type <_type>       unary_type
%type <_type>       variant_prolog
%type <_type>       variant_tag
%type <_type>       variant_tag_list

%start compilation_unit

%%

compilation_unit:

        expression
        {
            //
            //  Set the root node of the Process
            //

            if (CONTEXT->typeParsingMode())
            {
                ParseError(state, "internal error: expected type name");
                YYERROR;
            }
            else
            {
                AS->setProcessRoot($1);
            }
        }

        | /* empty */
        {
            //
            //  Set the root node of the Process
            //

            if (CONTEXT->typeParsingMode())
            {
                ParseError(state, "internal error: expected type name");
                YYERROR;
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(),
                                                    AS->emptyNodeList());
            }
        }

        | statement_list
        {
            if (CONTEXT->typeParsingMode())
            {
                ParseError(state, "internal error: expected type name");
                YYERROR;
            }
            else
            {
                Mu::Node *n = AS->endStackFrame($1);
                AS->setProcessRoot(n);
                AS->removeNodeList($1);
            }
        }

        | type
        {
            CONTEXT->setParsedType($1);
        }

        | error
        {
            if (CONTEXT->typeParsingMode())
            {
                CONTEXT->setParsedType(0);
            }
            else
            {
                AS->setProcessRoot(0);
            }
        }
;

statement_list:

        statement
        {
            $$ = $1 ? AS->newNodeList($1) : AS->emptyNodeList();
        }

        | statement_list statement
        {
            size_t s = $1.size();

            if (s != 0 &&
                $1.back() &&
                ($1.back()->symbol() == CONTEXT->returnFromFunction() ||
                 $1.back()->symbol() == CONTEXT->returnFromVoidFunction()))
            {
                ParseError(state, "Statement unreachable.");
                YYERROR;
            }
            else
            {
                if ($2 && $2->symbol() != CONTEXT->noop()) $1.push_back($2);
                $$=$1;
            }
        }
;

statement:

        /* empty */ ';'
        {
            $$ = AS->callBestOverloadedFunction(CONTEXT->noop(),
                                                AS->emptyNodeList());
        }
        | expression_statement
        | if_statement
        | case_statement
        | iteration_statement
        | compound_statement
        | declaration_statement
        | break_or_continue_statement
        | return_statement
        | try_catch_statement
        | throw_statement
        | module_statement
        | documentation_module              { $$ = 0; }
        | error ';'                         { $$ = 0; }
        | type_or_function_declaration      { $$ = 0; }
        | documentation_statement           { $$ = 0; }
        | require_statement                 { $$ = 0; }
        | use_statement                     { $$ = 0; }
        | symbolic_statement                { $$ = 0; }
;

type_or_function_declaration_list:
        type_or_function_declaration { $$ =0 ; }
        | type_or_function_declaration_list type_or_function_declaration { $$ =0 ; }
;

type_or_function_declaration:
        function_declaration                { $$ = 0; }
        | type_declaration                  { $$ = 0; }
;

type_declaration:
        class_or_interface_declaration      { $$ = 0; }
        | variant_type_declaration          { $$ = 0; }
;

module_prolog:
        
        MU_MODULE identifier_or_symbol 
        {
            AS->pushModuleScope(Mu::Name($2)); 
            $$ = static_cast<Mu::Module*>(AS->scope());
            AS->newStackFrame();
        }

        | MU_MODULE module 
        {
            AS->pushScope(static_cast<Mu::Module*>($$ = $2));
            AS->newStackFrame();
        }
;

module_statement:

        module_prolog '{' statement_list '}'
        {
            $$ = AS->endStackFrame($3);
            AS->removeNodeList($3);
            AS->popScope();
        }
;

require_statement:

        MU_REQUIRE identifier ';'
        {
            if (!($$ = Mu::Module::load($2, AS->process(), CONTEXT)))
            {
                ModuleLocationError(state, str($2));
                YYERROR;
            }
        }

        | MU_REQUIRE module ';'
        {
            $$ = $2;
        }
;

use_statement:

        MU_USE identifier ';'
        {
            if (!($$ = Mu::Module::load($2, AS->process(), CONTEXT)))
            {
                ModuleLocationError(state, str($2));
                YYERROR;
            }
            else
            {
                AS->pushScope($$, false);
            }
        }

        | MU_USE module ';'
        {
            AS->pushScope($$ = $2, false);
        }

        | MU_USE primary_type ';'
        {
            AS->pushScope($$ = $2, false);
        }
;

sym_assign_op:

        MU_OP_SYMASSIGN { $$ = MU_OP_SYMASSIGN; }
;

symbolic_statement:

        identifier sym_assign_op symbol_type_or_module ';'
        {
            $$ = new Mu::Alias(CONTEXT, Mu::Name($1).c_str(), $3);
            AS->scope()->addSymbol($$);
        }

        | MU_ALIAS identifier '=' symbol_type_or_module ';'
        {
            $$ = new Mu::Alias(CONTEXT, Mu::Name($2).c_str(), $4);
            AS->scope()->addSymbol($$);
        }

        | type_or_module sym_assign_op symbol_type_or_module ';'
        {
            if ($1 == $3)
            {
                ParseWarning(state, "So you're defining %s to be itself?",
                             $1->fullyQualifiedName().c_str());
            }
            else
            {
                ParseError(state, "Defining %s to be %s is a bad idea.",
                           $1->fullyQualifiedName().c_str(),
                           $3->fullyQualifiedName().c_str());
                YYERROR;
            }
        }

        | MU_ALIAS type_or_module '=' symbol_type_or_module ';'
        {
            if ($2 == $4)
            {
                ParseWarning(state, "So you're defining %s to be itself?",
                             $2->fullyQualifiedName().c_str());
            }
            else
            {
                ParseError(state, "Defining %s to be %s is a bad idea.",
                           $2->fullyQualifiedName().c_str(),
                           $4->fullyQualifiedName().c_str());
                YYERROR;
            }
        }

        // 
        //  This has to be value_expression because a symbol is an
        //  expression. Need to subdivide these rules to get rid of
        //  this problem.
        // 

        | identifier sym_assign_op value_expression ';'
        {
            if (AS->isConstant($3))
            {
                Mu::SymbolicConstant* s = 
                    AS->newSymbolicConstant(Mu::Name($1), $3);

                if (s)
                {
                    AS->scope()->addSymbol(s);
                }
                else
                {
                    ParseError(state, "defining %s of type %s",
                               str($1),
                               $3->symbol()->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "you can't define %s like that",
                           str($1));
                YYERROR;
            }
        }

        | identifier sym_assign_op '-' value_expression ';'
        {
            if (AS->isConstant($4))
            {
                const char* op = "-";

                Mu::SymbolicConstant* s = 
                    AS->newSymbolicConstant(Mu::Name($1), AS->unaryOperator(op, $4));

                if (s)
                {
                    AS->scope()->addSymbol(s);
                }
                else
                {
                    ParseError(state, "defining %s of type %s",
                               str($1),
                               $4->symbol()->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "you can't define %s like that",
                           str($1));
                YYERROR;
            }
        }

        | MU_ALIAS identifier '=' value_expression ';'
        {
            if (AS->isConstant($4))
            {
                Mu::SymbolicConstant* s = 
                    AS->newSymbolicConstant(Mu::Name($2), $4);

                if (s)
                {
                    AS->scope()->addSymbol(s);
                }
                else
                {
                    ParseError(state, "defining %s of type %s",
                               str($2),
                               $4->symbol()->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "you can't define %s like that",
                           str($2));
                YYERROR;
            }
        }
;

documentation_module:

        MU_DOCUMENTATION '{' 
        {
            AS->setDocumentationModule(AS->scope());
        }
            documentation_declaration_list '}'
        {
            AS->setDocumentationModule(0);
        }
;

documentation_declaration_list:

        documentation_declaration { $$ = 0; }
        | documentation_declaration_list documentation_declaration { $$ = 0; }
;

documentation_declaration:

        documentation_identifier MU_STRINGCONST
        { 
            AS->addDocumentation(Mu::Name($1), $2);
            $$ = 0; 
        }

        | documentation_identifier type MU_STRINGCONST
        { 
            AS->addDocumentation(Mu::Name($1), $3, $2);
            $$ = 0; 
        }

        | MU_STRINGCONST 
        { 
            AS->addDocumentation(CONTEXT->internName(""), $1);
            $$ = 0; 
        }
;

documentation_identifier:

        identifier_or_symbol_or_module_or_type
        | documentation_identifier '.' identifier_or_symbol_or_module_or_type
        {
            Mu::String n = Mu::Name($1);
            n += ".";
            n += Mu::Name($3).c_str();
            $$ = CONTEXT->internName(n.c_str()).nameRef();
        }
;

documentation_statement:

        MU_DOCUMENTATION documentation_declaration
;

variant_tag:

        identifier_or_symbol_or_module_or_type
        {
            Mu::VariantTagType* t = AS->declareVariantTagType(str($1), CONTEXT->voidType());
            $$ = t;
        }

        | identifier_or_symbol_or_module_or_type type
        {
            Mu::VariantTagType* t = AS->declareVariantTagType(str($1), $2);
            $$ = t;
        }
;

variant_tag_list:

        variant_tag
        | variant_tag_list '|' variant_tag { $$ = $3; }
;

variant_prolog:

        MU_UNION identifier_or_symbol_or_module_or_type 
        {
            $$ = AS->declareVariantType(str($2));
        }
;

variant_type_declaration:

        variant_prolog '{' type_or_function_declaration_list variant_tag_list '}'
        {
            AS->popScope();
        }

        | variant_prolog '{' variant_tag_list '}'
        {
            AS->popScope();
        }
;

class_or_interface_declaration:

        class_or_interface_prolog '{' statement_list '}'
        {
            $$ = $1;
            AS->removeNodeList($3);
            AS->popScope();

            if (Mu::Class* c = dynamic_cast<Mu::Class*>($1))
            {
                AS->generateDefaults(c);
            }
            else if (Mu::Interface* i = dynamic_cast<Mu::Interface*>($1))
            {
                AS->generateDefaults(i);
            }
        }
;

class_or_interface:

        MU_CLASS { $$ = MU_CLASS; }
        | MU_INTERFACE { $$ = MU_INTERFACE; } 
;

class_or_interface_imp_opt:

        /* empty */ { $$ = AS->emptySymbolList(); }

        | ':' type_list
        {
            $$ = $2;
        }
;

class_or_interface_prolog:

        class_or_interface identifier_or_symbol_or_module_or_type class_or_interface_imp_opt
        {
            if ($1 == MU_CLASS)
            {
                $$ = AS->declareClass(str($2), $3);
            }
            else
            {
                $$ = AS->declareInterface(str($2), $3);
            }

            AS->removeSymbolList($3);
        }
;

parameter_assignment:

        /* nothing */ { $$ = 0; }
        | '=' conditional_expression 
        { 
            if (yyConstantExpression($2))
            {
                $$ = $2; 
            }
            else
            {
                ParseError(state, "Parameter default value must "
                           "be a constant expression.");
                YYERROR;
            }
        }
;

parameter:

        declaration_type
        {
            $$ = new Mu::ParameterVariable(CONTEXT, "__", $1);
            AS->popType();
        }

        | declaration_type identifier_or_symbol parameter_assignment
        {
            if ($3)
            {
                if (Mu::Node *n = AS->cast($3, $1))
                {
                    Mu::Value v = $1->nodeEval(n, *AS->thread());
                    $$ = new Mu::ParameterVariable(CONTEXT, str($2), $1, v);
                    
                    if ($1->machineRep() == Mu::PointerRep::rep())
                    {
                        Mu::Object *o = 
                            reinterpret_cast<Mu::Object*>(v._Pointer);
                        //AS->process()->constants().push_back(o);
                    }
                }
                else
                {
                    ParseError(state, "Cannot cast %s to %s for "
                               "default value of parameter %s.",
                               $3->type()->fullyQualifiedName().c_str(),
                               $1->fullyQualifiedName().c_str(),
                               str($2));
                    YYERROR;
                }
            }
            else
            {
                $$ = new Mu::ParameterVariable(CONTEXT, str($2), $1);
            }

            AS->popType();
        }
;

parameter_list:

        parameter
        {
            $$ = AS->newSymbolList($1);
        }

        | parameter_list ',' parameter
        {
            Mu::ParameterVariable *last =
                static_cast<Mu::ParameterVariable*>($1.back());
            Mu::ParameterVariable *p = static_cast<Mu::ParameterVariable*>($3);

            if (last->hasDefaultValue() && !p->hasDefaultValue())
            {
                ParseError(state, "Default value missing for parameter %s.",
                           $3->name().c_str());
                YYERROR;
            }
            else
            {
                $$ = $1;
                $$.push_back($3);
            }
        }

        | /* empty */
        {
            $$ = AS->emptySymbolList();
        }
;

function_prolog:

        MU_OPERATOR identifier '(' type ';' parameter_list ')'
        {
            $$ = AS->declareFunction(str($2), $4, $6, 
                                     Mu::Function::ContextDependent|
                                     Mu::Function::Operator);

            AS->removeSymbolList($6);
            if (!$$) YYERROR;
        }

        | MU_OPERATOR identifier '(' parameter_list ')'
        {
            $$ = AS->declareFunction(str($2), 0, $4, 
                                     Mu::Function::ContextDependent|
                                     Mu::Function::Operator);
            AS->removeSymbolList($4);
            if (!$$) YYERROR;
        }

        | MU_FUNCTION identifier_or_symbol_or_type '(' type ';' parameter_list ')'
        {
            $$ = AS->declareFunction(str($2), $4, $6, 
                                     Mu::Function::ContextDependent);

            AS->removeSymbolList($6);
            if (!$$) YYERROR;
        }

        | MU_FUNCTION identifier_or_symbol_or_type '(' parameter_list ')'
        {
            $$ = AS->declareFunction(str($2), 0, $4, 
                                     Mu::Function::ContextDependent);

            AS->removeSymbolList($4);
            if (!$$) YYERROR;
        }

        | MU_METHOD identifier_or_symbol_or_type '(' type ';' parameter_list ')'
        {
            if (AS->classScope())
            {
                $$ = AS->declareMemberFunction(str($2), $4, $6, Mu::Function::ContextDependent);
                if (!$$) YYERROR;
            }
            else
            {
                ParseError(state, "method declaration not allowed in this context.");
                YYERROR;
            }

            AS->removeSymbolList($6);
        }

        | MU_METHOD identifier_or_symbol_or_type '(' parameter_list ')'
        {
            if (AS->classScope())
            {
                $$ = AS->declareMemberFunction(str($2), 0, $4,
                                               Mu::Function::ContextDependent);
            }
            else
            {
                ParseError(state, "method declaration not allowed in this context.");
                YYERROR;
            }
            
            AS->removeSymbolList($4);
            if (!$$) YYERROR;
        }
;

function_declaration:

        function_prolog compound_statement
        {
            if (!($$ = AS->declareFunctionBody($1, $2))) YYERROR;
        }

        | function_prolog ';'
        {
            if (!($$ = AS->declareFunctionBody($1, 0))) YYERROR;
        }
;

lambda_prolog:

        MU_FUNCTION '(' type ';' parameter_list ')'
        {
            $$ = AS->declareFunction(0, $3, $5, 
                                     Mu::Function::ContextDependent|
                                     Mu::Function::LambdaExpression);

            AS->removeSymbolList($5);
        }

        | MU_FUNCTION '(' parameter_list ')'
        {
            $$ = AS->declareFunction(0, 0, $3, 
                                     Mu::Function::ContextDependent|
                                     Mu::Function::LambdaExpression);

            AS->removeSymbolList($3);
        }
;

lambda_value_expression:

        lambda_prolog compound_statement
        {
            if (!($$ = AS->declareFunctionBody($1, $2))) YYERROR;
        }
;

compound_statement:

        '{' anonymous_scope statement_list '}'
        {
            NodeList nl = $3;

            if (nl.size() > 1)
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->simpleBlock(), $3);
            }
            else if (nl.size() == 1)
            {
                $$ = $3.front();
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(),
                                                    AS->emptyNodeList());
            }

            AS->popScope();
            AS->removeNodeList($3);

            if (!$$)
            {
                ParseError(state, "Internal parsing error in compound_statement");
                YYERROR;
            }
        }
;

anonymous_scope:

        /* empty */
        {
            AS->pushAnonymousScope("__");
        }
;

for_init_expression:

        assignment_expression
        | declaration
;

for_each_init_expressions:

        identifier_or_initializer_symbol ';' expression
        {
            $$[1] = $3;
            const Mu::Type* stype = $$[1]->type();

            if (stype->isUnresolvedType())
            {
                $$[0] = AS->unresolvableStackDeclaration($1);
            }
            else
            {
                if (stype->isCollection())
                {
                    if (const Mu::Type* ftype = stype->fieldType(0))
                    {
                        AS->declarationType(ftype);
                        
                        if (!($$[0] = AS->declareInitializer($1, 0)))
                        {
                            ParseError(state, "Unable to intialize %s of type %s", 
                                       Mu::Name($1).c_str(),
                                       ftype->fullyQualifiedName().c_str());
                            YYERROR;
                        }
                    }
                    else
                    {
                        YYERROR;
                    }
                }
                else 
                {
                    ParseError(state, "The for_each statement requires a collection; "
                               "Type %s is not a collection",
                               stype->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
        }
;

for_index_init_expressions:

        identifier_or_initializer_symbol_list ';' expression
        {
            const Mu::Type* stype = $3->type();
            NodeList nl = AS->emptyNodeList();
            $$ = nl;

            AS->declarationType(CONTEXT->intType());
                
            if (stype->isCollection() || stype->isUnresolvedType())
            {
                for (int i=0; i < $1.size(); i++)
                {
                    if (Mu::Node* n = AS->declareInitializer($1[i], 0))
                    {
                        nl.push_back(n);
                    }
                    else
                    {
                        ParseError(state, "Unable to intialize %s of type int", 
                                   Mu::Name($1[i]).c_str());
                        YYERROR;
                        break;
                    }
                }

                nl.push_back($3);
                $$ = nl;
            }
            else
            {
                ParseError(state, "The for_index statement requires an array; "
                           "Type %s is not a array",
                           stype->fullyQualifiedName().c_str());
                YYERROR;
            }

            AS->removeNameList($1);
        }
;

iteration_statement:

        MU_FOR anonymous_scope '(' for_init_expression ';'
                                expression_or_true ';'
                                assignment_expression ')' statement
        {
            NodeList nl = AS->newNodeList($4);
            nl.push_back($6);
            nl.push_back($8);
            nl.push_back($10);
            $$ = AS->callBestFunction("__for",nl);
            AS->removeNodeList(nl);
            AS->popScope();

            if (!$$)
            {
                if ($8->type() != CONTEXT->boolType())
                {
                    ParseError(state, "The for statement requires a "
                               "boolean test expression.");
                }
                else
                {
                    ParseError(state, "Malformed for statement.");
                }

                YYERROR;
            }
        }

        | MU_FOREACH anonymous_scope '(' for_each_init_expressions ')' statement
        {
            $$ = 0;

            if ($6)
            {
                $$ = AS->foreachStatement($4[0], $4[1], $6);
                AS->popScope();
            }

            if (!$$)
            {
                ParseError(state, "unable to generate for_each statement");
                YYERROR;
            }
        }

        | MU_FORINDEX anonymous_scope '(' for_index_init_expressions ')' statement
        {
            $4.push_back($6);
            $$ = AS->callBestFunction("__for_index", $4);
            AS->removeNodeList($4);
            AS->popScope();

            if (!$$)
            {
                ParseError(state, "unable to generate for_index statement");
                YYERROR;
            }
        }

        | MU_WHILE '(' expression ')' statement
        {
            NodeList nl = AS->newNodeList($3);
            nl.push_back($5);
            $$ = AS->callBestFunction("__while",nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                if ($3->type() != CONTEXT->boolType())
                {
                    ParseError(state, "The while statement requires a "
                               "boolean test expression.");
                }
                else
                {
                    ParseError(state, "Malformed while statement.");
                }

                YYERROR;
            }
        }

        | MU_REPEAT '(' expression ')' statement
        {
            NodeList nl = AS->newNodeList($3);
            nl.push_back($5);
            $$ = AS->callBestFunction("__repeat",nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                if ($3->type() != CONTEXT->intType())
                {
                    ParseError(state, "The repeat statement requires an "
                               "int count expression.");
                }
                else
                {
                    ParseError(state, "Malformed repeat statement.");
                }

                YYERROR;
            }
        }

        | MU_DO statement MU_WHILE '(' expression ')'
        {
            NodeList nl = AS->newNodeList($2);
            nl.push_back($5);
            $$ = AS->callBestFunction("__do_while",nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                if ($5->type() != CONTEXT->boolType())
                {
                    ParseError(state, "The do...while statement requires a "
                               "boolean test expression.");
                }
                else
                {
                    ParseError(state, "Malformed do...while statement.");
                }

                YYERROR;
            }
        }

        | MU_CONSTRUCT anonymous_scope
                '(' for_init_expression ';' argument_list_opt ')' statement
        {
            //
            //  Dynamic construct.
            //

            ParseError(state, "Not implemented yet.");
            YYERROR;
        }

        | MU_CONSTRUCT anonymous_scope '(' argument_list_opt ')' statement
        {
            //
            //  Construct. This is just a function call in which the
            //  arguments can be evaluated multiple times.
            //

            ParseError(state, "Not implemented yet.");
            YYERROR;
        }
;

if_statement:

        MU_IF '(' expression ')' statement %prec MU_JUSTIF
        {
            NodeList nl = AS->newNodeList($3);
            nl.push_back($5);
            $$ = AS->callBestFunction("__if",nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                ParseError(state, "Malformed if statement.");
                YYERROR;
            }
        }

        | MU_IF '(' expression ')' statement MU_ELSE statement
        {
            NodeList nl = AS->newNodeList($3);
            nl.push_back($5);
            nl.push_back($7);
            $$ = AS->callBestFunction("__if",nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                ParseError(state, "Malformed if/else statement.");
                YYERROR;
            }
        }
;

case_prolog:

        MU_CASE '(' expression ')' 
        {
            $$ = AS->beginCase($3);
        }
;

case_statement:

        case_prolog '{' case_pattern_statement_list '}' 
        {
            if (!($$ = AS->finishCase($1, $3))) YYERROR;
        }
;

case_pattern:

        anonymous_scope pattern 
        {
            if (!($$ = AS->casePattern($2))) 
            {
                ParseError(state, "invalid case pattern");
                YYERROR;
            }
        }
;

case_pattern_statement:

        case_pattern MU_ARROW statement
        {
            NodeList nl = AS->newNodeList($3);

            if (!($$ = AS->casePatternStatement($1, nl)))
            {
                ParseError(state, "Internal parsing error in case_pattern_statement");
                YYERROR;
            }

            AS->removeNodeList(nl);
        }
;

case_pattern_statement_list:

        case_pattern_statement
        {
            $$ = AS->newNodeList($1);
        }

        | case_pattern_statement_list case_pattern_statement
        {
            $$ = $1;
            $$.push_back($2);
        }
;

return_statement:

        MU_RETURN ';'
        {
            if (Mu::Function *f = AS->currentFunction())
            {
                f->hasReturn(true);
                if (!f->returnType()) f->setReturnType(CONTEXT->voidType());
                $$ = AS->callBestFunction("__return", AS->emptyNodeList());
            }
            else
            {
                ParseError(state, "Cannot return outside of a function.");
                YYERROR;
            }
        }

        | MU_RETURN expression ';'
        {
            if (Mu::Function *f = AS->currentFunction())
            {
                f->hasReturn(true);

                if (!f->returnType())
                {
                    f->setReturnType($2->type());
                }

                if (f->returnType() == CONTEXT->voidType())
                {
                    ParseError(state, "Cannot cast return expression of type %s to void in %s.",
                               $2->type()->fullyQualifiedName().c_str(),
                               f->fullyQualifiedName().c_str());
                    YYERROR;
                }
                else
                {
                    if (Mu::Node *n = AS->cast($2, f->returnType()))
                    {
                        NodeList nl = AS->newNodeList(n);
                        $$ = AS->callBestFunction("__return", nl);
                        AS->removeNodeList(nl);
                    }
                    else
                    {
                        ParseError(state, "Cannot cast %s to return %s.",
                                   $2->type()->fullyQualifiedName().c_str(),
                                   f->returnTypeName().c_str());
                        YYERROR;
                    }
                }
            }
            else
            {
                ParseError(state, "Cannot return outside of a function.");
                YYERROR;
            }
        }
;

break_or_continue_statement:

        MU_BREAK ';'
        {
            $$ = AS->callBestFunction("__break", AS->emptyNodeList());
        }

        | MU_CONTINUE ';'
        {
            $$ = AS->callBestFunction("__continue", AS->emptyNodeList());
        }
;

throw_statement:

        MU_THROW expression ';'
        {
            NodeList nl = AS->newNodeList($2);
            $$ = AS->callBestFunction("__throw", nl);
            AS->removeNodeList(nl);

            if (!$$)
            {
                ParseError(state, "Cannot throw value of type %s",
                           $2->type()->fullyQualifiedName().c_str());
                YYERROR;
            }
        }

        | MU_THROW ';'
        {
            $$ = AS->callBestFunction("__rethrow", AS->emptyNodeList());
            if (!$$)
            {
                ParseError(state, "Cannot rethrow value");
                YYERROR;
            }
        }
;

try_catch_statement:

        MU_TRY compound_statement catch_clause_list
        {
            $3.front() = $2;
            $$ = AS->callBestFunction("__try", $3);
            AS->removeNodeList($3);
        }
;

catch_clause:

        MU_CATCH anonymous_scope '(' catch_declaration ')' compound_statement
        {
            if ($4)
            {
                NodeList nl = AS->newNodeList($4);
                nl.push_back($6);
                $$ = AS->callBestFunction("__catch", nl);
                AS->removeNodeList(nl);
            }
            else
            {
                NodeList nl = AS->newNodeList($6);
                $$ = AS->callBestFunction("__catch_all", nl);
                AS->removeNodeList(nl);
            }

            AS->popScope();
        }
;

catch_clause_list:

        catch_clause
        {
            //
            //  This is hookey, but efficient. Reserve the first
            //  argument in the node list for the try portion of the
            //  try/catch statement.
            //

            $$ = AS->newNodeList(0);
            $$.push_back($1);
        }

        | catch_clause_list catch_clause
        {
            $1.push_back($2);
            $$ = $1;
        }
;

catch_declaration:

        declaration_type identifier_or_initializer_symbol
        {
            Mu::Node *n = AS->callBestFunction("__exception", AS->emptyNodeList());
            AS->declarationType($1);

            if (!($$ = AS->declareInitializer($2, n)))
            {
                ParseError(state, "Unexpected error in catch_declaration.");
                YYERROR;
            }
        }

        | MU_ELLIPSIS
        {
            $$ = 0;
        }
;


declaration_statement:

        declaration ';'                     { $$ = $1; }
;

declaration:

        declaration_type { AS->declarationType($1); } initializer_list
        {
            AS->popType();

            if ($3.empty())
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(), AS->emptyNodeList());
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->patternBlock(), $3);
                AS->removeNodeList($3);
            }
        }

        | MU_LET { AS->declarationType(0); } let_initializer_list
        {
            if ($3.empty())
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(), AS->emptyNodeList());
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->patternBlock(), $3);
                AS->removeNodeList($3);
            }
        }

        | MU_GLOBAL declaration_type { AS->declarationType($2, true); } initializer_list
        {
            AS->popType();

            if ($4.empty())
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(), AS->emptyNodeList());
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->patternBlock(), $4);
                AS->removeNodeList($4);
            }
        }

        | MU_GLOBAL MU_LET { AS->declarationType(0, true); } let_initializer_list
        {
            if ($4.empty())
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->noop(), AS->emptyNodeList());
            }
            else
            {
                $$ = AS->callBestOverloadedFunction(CONTEXT->patternBlock(), $4);
                AS->removeNodeList($4);
            }
        }
;

declaration_type:

        type
        {
            if ($1 == CONTEXT->voidType())
            {
                ParseError(state, "Type void makes no sense in this context.");
                YYERROR;
            }

            AS->pushType($1);
            $$ = $1;
        }
;

let_initializer_list:

        let_initializer
        {
            $$ = $1 ? AS->newNodeList($1) : AS->emptyNodeList();
        }

        | let_initializer_list ',' let_initializer
        {
            if ($3) $1.push_back($3);
            $$ = $1;
        }
;

let_initializer:

        pattern '=' conditional_expression             
        { 
            if (!($$ = AS->resolvePatterns($1,$3))) YYERROR;
        }
;


initializer_list:

        initializer
        {
            $$ = $1 ? AS->newNodeList($1) : AS->emptyNodeList();
        }

        | initializer_list ',' initializer
        {
            if ($3) $1.push_back($3);
            $$ = $1;
        }
;

initializer:

        identifier_or_initializer_symbol    
        { 
            $$ = AS->declareInitializer($1,0);
            if (!AS->classScope() && !AS->interfaceScope() && !$$) YYERROR;
        }

        | identifier_or_initializer_symbol '=' conditional_expression             
        { 
            if (!($$ = AS->declareInitializer($1,$3))) YYERROR;
        }

        | identifier_or_initializer_symbol '=' aggregate_initializer  
        { 
            if (!($$ = AS->declareInitializer($1,$3))) YYERROR;
        }
;

pattern:

        cons_pattern
        {
            if ($1->next)
            {
                AS->unflattenPattern($1, "?list", true);
                $$ = AS->newPattern($1, "?list");
            }
            else
            {
                $$ = $1;
            }
        }
;

cons_pattern:

        primary_pattern

        | cons_pattern ':' primary_pattern
        {
            AS->appendPattern($1, $3);
            $$ = $1;
        }
;

primary_pattern:

        identifier_or_initializer_symbol
        {
            $$ = AS->newPattern($1);
        }

        | constant_suffix
        {
            $$ = AS->newPattern($1);
        }

        | '-' constant_suffix
        {
            Mu::Node* n = 0;

            if ( !(n = AS->unaryOperator("-",$2)) )
            {
                $$ = 0;
                YYERROR;
            }
            else
            {
                $$ = AS->newPattern(n);
            }
        }

        | type_constructor
        {
            $$ = AS->newPattern(0);
            $$->constructor = $1;
        }

        | type_constructor pattern
        {
            $$ = $2;
            $$->constructor = $1;
        }

        | '_'
        {
            $$ = AS->newPattern(CONTEXT->internName("_").nameRef());
        }

        | '(' pattern_list ')'
        {
            $$ = AS->newPattern($2, "?tuple");
        }

        | '{' pattern_list '}'
        {
            $$ = AS->newPattern($2, "?class_not_tuple");
        }

        | '[' pattern_list ']'
        {
            AS->unflattenPattern($2, "?list");
            $$ = AS->newPattern($2, "?list");
        }
;


pattern_list:

        pattern
        | pattern_list ',' pattern
        {
            AS->appendPattern($1, $3);
            $$ = $1;
        }
;


identifier_or_initializer_symbol:

        identifier
        | initializer_symbol
        {
            $$ = $1->name().nameRef();
        }
;

identifier_or_initializer_symbol_list:

        identifier_or_initializer_symbol
        {
            $$ = AS->newNameList($1);
        }

        | identifier_or_initializer_symbol_list ',' identifier_or_initializer_symbol
        {
            $$ = $1;;
            $$.push_back($3);
        }
;

initializer_symbol:

        symbol
        {
            if (Mu::Function *f = AS->findTypeInScope<Mu::Function>($1->name()))
            {
                if (f->scope() == AS->scope())
                {
                    ParseError(state, "Declaration of \"%s\""
                               " shadows function of the same name.",
                               $1->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }

            if (Mu::Variable* v =
                AS->findTypeInScope<Mu::Variable>($1->name()))
            {
                if (v->scope() == AS->scope())
                {
                    ParseError(state, "Declaration of \"%s\""
                               " shadows existing declaration.",
                               $1->fullyQualifiedName().c_str());

                    YYERROR;
                }
            }

            $$ = $1;
        }
;

expression_statement:

        assignment_expression ';'              { $$ = $1; }
;

expression_or_true:

        expression

        | /* empty */
        {
            Mu::DataNode *node = AS->constant(CONTEXT->boolType());
            node->_data._bool = true;
            $$ = node;
        }
;


expression:

        tuple_expression
        {
            if ($1.size() == 1)
            {
                $$ = $1[0];
            }
            else
            {
                if (!($$ = AS->tupleNode($1))) YYERROR;
            }

            AS->removeNodeList($1);
        }
;

argument_opt:

        conditional_expression

        /*| type { $$ = 0; }*/

        | /* empty */
        {
            $$ = AS->callBestOverloadedFunction(CONTEXT->noop(),
                                                AS->emptyNodeList());
        }
;

argument_list_opt:

          /* nothing */     { $$ = AS->emptyNodeList(); }
        | argument_list
;

argument_list:

        conditional_expression
        {
            $$ = AS->newNodeList($1);
        }

        | argument_list ',' conditional_expression
        {
            $1.push_back($3); 
            $$ = $1;
        }
;

argument_slot_list_opt:

        argument_slot_list
        {
            if ($1.size() == 1 && $1[0]->symbol() == CONTEXT->noop())
            {
                delete $1[0];
                AS->removeNodeList($1);
                $$ = AS->emptyNodeList();
            }
            else
            {
                $$ = $1;
            }
        }
;

argument_slot_list:

        argument_opt
        {
            $$ = AS->newNodeList($1);
        }

        | argument_slot_list ',' argument_opt
        {
            $1.push_back($3); 
            $$ = $1;
        }
;

assignment_expression:

          conditional_expression

        | postfix_value_expression assignment_operator assignment_expression
        {
            if ( !($$ = AS->assignmentOperator($2,$1,$3)) )
            {
                OpError(state, $2,$1,$3);
                YYERROR;
            }
        }
;

assignment_operator:

          '='               { strcpy($$,"="); }
        | MU_OP_ASSIGN         { strcpy($$,$1);  }
;

tuple_expression:

        conditional_expression
        {
            $$ = AS->newNodeList($1);
        }

        | tuple_expression ',' conditional_expression
        {
            $$ = $1;
            $$.push_back($3);
        }
;

list_element_list:

        conditional_expression
        {
            $$ = AS->newNodeList($1);
        }

        | list_element_list ',' conditional_expression
        {
            $$ = $1;
            $$.push_back($3);
        }
;

conditional_expression:

          colon_expression
        | MU_IF colon_expression MU_THEN conditional_expression MU_ELSE conditional_expression
        {
            NodeList nl = AS->newNodeList($2);
            bool failed = false;

            if ($4->type() != CONTEXT->nilType())
            {
                if (Mu::Node* c = AS->cast($6, $4->type()))
                {
                    nl.push_back($4);
                    nl.push_back(c);
                }
                else
                {
                    failed = true;
                }
            }
            else if ($6->type() != CONTEXT->nilType())
            {
                if (Mu::Node* c = AS->cast($4, $6->type()))
                {
                    nl.push_back(c);
                    nl.push_back($6);
                }
                else
                {
                    failed = true;
                }
            }
            else
            {
                nl.push_back($4);
                nl.push_back($6);
            }

            if (failed)
            {
                ParseError(state, "if-then-else: illegal type conversion (%s and %s)",
                           $4->type()->fullyQualifiedName().c_str(),
                           $6->type()->fullyQualifiedName().c_str());
                YYERROR;
            }
            else
            {
                $$ = AS->callBestFunction("?:", nl);
            }

            AS->removeNodeList(nl);
        }
;


colon_expression:

        //
        //  NOTE: operator ':' associates right not left
        //

          lambda_expression
        | lambda_expression ':' colon_expression
        {
            const Mu::Type* ctype = $3->type();
            const Mu::Type* ltype = $1->type();
            const Mu::ListType* t = 0;

            if ((t = dynamic_cast<const Mu::ListType*>($3->type())) &&
                !t->elementType()->match($1->type()) &&
                !ltype->isUnresolvedType())
            {
                {
                    ParseError(state, "cannot cons an expression of type \"%s\" "
                               "onto a list of type \"%s\" using operator ':'",
                               $1->type()->fullyQualifiedName().c_str(),
                               t->fullyQualifiedName().c_str());

                    YYERROR;
                }
            }
            else
            {
                NodeList nl = AS->newNodeList($1);
                nl.push_back($3);
                
                $$ = AS->callBestFunction("cons",nl);
                if (!$$) YYERROR;
                AS->removeNodeList(nl);
            }
        }
;



lambda_expression:

        lambda_value_expression
        {
            $$ = AS->functionConstant($1);
        }

        | logical_OR_expression
;


logical_OR_expression:

          logical_AND_expression
        | logical_OR_expression MU_LOGICOR logical_AND_expression
        {
            if ( !($$ = AS->binaryOperator("||",$1,$3)) )
            {
                OpError(state, "||",$1,$3);
                YYERROR;
            }
        }

;

logical_AND_expression:

          inclusive_OR_expression
        | logical_AND_expression MU_LOGICAND inclusive_OR_expression
        {
            if ( !($$ = AS->binaryOperator("&&",$1,$3)) )
            {
                OpError(state, "&&",$1,$3);
                YYERROR;
            }
        }

;

inclusive_OR_expression:

          exclusive_OR_expression
        | inclusive_OR_expression '|' exclusive_OR_expression
        {
            if ( !($$ = AS->binaryOperator("|",$1,$3)) )
            {
                OpError(state, "|",$1,$3);
                YYERROR;
            }
        }

;

exclusive_OR_expression:

          AND_expression
        | exclusive_OR_expression '^' AND_expression
        {
            if ( !($$ = AS->binaryOperator("^",$1,$3)) )
            {
                OpError(state, "^",$1,$3);
                YYERROR;
            }
        }

;

AND_expression:

          equality_expression
        | AND_expression '&' equality_expression
        {
            if ( !($$ = AS->binaryOperator("&",$1,$3)) )
            {
                OpError(state, "&",$1,$3);
                YYERROR;
            }
        }

;

equality_expression:

          relational_expression

        | equality_expression MU_EQUALS relational_expression
        {
            if ( !($$ = AS->binaryOperator("==",$1,$3)) )
            {
                OpError(state, "==",$1,$3);
                YYERROR;
            }
        }

        | equality_expression MU_NOTEQUALS relational_expression
        {
            if ( !($$ = AS->binaryOperator("!=",$1,$3)) )
            {
                OpError(state, "!=",$1,$3);
                YYERROR;
            }
        }

        | equality_expression MU_EQ relational_expression
        {
            if ( !($$ = AS->binaryOperator("eq",$1,$3)) )
            {
                OpError(state, "eq",$1,$3);
                YYERROR;
            }
        }

        | equality_expression MU_NEQ relational_expression
        {
            if ( !($$ = AS->binaryOperator("neq",$1,$3)) )
            {
                OpError(state, "neq",$1,$3);
                YYERROR;
            }
        }
;

relational_expression:

          shift_expression

        | relational_expression '>' shift_expression
        {
            if ( !($$ = AS->binaryOperator(">",$1,$3)) )
            {
                OpError(state, ">",$1,$3);
                YYERROR;
            }
        }

        | relational_expression '<' shift_expression
        {
            if ( !($$ = AS->binaryOperator("<",$1,$3)) )
            {
                OpError(state, "<",$1,$3);
                YYERROR;
            }
        }

        | relational_expression MU_LESSEQUALS shift_expression
        {
            if ( !($$ = AS->binaryOperator("<=",$1,$3)) )
            {
                OpError(state, "<=",$1,$3);
                YYERROR;
            }
        }

        | relational_expression MU_GREATEREQUALS shift_expression
        {
            if ( !($$ = AS->binaryOperator(">=",$1,$3)) )
            {
                OpError(state, ">=",$1,$3);
                YYERROR;
            }
        }
;

shift_expression:

          additive_expression

        | shift_expression MU_LSHIFT additive_expression
        {
            if ( !($$ = AS->binaryOperator("<<",$1,$3)) )
            {
                OpError(state, "<<",$1,$3);
                YYERROR;
            }
        }

        | shift_expression MU_RSHIFT additive_expression
        {
            if ( !($$ = AS->binaryOperator(">>",$1,$3)) )
            {
                OpError(state, ">>",$1,$3);
                YYERROR;
            }
        }
;

additive_expression:

        multiplicative_expression

        | additive_expression '+' multiplicative_expression
        {
            if ( !($$ = AS->binaryOperator("+",$1,$3)) )
            {
                OpError(state, "+",$1,$3);
                YYERROR;
            }
        }

        | additive_expression '-' multiplicative_expression
        {
            if ( !($$ = AS->binaryOperator("-",$1,$3)) )
            {
                OpError(state, "-",$1,$3);
                YYERROR;
            }
        }
;

multiplicative_expression:

        cast_expression

        | multiplicative_expression '*' cast_expression
        {
            if ( !($$ = AS->binaryOperator("*",$1,$3)) )
            {
                OpError(state, "*",$1,$3);
                YYERROR;
            }
        }

        | multiplicative_expression '/' cast_expression
        {
            if ( !($$ = AS->binaryOperator("/",$1,$3)) )
            {
                OpError(state, "/",$1,$3);
                YYERROR;
            }
        }

        | multiplicative_expression '%' cast_expression
        {
            if ( !($$ = AS->binaryOperator("%",$1,$3)) )
            {
                OpError(state, "%",$1,$3);
                YYERROR;
            }
        }
;

cast_expression:

        unary_expression
        {
            //
            // This is possibly a reference type. Dereference it.  No
            // function can match the reference type argument that is
            // not a postfix_expression.
            //

            if ( !($$ = AS->dereferenceLValue($1)) )
            {
                ParseError(state, "unable to dereference");
                YYERROR;
            }
        }
;

unary_expression:

        postfix_value_expression

        | MU_OP_INC unary_expression
        {
            const char *op;
            if (*$1 == '+') op = "pre++";
            else op = "pre--";

            if ( !($$ = AS->unaryOperator(op,$2)) )
            {
                OpError(state, $1,$2,0);
                YYERROR;
            }
        }

        | '-' cast_expression
        {
            if ( !($$ = AS->unaryOperator("-",$2)) )
            {
                OpError(state, "-",$2,0);
                YYERROR;
            }
        }

        | '+' cast_expression
        {
            if ( !($$ = AS->unaryOperator("+",$2)) )
            {
                OpError(state, "+",$2,0);
                YYERROR;
            }
        }

        | '*' cast_expression
        {
            if ( !($$ = AS->unaryOperator("*",$2)) )
            {
                OpError(state, "*",$2,0);
                YYERROR;
            }
        }

        | '!' cast_expression
        {
            if ( !($$ = AS->unaryOperator("!",$2)) )
            {
                OpError(state, "!",$2,0);
                YYERROR;
            }
        }

        | '~' cast_expression
        {
            if ( !($$ = AS->unaryOperator("~",$2)) )
            {
                OpError(state, "~",$2,0);
                YYERROR;
            }
        }
;

value_prefix:

        postfix_value_expression '.'
        {
            //
            //  The expression may require dereferencing if its a
            //  reference type. Push the type of the LHS expression
            //  as the prefix scope. The lexer uses this scope as a
            //  higher priority lookup for a symbol.
            //

            $$ = $1;
            AS->prefixScope($$->type());
        }
;


postfix_value_expression:

          postfix_expression

        | value_expression
;

postfix_expression:

          primary_expression

        | postfix_expression MU_OP_INC
        {
            const char *op;
            if (*$2 == '+') op = "post++";
            else op = "post--";

            if ( !($$ = AS->unaryOperator(op,$1)) )
            {
                OpError(state, $2,$1,0);
                YYERROR;
            }
        }

	| postfix_expression '[' argument_list ']'
	{
            $$ = AS->memberOperator("[]", $1, $3);
            AS->removeNodeList($3);
            if (!$$) YYERROR;
        }
;

//
//  There is a deviation from C/C++ grammars here.
//
//  primary_expression (ala C) is factored into a value_expression and
//  a primary_expression. This prevents the illogical expressions:
//  constant++, constant[expression], nil++, nil[expression]. The
//  resolution is to join the two parts of the primary_expression
//  later in the grammar
//

value_expression:

        constant_suffix

        | aggregate_value

        | MU_NIL
        {
            Mu::DataNode *node = AS->constant(CONTEXT->nilType());
            node->_data._Pointer = NULL;
            $$ = node;
        }
;

//
//  primary_expression has some shift/reduce problems (as seen at top
//  of this file). These are resolved correctly (shift) and are caused
//  by limited look-ahead. They could be eliminated by factoring out a
//  function_call_expression or some such that looks like this:
//
//      function_call:
//              primary_expression
//              primary_expression '(' argument_slot_list_opt ')'
//              type '(' argument_slot_list_opt ')'
//
//  or something similar. However, this would create a lot of
//  additional complexity because the primary expression would have to
//  be converted back to a symbol (if its a constant) etc, blah,
//  blah. Maybe someday.
//

primary_expression:

        value_prefix MU_MEMBERVARIABLE
        {
            if (!($$ = AS->referenceMemberVariable($2,$1)))
            {
                ParseError(state, "Unable to reference member variable %s "
                           "of type %s",
                           $2->fullyQualifiedName().c_str(),
                           $1->type()->fullyQualifiedName().c_str());
                YYERROR;
            }

            AS->prefixScope(0);
        }

        | value_prefix symbol
        {
            if (Mu::MemberFunction* f = dynamic_cast<Mu::MemberFunction*>($2))
            {
                if (! ($$ = AS->methodThunk(f, $1)) ) YYERROR;
            }
            else if (! ($$ = AS->unresolvableMemberReference($2->name(), $1)) )
            {
                YYERROR;
            }

            AS->prefixScope(0);
        }

        | value_prefix symbol '(' {AS->prefixScope(0);} argument_slot_list_opt ')'
        {
            AS->prefixScope($1->type());

            if (Mu::Function* f =
                $2->scope()->findSymbolOfType<Mu::Function>($2->name()))
            {
                AS->insertNodeAtFront($5, AS->dereferenceLValue($1)); 
                $$ = AS->call(f, $5);
                AS->removeNodeList($5);

                if (!$$)
                {
                    ParseError(state, "Function argument mis-match");
                    YYERROR;
                }
            }
            else
            {
                AS->removeNodeList($5);
                ParseError(state, "Expected an object method for \"%s\".", $2->fullyQualifiedName().c_str());
                YYERROR;
            }

            AS->prefixScope(0);
        }

        | value_prefix identifier
        {
            if (! ($$ = AS->unresolvableMemberReference($2, $1)) ) YYERROR;
            AS->prefixScope(0);
        }


        | '(' expression ')'
        {
            $$ = $2;
        }

        | '[' list_element_list opt_comma ']'
        {
            if (!($$ = AS->listNode($2))) YYERROR;
            AS->removeNodeList($2);
        }

        | '(' naked_operator ')'
        {
            if (Mu::Name op = CONTEXT->lookupName($2))
            {
                if (Mu::Function* f = AS->findTypeInScope<Mu::Function>(op))
                {
                    if (!($$ = AS->functionConstant(f)))
                    {
                        ParseError(state, "Unable to reference function %s",
                                   f->fullyQualifiedName().c_str());
                        YYERROR;
                    }
                }
            }
            else
            {
                ParseError(state, "Operator %s not defined in this scope", $2);
                YYERROR;
            }
        }

        | scope_prefix naked_operator 
        { 
            if (Mu::Name op = CONTEXT->lookupName($2))
            {
                if (Mu::Function* f = AS->findTypeInScope<Mu::Function>(op))
                {
                    if (!($$ = AS->functionConstant(f)))
                    {
                        ParseError(state, "Unable to reference function %s",
                                   f->fullyQualifiedName().c_str());
                        YYERROR;
                    }
                }
            }
            else
            {
                ParseError(state, "Operator %s not defined in this scope", $2);
                YYERROR;
            }

            AS->prefixScope(0);
        }

        | symbol 
        {
            if (Mu::Variable *v = AS->findTypeInScope<Mu::Variable>($1->name()))
            {
                if (Mu::StackVariable* sv =
                    dynamic_cast<Mu::StackVariable*>(v))
                {
                    if (!AS->findStackVariable(sv))
                    {
                        if (AS->currentFunction())
                        {
                            v = AS->declareFreeVariable(v->storageClass(), v->name());
                            AS->currentFunction()->addSymbol(v);
                        }
                        else
                        {
                            v = 0;
                            YYERROR;
                        }
                    }
                }

                if (v)
                {
                    if (!($$ = AS->referenceVariable(v)))
                    {
                        ParseError(state, "Unable to reference variable %s",
                                   v->fullyQualifiedName().c_str());
                        YYERROR;
                    }
                }
            }
            else if (const Mu::MemberFunction* f =
                     dynamic_cast<const Mu::MemberFunction*>($1))
            {
                if (! ($$ = AS->methodThunk(f, AS->dereferenceVariable("this")) ) )
                {
                    ParseError(state, "Unable to create method thunk for %s",
                               f->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else if (const Mu::Function* f = 
                     dynamic_cast<const Mu::Function*>($1))
            {
                if (!($$ = AS->functionConstant(f)))
                {
                    ParseError(state, "Unable to reference function %s",
                               f->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "Expecting a variable.");
                YYERROR;
            }
        }

        | symbol '(' argument_slot_list_opt ')'
        {
            if (Mu::Variable* v = dynamic_cast<Mu::Variable*>($1))
            {
                if (Mu::StackVariable* sv =
                    dynamic_cast<Mu::StackVariable*>(v))
                {
                    if (!AS->findStackVariable(sv))
                    {
                        if (AS->currentFunction())
                        {
                            v = AS->declareFreeVariable(v->storageClass(), v->name());
                            AS->currentFunction()->addSymbol(v);
                        }
                        else
                        {
                            v = 0;
                            YYERROR;
                        }
                    }
                }

                $$ = AS->call(v, $3);
            }
            else if (const Mu::MemberFunction* f =
                     dynamic_cast<const Mu::MemberFunction*>($1))
            {
                AS->insertNodeAtFront($3, AS->dereferenceVariable("this"));
                $$ = AS->call($1, $3);
            }
            else
            {
                $$ = AS->call($1, $3);
            }

            if (!$$)
            {
                if (AS->containsNoOps($3))
                {
                    ParseError(state, "bad partial evaluation of %s",
                               $1->fullyQualifiedName().c_str());
                }
                else
                {
                    ParseError(state, "trying to call function through %s",
                               $1->fullyQualifiedName().c_str());
                }

                YYERROR;
            }

            AS->removeNodeList($3);

        }

        | identifier 
        { 
            if (! ($$ = AS->unresolvableReference($1)) )YYERROR;
        }

        | prefixed_symbol
        {
            if (Mu::Variable *v = 
                $1->scope()->findSymbolOfType<Mu::Variable>($1->name()))
            {
                if (!($$ = AS->referenceVariable(v)))
                {
                    ParseError(state, "Unable to reference variable %s",
                               v->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else if (const Mu::Function* f = 
                     dynamic_cast<const Mu::Function*>($1))
            {
                if (!($$ = AS->functionConstant(f)))
                {
                    ParseError(state, "Unable to reference function %s",
                               f->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "Expecting a variable.");
                YYERROR;
            }
        }

        | prefixed_symbol '(' argument_slot_list_opt ')'
        {
            /*
            //  NOTE: the thrird arg to call() will prevent dynamic
            //  dispatch. 
            */

            AS->prefixScope($1->scope());
            $$ = AS->call($1, $3, false);
            AS->prefixScope(0);
            AS->removeNodeList($3);

            if (!$$)
            {
                ParseError(state, "Function argument mis-match");
                YYERROR;
            }
        }

        | type_constructor
        {
            if (Mu::VariantTagType* tt = dynamic_cast<Mu::VariantTagType*>($1))
            {
                if (tt->representationType() == CONTEXT->voidType())
                {
                    NodeList nl = AS->emptyNodeList();
                    $$ = AS->call($1, nl);
                    AS->removeNodeList(nl);

                    if (!$$)
                    {
                        ParseError(state, "trying to use type constructor %s",
                                   $1->fullyQualifiedName().c_str());
                        YYERROR;
                    }
                }
                else
                {
                    ParseError(state, "type constructor %s uses %s "
                               "construction semantics",
                               tt->fullyQualifiedName().c_str(),
                               tt->representationType()->fullyQualifiedName().c_str());
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "%s is not a tag (internal error?)",
                           $1->fullyQualifiedName().c_str());
                YYERROR;
            }
        }

        | type_constructor '(' argument_slot_list_opt ')'
        {
            $$ = AS->call($1, $3);

            if (!$$)
            {
                if (AS->containsNoOps($3))
                {
                    ParseError(state, "bad partial evaluation of type constuctor %s",
                               $1->fullyQualifiedName().c_str());
                }
                else
                {
                    ParseError(state, "trying to use type constuctor %s",
                               $1->fullyQualifiedName().c_str());
                }

                YYERROR;
            }

            AS->removeNodeList($3);
        }

        | type '(' argument_slot_list_opt ')'
        {
            $$ = AS->call($1, $3);

            if (!$$)
            {
                if (AS->containsNoOps($3))
                {
                    ParseError(state, "bad partial evaluation of %s",
                               $1->fullyQualifiedName().c_str());
                }
                else
                {
                    ParseError(state, "trying to call function through %s",
                               $1->fullyQualifiedName().c_str());
                }

                YYERROR;
            }

            AS->removeNodeList($3);
        }

        | identifier '(' argument_slot_list_opt ')'
        {
            $$ = AS->unresolvableCall($1, $3);
            AS->removeNodeList($3);

            if (!$$)
            {
                ParseError(state, "internal error creating "
                           "unresolved call through %s",
                           str($1));
                YYERROR;
            }
        }

        | postfix_expression '(' argument_slot_list_opt ')'
        {
            if (!($$ = AS->call($1, $3)))
            {
                ParseError(state, "cannot call expression");
                YYERROR;
            }

            AS->removeNodeList($3);
        }
;

aggregate_value:

        type { AS->pushType($1); } aggregate_initializer
        {
            $$ = $3;
            AS->popType();
        }
;

opt_comma:

        /* empty */ { $$ = 0; }
        | ','       { $$ = 0; }
;
 

aggregate_next_field_type:

        aggregate_initializer_list ','
        {
            AS->popType();
            AS->pushType(AS->topType()->fieldType($1.size())); 
            $$ = $1;
        }
;

aggregate_initializer_list:

        aggregate_initializer
        {
            $$ = AS->newNodeList($1);
        }

        | aggregate_next_field_type aggregate_initializer
        {
            $$ = $1;
            $$.push_back($2);
        }

        | aggregate_next_field_type conditional_expression
        { 
            $$ = $1; 
            $$.push_back($2);
        }
;

aggregate_initializer:

        '{' '}'
        {
            if (AS->topType())
            {
                const Mu::Type* t = AS->topType();
                NodeList nl = AS->emptyNodeList();
                $$ = AS->call(t, nl);
                AS->removeNodeList(nl);
                
                if (!$$)
                {
                    ParseError(state, "Aggregate initializer type mismatch");
                    YYERROR;
                }
            }
            else
            {
                ParseError(state, "A type is required for this aggregate initializer");
                YYERROR;
            }
        }

        | '{' 
        { 
            if (AS->topType())
            {
                AS->pushType(AS->topType()->fieldType(0)); 
            }
            else
            {
                ParseError(state, "A type is required for this aggregate initializer");
                YYERROR;
            }
        } 

           argument_list '}'
        {
            //
            // NOTE: continuation of above
            //

            AS->popType();
            const Mu::Type* t = AS->topType();
            $$ = AS->call(t, $3);
            AS->removeNodeList($3);

            if (!$$)
            {
                ParseError(state, "Aggregate initializer type mismatch");
                YYERROR;
            }
        }

        | '{' 
        { 
            if (AS->topType())
            {
                AS->pushType(AS->topType()->fieldType(0)); 
            }
            else
            {
                ParseError(state, "A type is required for this aggregate initializer");
                YYERROR;
            }
        }

        aggregate_initializer_list opt_comma  '}'
        {   
            //
            // NOTE: continuation of above
            //
            AS->popType();
            const Mu::Type* t = AS->topType();
            const Mu::Function* f = 
                t->findSymbolOfType<Mu::Function>(t->name());

            if (f)
            {
                $$ = AS->call(t, $3);
            }
            else
            {
                $$ = AS->call(yyFunction(t), $3);
            }

            AS->removeNodeList($3);

            if (!$$)
            {
                ParseError(state, "Aggregate initializer type mismatch");
                YYERROR;
            }
        }
;

symbol_type_or_module:

        symbol

        | prefixed_symbol

        | type_or_module
;

type_or_module:

        type { $$ = $1; }

        | module { $$ = $1; }
;

prefixed_symbol:

        MU_PREFIXEDSYMBOL

        | scope_prefix symbol
        {
            $$ = $2;
            AS->prefixScope(0);
        }
;


symbol:

        MU_SYMBOL
;

scope_prefix:

        postfix_type '.'
        {
            AS->prefixScope($$ = $1);
        }

        | module '.'
        {
            AS->prefixScope($$ = $1);
        }
;


type:
        unary_type
;

type_list:

        /* empty */
        {
            $$ = AS->emptySymbolList();
        }

        | unary_type
        {
            $$ = AS->newSymbolList($1);
        }

        | type_list ',' unary_type
        {
            $$ = $1;
            $$.push_back($3);
        }
;

type_constructor:

        MU_TYPECONSTRUCTOR
        | scope_prefix MU_TYPECONSTRUCTOR
        {
            $$ = $2;
            AS->prefixScope(0);
        }
;

unary_type:

        postfix_type

        | MU_TYPEMODIFIER unary_type
        {
            if ( !($$ = (Mu::Type*)$1->transform($2, CONTEXT)) )
            {
                ParseError(state, "%s type modifier does not apply to type %s",
                           $1->fullyQualifiedName().c_str(),
                           $2->fullyQualifiedName().c_str());
            }
        }
;

postfix_type:

        primary_type

        | postfix_type '[' type ']' 
        { 
            //
            //  map type (e.g., string[string])
            //

            $$ = $1; 
        }

        | postfix_type '[' array_multidimension_declaration ']'
        {
            if ( !($$ = CONTEXT->arrayType($1, AS->intList())) )
            {
                bool dynamic = false;
                for (int i=0; i < AS->intList().size(); i++)
                    if (AS->intList()[i] == 0) dynamic = true;

                if (AS->intList().size() > 1 && dynamic)
                {
                    ParseError(state, "unable to make "
                               "%d dimensional dynamic array of %s",
                               AS->intList().size(),
                               $1->fullyQualifiedName().c_str());
                }
                else
                {
                    ParseError(state, "unable to make "
                               "%d dimensional array of %s",
                               AS->intList().size(),
                               $1->fullyQualifiedName().c_str());
                }

                YYERROR;
            }
            AS->clearInts();
        }
;


primary_type:

        MU_TYPE

        | scope_prefix MU_TYPE
        {
            $$ = $2;
            AS->prefixScope(0);
        }

        | '\'' identifier_or_symbol
        {
            $$ = AS->declareTypeVariable(str($2));
        }

        | '\'' constant
        {
            //$$ = AS->declarePlacementParameter($2);
            ParseError(state, "placeholder parameter not yet implemented");
            YYERROR;
        }

        | '[' type ']'
        {
            //
            //  List type
            //

            $$ = CONTEXT->listType($2);
        }

        | '(' type_list ')' /* tuple type -or- parens */
        {
            if ($2.size() == 1)
            {
                $$ = static_cast<Mu::Type*>($2.front());
            }
            else
            {
                $$ = AS->declareTupleType($2);
            }

            AS->removeSymbolList($2);
        }

        | '(' type ';' type_list ')'
        {
            //
            //  Function type
            //

            Mu::Signature* s = new Mu::Signature();
            s->push_back($2);
            for (int i=0; i < $4.size(); i++) 
                s->push_back(static_cast<const Mu::Type*>($4[i]));
            $$ = CONTEXT->functionType(s);
            AS->removeSymbolList($4);
        }
;

array_multidimension_declaration:

        array_dimension_declaration
        {
            AS->addInt($1);
        }

        | array_multidimension_declaration ',' array_dimension_declaration
        {
            AS->addInt($3);
        }
;

array_dimension_declaration:

        /* empty */
        {
            $$ = 0;
        }
        | MU_INTCONST
;

module:

        MU_MODULESYMBOL

        | scope_prefix MU_MODULESYMBOL
        {
            $$ = $2;
            AS->prefixScope(0);
        }
;

identifier_or_symbol_or_module_or_type:

        identifier_or_symbol_or_type
        | MU_MODULESYMBOL  { $$ = $1->name().nameRef(); }
;

identifier_or_symbol_or_type:

        identifier_or_symbol
        | MU_TYPE          { $$ = $1->name().nameRef(); }
;

identifier_or_symbol:

        identifier
        | symbol        { $$ = $1->name().nameRef(); }
;

naked_operator:

          '+' { $$[0] = '+'; $$[1] = 0; }
        | '-' { $$[0] = '-'; $$[1] = 0; }
        | '/' { $$[0] = '/'; $$[1] = 0; }
        | '*' { $$[0] = '*'; $$[1] = 0; }
        | '%' { $$[0] = '%'; $$[1] = 0; }
        | '^' { $$[0] = '^'; $$[1] = 0; }
        | '|' { $$[0] = '|'; $$[1] = 0; }
        | '&' { $$[0] = '&'; $$[1] = 0; }
        | '<' { $$[0] = '<'; $$[1] = 0; }
        | '>' { $$[0] = '>'; $$[1] = 0; }
        | ':' { $$[0] = ':'; $$[1] = 0; }
        | '[' ']' { $$[0] = '['; $$[1] = ']'; $$[2] = 0; }
        | '(' ')' { $$[0] = '('; $$[1] = ')'; $$[2] = 0; }
        | MU_LESSEQUALS
        | MU_GREATEREQUALS
        | MU_LSHIFT
        | MU_RSHIFT
        | MU_EQUALS
        | MU_NOTEQUALS
        | MU_EQ
        | MU_NEQ
;

identifier:

        MU_IDENTIFIER
;

constant_suffix:

        constant

        | constant identifier_or_symbol 
        {    
            $$ = AS->suffix($1, $2);
        }
;

constant:

        MU_FLOATCONST
        {
            Mu::DataNode *node = AS->constant(CONTEXT->floatType());
            node->_data._float = $1;
            $$ = node;
        }

        | MU_INTCONST
        {
            if ($1 >= Mu::int64(numeric_limits<int>::max()) ||
                $1 <= Mu::int64(numeric_limits<int>::min()))
            {
                Mu::DataNode *node = AS->constant(CONTEXT->int64Type());
                node->_data._int64 = $1;
                $$ = node;
            }
            else
            {
                Mu::DataNode *node = AS->constant(CONTEXT->intType());
                node->_data._int = int($1);
                $$ = node;
            }
        }

        | MU_BOOLCONST
        {
            Mu::DataNode *node = AS->constant(CONTEXT->boolType());
            node->_data._bool = $1;
            $$ = node;
        }

        | MU_STRINGCONST
        {
            Mu::DataNode *node = AS->constant(CONTEXT->stringType(), $1);
            node->_data._Pointer = (Mu::Pointer)$1;
            $$ = node;
        }

        | MU_CHARCONST
        {
            if ($1 == -1)
            {
                ParseError(state, "invalid character constant");
                YYERROR;
            }
            else
            {
                Mu::DataNode *node = AS->constant(CONTEXT->charType());
                node->_data._int = $1;
                $$ = node;
            }
        }

        | MU_SYMBOLICCONST
        {
            $$ = AS->constant($1);
        }

        | scope_prefix MU_SYMBOLICCONST
        {
            $$ = AS->constant($2);
            AS->prefixScope(0);
        }
;

%%

void
ParseError(void* state, const char *text, ...)
{
    char temp[256];
    yyFlexLexer* lexer = reinterpret_cast<yyFlexLexer*>(state);

    va_list ap;
    va_start(ap,text);
    vsprintf(temp,text,ap);
    va_end(ap);

    reinterpret_cast<yyFlexLexer*>(state)->assembler()->reportError(temp);
}

void
OpError(void* state, const char *op, Mu::Node *a, Mu::Node *b)
{
    using namespace Mu;

    if (!a)
    {
        ParseError(state, "operator%s internal error", op);
        return;
    }

    const Type *atype = a->type();

    if (a && b)
    {
        const Type *btype = b->type();
        ParseError(state, "operator%s is not defined for: %s %s %s",
                   op,
                   atype->fullyQualifiedName().c_str(),
                   op,
                   btype->fullyQualifiedName().c_str());
    }
    else
    {
        ParseError(state, "operator%s is not defined for: %s%s",
                   op,
                   op,
                   atype->fullyQualifiedName().c_str());
    }
}

void
ModuleLocationError(void* state, const char *text, ...)
{
    const Mu::Environment::SearchPath& paths = Mu::Environment::modulePath();

    for (int i=0; i < paths.size(); i++)
    {
        const Mu::String& p = paths[i];

        ParseError(state, "Can't locate module named \"%s\" in path (%s).", 
                   text, p.c_str());
    }
}


void
ParseWarning(void* state, const char *text, ...)
{
    char temp[256];
    yyFlexLexer* lexer = (yyFlexLexer*)state;

    va_list ap;
    va_start(ap,text);
    vsprintf(temp,text,ap);
    va_end(ap);

    reinterpret_cast<yyFlexLexer*>(state)->assembler()->reportWarning(temp);
}

int
yylex(void* yylval, void* state)
{
    yyFlexLexer* lexer = reinterpret_cast<yyFlexLexer*>(state);
    lexer->setYYSTYPE(yylval);
    return lexer->yylex();
}

namespace Mu {

Mu::Process*
Parse(const char *sourceName, Mu::NodeAssembler *assembler)
{
    //
    //  Initialize state
    //

    ::yydebug = 0;
    yyFlexLexer lexer(&assembler->context()->inputStream(),
                      &assembler->context()->outputStream());
    
    Mu::Context::SourceFileScope sourceScope(assembler->context(), 
                                             assembler->context()->internName(sourceName));

    lexer.init(sourceName, assembler);

    //
    //  Call the yacc parser
    //

    if (yyparse(&lexer)) return 0;

    //
    //  Patch any unresolved symbols
    //

    assembler->patchUnresolved();

    //
    //  Finish
    //

    return assembler->process();
}   

} // namespace Mu

const Mu::Function*
yyFunction(const Mu::Symbol *sym)
{
    using namespace Mu;

    for (const Symbol* s = sym->firstOverload(); s; s = s->nextOverload())
    {
        if (const Function *f = dynamic_cast<const Function*>(s))
        {
            return f;
        }
    }

    return 0;
}

const Mu::Type*
yyConstantExpression(Mu::Node *node)
{
    if (node)
    {
        if (const Mu::Type *type = 
            dynamic_cast<const Mu::Type*>(node->symbol()))
        {
            return type;
        }
    }

    return 0;
}

