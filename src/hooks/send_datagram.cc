#include <precompiled.hh>

#include <sdk/convar.hh>
#include <sdk/hooks.hh>
#include <sdk/sdk.hh>

#include "../sdk/log.hh"
#include <modules/backtrack.hh>

#if doghook_platform_msvc()
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#endif

extern sdk::UserCmd *last_cmd;

using namespace sdk;

class bf_write;

ConvarWrapper sv_maxusrcmdprocessticks{"sv_maxusrcmdprocessticks"}; // 24 is default but someone might be S-M-R-T enough to higher it up or lower it

static Convar<int> doghook_tickbaseshift     = Convar<int>{"doghook_tickbaseshift_value", 1, 0, 15 /* Convar<int>::no_value */, nullptr}; // maximum chocked is 15 rn sadly
static Convar<int> doghook_tickbaseshift_key = Convar<int>{"doghook_tickbaseshift_key", ButtonCode::KEY_E, 0, KEY_COUNT, nullptr};

namespace send_datagram {

std::unique_ptr<hooks::HookFunction<NetChannel, 0>> send_datagram_hook;

#if doghook_platform_windows()
u32 __fastcall hooked_send_datagram(NetChannel *channel, void *, sdk::bf_write *datagram)
#else
u32 hooked_send_datagram(NetChannel *channel, bf_write *datagram)
#endif
{
    auto in_state    = channel->in_reliable_state();
    auto in_sequence = channel->in_sequence();

    //TODO: fix bs about sequences!
    //backtrack::add_latency_to_netchannel(channel);

    /*//STK's tickbase manip try
#if doghook_platform_windows()
    static u32 *cl_lastoutgoingcommand_ptr              = *(u32 **)(signature::find_pattern("engine", "8B 3D ? ? ? ? 40") + 2);
    static u32 *cl_chokedcommands_ptr                   = (u32 *)((u8 *)cl_lastoutgoingcommand_ptr + 4);                                            // cl_lastoutgoingcommand_ptr+4
    static u32 *cl_m_nDeltaTick                         = *(u32 **)(signature::find_pattern("engine", "8B 0D ? ? ? ? A1 ? ? ? ? F3 0F 11 45") + 7); // 8B 0D ? ? ? ? A1 ? ? ? ? F3 0F 11 45
    static u32 *cl_getservertickcount                   = *(u32 **)(signature::find_pattern("engine", "FF 50 34 FF 35") + 5);                       //FF 50 34 FF 35
    static u32 *host_frametime_stddeviation             = *(u32 **)(signature::find_pattern("engine", "F3 0F 10 05 ? ? ? ? 51") + 4);
    static u32 *host_frametime                          = *(u32 **)(signature::find_pattern("engine", "F3 0F 10 05 ? ? ? ? 8D 4D C4") + 4);
    static u32  NET_TickVtable                          = *(u32 *)(signature::find_pattern("engine", "C7 45 ? ? ? ? ? C6 45 88 00") + 3);
    using cl_sendmove_func                              = void (*)(void);
    static auto  cl_sendmove                            = (cl_sendmove_func)signature::resolve_callgate(signature::find_pattern("engine", "E8 ? ? ? ? 80 7D FF 00 0F 84"));
    static void *ret_addr                               = (void *)(signature::find_pattern("engine", "FF D0 83 3D ? ? ? ? ? A3 ? ? ? ?") + 2);
#else
	#error Implement me!
#endif

	static void *cmdbaks = malloc(90 << 6); // MULTIPLAYER_BACKUP * 64
    memcpy(cmdbaks, (void *)iface::input->get_verified_user_cmd_array(), 90 << 6);

	if (/*(_ReturnAddress() == ret_addr) &&* / 0 && (int) doghook_tickbaseshift_key && iface::input_system->is_button_down((ButtonCode)(int)doghook_tickbaseshift_key)) {
        auto last_command     = *cl_lastoutgoingcommand_ptr;
        auto choked_commands = *cl_chokedcommands_ptr;

        int maxcmds = sv_maxusrcmdprocessticks.get_int();

        auto last_tick_idx = std::max(0, std::min((int)doghook_tickbaseshift, maxcmds-2));
        //auto last_cmd      = iface::input->get_user_cmd(iface::globals->tickcount);
        for (int i = 0; i < (doghook_tickbaseshift + 2); i++) {
            u32 next_command_number = last_command + choked_commands + 1;
            //SMH get cmd right here
            UserCmd *cmd = (UserCmd*)iface::input->get_verified_user_cmd(next_command_number); // am i just dumb??? or not? :th3:
            UserCmd *lst = (UserCmd*)iface::input->get_verified_user_cmd(iface::globals->tickcount);
            if (cmd) {
                if (last_cmd)
                    *cmd = *last_cmd;
                cmd->command_number = next_command_number++;
                cmd->tick_count     = *cl_getservertickcount + iface::globals->time_to_ticks(0.5f) + i; //???
                //if (i == last_tick_idx && lst->buttons & IN_ATTACK) {
                    cmd->buttons |= IN_ATTACK;
                //    lst->buttons &= ~IN_ATTACK;
                //}
                if (i != (doghook_tickbaseshift + 1))
                    choked_commands++;
            } else
                logging::msg("GOT NULL CMD!!!");
        }
        logging::msg("TICKBASE SHIFT XD GUYS! %i", iface::globals->tickcount);
        send_datagram_hook->call_original<u32>(NULL);
        //iface::engine->net_channel_info()->transmit();
        static void *tmp         = malloc(64); // seems like an overkill for this; // its 32
        *(u32 *)((u8 *)tmp)      = NET_TickVtable;
        *(u32 *)((u8 *)tmp + 4)  = 0;
        *(u32 *)((u8 *)tmp + 8)  = 0;
        *(u32 *)((u8 *)tmp + 12) = 0;
        *(u32 *)((u8 *)tmp + 16) = 0;                            //SKIP???
        *(u32 *)((u8 *)tmp + 20) = *cl_m_nDeltaTick;
        *(u32 *)((u8 *)tmp + 24) = *host_frametime;              //wtf
        *(u32 *)((u8 *)tmp + 28) = *host_frametime_stddeviation; // wtf
        //for (u32 i=0; i<0xFFFF; i++)
        iface::engine->net_channel_info()->send_net_msg(tmp);
        //logging::msg("B00m!");

		*(u32 *)((u8 *)iface::engine->net_channel_info() + 28) = choked_commands;
        *cl_chokedcommands_ptr = choked_commands;

		//for (u32 i = 0; i < 0xFFFF; i++)
		cl_sendmove(); // TODO: rebuild this to bypass limit of 15 choked

		// TODO: make cooler & non repetetive exit
		auto ret = send_datagram_hook->call_original<u32>(datagram);

		//memcpy((void *)iface::input->get_verified_user_cmd_array(), cmdbaks, 90 << 6);

		*cl_lastoutgoingcommand_ptr = ret;
        *cl_chokedcommands_ptr      = 0;

		channel->in_reliable_state() = in_state;
        channel->in_sequence()       = in_sequence;

        return ret;
    }
	//*/
    if (iface::input_system->is_button_down(ButtonCode::KEY_E) /* && !(iface::globals->tickcount % 25)*/) {
        channel->out_sequence() += 150;
    }
    auto ret = send_datagram_hook->call_original<u32>(datagram);

    channel->in_reliable_state() = in_state;
    channel->in_sequence()       = in_sequence;

    return ret;
}

//todo move to more appropriate place
std::unique_ptr<hooks::HookFunction<NetChannel, 0>> send_net_msg_hook;

class net_msg_proto {
public:
    u32 get_group() {
        return_virtual_func(get_group, 8, -1, -1, 0);
    }
};

class clc_move_proto {
public:
    int           VTable;
    char          reliable;
    int           netchan;
    int           field_C;
    int           field_10;
    int           cl_cmdbackup;
    int           m_nNewCommands;
    int           m_nLength;
    sdk::bf_write m_DataIn;
    sdk::bf_write m_dataOut;
};

//bool altern = 0;

// why am i releasing hot shit tho :-(, no credit gang rise up
#if doghook_platform_windows()
u8 __fastcall hooked_send_net_msg(NetChannel *channel, void *, net_msg_proto *msg, bool reliable, bool voice)
#else
u8  hooked_send_net_msg(NetChannel *channel, net_msg_proto *msg, bool reliable, bool voice)
#endif
{
    //auto ret =

    //if (iface::globals->tickcount % 25) {
    if (iface::input_system->is_button_down(ButtonCode::KEY_F) && msg->get_group() == 0xA) { // CLC_Move
                                                                                             //send_net_msg_hook->call_original<u8>(msg, reliable, voice);
        auto movemsg            = (clc_move_proto *)msg;
        movemsg->cl_cmdbackup   = 0; // TODO: find out if backup can help in rising the shit i need
        movemsg->m_nNewCommands = 0;
        // i have a feeling setting netchan is more of a server thing so i skip it since it's not set in CL_SendMove
        // Nulling it for lulz
        movemsg->m_dataOut.m_bOverflow  = 0;
        movemsg->m_dataOut.m_nDataBits  = 0;
        movemsg->m_dataOut.m_nDataBytes = 0;
        movemsg->m_dataOut.m_iCurBit    = 0;
        logging::msg("LULZ HAPPENING XD GUYS!11");
        //for (u32 i = 0; i < 0xFF; i++)
        //    send_net_msg_hook->call_original<u8>(msg, reliable, voice);
        //altern = 0;
    }
    //}

    //logging::msg("hook happening!");

    return send_net_msg_hook->call_original<u8>(msg, reliable, voice);
}

void level_init() {
    assert(send_datagram_hook == nullptr);
    send_datagram_hook = std::make_unique<hooks::HookFunction<NetChannel, 0>>(iface::engine->net_channel_info(), 46, 47, 47, reinterpret_cast<void *>(&hooked_send_datagram));
    assert(send_net_msg_hook == nullptr);
    send_net_msg_hook = std::make_unique<hooks::HookFunction<NetChannel, 0>>(iface::engine->net_channel_info(), 40, -1, -1, reinterpret_cast<void *>(&hooked_send_net_msg));
}

void level_shutdown() {
    send_datagram_hook.reset();
    send_datagram_hook = nullptr;
    send_net_msg_hook.reset();
    send_net_msg_hook = nullptr;
}

} // namespace send_datagram
