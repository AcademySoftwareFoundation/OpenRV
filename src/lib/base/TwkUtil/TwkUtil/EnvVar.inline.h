///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkUtil__EnvVar__inline__h__
#define __TwkUtil__EnvVar__inline__h__

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <TwkUtil/Macros.h>
#include <TwkUtil/dll_defs.h>

#include <assert.h>
#include <iomanip>
#include <typeinfo>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <locale>

namespace TwkUtil
{

    namespace EnvVarUtils
    {
        //------------------------------------------------------------------------------
        //
        inline void initialize(const EnvVar<bool>& var)
        {
            if (!var._base._ready)
            {
                // Initialization not done yet, so cast-away constness to
                // initialize now.
                EnvVar<bool>& modVar = const_cast<EnvVar<bool>&>(var);
                modVar._defaultValue = var._defaultValue;
                modVar._base._ready = true;
                modVar.refresh();
            }
        }

        inline void initialize(const EnvVar<int>& var)
        {
            if (!var._base._ready)
            {
                // Initialization not done yet, so cast-away constness to
                // initialize now.
                EnvVar<int>& modVar = const_cast<EnvVar<int>&>(var);
                modVar._defaultValue = var._defaultValue;
                modVar._base._ready = true;
                modVar.refresh();
            }
        }

        inline void initialize(const EnvVar<float>& var)
        {
            if (!var._base._ready)
            {
                // Initialization not done yet, so cast-away constness to
                // initialize now.
                EnvVar<float>& modVar = const_cast<EnvVar<float>&>(var);
                modVar._defaultValue = var._defaultValue;
                modVar._base._ready = true;
                modVar.refresh();
            }
        }

        inline void initialize(const EnvVar<std::string>& var)
        {
            if (!var._base._ready)
            {
                // Initialization not done yet, so cast-away constness to
                // initialize now.
                EnvVar<std::string>& modVar =
                    const_cast<EnvVar<std::string>&>(var);
                modVar._defaultValue = strdup(var._defaultValue);
                modVar._base._ready = true;
                modVar.refresh();
            }
        }

        //------------------------------------------------------------------------------
        //
        inline void cleanup(EnvVar<bool>&) {}

        inline void cleanup(EnvVar<int>&) {}

        inline void cleanup(EnvVar<float>&) {}

        inline void cleanup(EnvVar<std::string>& var)
        {
            if (var._value)
            {
                free((void*)var._value);
                var._value = 0;
            }

            if (var._base._ready)
            {
                free((void*)var._defaultValue);
                var._defaultValue = "";
            }
        }

        //------------------------------------------------------------------------------
        //
        inline bool compareValues(const char* defaultVal, const char* val)
        {
            return (strcmp(defaultVal, val ? val : "") == 0);
        }

        inline bool compareValues(bool defaultVal, bool val)
        {
            return (defaultVal == val);
        }

        inline bool compareValues(int defaultVal, int val)
        {
            return (defaultVal == val);
        }

        inline bool compareValues(float defaultVal, float val)
        {
            return (EQUAL(defaultVal, val));
        }

        //------------------------------------------------------------------------------
        //
        inline void doSetValueOnly(EnvVar<bool>& var, bool newValue)
        {
            var._value = newValue;
        }

        inline void doSetValueOnly(EnvVar<int>& var, int newValue)
        {
            var._value = newValue;
        }

        inline void doSetValueOnly(EnvVar<float>& var, float newValue)
        {
            var._value = newValue;
        }

        inline void doSetValueOnly(EnvVar<std::string>& var,
                                   std::string newValue)
        {
            if (var._value)
            {
                free((void*)var._value);
                var._value = 0;
            }
            var._value = strdup(newValue.c_str());
        }

        //------------------------------------------------------------------------------
        //
        inline void doSetDefaultValue(EnvVar<bool>& var, bool newValue)
        {
            var._defaultValue = newValue;
        }

