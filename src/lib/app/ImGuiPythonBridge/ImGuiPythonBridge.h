
#include <vector>
#include <memory>

// Forward declare PyObject to avoid including Python.h in headers
struct _object;
typedef _object PyObject;

#ifndef IMGUIPYTHONBRIDGE_H
#define IMGUIPYTHONBRIDGE_H

namespace Rv
{
    class ImGuiPythonBridge
    {
    public:
        static void registerCallback(PyObject* callable);
        static void callCallbacks();

        struct PyObjectDeleter
        {
            void operator()(PyObject* obj) const;
        };

        using PyObjectPtr = std::unique_ptr<PyObject, PyObjectDeleter>;

    private:
        static std::vector<PyObjectPtr> s_callbacks;
    };
} // namespace Rv

#endif // IMGUIPYTHONBRIDGE_H
