//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <PyTwkApp/PyInterface.h>
#include <PyTwkApp/PyCommands.h>
#include <PyTwkApp/PyEventType.h>
#include <PyTwkApp/PyFunctionAction.h>
#include <PyTwkApp/PyMenuState.h>
#include <PyTwkApp/PyMu.h>
#include <PyTwkApp/PyMuSymbolType.h>

#include <MuPy/PyModule.h>
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/MuInterface.h>
#include <TwkApp/Menu.h>
#include <TwkPython/PyLockObject.h>
#include <Python.h>
#include <TwkUtil/File.h>

#include <QByteArray>
#include <QtGlobal>

#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string>

namespace TwkApp
{
  using namespace std;

  // extern "C" { void initcStringIO(void); }

  static Menu::Item* tupleToMenuItem( PyObject* );
  static Menu* listToMenu( const char*, PyObject* );

  static Menu::Item* tupleToMenuItem( PyObject* t )
  {
    PyLockObject locker;

    size_t n = PyTuple_Size( t );

    if( n == 2 )
    {
      //
      //  Sub-Menu item
      //

      const char* label;
      PyObject* obj;

      if( PyArg_ParseTuple( t, "sO", &label, &obj ) )
      {
        if( PyList_Check( obj ) )
        {
          return new Menu::Item( label, 0, listToMenu( label, obj ) );
        }
        else if( obj == Py_None || PyCallable_Check( obj ) )
        {
          return new Menu::Item(
              label, obj == Py_None ? NULL : new PyFunctionAction( obj ) );
        }
      }

      ostringstream str;
      str << "ERROR:" << ( label ? label : "" );
      return new Menu::Item( str.str(), NULL );
    }
    else if( n == 4 )
    {
      const char* label;
      PyObject* actionFunc;
      PyObject* keyObj;
      PyObject* stateFunc;

      if( PyArg_ParseTuple( t, "sOOO", &label, &actionFunc, &keyObj,
                            &stateFunc ) )
      {
        if( ( actionFunc == Py_None || PyCallable_Check( actionFunc ) ) &&
            ( stateFunc == Py_None || PyCallable_Check( stateFunc ) ) )
        {
          std::string title = label;
          std::string key;

          if( keyObj == Py_None )
          {
            key = "";
          }
          else if( PyBytes_Check( keyObj ) )
          {
            key = PyBytes_AsString( keyObj );
          }
          else if( PyUnicode_Check( keyObj ) )
          {
            PyObject* keyObjBytes = PyUnicode_AsUTF8String( keyObj );
            key = PyBytes_AsString( keyObjBytes );
            Py_XDECREF( keyObjBytes );
          }
          else
          {
            title = std::string( "ERROR: " ) + title +
                    std::string( " (Invalid Shortcuts)" );
          }

          return new Menu::Item(
              title,
              actionFunc == Py_None ? NULL : new PyFunctionAction( actionFunc ),
              key, stateFunc == Py_None ? NULL : new PyStateFunc( stateFunc ) );
        }
      }

      ostringstream str;
      str << "ERROR: " << ( label ? label : "" );
      return new Menu::Item( str.str(), NULL );
    }
    else if( n > 0 )
    {
      PyObject* l = PyTuple_GetItem( t, 0 );
      ostringstream str;
      str << "ERROR: ";

      if( PyBytes_Check( l ) )
      {
        const char* s = PyBytes_AsString( l );
        str << s;
      }

      return new Menu::Item( str.str(), NULL );
    }
    else
    {
      return NULL;
    }
  }

  static Menu* listToMenu( const char* name, PyObject* l )
  {
    size_t n = PyList_Size( l );
    Menu* menu = new Menu( name );

    for( size_t i = 0; i < n; i++ )
    {
      PyObject* t = PyList_GetItem( l, i );

      if( t && PyTuple_CheckExact( t ) )
      {
        if( Menu::Item* item = tupleToMenuItem( t ) )
        {
          menu->addItem( item );
        }
      }
    }

    return menu;
  }

  Menu* pyListToMenu( const char* name, void* p )
  {
    return listToMenu( name, (PyObject*)p );
  }

  namespace
  {
    bool pythonHasBeenInitialized = false;
  };