        inline void doSetDefaultValue(EnvVar<int>& var, int newValue)
        {
            var._defaultValue = newValue;
        }

        inline void doSetDefaultValue(EnvVar<float>& var, float newValue)
        {
            var._defaultValue = newValue;
        }

        inline void doSetDefaultValue(EnvVar<std::string>& var,
                                      std::string& newValue)
        {
            if (var._defaultValue)
                free((void*)var._defaultValue);

            var._defaultValue = strdup(newValue.c_str());
        }

        //------------------------------------------------------------------------------
        //
        inline void convertFromChar(const char*& dest, const char* src)
        {
            if (dest)
            {
                free((void*)dest);
            }

            dest = strdup(src);
        }

        inline void convertFromChar(bool& dest, const char* src)
        {
            std::string src_str = src;
            if (src_str == "false" || src_str == "FALSE" || src_str == "0")
            {
                dest = false;
            }
            else
            {
                dest = true;
            }
        }

        inline void convertFromChar(int& dest, const char* src)
        {
            dest = ::atoi(src);
        }

        inline void convertFromChar(float& dest, const char* src)
        {
            dest = static_cast<float>(::atof(src));
        }

        inline char myToUpperFunctor(char s)
        {
#ifdef _WIN32
            return ::toupper(s);
#else
            return std::toupper(s);
#endif
        }

        // specialisations, string, int, bool and floats
        inline void valueToStr(const char* val, char* strPtr, int size)
        {
            strncpy(strPtr, val, size);
        }

        inline void valueToStr(bool val, char* strPtr, int size)
        {
            if (val)
            {
                strncpy(strPtr, "true", size);
            }
            else
            {
                strncpy(strPtr, "false", size);
            }
        }

        inline void valueToStr(int val, char* strPtr, int size)
        {
            snprintf(strPtr, size, "%d", val);
        }

        inline void valueToStr(const double val, char* strPtr, int size)
        {
            snprintf(strPtr, size, "%.3f", val);
        }

    } // end namespace EnvVarUtils

    //==============================================================================
    // LOCAL CLASS EnvVarRegistryImp
    //==============================================================================

    class TWKUTIL_EXPORT EnvVarRegistryImp : public EnvVarRegistry
    {

    public:
        /*----- types and enumerations ----*/

        typedef std::vector<BaseRegEnvVar*> Container;

        /*----- static member functions -----*/

        static EnvVarRegistryImp& getInstance();

        /*----- member functions -----*/

        // Construct/destruct.
        EnvVarRegistryImp();
        ~EnvVarRegistryImp();

        // Add/remove variables.
        void add(BaseRegEnvVar* var);
        void remove(BaseRegEnvVar* var);

        // get number of elements
        int getNumElements() const;

        // get pointer to given element at given idx
        BaseRegEnvVar* getEnvVar(int index) const;

        // get a list of envvar* that match the search string
        std::vector<int> search(const char* searchStr, int searchType,
                                bool exact);

        // get a specified field in string from an Env var.
        void getFieldValueAsStr(char* val, int searchType, int envVarNum) const;

        // See base class.
        void dump(std::ostream& os) const;
        void dumpHtml(std::ostream& os) const;

    private:
        /*----- data members -----*/

        Container _vars;
    };

    //==============================================================================
    // CLASS EnvVarRegistry
    //==============================================================================

    //------------------------------------------------------------------------------
    //
    inline EnvVarRegistry& EnvVarRegistry::getInstance()
    {
        return EnvVarRegistryImp::getInstance();
    }

    //------------------------------------------------------------------------------
    //
    inline EnvVarRegistry::EnvVarRegistry() {}

    //------------------------------------------------------------------------------
    //
    inline EnvVarRegistry::~EnvVarRegistry() {}

    //==============================================================================
    // CLASS BaseEnvVar
    //==============================================================================
    //------------------------------------------------------------------------------
    //
    inline const char* BaseEnvVar::getName() const { return _name; }

