#include <precompiled.hh>

#include "misc.hh"

#include <sdk/convar.hh>
#include <sdk/interface.hh>
#include <sdk/netvar.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>

using namespace sdk;

namespace misc {

// TODO: cleanup and move out of here
enum cvar_flags {
    FCVAR_UNREGISTERED    = (1 << 0), // If this is set, don't add to linked list, etc.
    FCVAR_DEVELOPMENTONLY = (1 << 1), // Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
    FCVAR_GAMEDLL         = (1 << 2), // defined by the game DLL
    FCVAR_CLIENTDLL       = (1 << 3), // defined by the client DLL
    FCVAR_HIDDEN          = (1 << 4), // Hidden. Doesn't appear in find or autocomplete. Like DEVELOPMENTONLY, but can't be compiled out.

    // ConVar only
    FCVAR_PROTECTED = (1 << 5),  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
    FCVAR_SPONLY    = (1 << 6),  // This cvar cannot be changed by clients connected to a multiplayer server.
    FCVAR_ARCHIVE   = (1 << 7),  // set to cause it to be saved to vars.rc
    FCVAR_NOTIFY    = (1 << 8),  // notifies players when changed
    FCVAR_USERINFO  = (1 << 9),  // changes the client's info string
    FCVAR_CHEAT     = (1 << 14), // Only useable in singleplayer / debug / multiplayer & sv_cheats

    FCVAR_PRINTABLEONLY   = (1 << 10), // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
    FCVAR_UNLOGGED        = (1 << 11), // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
    FCVAR_NEVER_AS_STRING = (1 << 12), // never try to print that cvar

    // It's a ConVar that's shared between the client and the server.
    // At signon, the values of all such ConVars are sent from the server to the client (skipped for local
    //  client, of course )
    // If a change is requested it must come from the console (i.e., no remote client changes)
    // If a value is changed while a server is active, it's replicated to all connected clients
    FCVAR_REPLICATED       = (1 << 13), // server setting enforced on clients, TODO rename to FCAR_SERVER at some time
    FCVAR_DEMO             = (1 << 16), // record this cvar when starting a demo file
    FCVAR_DONTRECORD       = (1 << 17), // don't record these command in demofiles
    FCVAR_RELOAD_MATERIALS = (1 << 20), // If this cvar changes, it forces a material reload
    FCVAR_RELOAD_TEXTURES  = (1 << 21), // If this cvar changes, if forces a texture reload

    FCVAR_NOT_CONNECTED          = (1 << 22), // cvar cannot be changed by a client that is connected to a server
    FCVAR_MATERIAL_SYSTEM_THREAD = (1 << 23), // Indicates this cvar is read from the material system thread
    FCVAR_ARCHIVE_XBOX           = (1 << 24), // cvar written to config.cfg on the Xbox

    FCVAR_ACCESSIBLE_FROM_THREADS = (1 << 25), // used as a debugging tool necessary to check material system thread convars

    FCVAR_SERVER_CAN_EXECUTE    = (1 << 28), // the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
    FCVAR_SERVER_CANNOT_QUERY   = (1 << 29), // If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
    FCVAR_CLIENTCMD_CAN_EXECUTE = (1 << 30), // IVEngineClient::ClientCmd is allowed to execute this command.
                                             // Note: IVEngineClient::ClientCmd_Unrestricted can run any client command.
};

Netvar local_angles{"DT_BasePlayer", "pl", "deadflag"};

void init_all() {
    for (auto c : sdk::ConvarWrapper::get_range()) {
        auto flags = c.flags();
        flags &= ~FCVAR_CHEAT;
        flags &= ~FCVAR_DEVELOPMENTONLY;
        flags &= ~FCVAR_PROTECTED;
        flags &= ~FCVAR_SPONLY;
        flags &= ~FCVAR_CHEAT;
        flags &= ~FCVAR_REPLICATED;
        flags &= ~FCVAR_NOT_CONNECTED;
        flags &= ~FCVAR_HIDDEN;

        c.set_flags(flags);
    }

    local_angles.offset_delta(4);
}

Convar<bool> doghook_misc_show_aa{"doghook_misc_show_aa", true, nullptr};

math::Vector last_viewangles;

void create_move(sdk::UserCmd *cmd) {
    last_viewangles = cmd->viewangles;
}

void update(float frametime) {
    if (!IFace<Engine>()->in_game()) return;
}

} // namespace misc
