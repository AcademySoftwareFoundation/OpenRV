///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkUtil__EnvVar__h__
#define __TwkUtil__EnvVar__h__

/*==============================================================================
 * EXTERNAL DECLARATIONS
 *============================================================================*/

#include <iosfwd>
#include <string>
#include <vector>

namespace TwkUtil
{

    /*==============================================================================
     * USEFULL MACROS
     *============================================================================*/

#define ENVVAR_CONCAT2(a, b) a##b
#define ENVVAR_CONCAT(a, b) ENVVAR_CONCAT2(a, b)

    // Use the following for global and static EnvVar variables.

#define ENVVAR_STRING(T, V, D)                                                 \
    TwkUtil::EnvVar<std::string> T = {{V, false, 0, 0}, D, 0, false};          \
    static TwkUtil::RegEnvVar<std::string> ENVVAR_CONCAT(evRegUniq, __LINE__)( \
        __FILE__, T)

#ifdef INTERNAL
#define ENVVAR_STRING_INTERNAL ENVVAR_STRING
#else
#define ENVVAR_STRING_INTERNAL(T, V, D)            \
    static struct                                  \
    {                                              \
        std::string getValue() const { return D; } \
    } T
#endif

#define ENVVAR_BOOL(T, V, D)                                   \
    TwkUtil::EnvVar<bool> T = {{V, false, 0, 0}, D, D, false}; \
    static TwkUtil::RegEnvVar<bool> ENVVAR_CONCAT(evRegUniq,   \
                                                  __LINE__)(__FILE__, T)

#ifdef INTERNAL
#define ENVVAR_BOOL_INTERNAL ENVVAR_BOOL
#else
#define ENVVAR_BOOL_INTERNAL(T, V, D)       \
    static struct                           \
    {                                       \
        bool getValue() const { return D; } \
    } T
#endif

#define ENVVAR_INT(T, V, D)                                   \
    TwkUtil::EnvVar<int> T = {{V, false, 0, 0}, D, D, false}; \
    static TwkUtil::RegEnvVar<int> ENVVAR_CONCAT(evRegUniq,   \
                                                 __LINE__)(__FILE__, T)

#ifdef INTERNAL
#define ENVVAR_INT_INTERNAL ENVVAR_INT
#else
#define ENVVAR_INT_INTERNAL(T, V, D)       \
    static struct                          \
    {                                      \
        int getValue() const { return D; } \
    } T
#endif

#define ENVVAR_FLOAT(T, V, D)                                   \
    TwkUtil::EnvVar<float> T = {{V, false, 0, 0}, D, D, false}; \
    static TwkUtil::RegEnvVar<float> ENVVAR_CONCAT(evRegUniq,   \
                                                   __LINE__)(__FILE__, T)

#ifdef INTERNAL
#define ENVVAR_FLOAT_INTERNAL ENVVAR_FLOAT
#else
#define ENVVAR_FLOAT_INTERNAL(T, V, D)       \
    static struct                            \
    {                                        \
        float getValue() const { return D; } \
    } T
#endif

    // Use the following for temporary local on-stack EnvVar variables.

#define ONSTACK_ENVVAR_STRING(T, V, D)                                \
    TwkUtil::EnvVar<std::string> T = {{V, false, 0, 0}, D, 0, false}; \
    TwkUtil::RegEnvVar<std::string> ENVVAR_CONCAT(evRegUniq,          \
                                                  __LINE__)(__FILE__, T)

#define ONSTACK_ENVVAR_BOOL(T, V, D)                           \
    TwkUtil::EnvVar<bool> T = {{V, false, 0, 0}, D, D, false}; \
    TwkUtil::RegEnvVar<bool> ENVVAR_CONCAT(evRegUniq, __LINE__)(__FILE__, T)

#define ONSTACK_ENVVAR_INT(T, V, D)                           \
    TwkUtil::EnvVar<int> T = {{V, false, 0, 0}, D, D, false}; \
    TwkUtil::RegEnvVar<int> ENVVAR_CONCAT(evRegUniq, __LINE__)(__FILE__, T)

#define ONSTACK_ENVVAR_FLOAT(T, V, D)                           \
    TwkUtil::EnvVar<float> T = {{V, false, 0, 0}, D, D, false}; \
    TwkUtil::RegEnvVar<float> ENVVAR_CONCAT(evRegUniq, __LINE__)(__FILE__, T)