    //------------------------------------------------------------------------------
    //
    inline void BaseEnvVar::setChangeValueCallback(
        ChangeValueCallBackFunctionDef changeValueFunctionCallback,
        void* changeValueCallbackParameter)
    {
        _changeValueFunctionCallback = changeValueFunctionCallback;
        _changeValueCallbackParameter = changeValueCallbackParameter;
    }

    //------------------------------------------------------------------------------
    //
    inline void BaseEnvVar::invokeChangeValueCallback()
    {
        if (_changeValueFunctionCallback != NULL)
        {
            // Execute the function
            (*_changeValueFunctionCallback)(_changeValueCallbackParameter);
        }
    }

    //==============================================================================
    // CLASS BaseRegEnvVar
    //==============================================================================
    //------------------------------------------------------------------------------
    //
    inline BaseRegEnvVar::BaseRegEnvVar(const char* /*fileName*/
    )
    {
    }

    //------------------------------------------------------------------------------
    //
    inline BaseRegEnvVar::~BaseRegEnvVar() {}

    //------------------------------------------------------------------------------
    //
    inline const char* BaseRegEnvVar::getFileName() const
    {
        return _fileName.c_str();
    }

    //------------------------------------------------------------------------------
    //
    inline void BaseRegEnvVar::dynSetValue(bool /*newValue*/) { assert(0); }

    inline void BaseRegEnvVar::dynSetValue(int /*newValue*/) { assert(0); }

    inline void BaseRegEnvVar::dynSetValue(float /*newValue*/) { assert(0); }

    inline void BaseRegEnvVar::dynSetValue(std::string /*newValue*/)
    {
        assert(0);
    }

