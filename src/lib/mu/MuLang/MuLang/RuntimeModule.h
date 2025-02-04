#ifndef __MuLang__RuntimeModule__h__
#define __MuLang__RuntimeModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{

    class RuntimeModule : public Module
    {
    public:
        RuntimeModule(Context* c, const char* name);
        virtual ~RuntimeModule();

        virtual void load();

        static void init();

        static NODE_DECLARATION(gc_call_on_collect, void);
        static NODE_DECLARATION(gc_perform_collection, void);
        static NODE_DECLARATION(gc_parallel_enabled, bool);
        static NODE_DECLARATION(gc_all_interior_pointers, bool);
        static NODE_DECLARATION(gc_num_collections, int);
        static NODE_DECLARATION(gc_dump, void);
        static NODE_DECLARATION(gc_enable, void);
        static NODE_DECLARATION(gc_disable, void);
        static NODE_DECLARATION(gc_get_heap_size, int64);
        static NODE_DECLARATION(gc_get_free_bytes, int64);
        static NODE_DECLARATION(gc_get_bytes_since_gc, int64);
        static NODE_DECLARATION(gc_get_total_bytes, int64);
        static NODE_DECLARATION(gc_set_warning_function, Pointer);
        static NODE_DECLARATION(gc_push_api, void);
        static NODE_DECLARATION(gc_pop_api, void);

        static NODE_DECLARATION(eval, Pointer);
        static NODE_DECLARATION(varying_size, int);
        static NODE_DECLARATION(set_varying_size, void);
        static NODE_DECLARATION(dump_symbols, Pointer);
        static NODE_DECLARATION(layout_traits, Pointer);
        static NODE_DECLARATION(machine_types, Pointer);
        static NODE_DECLARATION(exit, void);
        static NODE_DECLARATION(stack_traits, Pointer);
        static NODE_DECLARATION(main_arena_size, int);
        static NODE_DECLARATION(aux_arena_size, int);
        static NODE_DECLARATION(gc_scanned_objects, int);
        static NODE_DECLARATION(gc_collected_objects, int);
        static NODE_DECLARATION(module_locations, Pointer);
        static NODE_DECLARATION(backtrace, Pointer);
        static NODE_DECLARATION(load_module, bool);
        static NODE_DECLARATION(intern_name, Pointer);
        static NODE_DECLARATION(lookup_name, Pointer);
        static NODE_DECLARATION(lookup_type, Pointer);
        static NODE_DECLARATION(lookup_function, Pointer);
        static NODE_DECLARATION(build_os, Pointer);
        static NODE_DECLARATION(build_arch, Pointer);
        static NODE_DECLARATION(build_compiler, Pointer);

        // static NODE_DECLARATION(symbol_name, Pointer);
        static NODE_DECLARATION(symbol_equals, bool);
        static NODE_DECLARATION(symbol_nequals, bool);
        static NODE_DECLARATION(symbol_from_name, Pointer);
        static NODE_DECLARATION(symbol_name, Pointer);
        static NODE_DECLARATION(symbol_fully_qualified_name, Pointer);
        static NODE_DECLARATION(symbol_is_nil, bool);
        static NODE_DECLARATION(symbol_is_type, bool);
        static NODE_DECLARATION(symbol_is_module, bool);
        static NODE_DECLARATION(symbol_is_function, bool);
        static NODE_DECLARATION(symbol_is_method, bool);
        static NODE_DECLARATION(symbol_is_symbolic_constant, bool);
        static NODE_DECLARATION(symbol_is_variable, bool);
        static NODE_DECLARATION(symbol_is_parameter, bool);
        static NODE_DECLARATION(symbol_is_type_modifier, bool);
        static NODE_DECLARATION(symbol_documentation, Pointer);
        static NODE_DECLARATION(symbol_scope, Pointer);
        static NODE_DECLARATION(symbol_symbols_in_scope, Pointer);
        static NODE_DECLARATION(symbol_overloaded_symbols, Pointer);
        static NODE_DECLARATION(cast_to_symbol, Pointer);

        static NODE_DECLARATION(type_from_symbol, Pointer);

        static NODE_DECLARATION(function_from_symbol, Pointer);
        static NODE_DECLARATION(function_type, Pointer);
        static NODE_DECLARATION(function_signature, Pointer);

        static NODE_DECLARATION(variable_from_symbol, Pointer);
        static NODE_DECLARATION(variable_type, Pointer);

        static NODE_DECLARATION(parameter_from_symbol, Pointer);
        static NODE_DECLARATION(parameter_default_value_as_string, Pointer);

        static NODE_DECLARATION(type_is_union, bool);
        static NODE_DECLARATION(type_is_union_tag, bool);
        static NODE_DECLARATION(type_is_reference_type, bool);
        static NODE_DECLARATION(type_is_class, bool);
        static NODE_DECLARATION(type_is_interface, bool);
        static NODE_DECLARATION(type_is_opaque, bool);

        static NODE_DECLARATION(type_structure_info, Pointer);

        static NODE_DECLARATION(symbolic_constant_from_symbol, Pointer);
        static NODE_DECLARATION(symbolic_constant_value_as_string, Pointer);
    };

} // namespace Mu

#endif // __MuLang__RuntimeModule__h__
