#include <precompiled.hh>

#include "misc.hh"

#include <sdk/convar.hh>
#include <sdk/interface.hh>
#include <sdk/sdk.hh>

using namespace sdk;

namespace misc {

sdk::ConvarWrapper sv_cheats{"sv_cheats"};

void init_all() {
    sv_cheats.set_flags(0);
}
} // namespace misc
