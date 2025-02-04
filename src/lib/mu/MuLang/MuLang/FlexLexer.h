
// -*-C++-*-
// FlexLexer.h -- define interfaces for lexical analyzer classes generated
// by flex

// Copyright (c) 1993 The Regents of the University of California.
// All rights reserved.
//
//
// This code is derived from software contributed to Berkeley by
// Kent Williams and Tom Epperly.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:

//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.

//  Neither the name of the University nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.

//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE.

// This file defines FlexLexer, an abstract class which specifies the
// external interface provided to flex C++ lexer objects, and yyFlexLexer,
// which defines a particular lexer class.
//
// If you want to create multiple lexer classes, you use the -P flag
// to rename each yyFlexLexer to some other xxFlexLexer.  You then
// include <FlexLexer.h> in your other sources once per lexer class:
//
//	#undef yyFlexLexer
//	#define yyFlexLexer xxFlexLexer
//	#include <FlexLexer.h>
//
//	#undef yyFlexLexer
//	#define yyFlexLexer zzFlexLexer
//	#include <FlexLexer.h>
//	...

#ifndef __FLEX_LEXER_H
// Never included before - need to define base class.
#define __FLEX_LEXER_H

#include <iostream>

#if MU_FLEX_APPLE == 1
#define FLEX_SIZE_TYPE size_t
#else
#define FLEX_SIZE_TYPE int
#endif

extern "C++"
{

#include <Mu/config.h>
#include <Mu/Alias.h>
#include <Mu/Construct.h>
#include <Mu/Context.h>
#include <Mu/Function.h>
#include <Mu/GlobalVariable.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/StackVariable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/TypeModifier.h>
#include <Mu/VariantTagType.h>
#include <Mu/GarbageCollector.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuLang/StringType.h>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>

    struct yy_buffer_state;
    typedef int yy_state_type;

    class FlexLexer
    {
    public:
        MU_GC_NEW_DELETE
        virtual ~FlexLexer() {}

        const char* YYText() const { return yytext; }

        int YYLeng() const { return yyleng; }

        virtual void
        yy_switch_to_buffer(struct yy_buffer_state* new_buffer) = 0;
        virtual struct yy_buffer_state* yy_create_buffer(std::istream* s,
                                                         int size) = 0;
#if MU_FLEX_MINOR_VERSION >= 6
        virtual struct yy_buffer_state* yy_create_buffer(std::istream& s,
                                                         int size) = 0;
#endif
        virtual void yy_delete_buffer(struct yy_buffer_state* b) = 0;
        virtual void yyrestart(std::istream* s) = 0;
#if MU_FLEX_MINOR_VERSION >= 6
        virtual void yyrestart(std::istream& s) = 0;
#endif

        virtual int yylex() = 0;

        // Call yylex with new input/output sources.
        int yylex(std::istream* new_in, std::ostream* new_out = 0)
        {
            switch_streams(new_in, new_out);
            return yylex();
        }

        // Switch to new input/output streams.  A nil stream pointer
        // indicates "keep the current one".
#if MU_FLEX_MINOR_VERSION >= 6
        virtual void switch_streams(std::istream& new_in,
                                    std::ostream& new_out) = 0;
#endif
        virtual void switch_streams(std::istream* new_in = 0,
                                    std::ostream* new_out = 0) = 0;

        int lineno() const { return yylineno; }

        int debug() const { return yy_flex_debug; }

        void set_debug(int flag) { yy_flex_debug = flag; }

    protected:
        char* yytext;
        int yyleng;
        int yylineno;      // only maintained if you use %option yylineno
        int yy_flex_debug; // only has effect with -d or "%option debug"
    };
}
#endif

#if defined(yyFlexLexer) || !defined(yyFlexLexerOnce)
// Either this is the first time through (yyFlexLexerOnce not defined),
// or this is a repeated include to define a different flavor of
// yyFlexLexer, as discussed in the flex man page.
#define yyFlexLexerOnce

