%{
/* -*- mode: C++ -*-
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//"alias"                 { as->charNum()+=yyleng; return MU_ALIAS; }
//"type"                  { as->charNum()+=yyleng; return MU_TYPE_KEY; }
//"kind"                  { as->charNum()+=yyleng; return MU_KIND_KEY; }
*/

#include <MuLang/generated/Grammar.h>
#include <iostream>
#include <sstream>
#include <Mu/config.h>
#include <Mu/utf8_v2/checked.h>
using namespace std;

#define YY_NO_UNISTD_H

#define yydebug MUYYdebug
extern int	yydebug;
#define LVAL reinterpret_cast<YYSTYPE*>(_yystype)

%}

%option always-interactive
%option noyywrap
%option c++

Identifier		[A-Za-z_][A-Za-z_0-9]*
NewLine 		\r?[\n]
WhiteSpace		[[:blank:]]+
NonWhiteSpace		[^[:blank:]]+
FloatNumA 		[[:digit:]]*\.[[:digit:]]+([Ee][+-]?[[:digit:]]+)?
FloatNumB 		[[:digit:]]+\.?([Ee][+-]?[[:digit:]]+)?
IntNum			[[:digit:]]+
HexNum			0x[[:xdigit:]]+
UnicodeEsc		\\u[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]+
CUnicodeEsc             \'\\u[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]+\'
OctalEsc		\\[0-3][0-7][0-7]
BOM                     \xef\xbb\xbf

%x COMMENT
%x STRING
%x VERBATIM
%x IDMODE
%x CHAR

%%

{BOM}                   { /* ignore BOMs in file */ }
{WhiteSpace}            { as->addChar(yyleng);		        }
{NewLine}               { as->addLine(1); as->setChar(0);	}

"true" |
"false"			{
			    as->addChar(1);
			    LVAL->_int = *yytext == 't';
			    return MU_BOOLCONST;
			}

"nil"			{ as->addChar(yyleng); return MU_NIL; }

"_"                     { as->addChar(yyleng); return '_'; }
"if"			{ as->addChar(yyleng); return MU_IF; }
"else"			{ as->addChar(yyleng); return MU_ELSE; }
"then"			{ as->addChar(yyleng); return MU_THEN; }
"case"			{ as->addChar(yyleng); return MU_CASE; }
"do"			{ as->addChar(yyleng); return MU_DO; }
"for"			{ as->addChar(yyleng); return MU_FOR; }
"let"                   { as->addChar(yyleng); return MU_LET; }
"for_each"		{ as->addChar(yyleng); return MU_FOREACH; }
"for_index"		{ as->addChar(yyleng); return MU_FORINDEX; }
"repeat"		{ as->addChar(yyleng); return MU_REPEAT; }
"while"			{ as->addChar(yyleng); return MU_WHILE; }
"break"			{ as->addChar(yyleng); return MU_BREAK; }
"continue"		{ as->addChar(yyleng); return MU_CONTINUE; }
"try"			{ as->addChar(yyleng); return MU_TRY; }
"catch"			{ as->addChar(yyleng); return MU_CATCH; }
"const"			{ as->addChar(yyleng); return MU_CONST; }
"throw"			{ as->addChar(yyleng); return MU_THROW; }
"return"		{ as->addChar(yyleng); return MU_RETURN; }
"require"		{ as->addChar(yyleng); return MU_REQUIRE; }
"use"			{ as->addChar(yyleng); return MU_USE; }
"global"		{ as->addChar(yyleng); return MU_GLOBAL; }
"yield"                 { as->addChar(yyleng); return MU_YIELD; }
"class:"                { as->addChar(yyleng); return MU_CLASS; }
"interface:"            { as->addChar(yyleng); return MU_INTERFACE; }
"method:"               { as->addChar(yyleng); return MU_METHOD; }
"module:"               { as->addChar(yyleng); return MU_MODULE; }
"function:"|"\\:"	{ as->addChar(yyleng); return MU_FUNCTION; }
"documentation:"	{ as->addChar(yyleng); return MU_DOCUMENTATION; }
"union:"                { as->addChar(yyleng); return MU_UNION; }
"type:"                 { as->addChar(yyleng); return MU_TYPE_KEY; }
"alias:"                { as->addChar(yyleng); return MU_ALIAS; }
"operator:"		{ as->addChar(yyleng); BEGIN IDMODE; return MU_OPERATOR; }

