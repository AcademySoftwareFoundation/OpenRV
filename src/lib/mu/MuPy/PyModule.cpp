//
// Copyright (c) 2011, Jim Hourihan
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0 
//

#include <MuPy/PyModule.h>
#include <MuPy/PyObjectType.h>
#include <MuPy/PyTypeObjectType.h>

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <half.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/ListType.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Signature.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <Mu/NodeAssembler.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <stdexcept>
#include <sstream>



namespace Mu
{
  using namespace std;

  static void throwBadArgumentException( const Mu::Node& node,
                                         Mu::Thread& thread, std::string msg )
  {
    ostringstream str;
    const Mu::MuLangContext* context =
        static_cast<const Mu::MuLangContext*>( thread.context() );
    ExceptionType::Exception* e =
        new ExceptionType::Exception( context->exceptionType() );
    str << "in " << node.symbol()->fullyQualifiedName() << ": " << msg;
    e->string() += str.str().c_str();
    thread.setException( e );
    BadArgumentException exc;
    exc.message() = e->string();
    throw exc;
  }

  struct Converter
  {
    PyModule::MuToPyObjectConverter toPy;
    PyModule::PyObjectToMuConverter toMu;
    PyModule::MuOpaqueToPyObjectConverter toPyOpaque;
    PyModule::PyObjectToMuOpaqueConverter toMuOpaque;
  };

  static vector<Converter>* externalConverters = 0;

  void PyModule::addConverterFunctions( MuToPyObjectConverter m2p,
                                        PyObjectToMuConverter p2m,
                                        MuOpaqueToPyObjectConverter mo2p,
                                        PyObjectToMuOpaqueConverter p2mo )
  {
    if( !externalConverters )
    {
      //
      //  DLL shite requires we don't have a static constructor
      //  *sigh*
      //

      externalConverters = new vector<Converter>();
    }

    Converter c;
    c.toPy = m2p;
    c.toMu = p2m;
    c.toPyOpaque = mo2p;
    c.toMuOpaque = p2mo;
    externalConverters->push_back( c );
  }

  PyModule::PyModule( Context* c, const char* name ) : Module( c, name ) {}

  PyModule::~PyModule() {}

