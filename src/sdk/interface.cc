#include "precompiled.hh"

#include "interface.hh"
#include "signature.hh"

#include "platform.hh"

using InstantiateInterfaceFn = void *(*)();

class InterfaceReg {
public:
    InterfaceReg() = delete;

public:
    InstantiateInterfaceFn create_fn;
    const char *           name;

    InterfaceReg *next;
};

void *interface_helpers::find_interface(const char *module_name, const char *interface_name) {

    InterfaceReg *interface_reg_head;

    if constexpr (doghook_platform::windows()) {
        interface_reg_head = **signature::find_pattern<InterfaceReg ***>(module_name, "8B 35 ? ? ? ? 57 85 F6 74 38", 2);
    } else if constexpr (doghook_platform::linux()) {
        interface_reg_head = **signature::find_pattern<InterfaceReg ***>(module_name, "8B 1D ? ? ? ? 8B 75 08 8B 7D 0C 85 DB 75 0E EB 35 90 8D 74 26 00 8B 5B 08 85 DB 74 29 89 74 24 04 8B 43 04", 2);
    } else if constexpr (doghook_platform::osx()) {
    }

    assert(interface_reg_head);

    for (auto r = interface_reg_head; r != nullptr; r = r->next) {
        auto match = strstr(r->name, interface_name);

        // Only match if the next character after the name is the number
        if (match != nullptr && !isalpha(*(match + strlen(interface_name)))) return r->create_fn();
    }

    // if you get here either
    // 1: your interface name is wrong
    // 2: your dll name is wrong
    assert(0);

    return nullptr;
}