extern "C++"
{

    class MUYYFlexLexer : public FlexLexer
    {
    public:
        // arg_yyin and arg_yyout default to the cin and cout, but we
        // only make that assignment when initializing in yylex().
#if MU_FLEX_MINOR_VERSION >= 6
        MUYYFlexLexer(std::istream& arg_yyin, std::ostream& arg_yyout);
#endif
        MUYYFlexLexer(std::istream* arg_yyin = 0, std::ostream* arg_yyout = 0);

#if MU_FLEX_MINOR_VERSION >= 6
    private:
        void ctor_common();
#endif

    public:
        void init(const char* sourceName, Mu::NodeAssembler* a);

        void setYYSTYPE(void* s) { _yystype = s; }

        virtual ~MUYYFlexLexer();

        void yy_switch_to_buffer(struct yy_buffer_state* new_buffer);
        struct yy_buffer_state* yy_create_buffer(std::istream* s, int size);
#if MU_FLEX_MINOR_VERSION >= 6
        struct yy_buffer_state* yy_create_buffer(std::istream& s, int size);
#endif
        void yy_delete_buffer(struct yy_buffer_state* b);
        void yyrestart(std::istream* s);
#if MU_FLEX_MINOR_VERSION >= 6
        void yyrestart(std::istream& s);
#endif

        void yypush_buffer_state(struct yy_buffer_state* new_buffer);
        void yypop_buffer_state(void);

        virtual int yylex();
#if MU_FLEX_MINOR_VERSION >= 6
        virtual void switch_streams(std::istream& new_in,
                                    std::ostream& new_out);
#endif
        virtual void switch_streams(std::istream* new_in,
                                    std::ostream* new_out);
        virtual int yywrap();

        const char* sourceName() const { return as->sourceName().c_str(); }

        int lineNum() const { return as->lineNum(); }

        int charNum() const { return as->charNum(); }

        Mu::NodeAssembler* assembler() { return as; }

    protected:
        virtual FLEX_SIZE_TYPE LexerInput(char* buf, FLEX_SIZE_TYPE max_size);
        virtual void LexerOutput(const char* buf, FLEX_SIZE_TYPE size);
        virtual void LexerError(const char* msg);

        void yyNewString();
        void yyAddToString(Mu::UTF32Char);
        Mu::StringType::String* yyReturnString();
        int yyIdentifier(const char*);

        void yyunput(int c, char* buf_ptr);
        int yyinput();

        void yy_load_buffer_state();
#if MU_FLEX_MINOR_VERSION >= 6
        void yy_init_buffer(struct yy_buffer_state* b, std::istream& s);
#else
        void yy_init_buffer(struct yy_buffer_state* b, std::istream* s);
#endif
        void yy_flush_buffer(struct yy_buffer_state* b);

        int yy_start_stack_ptr;
        int yy_start_stack_depth;
        int* yy_start_stack;

        void yy_push_state(int new_state);
        void yy_pop_state();
        int yy_top_state();

        yy_state_type yy_get_previous_state();
        yy_state_type yy_try_NUL_trans(yy_state_type current_state);
        int yy_get_next_buffer();

        // Type of yyin/yyout changed in flex version 2.6.x (from 2.5.x)
#if MU_FLEX_MINOR_VERSION >= 6
        std::istream yyin;  // input source for default LexerInput
        std::ostream yyout; // output sink for default LexerOutput
#else
        std::istream* yyin;  // input source for default LexerInput
        std::ostream* yyout; // output sink for default LexerOutput
#endif

        struct yy_buffer_state* yy_current_buffer;

        // yy_hold_char holds the character lost when yytext is formed.
        char yy_hold_char;

        // Number of characters read into yy_ch_buf.
        int yy_n_chars;

        // Points to current character in buffer.
        char* yy_c_buf_p;

        int yy_init;  // whether we need to initialize
        int yy_start; // start state number

        // Flag which is used to allow yywrap()'s to do buffer switches
        // instead of setting up a fresh yyin.  A bit of a hack ...
        int yy_did_buffer_switch_on_eof;

        size_t yy_buffer_stack_top;               /**< index of top of stack. */
        size_t yy_buffer_stack_max;               /**< capacity of stack. */
        struct yy_buffer_state** yy_buffer_stack; /**< Stack as an array. */
        void yyensure_buffer_stack(void);

        // The following are not always needed, but may be depending
        // on use of certain flex features (like REJECT or yymore()).

        yy_state_type yy_last_accepting_state;
        char* yy_last_accepting_cpos;

        yy_state_type* yy_state_buf;
        yy_state_type* yy_state_ptr;

        char* yy_full_match;
        int* yy_full_state;
        int yy_full_lp;

        int yy_lp;
        int yy_looking_for_trail_begin;

        int yy_more_flag;
        int yy_more_len;
        int yy_more_offset;
        int yy_prev_more_offset;

        Mu::NodeAssembler* as;
        void* _yystype;
        Mu::String stringBuffer;
    };
}

#endif