<IDMODE>{WhiteSpace}	{ as->addChar(yyleng); }
<IDMODE>{NewLine}	{ as->addLine(1); as->setChar(0); }
<IDMODE>{NonWhiteSpace}	{
			  as->addChar(yyleng);
			  LVAL->_name = as->context()->internName(yytext).nameRef();
			  BEGIN 0;
			  return MU_IDENTIFIER;
			}

\(			{ as->addChar(1); return '('; }
\)			{ as->addChar(1); return ')'; }

"//".*{NewLine}		{ as->addLine(1); as->setChar(0);		    }
"/*"			{ as->addChar(yyleng); BEGIN COMMENT;	    }
<COMMENT>{NewLine}	{ as->addLine(1); as->setChar(0);		    }
<COMMENT>"*/"		{ as->addChar(yyleng); BEGIN 0;		    }
<COMMENT>.		{ as->addChar(1);				    }

"\"\"\""                { as->addChar(1); yyNewString(); BEGIN VERBATIM; }

<VERBATIM>{UnicodeEsc}	{
			  as->addChar(yyleng);
			  unsigned int u = strtol(yytext+2,(char**)0,16);
			  yyAddToString(u);
			}

<VERBATIM>{OctalEsc}	{
			  as->addChar(yyleng);
			  unsigned int u = strtol(yytext+1,(char**)0,8);
			  yyAddToString(u);
			}

<VERBATIM>{NewLine}     { as->addLine(1); as->setChar(0); yyAddToString('\n'); }
<VERBATIM>"\\r"         { as->addChar(yyleng); yyAddToString('\r'); }
<VERBATIM>"\\\\"	{ as->addChar(yyleng); yyAddToString('\\'); }
<VERBATIM>"\\\""	{ as->addChar(yyleng); yyAddToString('"'); }

<VERBATIM>"\"\"\""	{
			  as->addChar(1);
                          BEGIN 0;

                          LVAL->_object = yyReturnString();
                          return MU_STRINGCONST;
			}

<VERBATIM>.		{ as->addChar(1); yyAddToString(*yytext);}

"\""			{ as->addChar(1); yyNewString(); BEGIN STRING; }

<STRING>{UnicodeEsc}	{
			  as->addChar(yyleng);
                          Mu::UTF32Char u = strtol(yytext+2,(char**)0,16);
			  yyAddToString(u);
			}

<STRING>{OctalEsc}	{
			  as->addChar(yyleng);
                          Mu::UTF32Char u = strtol(yytext+1,(char**)0,8);
			  yyAddToString(u);
			}

<STRING>{NewLine}       { as->addLine(1); as->setChar(0); yyAddToString('\n'); }
<STRING>"\\b"		{ as->addChar(yyleng); yyAddToString('\b'); }
<STRING>"\\t"		{ as->addChar(yyleng); yyAddToString('\t'); }
<STRING>"\\n"		{ as->addChar(yyleng); yyAddToString('\n'); }
<STRING>"\\f"		{ as->addChar(yyleng); yyAddToString('\f'); }
<STRING>"\\r"		{ as->addChar(yyleng); yyAddToString('\r'); }
<STRING>"\\\""		{ as->addChar(yyleng); yyAddToString('"'); }
<STRING>"\'"		{ as->addChar(yyleng); yyAddToString('\''); }
<STRING>"\\\\"		{ as->addChar(yyleng); yyAddToString('\\'); }