    /*==============================================================================
     * CLASS EnvVarRegistry
     *============================================================================*/

    /*==============================================================================
     * CLASS EnvVar
     *============================================================================*/

    // <summary> Statically initializable env. variable descriptor. </summary>

    // Allows itself to be completely statically initialisable by being a POT
    // (plain-old-data), avoiding order-of-initialization problems. Thus it
    // must:
    //
    //    - *not* contain data with constructor or virtual functions.
    //    - *not* contain virtual functions.
    //    - *not* have constructor, destructor and assignment operator.
    struct BaseEnvVar
    {

        // Definition of a function pointer for the callback
        typedef void (*ChangeValueCallBackFunctionDef)(void* data);

        // Enum
        // data type
        enum EnvVarType
        {
            EV_NONE,
            EV_STRING,
            EV_BOOL,
            EV_INT,
            EV_FLOAT
        };

        // available field
        enum EnvVarField
        {
            EV_NAME,
            EV_VALUE,
            EV_DEF_VALUE,
            EV_FILE_NAME
        };

        // defines
        static const int MAX_NAME_LENGTH = 128;
        static const int MAX_STR_VALUE_LENGTH = 128;

        /*----- member functions -----*/

        // Get name.
        const char* getName() const;

        // Set a callback
        void setChangeValueCallback(
            ChangeValueCallBackFunctionDef changeValueFunctionCallback,
            void* changeValueCallbackParameter);

        // Called when the value of this envVar change, invoke the callback
        // function if it have been set.
        void invokeChangeValueCallback();

        /*----- data members -----*/

        // What environment string to use?
        const char* const _name;

        // Since the envvars are often declared statically, and there
        // is no way to predict the static initialization order, we have to
        // ensure the object is fully constructed before using it.
        bool _ready;

        // Pointer on the funciton to be called
        ChangeValueCallBackFunctionDef _changeValueFunctionCallback;
        // The callback parameters
        void* _changeValueCallbackParameter;
    };

    // <summary> Maps the type of the user-level class template
    //           to its implementation type. </summary>

    template <class T> struct EnvVarMapper;

    template <> struct EnvVarMapper<bool>
    {
        typedef bool type;
    };

    template <> struct EnvVarMapper<int>
    {
        typedef int type;
    };

    template <> struct EnvVarMapper<float>
    {
        typedef float type;
    };

    template <> struct EnvVarMapper<std::string>
    {
        typedef const char* type;
    };

    // <summary> Concrete env. var type. </summary>
    template <typename T> struct EnvVar
    {
        /*----- member types -----*/

        typedef typename EnvVarMapper<T>::type U;

        // Get name.
        const char* getName() const;

        // Set a callback
        void setChangeValueCallback(BaseEnvVar::ChangeValueCallBackFunctionDef
                                        changeValueFunctionCallback,
                                    void* changeValueCallbackParameter);

        // Called when the value of this envVar change, invoke the callback
        // function if it have been set.
        void invokeChangeValueCallback();

        // Get type
        BaseEnvVar::EnvVarType getType() const;

        // Get value.
        T getValue() const;

        // Get value as char
        void getValueAsStr(char* strPtr, int size) const;

        // Get default value.
        T getDefaultValue() const;

        // Get the default status.
        bool isDefault() const;

        // Get default value as char
        void getDefaultValueAsStr(char* strPtr, int size) const;

        // Get the readonly status.
        bool isReadOnly() const;

        // set value and propagate to all
        void setValue(T newValue);

        // set value but don't propagate
        void setValueOnly(T newValue);

        // set default value
        void setDefaultValue(T newValue);

        // set the readonly status.
        void setReadOnlyStatus(bool isReadOnly);

        // Refresh value.
        void refresh();

        // Dump.
        std::ostream& dump(std::ostream& os) const;

        /*----- data members -----*/

        BaseEnvVar _base;
        // Current default value.
        // In the case of const char*, allocated memory via strdup() if _ready.
        U _defaultValue;
        // Current value.
        // In the case of const char*, allocated memory via strdup() if
        // non-null.
        U _value;
        // If true the value cannot be changed.
        bool _isReadOnly;
    };