  void* callPythonFunction( const char* name, const char* module )
  {
    if( !pythonHasBeenInitialized ) return 0;

    PyObject *pModule, *pModuleDict, *pFunc;
    PyObject *pArgs = 0, *pValue = 0;
    PyLockObject locker;

    if( !module ) module = "__main__";

    pModule = PyImport_ImportModule( module );

    if( pModule == NULL )
    {
      if( PyErr_Occurred() )
      {
        PyErr_Print();
      }
      return 0;
    }

    if( !PyObject_HasAttrString( pModule, name ) )
    {
      Py_XDECREF( pModule );
      return 0;
    }

    pFunc = PyObject_GetAttrString( pModule, name );

    if( pFunc == NULL )
    {
      if( PyErr_Occurred() )
      {
        PyErr_Print();
      }
      return 0;
    }

    if( PyCallable_Check( pFunc ) )
    {
      pArgs = PyTuple_New( 0 );
      pValue = PyObject_CallObject( pFunc, pArgs );

      Py_XDECREF( pFunc );
      Py_XDECREF( pArgs );

      if( pValue != NULL )
      {
        return pValue;
      }
      else
      {
        if( PyErr_Occurred() )
        {
          PyErr_Print();
        }
        return 0;
      }
    }
    else
    {
      cerr << name << " is not callable" << endl;
      Py_XDECREF( pFunc );
      return 0;
    }
  }

  void disposeOfPythonObject( void* obj )
  {
    PyLockObject locker;

    if( obj )
    {
      PyObject* pobj = (PyObject*)obj;
      Py_XDECREF( pobj );
    }
  }

  void evalPython( const char* text )
  {
    PyLockObject locker;

    PyRun_SimpleString( text );
  }