  void PyModule::load()
  {
    USING_MU_FUNCTION_SYMBOLS;
    MuLangContext* c = (MuLangContext*)globalModule()->context();

    Type* pyobj = new PyObjectType( c, "PyObject" );
    Type* pytypeobj = new PyTypeObjectType( c, "PyTypeObject" );
    const char* pn = pyobj->fullyQualifiedName().c_str();

    addSymbols( pyobj, pytypeobj,

                new Function( c, "PyObject", classInstanceToPyObject, Cast,
                              Return, "python.PyObject", Args, "?class", End ),

                EndArguments );

    globalScope()->addSymbols(

        new Function( c, "to_string", PyObjectToString, None, Return, "string",
                      Args, "python.PyObject", End ),

        new Function( c, "to_bool", PyObjectToBool, None, Return, "bool", Args,
                      "python.PyObject", End ),

        new Function( c, "to_int", PyObjectToInt, None, Return, "int", Args,
                      "python.PyObject", End ),

        new Function( c, "to_float", PyObjectToFloat, None, Return, "float",
                      Args, "python.PyObject", End ),

        EndArguments );

    addSymbols(
        new Function(
            c, "Py_INCREF", nPy_INCREF, None, Return, "void", Parameters,
            new ParameterVariable( c, "obj", "python.PyObject" ), End ),

        new Function(
            c, "Py_DECREF", nPy_DECREF, None, Return, "void", Parameters,
            new ParameterVariable( c, "obj", "python.PyObject" ), End ),

        new Function( c, "PyErr_Print", nPyErr_Print, None, Return, "void",
                      End ),

        new Function( c, "PyObject_GetAttr", nPyObject_GetAttr, None, Return,
                      "python.PyObject", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      new ParameterVariable( c, "name", "string" ), End ),

        new Function( c, "PyObject_CallObject", nPyObject_CallObject, None,
                      Return, "python.PyObject", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      new ParameterVariable( c, "args", "python.PyObject" ),
                      End ),

        new Function( c, "PyObject_CallObject", nPyObject_CallObject2, None,
                      Return, "python.PyObject", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      new ParameterVariable( c, "args", "?class" ), End ),

        new Function( c, "PyTuple_New", nPyTuple_New, None, Return,
                      "python.PyObject", Parameters,
                      new ParameterVariable( c, "size", "int" ), End ),

        new Function(
            c, "PyTuple_Size", nPyTuple_Size, None, Return, "int", Parameters,
            new ParameterVariable( c, "obj", "python.PyObject" ), End ),

        new Function(
            c, "PyTuple_SetItem", nPyTuple_SetItem, None, Return, "int",
            Parameters, new ParameterVariable( c, "obj", "python.PyObject" ),
            new ParameterVariable( c, "index", "int" ),
            new ParameterVariable( c, "element", "python.PyObject" ), End ),

        new Function( c, "PyTuple_GetItem", nPyTuple_GetItem, None, Return,
                      "python.PyObject", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      new ParameterVariable( c, "index", "int" ), End ),

        new Function( c, "PyImport_Import", nPyImport_Import, None, Return,
                      "python.PyObject", Parameters,
                      new ParameterVariable( c, "moduleName", "string" ), End ),

        new Function( c, "PyString_Check", nPyString_Check, None, Return,
                      "bool", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function( c, "PyBytes_Check", nPyString_Check, None, Return,
              "bool", Parameters,
              new ParameterVariable( c, "obj", "python.PyObject" ),
              End ),

        new Function( c, "PyFunction_Check", nPyFunction_Check, None, Return,
                      "bool", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function( c, "Py_TYPE", nPy_TYPE, None, Return,
                      "python.PyTypeObject", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function( c, "PyModule_GetName", nPyModule_GetName, None, Return,
                      "string", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function( c, "PyModule_GetFilename", nPyModule_GetFilename, None,
                      Return, "string", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function(
            c, "type_name", type_name, None, Return, "string", Parameters,
            new ParameterVariable( c, "obj", "python.PyTypeObject" ), End ),

        new Function( c, "is_nil", is_nil, None, Return, "bool", Parameters,
                      new ParameterVariable( c, "obj", "python.PyObject" ),
                      End ),

        new Function( c, "is_nil", is_nil, None, Return, "bool", Parameters,
                      new ParameterVariable( c, "obj", "python.PyTypeObject" ),
                      End ),

        EndArguments );
  }

  Mu::FunctionObject* createFunctionObjectFromPyObject(
      const Mu::FunctionType* t, PyObject* pyobj )
  {
    Mu::MuLangContext* c = (Mu::MuLangContext*)t->context();
    Mu::NodeAssembler as( c );
    const Mu::Signature* sig = t->signature();
    size_t nargs = sig->size() - 1;
    const Mu::Type* rtype = sig->returnType();
    const Mu::Type* ptype = c->findSymbolOfTypeByQualifiedName<Mu::Type>(
        c->internName( "python.PyObject" ) );
    const Mu::Function* C = c->findSymbolOfTypeByQualifiedName<Mu::Function>(
        c->internName( "python.PyObject_CallObject" ) );
    assert( C );
    assert( ptype );
    PyLockObject locker;

    //
    //  Search for a cast function from PyObject to the type we
    //  need. If not found throw out of here.
    //

    string cname = "to_";
    cname += rtype->name().c_str();
    const Mu::Function* Fcast =
        c->findSymbolOfTypeByQualifiedName<Mu::Function>(
            c->internName( cname.c_str() ) );

    if( Fcast )
    {
      bool found = false;

      for( const Mu::Symbol* s = Fcast->firstOverload(); s;
           s = s->nextOverload() )
      {
        if( const Mu::Function* f = dynamic_cast<const Mu::Function*>( s ) )
        {
          if( f->returnType() == rtype && f->numArgs() == 1 &&
              f->argType( 0 ) == ptype )
          {
            found = true;
            break;
          }
        }
      }

      if( !found ) Fcast = 0;
    }

    if( !Fcast && rtype != c->voidType() )
    {
      ostringstream str;
      str << "can't create PyCallable thunk which returns "
          << rtype->fullyQualifiedName();
      throw invalid_argument( str.str().c_str() );
    }

    //
    //  Assemble the thunk
    //

    Mu::NodeAssembler::SymbolList params = as.emptySymbolList();
    Mu::FunctionObject* fobj = new Mu::FunctionObject( t );

    for( size_t i = 0; i < nargs; i++ )
    {
      ostringstream name;
      name << "_" << i;
      const Mu::Type* argType = sig->argType( i );
      params.push_back(
          new Mu::ParameterVariable( c, name.str().c_str(), argType ) );
    }

    as.newStackFrame();

    Mu::Function* F = 0;

    if( nargs > 0 )
    {
      F = new Mu::Function(
          c, "__lambda", rtype, nargs, (Mu::ParameterVariable**)&params.front(),
          0, Mu::Function::ContextDependent | Mu::Function::LambdaExpression );
    }
    else
    {
      F = new Mu::Function(
          c, "__lambda", rtype, 0, 0, 0,
          Mu::Function::ContextDependent | Mu::Function::LambdaExpression );
    }

    as.scope()->addAnonymousSymbol( F );
    as.pushScope( F );
    as.declareParameters( params );
    as.removeSymbolList( params );

    // Mu::Node* root = ...

    // Make a constant for the pyobj
    Mu::DataNode* pynode = as.constant( ptype );
    pynode->_data._Pointer = pyobj;
    //  We're adding a ref to this pyobj, so inc it's refcount
    Py_XINCREF( pyobj );

    Mu::Node* argNode = 0;

    if( nargs > 1 )
    {
      // make a tuple out of the rest of the args
      Mu::NodeAssembler::NodeList nl = as.emptyNodeList();

      for( size_t i = 0; i < nargs; i++ )
      {
        nl.push_back( as.dereferenceVariable( F->parameter( i ) ) );
      }

      Mu::Node* tnode = as.tupleNode( nl );
      as.removeNodeList( nl );
      argNode = tnode;
    }
    else if( nargs == 1 )
    {
      argNode = as.dereferenceVariable( F->parameter( 0 ) );
    }
    else
    {
      Mu::DataNode* dn = as.constant( t );  // t could be anything as long as
                                            // its a Mu::Class
      dn->_data._Pointer = 0;
      argNode = dn;
    }

    Mu::NodeAssembler::NodeList nl = as.newNodeList( pynode );
    nl.push_back( argNode );
    Mu::Node* root = as.callFunction( C, nl );
    as.removeNodeList( nl );

    if( Fcast )
    {
      nl = as.newNodeList( root );
      root = as.callFunction( Fcast, nl );
      as.removeNodeList( nl );
    }

    int stackSize = as.endStackFrame();
    as.popScope();
    F->stackSize( stackSize );

    if( F->hasReturn() )
    {
      F->setBody( root );
    }
    else if( Mu::Node* code = as.cast( root, F->returnType() ) )
    {
      F->setBody( code );
    }
    else
    {
      throw invalid_argument(
          "failed to build closue for PyObject because of bad cast" );
    }

    fobj->setFunction( F );
    return fobj;
  }

  //
  //  Convert a PyObject to a Value
  //
  Value PyModule::py2mu( MuLangContext* c, Process* p, const Type* t,
                         PyObject* pobj )
  {
    PyLockObject locker;

    if( t == c->intType() )
    {
      if( PyLong_Check( pobj ) )
      {
        return Value( int( PyLong_AsLong( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting int" );
      }
    }
    else if( t == c->int64Type() && PyLong_Check( pobj ) )
    {
      if( PyLong_Check( pobj ) )
      {
        return Value( int64( PyLong_AsSsize_t( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting long to convert to int64" );
      }
    }
    else if( t == c->shortType() )
    {
      if( PyLong_Check( pobj ) )
      {
        return Value( short( PyLong_AsLong( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting int to convert to short" );
      }
    }
    else if( t == c->byteType() )
    {
      if( PyLong_Check( pobj ) )
      {
        return Value( byte( PyLong_AsLong( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting int to convert to byte" );
      }
    }
    else if( t == c->boolType() )
    {
      if( PyBool_Check( pobj ) )
      {
        return Value( pobj == Py_True );
      }
      else
      {
        throw invalid_argument( "expecting bool" );
      }
    }
    else if( t == c->halfType() )
    {
      if( PyFloat_Check( pobj ) )
      {
        half h = PyFloat_AsDouble( pobj );
        return Value( short( h.bits() ) );
      }
      else
      {
        throw invalid_argument( "expecting float to convert to half" );
      }
    }
    else if( t == c->floatType() )
    {
      if( PyFloat_Check( pobj ) )
      {
        return Value( float( PyFloat_AsDouble( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting float" );
      }
    }
    else if( t == c->doubleType() )
    {
      if( PyFloat_Check( pobj ) )
      {
        return Value( PyFloat_AsDouble( pobj ) );
      }
      else
      {
        throw invalid_argument( "expecting float to convert to double" );
      }
    }
    else if( t == c->stringType() )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      if( PyUnicode_Check( pobj ) )
      {
        PyObject* utf8 = PyUnicode_AsUTF8String( pobj );
        Value ret( Pointer( c->stringType()->allocate( PyBytes_AsString( utf8 ) ) ) );
        Py_XDECREF( utf8 );
        return ret;
      }
      else if( PyBytes_Check( pobj ) )
      {
        return Value(
            Pointer( c->stringType()->allocate( PyBytes_AsString( pobj ) ) ) );
      }
      else
      {
        throw invalid_argument( "expecting string or unicode" );
      }
    }
    else if( t == c->nameType() )
    {
      if( PyUnicode_Check( pobj ) )
      {
        PyObject* utf8 = PyUnicode_AsUTF8String( pobj );
        Value ret( c->internName( PyBytes_AsString( utf8 ) ) );
        Py_XDECREF( utf8 );
        return ret;
      }
      else if( PyBytes_Check( pobj ) )
      {
        return Value( c->internName( PyBytes_AsString( pobj ) ) );
      }
      else
      {
        throw invalid_argument( "expecting string or unicode" );
      }
    }
    else if( t == c->vec2fType() )
    {
      if( PyTuple_Check( pobj ) && PyTuple_Size( pobj ) == 2 )
      {
        Vector2f vec;
        vec[0] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 0 ) )._float;
        vec[1] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 1 ) )._float;
        return Value( vec );
      }
      else
      {
        throw invalid_argument( "expecting (float, float) tuple" );
      }
    }
    else if( t == c->vec3fType() )
    {
      if( PyTuple_Check( pobj ) && PyTuple_Size( pobj ) == 3 )
      {
        Vector3f vec;
        vec[0] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 0 ) )._float;
        vec[1] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 1 ) )._float;
        vec[2] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 2 ) )._float;
        return Value( vec );
      }
      else
      {
        throw invalid_argument( "expecting (float, float, float) tuple" );
      }
    }
    else if( t == c->vec4fType() )
    {
      if( PyTuple_Check( pobj ) && PyTuple_Size( pobj ) == 4 )
      {
        Vector4f vec;
        vec[0] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 0 ) )._float;
        vec[1] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 1 ) )._float;
        vec[2] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 2 ) )._float;
        vec[3] =
            py2mu( c, p, c->floatType(), PyTuple_GetItem( pobj, 3 ) )._float;
        return Value( vec );
      }
      else
      {
        throw invalid_argument(
            "expecting (float, float, float, float) tuple" );
      }
    }
    else if( const TupleType* ttype = dynamic_cast<const TupleType*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyTuple_Check( pobj ) )
      {
        const size_t n = PyTuple_Size( pobj );
        const TupleType::Types& types = ttype->tupleFieldTypes();

        if( n == types.size() )
        {
          ClassInstance* mutuple = ClassInstance::allocate( ttype );

          for( size_t i = 0; i < n; i++ )
          {
            Value pyval = py2mu( c, p, types[i], PyTuple_GetItem( pobj, i ) );
            ValuePointer ptr = types[i]->machineRep()->valuePointer( pyval );
            memcpy( mutuple->field( i ), ptr, types[i]->machineRep()->size() );
          }

          return Value( mutuple );
        }
        else
        {
          throw invalid_argument( "wrong size tuple" );
        }
      }
      else
      {
        throw invalid_argument( "expecting tuple" );
      }
    }
    else if( const ListType* ltype = dynamic_cast<const ListType*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyList_Check( pobj ) )
      {
        const size_t n = PyList_Size( pobj );
        const Type* etype = ltype->elementType();
        List list( p, ltype );

        for( size_t i = 0; i < n; i++ )
        {
          list.appendDefaultValue();
          Value pyval = py2mu( c, p, etype, PyList_GetItem( pobj, i ) );
          ValuePointer ptr = etype->machineRep()->valuePointer( pyval );
          memcpy( list.valuePointer(), ptr, etype->machineRep()->size() );
        }

        return Value( list.head() );
      }
      else
      {
        throw invalid_argument( "expecting list" );
      }
    }
    else if( const DynamicArrayType* atype =
                 dynamic_cast<const DynamicArrayType*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyList_Check( pobj ) )
      {
        const size_t n = PyList_Size( pobj );
        const Type* etype = atype->elementType();
        DynamicArray* array = new DynamicArray( atype, 1 );
        array->resize( n );

        for( size_t i = 0; i < n; i++ )
        {
          Value pyval = py2mu( c, p, etype, PyList_GetItem( pobj, i ) );
          ValuePointer ptr = etype->machineRep()->valuePointer( pyval );
          memcpy( array->elementPointer( i ), ptr,
                  etype->machineRep()->size() );
        }

        return Value( array );
      }
      else
      {
        throw invalid_argument( "expecting dynamic array" );
      }
    }
    else if( const FixedArrayType* atype =
                 dynamic_cast<const FixedArrayType*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyList_Check( pobj ) )
      {
        const size_t n = atype->fixedSize();

        if( n == PyList_Size( pobj ) )
        {
          const Type* etype = atype->elementType();
          FixedArray* array =
              static_cast<FixedArray*>( ClassInstance::allocate( atype ) );

          for( size_t i = 0; i < n; i++ )
          {
            Value pyval = py2mu( c, p, etype, PyList_GetItem( pobj, i ) );
            ValuePointer ptr = etype->machineRep()->valuePointer( pyval );
            memcpy( array->elementPointer( i ), ptr,
                    etype->machineRep()->size() );
          }

          return Value( array );
        }
        else
        {
          throw invalid_argument( "wrong list size" );
        }
      }
      else
      {
        throw invalid_argument( "expecting fixed array" );
      }
    }
    else if( const FunctionType* ftype =
                 dynamic_cast<const FunctionType*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyCallable_Check( pobj ) )
      {
        return Value( createFunctionObjectFromPyObject( ftype, pobj ) );
      }
      else
      {
        throw invalid_argument( "expecting a callable object" );
      }
    }
    else if( const Class* ctype = dynamic_cast<const Class*>( t ) )
    {
      if( pobj == Py_None )
      {
        return Value( Pointer( 0 ) );
      }
      else if( PyDict_Check( pobj ) )
      {
        ClassInstance* muobj = ClassInstance::allocate( ctype );
        Class::MemberVariableVector vars;
        ctype->allMemberVariables( vars );
        const size_t n = vars.size();

        for( size_t i = 0; i < n; i++ )
        {
          const Type* ftype = muobj->fieldType( i );
          const char* name = vars[i]->name().c_str();
          PyObject* key =
              PyUnicode_DecodeUTF8( name, strlen( name ), "ignore" );
          PyObject* val = PyDict_GetItem( pobj, key );  // borrowed ref
          Value muval = py2mu( c, p, ftype, val );
          ValuePointer ptr = ftype->machineRep()->valuePointer( muval );
          memcpy( muobj->field( i ), ptr, ftype->machineRep()->size() );

          //
          //  Since we think PyUnicode_DecodeUTF8 returns a borrowed reference,
          //  don't decref.
          //
          Py_XDECREF( key );
        }

        return Value( muobj );
      }
      else if( externalConverters )
      {
        for( size_t i = 0; i < externalConverters->size(); i++ )
        {
          Converter& c = ( *externalConverters )[i];

          if( c.toMu )
          {
            if( ClassInstance* muobj = c.toMu( t, pobj ) )
            {
              return Value( muobj );
            }
          }
        }
      }
    }
    else if( dynamic_cast<const OpaqueType*>( t ) )
    {
      if( externalConverters )
      {
        for( size_t i = 0; i < externalConverters->size(); i++ )
        {
          Converter& c = ( *externalConverters )[i];

          if( c.toMuOpaque )
          {
            Pointer p = c.toMuOpaque( t, pobj );
            if( p ) return Value( p );
          }
        }
      }
    }
    else if( !strcmp( t->name().c_str(), "PyObject" ) )
    {
      return Value( Pointer( pobj ) );
    }

    return Value();
  }

  static PyObject* incret( PyObject* p )
  {
    if( p ) Py_XINCREF( p );
    return p;
  }

  //
  //  Convert a Value to a PyObject
  //
  PyObject* PyModule::mu2py( MuLangContext* c, Process* p, const Type* t,
                             const Value& v )
  {
    PyLockObject locker;

    if( t == c->voidType() )
    {
      Py_RETURN_NONE;
    }
    else if( t == c->intType() )
    {
      return PyLong_FromLong( v._int );
    }
    else if( t == c->int64Type() )
    {
      return PyLong_FromSsize_t( v._int64 );
    }
    else if( t == c->shortType() )
    {
      return PyLong_FromLong( v._short );
    }
    else if( t == c->byteType() )
    {
      return PyLong_FromLong( v._byte );
    }
    else if( t == c->boolType() )
    {
      if( v._bool )
        Py_RETURN_TRUE;
      else
        Py_RETURN_FALSE;
    }
    else if( t == c->halfType() )
    {
      half h;
      h.setBits( v._short );
      return PyFloat_FromDouble( h );
    }
    else if( t == c->floatType() )
    {
      return PyFloat_FromDouble( v._float );
    }
    else if( t == c->doubleType() )
    {
      return PyFloat_FromDouble( v._double );
    }
    else if( t == c->stringType() )
    {
      if( !v._Pointer ) Py_RETURN_NONE;
      StringType::String* s =
          reinterpret_cast<StringType::String*>( v._Pointer );

      const char* utf8 = s->c_str();  // its utf8 by definition
      PyObject* o = PyUnicode_DecodeUTF8( utf8, strlen( utf8 ), "ignore" );
      return o;
    }
    else if( t == c->nameType() )
    {
      Name name = v._name;
      return PyBytes_FromString( name.c_str() );
    }
    else if( t == c->vec2fType() )
    {
      return Py_BuildValue( "(ff)", v._Vector2f[0], v._Vector2f[1] );
    }
    else if( t == c->vec3fType() )
    {
      return Py_BuildValue( "(fff)", v._Vector3f[0], v._Vector3f[1],
                            v._Vector3f[2] );
    }
    else if( t == c->vec4fType() )
    {
      return Py_BuildValue( "(ffff)", v._Vector4f[0], v._Vector4f[1],
                            v._Vector4f[2], v._Vector4f[3] );
    }
    else if( const TupleType* ttype = dynamic_cast<const TupleType*>( t ) )
    {
      if( !v._Pointer ) Py_RETURN_NONE;
      ClassInstance* o = reinterpret_cast<ClassInstance*>( v._Pointer );
      const TupleType::Types& types = ttype->tupleFieldTypes();
      const size_t n = types.size();

      PyObject* tuple = PyTuple_New( n );

      for( size_t i = 0; i < n; i++ )
      {
        ValuePointer vp = o->field( i );
        PyObject* pyobj =
            mu2py( c, p, types[i], types[i]->machineRep()->value( vp ) );
        PyTuple_SetItem( tuple, i, pyobj );
      }

      return tuple;
    }
    else if( const DynamicArrayType* atype =
                 dynamic_cast<const DynamicArrayType*>( t ) )
    {
      if( !v._Pointer ) Py_RETURN_NONE;
      DynamicArray* array = reinterpret_cast<DynamicArray*>( v._Pointer );
      const Type* etype = atype->elementType();
      const size_t n = array->size();

      PyObject* list = PyList_New( n );

      for( size_t i = 0; i < n; i++ )
      {
        ValuePointer vp = (ValuePointer)array->elementPointer( i );
        PyObject* pyobj =
            mu2py( c, p, etype, etype->machineRep()->value( vp ) );
        PyList_SetItem( list, i, pyobj );
      }

      return list;
    }
    else if( const FixedArrayType* atype =
                 dynamic_cast<const FixedArrayType*>( t ) )
    {
      if( !v._Pointer ) Py_RETURN_NONE;
      FixedArray* array = reinterpret_cast<FixedArray*>( v._Pointer );
      const Type* etype = atype->elementType();
      const size_t n = atype->fixedSize();

      PyObject* list = PyList_New( n );

      for( size_t i = 0; i < n; i++ )
      {
        ValuePointer vp = (ValuePointer)array->elementPointer( i );
        PyObject* pyobj =
            mu2py( c, p, etype, etype->machineRep()->value( vp ) );
        PyList_SetItem( list, i, pyobj );
      }

      return list;
    }
    else if( const ListType* ltype = dynamic_cast<const ListType*>( t ) )
    {
      if( !v._Pointer ) Py_RETURN_NONE;
      ClassInstance* mlist = reinterpret_cast<ClassInstance*>( v._Pointer );
      const Type* etype = ltype->elementType();
      size_t n = 0;

      for( List list( p, mlist ); !list.isNil(); list++, n++ )
        ;
      PyObject* pylist = PyList_New( n );
      size_t i = 0;

      for( List list( p, mlist ); !list.isNil(); list++, i++ )
      {
        PyObject* pyobj = mu2py(
            c, p, etype, etype->machineRep()->value( list.valuePointer() ) );
        PyList_SetItem( pylist, i, pyobj );
      }

      return pylist;
    }
    else if( const Class* ctype = dynamic_cast<const Class*>( t ) )
    {
      if( !v._Pointer ) Py_RETURN_NONE;

      ClassInstance* obj = reinterpret_cast<ClassInstance*>( v._Pointer );

      if( externalConverters )
      {
        for( size_t i = 0; i < externalConverters->size(); i++ )
        {
          Converter& c = ( *externalConverters )[i];

          if( c.toPy )
          {
            if( PyObject* pyobj = (PyObject*)c.toPy( t, obj ) )
            {
              return pyobj;
            }
          }
        }
      }

      PyObject* dict = PyDict_New();
      Class::MemberVariableVector vars;
      ctype->allMemberVariables( vars );

      for( size_t i = 0; i < vars.size(); i++ )
      {
        const Type* ftype = obj->fieldType( i );
        assert( ftype == vars[i]->storageClass() );
        PyObject* pyobj =
            mu2py( c, p, ftype, ftype->machineRep()->value( obj->field( i ) ) );
        const char* name = vars[i]->name().c_str();
        PyObject* key = PyUnicode_DecodeUTF8( name, strlen( name ), "ignore" );
        PyDict_SetItem( dict, key, pyobj );
        //
        // We have to Py_XDECREF key and pyobj otherwise
        // memory leaks. For example the use of the pymu call
        // sourcesRendered() will cause memory to leak.
        Py_XDECREF( key );
        Py_XDECREF( pyobj );
      }

      return dict;
    }
    else if( !strcmp( t->name().c_str(), "PyObject" ) )
    {
      return reinterpret_cast<PyObject*>( v._Pointer );
    }

    Py_RETURN_NONE;
  }

  NODE_IMPLEMENTATION( PyModule::nPyObject_GetAttr, Pointer )
  {
    PyLockObject locker;
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    StringType::String* name = NODE_ARG_OBJECT( 1, StringType::String );
    NODE_RETURN( PyObject_GetAttrString( obj, name->c_str() ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyTuple_New, Pointer )
  {
    PyLockObject locker;
    int n = NODE_ARG( 0, int );
    NODE_RETURN( PyTuple_New( n ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyTuple_Size, int )
  {
    PyLockObject locker;
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( int( PyTuple_Size( obj ) ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyTuple_SetItem, int )
  {
    PyLockObject locker;
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    int index = NODE_ARG( 1, int );
    PyObject* elem = NODE_ARG_OBJECT( 2, PyObject );

    NODE_RETURN( int( PyTuple_SetItem( obj, index, elem ) ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyTuple_GetItem, Pointer )
  {
    PyLockObject locker;
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    int index = NODE_ARG( 1, int );

    NODE_RETURN( Pointer( PyTuple_GetItem( obj, index ) ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyImport_Import, Pointer )
  {
    PyLockObject locker;
    StringType::String* module = NODE_ARG_OBJECT( 0, StringType::String );
    PyObject* pModule = PyImport_ImportModule( module->c_str() );

    if( !pModule )
    {
      if( PyErr_Occurred() )
      {
        PyErr_Print();
      }
    }

    NODE_RETURN( pModule );
  }

  NODE_IMPLEMENTATION( PyModule::nPy_TYPE, Pointer )
  {
    PyLockObject locker;
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( Py_TYPE( obj ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyErr_Print, void )
  {
    PyLockObject locker;
    PyErr_Print();
  }

  NODE_IMPLEMENTATION( PyModule::nPyModule_GetName, Pointer )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( c->stringType()->allocate( PyModule_GetName( obj ) ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyModule_GetFilename, Pointer )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( c->stringType()->allocate( PyModule_GetFilename( obj ) ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyFunction_Check, bool )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( PyFunction_Check( obj ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyString_Check, bool )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( PyBytes_Check( obj ) );
  }

  NODE_IMPLEMENTATION( PyModule::nPyObject_CallObject, Pointer )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    PyObject* args = NODE_ARG_OBJECT( 1, PyObject );
    PyObject* tuple = 0;
    bool newt = false;

    if( !obj )
    {
      throwBadArgumentException( NODE_THIS, NODE_THREAD, "Nil PyObject given" );
    }

    if( !PyTuple_Check( args ) )
    {
      if( args != Py_None )
      {
        //
        //  Reference counting: If we make a new tuple to pass to
        //  CallObject, we must DecRef it after use, since at that point
        //  there will be no other references to that object (we "own the
        //  reference").  BUT, we assign the incoming "args" object from
        //  the user to be the contents of the tuple and PyTuple_SetItem
        //  "steals the reference", which means that it will DecRef it when
        //  the Tuple is collected, since we did not create the object and
        //  it needs to persist after we return, so IncRef it.
        //
        tuple = PyTuple_New( 1 );
        newt = true;
        Py_XINCREF( args );
        PyTuple_SetItem( tuple, 0, args );
      }
      else
      {
        tuple = PyTuple_New( 0 );
        newt = true;
      }
    }
    else
    {
      tuple = args;
    }

    PyObject* value = PyObject_CallObject( obj, tuple );
    if( newt ) Py_XDECREF( tuple );

    if( !value ) PyErr_Print();

    NODE_RETURN( value );
  }

  NODE_IMPLEMENTATION( PyModule::nPyObject_CallObject2, Pointer )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* pyobj = NODE_ARG_OBJECT( 0, PyObject );
    ClassInstance* obj = NODE_ARG_OBJECT( 1, ClassInstance );
    bool newt = false;

    if( !pyobj )
    {
      throwBadArgumentException( NODE_THIS, NODE_THREAD, "Nil PyObject given" );
    }

    //  Assume mu2py returns "new reference"
    //
    PyObject* args =
        mu2py( c, NODE_THREAD.process(), obj->type(), Value( obj ) );

    if( !PyTuple_Check( args ) )
    {
      //
      //  We have new reference for args, so no need to IncRef, but still
      //  need to DecRef newly created tuple.  See comment above for details.
      //
      PyObject* pytuple = PyTuple_New( 1 );
      newt = true;
      PyTuple_SetItem( pytuple, 0, args );
      args = pytuple;
    }

    PyObject* value = PyObject_CallObject( pyobj, args );
    if( newt ) Py_XDECREF( args );

    if( !value ) PyErr_Print();

    NODE_RETURN( value );
  }

  NODE_IMPLEMENTATION( PyModule::nPy_INCREF, void )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    Py_XINCREF( obj );
  }

  NODE_IMPLEMENTATION( PyModule::nPy_DECREF, void )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    Py_XDECREF( obj );
  }

  NODE_IMPLEMENTATION( PyModule::is_nil, bool )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* obj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( obj == 0 );
  }

  NODE_IMPLEMENTATION( PyModule::type_name, Pointer )
  {
    PyLockObject locker;
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );

    PyTypeObject* obj = NODE_ARG_OBJECT( 0, PyTypeObject );
    NODE_RETURN( c->stringType()->allocate( obj->tp_name ) );
  }

  NODE_IMPLEMENTATION( PyModule::classInstanceToPyObject, Pointer )
  {
    PyLockObject locker;
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    ClassInstance* i = NODE_ARG_OBJECT( 0, ClassInstance );
    if( i )
    {
      Value v( i );
      NODE_RETURN( mu2py( c, p, i->type(), v ) );
    }
    else
    {
      Py_XINCREF( Py_None );
      return Py_None;
    }
  }

  NODE_IMPLEMENTATION( PyModule::PyObjectToString, Pointer )
  {
    PyLockObject locker;
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* pyobj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( py2mu( c, p, c->stringType(), pyobj )._Pointer );
  }

  NODE_IMPLEMENTATION( PyModule::PyObjectToBool, bool )
  {
    PyLockObject locker;
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* pyobj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( py2mu( c, p, c->boolType(), pyobj )._bool );
  }

  NODE_IMPLEMENTATION( PyModule::PyObjectToInt, int )
  {
    PyLockObject locker;
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* pyobj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( py2mu( c, p, c->intType(), pyobj )._int );
  }

  NODE_IMPLEMENTATION( PyModule::PyObjectToFloat, float )
  {
    PyLockObject locker;
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>( NODE_THREAD.context() );
    PyObject* pyobj = NODE_ARG_OBJECT( 0, PyObject );
    NODE_RETURN( py2mu( c, p, c->floatType(), pyobj )._float );
  }

}  // namespace Mu