    /*==============================================================================
     * CLASS RegEnvVar
     *============================================================================*/

    // <summary> Environment string wrapper. </summary>

    // Wraps 'getenv', gets named environment string on construction and
    // keep the value. Value can be explicitly refreshed.
    class BaseRegEnvVar
    {

    public:
        /*----- member functions -----*/

        BaseRegEnvVar();

        // Destruct, no op.
        virtual ~BaseRegEnvVar();

        BaseRegEnvVar(const char* fileName);

        const char* getFileName() const;

        // Get name.
        virtual const char* getName() const = 0;

        // Get type
        virtual BaseEnvVar::EnvVarType getType() const = 0;

        // Get value as char
        virtual void getValueAsStr(char* strPtr, int size) const = 0;

        // Get default value as char
        virtual void getDefaultValueAsStr(char* strPtr, int size) const = 0;

        // Get the readonly status.
        virtual bool isReadOnly() const = 0;

        // Dump.
        virtual std::ostream& dump(std::ostream& os) const = 0;

    private:
        // Set the value. Only set if the value is of the proper type, else
        // asserts.
        virtual void dynSetValue(bool newValue);
        virtual void dynSetValue(int newValue);
        virtual void dynSetValue(float newValue);
        virtual void dynSetValue(std::string newValue);

        /*----- data members -----*/

        // The file where the environment variable is created
        std::string _fileName;

        // Give access to dynSetValue.
        friend struct EnvVar<std::string>;
        friend struct EnvVar<int>;
        friend struct EnvVar<bool>;
        friend struct EnvVar<float>;
    };

    template <class T> class RegEnvVar : public BaseRegEnvVar
    {

    public:
        /*----- member functions -----*/

        RegEnvVar();

        // Destruct, no op.
        ~RegEnvVar();

        RegEnvVar(const char* fileName, EnvVar<T>& var);

        // Get name.
        const char* getName() const;

        // Get type
        BaseEnvVar::EnvVarType getType() const;

        // Get value as char
        void getValueAsStr(char* strPtr, int size) const;

        // Get default value as char
        void getDefaultValueAsStr(char* strPtr, int size) const;

        // Get the readonly status.
        bool isReadOnly() const;

        // Dump.
        std::ostream& dump(std::ostream& os) const;

        // Get value.
        inline T getValue() const { return _var.getValue(); }

        // set value and propagate to all
        inline void setValue(T newValue) { _var.setValue(newValue); }

        // Get default value.
        inline T getDefaultValue() const { return _var.getDefaultValue(); }

    private:
        // Set value implementation detail.
        void dynSetValue(T newValue);

        /*----- data members -----*/

        // The real env. variable.
        EnvVar<T>& _var;
    };

    class EnvVarRegistry
    {

    public:
        /*----- static member functions -----*/

        static EnvVarRegistry& getInstance();

        /*---- member functions -----*/

        virtual ~EnvVarRegistry();

        virtual BaseRegEnvVar* getEnvVar(int i) const = 0;

        virtual std::vector<int> search(const char* searchStr, int searchType,
                                        bool exact) = 0;

        virtual int getNumElements() const = 0;

        virtual void remove(BaseRegEnvVar* var) = 0;
        virtual void dump(std::ostream& os) const = 0;

        virtual void dumpHtml(std::ostream& os) const = 0;

    protected:
        /*----- member functions -----*/

        EnvVarRegistry();
    };

    // <summary> EnvVar global functions </summary>
    // Output operator.
    std::ostream& operator<<(std::ostream& os, const BaseRegEnvVar& var);

    std::ostream& operator<<(std::ostream& os, const EnvVarRegistry& reg);

} // end namespace TwkUtil

#include <TwkUtil/EnvVar.inline.h>

#endif // __TwkUtil__EnvVar__h__