<STRING>"\""		{
			  as->addChar(1);
                          BEGIN 0;

                          LVAL->_object = yyReturnString();
                          return MU_STRINGCONST;
			}

<STRING>.		{ as->addChar(1); yyAddToString(*yytext);}

{IntNum} |
{HexNum}		{
			    LVAL->_int   = strtoll(yytext,0,0);
			    as->addChar(yyleng);
			    return MU_INTCONST;
			}

{FloatNumA} |
{FloatNumB}             {
			    LVAL->_float = atof(yytext);
			    as->addChar(yyleng);
			    return MU_FLOATCONST;
			}


\'\\[btnfr\\]\'         {
                            switch (yytext[2])
                            {
                              case 'b': LVAL->_int = int('\b'); break;
                              case 't': LVAL->_int = int('\t'); break;
                              case 'n': LVAL->_int = int('\n'); break;
                              case 'f': LVAL->_int = int('\f'); break;
                              case 'r': LVAL->_int = int('\r'); break;
                              default:
                                  LVAL->_int = yytext[2]; break;
                            }

                            as->addChar(yyleng);
                            return MU_CHARCONST;
                        }

{CUnicodeEsc}           {
                            as->addChar(yyleng);
                            unsigned int u = strtol(yytext+3,(char**)0,16);
                            LVAL->_int = u;
                            return MU_CHARCONST;
                        }

\'[^\']{1,4}\' |
\'\\\'\'                {
                            as->addChar(1);

                            if (!strcmp(yytext, "\'\\\'\'"))
                            {
                                LVAL->_int = '\'';
                            }
                            else
                            {
                                try
                                {
                                    char* t = yytext + 1;
                                    char* e = t + yyleng - 2;
                                    LVAL->_int = utf8::next(t, e);

                                    // bad
                                    if (t != e) LVAL->_int = -1;
                                }
                                catch (std::exception &exc)
                                {
                                    cerr << "ERROR: " << exc.what() << endl;
                                    LVAL->_int = -1;
                                }
                            }

                            return MU_CHARCONST;
                        }

"{%%debug%%}"           {
			    yydebug=!yydebug;
			    as->addChar(yyleng);
			}

".."                    {
			    as->addChar(yyleng);
			    return MU_DOUBLEDOT;
                        }

"..."			{
			    as->addChar(yyleng);
			    return MU_ELLIPSIS;
			}

"...."			{
			    as->addChar(yyleng);
			    return MU_QUADRUPLEDOT;
			}
"++" |
"--"			{
			    as->addChar(yyleng);
			    strcpy(LVAL->_op,yytext);
			    return MU_OP_INC;
			}

":="			{
			    as->addChar(yyleng);
			    strcpy(LVAL->_op,yytext);
			    return MU_OP_SYMASSIGN;
			}

"+="  |
"-="  |
"*="  |
"/="  |
"%="  |
"&="  |
"^="  |
"|="  |
"<<=" |
">>="			{
			    as->addChar(yyleng);
			    strcpy(LVAL->_op,yytext);
			    return MU_OP_ASSIGN;
			}

"eq"			{ as->addChar(yyleng); strcpy(LVAL->_op,yytext); return MU_EQ; }
"neq"			{ as->addChar(yyleng); strcpy(LVAL->_op,yytext); return MU_NEQ; }

"=>"			{ as->addChar(yyleng); return MU_DOUBLEARROW; }
"->"			{ as->addChar(yyleng); return MU_ARROW; }

">>"			{   as->addChar(yyleng); return MU_RSHIFT; }
"<<"			{   as->addChar(yyleng); return MU_LSHIFT; }
"<="			{   as->addChar(yyleng); return MU_LESSEQUALS;	}
">="			{   as->addChar(yyleng); return MU_GREATEREQUALS;  }
"=="			{   as->addChar(yyleng); return MU_EQUALS;		}
"!="			{   as->addChar(yyleng); return MU_NOTEQUALS;	}
"&&"			{   as->addChar(yyleng); return MU_LOGICAND;	}
"||"			{   as->addChar(yyleng); return MU_LOGICOR;	}
">"			{   as->addChar(1); return int('>'); }
"<"			{   as->addChar(1); return int('<'); }

