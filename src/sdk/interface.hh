#pragma once

namespace interface_helpers {

// this auto-resolves the library using Signature::
void *find_interface(const char *module_name, const char *interface_name);
} // namespace interface_helpers

template <typename T>
class Interface {
    static T *value;

public:
    // interface name should be the name of the interface
    // without the version number
    // e.g. "VClient017" -> "VClient"
    static auto set_from_interface(const char *module_name, const char *interface_name) {
        value = static_cast<T *>(
            interface_helpers::find_interface(module_name, interface_name));
    }

    // set from a pointer (you should only do this for non-exported
    //  interfaces e.g. IInput or CClientModeShared)
    static auto set_from_pointer(T *new_value) {
        value = new_value;
    }

    auto operator-> () -> T *& {
        return value;
    }

    auto get() -> T *& {
        return value;
    }

    operator bool() {
        return value != nullptr;
    }
};

template <typename T>
T *Interface<T>::value = nullptr;

template <typename T>
using IFace = Interface<T>;