  void initPython( int argc, char** argv )
  {
    // PreInitialize Python
    // Note: This is necessary for Python to utilize environment variables like PYTHONUTF8
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);
    PyStatus status=Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
      Py_ExitStatusException(status);
    }

    Py_InitializeEx( 1 );
    static wchar_t delim = L'\0';

    wchar_t** w_argv = new wchar_t*[argc + 1];
    w_argv[argc] = &delim;

    for (int i = 0; i < argc; ++i ){
      w_argv[i] = Py_DecodeLocale(argv[i], nullptr);
    }

    PySys_SetArgvEx( argc, w_argv, 0 );

    //
    //  Initialize python threading, acquire the GIL for the main
    //  thread.
    //
    PyEval_InitThreads();

    //
    //  Release the GIL
    //
    (void)PyEval_SaveThread();

    //
    //  Add MuPy to the default Mu modules
    //

    Mu::Module* python = new Mu::PyModule( muContext(), "python" );
    muContext()->globalScope()->addSymbol( python );

    //
    //  Set python3 as multiprocessing executable
    //

    evalPython(""
        "import multiprocessing, os, sys\n"
        "bin_dir, bin_name = os.path.split(sys.executable)\n"
        "bin_name, bin_ext = os.path.splitext(bin_name)\n"
        "for python_process in ('python', 'python3', 'py-interp'):\n"
        "  python_exe = os.path.join(bin_dir, python_process + bin_ext)\n"
        "  if os.path.exists(python_exe):\n"
        "    multiprocessing.set_executable(python_exe)\n"
        "    break\n"
        ""
    );

  }

  void finalizePython()
  {
    // Deliberately not using PyLockObject to avoid calling
    // PyGILState_Release in its destructor.
    PyGILState_Ensure();

    Py_Finalize();
  }

  //
  //  Used by PyMu module to convert unknown types. In this case the
  //  Event type
  //

  static Mu::ClassInstance* convertEventToMu( const Mu::Type* t, void* p )
  {
    PyLockObject locker;

    PyObject* pobj = (PyObject*)p;

    if( Py_TYPE( pobj ) == pyEventType() )
    {
      PyEventObject* eobj = reinterpret_cast<PyEventObject*>( pobj );
      Mu::Name ename = muContext()->lookupName( "Event" );
      const EventType* etype =
          muContext()->findSymbolOfType<EventType>( ename );
      assert( etype );

      if( etype == t )
      {
        EventType::EventInstance* e = new EventType::EventInstance( etype );
        e->event = eobj->event;
        e->document = eobj->document;
        return e;
      }
    }
    else if( Py_TYPE( pobj ) == pyEventType() )
    {
      PyLockObject locker;

      PyEventObject* eobj = reinterpret_cast<PyEventObject*>( pobj );
      Mu::Name ename = muContext()->lookupName( "Event" );
      const EventType* etype =
          muContext()->findSymbolOfType<EventType>( ename );
      assert( etype );

      if( etype == t )
      {
        EventType::EventInstance* e = new EventType::EventInstance( etype );
        e->event = eobj->event;
        e->document = eobj->document;
        return e;
      }
    }

    return 0;
  }

  static void* convertEventToPy( const Mu::Type* t, Mu::ClassInstance* muobj )
  {
    PyLockObject locker;

    if( const EventType* etype = dynamic_cast<const EventType*>( t ) )
    {
      EventType::EventInstance* obj =
          reinterpret_cast<EventType::EventInstance*>( muobj );
      return PyEventFromEvent( obj->event, obj->document );
    }

    return 0;
  }

  static Mu::Pointer convertMuSymbolToSymbol( const Mu::Type* t, void* p )
  {
    PyLockObject locker;

    PyObject* pobj = (PyObject*)p;

    if( t == muContext()->symbolType() )
    {
      if( Py_TYPE( pobj ) == pyMuSymbolType() )
      {
        PyMuSymbolObject* o = reinterpret_cast<PyMuSymbolObject*>( pobj );
        return Mu::Pointer( o->symbol );
      }
    }

    return Mu::Pointer( 0 );
  }

  static void* convertSymbolToMuSymbol( const Mu::Type* t, void* p )
  {
    PyLockObject locker;
    if( t == muContext()->symbolType() )
    {
      if( PyMuSymbolObject* o = (PyMuSymbolObject*)pyMuSymbolType()->tp_alloc( pyMuSymbolType(), 0 ) )
      {
        o->symbol = reinterpret_cast<const Mu::Symbol*>( p );
        o->function = dynamic_cast<const Mu::Function*>( o->symbol );
        return (PyObject*)o;
      }
      }

    return 0;
  }

  //------

  void pyInitWithFile( const char* rcfile, void* commands0, void* commands1 )
  {
    vector<PyMethodDef> methods;
    PyMethodDef* d = 0;
    PyLockObject locker;

    Mu::PyModule::addConverterFunctions( convertEventToPy, convertEventToMu, 0,
                                         0 );
    Mu::PyModule::addConverterFunctions( 0, 0, convertSymbolToMuSymbol,
                                         convertMuSymbolToSymbol );

    if( commands0 )
    {
      d = (PyMethodDef*)commands0;
      for( size_t i = 0; d[i].ml_name; i++ ) methods.push_back( d[i] );
    }
    if( commands1 )
    {
      d = (PyMethodDef*)commands1;
      for( size_t i = 0; d[i].ml_name; i++ ) methods.push_back( d[i] );
    }

    PyMethodDef nulldef = { NULL };
    methods.push_back( nulldef );
    d = methods.empty() ? 0 : &methods.front();

    // initcStringIO();
    pyInitCommands( d );
    PyObject* pymu = initPyMu();

    PyObject* pModule = PyImport_ImportModule( "__main__" );

    pyEventType()->tp_new = reinterpret_cast<newfunc>( PyType_GenericNew );

    if( PyType_Ready( pyEventType() ) >= 0 )
    {
      Py_XINCREF( pyEventType() );
      PyModule_AddObject( pModule, "Event",
                          reinterpret_cast<PyObject*>( pyEventType() ) );
    }

    if( PyType_Ready( pyMuSymbolType() ) >= 0 )
    {
      Py_XINCREF( pyMuSymbolType() );
      PyModule_AddObject( pymu, "MuSymbol", (PyObject*)pyMuSymbolType() );
    }

    Py_XDECREF( pModule );

    PyObject* rcs = PyImport_ImportModule( "rv_commands_setup" );

    std::ifstream fileSteam( rcfile );
    if( fileSteam.is_open() )
    {
      std::string content( ( std::istreambuf_iterator<char>( fileSteam ) ),
                           ( std::istreambuf_iterator<char>() ) );
      PyRun_SimpleString( content.c_str() );
    }

    pythonHasBeenInitialized = true;

    Py_XDECREF( rcs );
  }

}  // namespace TwkApp