    //==============================================================================
    // CLASS RegEnvVar
    //==============================================================================
    //------------------------------------------------------------------------------
    //
    template <class T>
    RegEnvVar<T>::RegEnvVar(const char* fileName, EnvVar<T>& var)
        : BaseRegEnvVar(fileName)
        , _var(var)
    {
        EnvVarUtils::initialize(var);
        // Add to registry.
        EnvVarRegistryImp::getInstance().add(this);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> RegEnvVar<T>::~RegEnvVar()
    {
        // Remove from registry.
        EnvVarRegistryImp::getInstance().remove(this);

        EnvVarUtils::cleanup(_var);
        _var._base._ready = false;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> const char* RegEnvVar<T>::getName() const
    {
        return _var.getName();
    }

    //------------------------------------------------------------------------------
    //
    template <class T> BaseEnvVar::EnvVarType RegEnvVar<T>::getType() const
    {
        return _var.getType();
    }

    //------------------------------------------------------------------------------
    //
    template <class T>
    void RegEnvVar<T>::getValueAsStr(char* strPtr, int size) const
    {
        _var.getValueAsStr(strPtr, size);
    }

    //------------------------------------------------------------------------------
    //
    template <class T>
    void RegEnvVar<T>::getDefaultValueAsStr(char* strPtr, int size) const
    {
        _var.getDefaultValueAsStr(strPtr, size);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> bool RegEnvVar<T>::isReadOnly() const
    {
        return _var.isReadOnly();
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void RegEnvVar<T>::dynSetValue(T newValue)
    {
        _var.setValueOnly(newValue);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> std::ostream& RegEnvVar<T>::dump(std::ostream& os) const
    {
        using namespace std;

        os << "file : " << getFileName() << endl;
        return _var.dump(os);
    }

    //==============================================================================
    // CLASS EnvVar
    //==============================================================================
    //------------------------------------------------------------------------------
    //
    template <class T> const char* EnvVar<T>::getName() const
    {
        return _base.getName();
    }

    //------------------------------------------------------------------------------
    //
    template <class T>
    void EnvVar<T>::setChangeValueCallback(
        BaseEnvVar::ChangeValueCallBackFunctionDef changeValueFunctionCallback,
        void* changeValueCallbackParameter)
    {
        _base.setChangeValueCallback(changeValueFunctionCallback,
                                     changeValueCallbackParameter);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::invokeChangeValueCallback()
    {
        _base.invokeChangeValueCallback();
    }

    //------------------------------------------------------------------------------
    //
    template <class T> BaseEnvVar::EnvVarType EnvVar<T>::getType() const
    {
        if (typeid(T) == typeid(std::string))
        {
            return BaseEnvVar::EV_STRING;
        }
        else if (typeid(T) == typeid(bool))
        {
            return BaseEnvVar::EV_BOOL;
        }
        else if (typeid(T) == typeid(int))
        {
            return BaseEnvVar::EV_INT;
        }
        else if (typeid(T) == typeid(float))
        {
            return BaseEnvVar::EV_FLOAT;
        }
        return BaseEnvVar::EV_NONE;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> T EnvVar<T>::getValue() const
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);
        return _value;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> T EnvVar<T>::getDefaultValue() const
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);
        return _defaultValue;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> bool EnvVar<T>::isDefault() const
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);
        return EnvVarUtils::compareValues(_defaultValue, _value);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::setValue(T newValue)
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);

        // Lets find all envvars that have that same name
        std::vector<int> found = EnvVarRegistry::getInstance().search(
            _base._name, BaseEnvVar::EV_NAME, 1);

        // And set the new value
        for (std::vector<int>::const_iterator it = found.begin();
             it != found.end(); ++it)
        {
            BaseRegEnvVar* ev = EnvVarRegistry::getInstance().getEnvVar(*it);
            // Make sure all found types are the same, otherwise boom
            assert(ev->getType() == getType());
            ev->dynSetValue(newValue);
        }
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::setValueOnly(T newValue)
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);

        if (_isReadOnly)
        {
#ifdef DL_DEBUG
            assert("ERROR: cannot change the value of a read-only EnvVar"
                   == NULL);
#else
            return;
#endif
        }
        else
        {
            EnvVarUtils::doSetValueOnly(*this, newValue);
            _base.invokeChangeValueCallback();
        }
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::setDefaultValue(T newValue)
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);

        EnvVarUtils::doSetDefaultValue(*this, newValue);

        refresh();
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::setReadOnlyStatus(bool state)
    {
        _isReadOnly = state;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> void EnvVar<T>::refresh()
    {
        if (!_base._ready) // Static init sanity check (see comment in .h file).
            EnvVarUtils::initialize(*this);

        const char* newval = getenv(_base._name);
        if (newval)
        {
            EnvVarUtils::convertFromChar(_value, newval);
        }
        else
        {
            EnvVarUtils::doSetValueOnly(*this, _defaultValue);
        }
    }

    //------------------------------------------------------------------------------
    //
    template <class T>
    void EnvVar<T>::getValueAsStr(char* strPtr, int size) const
    {
        EnvVarUtils::valueToStr(_value, strPtr, size);
    }

    template <class T>
    void EnvVar<T>::getDefaultValueAsStr(char* strPtr, int size) const
    {
        EnvVarUtils::valueToStr(_defaultValue, strPtr, size);
    }

    //------------------------------------------------------------------------------
    //
    template <class T> bool EnvVar<T>::isReadOnly() const
    {
        return _isReadOnly;
    }

    //------------------------------------------------------------------------------
    //
    template <class T> std::ostream& EnvVar<T>::dump(std::ostream& os) const
    {
        using namespace std;

        os << "name : " << _base.getName() << endl
           << "value : " << _value << endl;

        return os;
    }

    //------------------------------------------------------------------------------
    //
    inline std::ostream& operator<<(std::ostream& os,
                                    const EnvVarRegistry& /* reg */
    )
    {
        EnvVarRegistryImp::getInstance().dump(os);
        return os;
    }

    //------------------------------------------------------------------------------
    //
    inline std::ostream& operator<<(std::ostream& os, const BaseRegEnvVar& var)
    {
        return var.dump(os);
    }

} // end namespace TwkUtil

#endif // __TwkUtil__EnvVar__inline__h__
