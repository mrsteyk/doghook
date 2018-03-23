#include "precompiled.hh"

#include "log.hh"
#include "signature.hh"

auto logging::msg(const char *format, ...) -> void {

    char    buffer[1024];
    va_list vlist;
    va_start(vlist, format);
    vsnprintf(buffer, 1024, format, vlist);
    va_end(vlist);

    using MessageFn = void (*)(const char *format, ...);

    static auto msg_fn = (MessageFn)signature::resolve_import(signature::resolve_library("tier0"), "Msg");

    msg_fn("[Woof] %s\n", buffer);

    printf("[Woof] %s\n", buffer);
}