{Identifier}		{
			    as->addChar(yyleng);
			    return yyIdentifier(yytext);
			}

<<EOF>>			{
			    yyterminate();
			}

.			{
                            as->addChar(1);
			    return *yytext;
			}

%%

int
yyFlexLexer::yyIdentifier(const char *text)
{
    //
    //	This function determines the priority that various identifiers
    //	have during parsing. For example, if there is a type and
    //	function which both have the same scope and name, then the
    //	type will be returned.
    //

    const Mu::NodeAssembler::ScopeState* scope = as->scopeState();

    if (Mu::Name n = as->context()->lookupName(text))
    {
	Mu::Symbol*           symbol    = 0;
	Mu::Module*           module    = 0;
	Mu::Type*             type      = 0;
	Mu::SymbolicConstant* constant  = 0;
	Mu::TypeModifier*     typemod   = 0;
	Mu::Alias*            alias     = 0;
	Mu::MemberVariable*   mvar      = 0;
	Mu::GlobalVariable*   gvar      = 0;
	Mu::Construct*        construct = 0;

	if (as->prefixScope())
	{
	    if (Mu::Symbol *s = as->prefixScope()->findSymbol(n))
	    {
                bool followedAlias = false;

		while (1)
		{
		    module    = 0;
		    type      = 0;
		    alias     = 0;
		    mvar      = 0;
		    constant  = 0;

		    for (Mu::Symbol *sym = s->firstOverload();
			 sym;
			 sym = sym->nextOverload())
		    {
			if (!type)
			{
			    if (type  = dynamic_cast<Mu::Type*>(sym))
				continue;
			}

			if (!module)
			{
			    if (module = dynamic_cast<Mu::Module*>(sym))
				continue;
			}

			if (!mvar)
			{
			    if (mvar = dynamic_cast<Mu::MemberVariable*>(sym))
				continue;
			}

			if (!alias)
			{
			    if (alias  = dynamic_cast<Mu::Alias*>(sym))
				continue;
			}

			if (!constant)
			{
			    if (constant =
				dynamic_cast<Mu::SymbolicConstant*>(sym))
				continue;
			}
		    }

		    if (type)
		    {
			LVAL->_type = type;
			return dynamic_cast<Mu::VariantTagType*>(type) ? MU_TYPECONSTRUCTOR : MU_TYPE;
		    }

		    if (module)
		    {
			LVAL->_module = module;
			return MU_MODULESYMBOL;
		    }

		    if (mvar)
		    {
			LVAL->_memberVar = mvar;
			return MU_MEMBERVARIABLE;
		    }

		    if (constant)
		    {
			LVAL->_symConstant = constant;
			return MU_SYMBOLICCONST;
		    }

		    if (alias)
		    {
                        followedAlias = true;
			s = alias->alias();
		    }
		    else
		    {
			LVAL->_symbol = s;
			return followedAlias ? MU_PREFIXEDSYMBOL : MU_SYMBOL;
		    }
		}
	    }

	    LVAL->_name = as->context()->internName(text).nameRef();
	    return MU_IDENTIFIER;
	}


	//
	//  If there's a type anywhere in the current scope, that must
	//  be returned. Type symbols cannot be shadowed.
	//

	//for (int i=scope.size(); i--;)
        for (const Mu::NodeAssembler::ScopeState* ss = scope; ss; ss = ss->parent)
	{
	    //
	    //  XXX Some combination of calling rv.runtime.eval (mu eval from
	    //  python) early (during the creation of an "immediate" python
	    //  minor mode) produces a situation where ss->symbol is NULL,
	    //  hence a crash.  I'm adding this check to turn the crash into an
	    //  exception since I can't figure out how to actually fix it.
	    //
	    if (ss->symbol)
	    if (Mu::Symbol *s = ss->symbol->findSymbol(n))
	    {
                bool followedAlias = false;

		while (1)
		{
		    module    = 0;
		    type      = 0;
		    alias     = 0;
		    construct = 0;
		    typemod   = 0;
		    constant  = 0;

		    for (Mu::Symbol *sym = s->firstOverload();
			 sym;
			 sym = sym->nextOverload())
		    {
			if (!type)
			{
			    if (type = dynamic_cast<Mu::Type*>(sym))
				continue;
			}

			if (!typemod)
			{
			    if (typemod = dynamic_cast<Mu::TypeModifier*>(sym))
				continue;
			}

			if (!module)
			{
			    if (module = dynamic_cast<Mu::Module*>(sym))
				continue;
			}

			if (!constant)
			{
			    if (constant =
				dynamic_cast<Mu::SymbolicConstant*>(sym))
				continue;
			}

			if (!alias)
			{
			    if (alias = dynamic_cast<Mu::Alias*>(sym))
				continue;
			}

			if (!construct)
			{
			    if (construct = dynamic_cast<Mu::Construct*>(sym))
				continue;
			}
		    }

		    if (type)
		    {
			LVAL->_type = type;
			return dynamic_cast<Mu::VariantTagType*>(type) ? MU_TYPECONSTRUCTOR : MU_TYPE;
		    }

		    if (typemod)
		    {
			LVAL->_typeModifier = typemod;
			return MU_TYPEMODIFIER;
		    }

		    if (module)
		    {
			LVAL->_module = module;
			return MU_MODULESYMBOL;
		    }

		    if (constant)
		    {
			LVAL->_symConstant = constant;
			return MU_SYMBOLICCONST;
		    }

		    if (construct)
		    {
			LVAL->_construct = construct;
			return MU_CONSTRUCT;
		    }

		    //
		    //  Symbol found.
		    //

		    if (Mu::Variable *v =
			dynamic_cast<Mu::Variable*>(s))
		    {
			LVAL->_symbol = s;
			return followedAlias ? MU_PREFIXEDSYMBOL : MU_SYMBOL;
		    }

		    if (alias)
		    {
			s = alias->alias();
                        followedAlias = true;
		    }
                    else
                    {
                        if (Mu::Function* f = dynamic_cast<Mu::Function*>(s))
                        {
                            if (f->scope()->name() == f->name())
                            {
                                if (Mu::Type* t = dynamic_cast<Mu::Type*>(f->scope()))
                                {
                                    //
                                    //  Constructor: return the type instead
                                    //

                                    LVAL->_type = t;
                                    return dynamic_cast<Mu::VariantTagType*>(t) ? MU_TYPECONSTRUCTOR : MU_TYPE;
                                }
                            }
                        }


                        LVAL->_symbol = s;
                        return followedAlias ? MU_PREFIXEDSYMBOL : MU_SYMBOL;
                    }
		}
	    }
	}
    }

    //
    //  Intern the name and give it to the parser as a generic identifier.
    //

    LVAL->_name = as->context()->internName(text).nameRef();
    return MU_IDENTIFIER;
}


void
yyFlexLexer::yyNewString()
{
#if COMPILER == GCC2_96
    stringBuffer = "";
#else
    stringBuffer.clear();
#endif
}

void
yyFlexLexer::yyAddToString(Mu::UTF32Char c)
{
    if (c >= 127)
    {
        utf8::utf32to8(&c, &c+1, back_inserter(stringBuffer));
    }
    else
    {
        stringBuffer.push_back(c);
    }
}

Mu::StringType::String*
yyFlexLexer::yyReturnString()
{
    Mu::MuLangContext* c = static_cast<Mu::MuLangContext*>(as->context());
    return c->stringType()->allocate(stringBuffer);
}

void
yyFlexLexer::init(const char* source, Mu::NodeAssembler* a)
{
    as = a;
    as->setSourceName(source);
    as->setLine(1);
    as->setChar(1);
}
