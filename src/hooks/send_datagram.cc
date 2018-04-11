#include <precompiled.hh>

#include <sdk/convar.hh>
#include <sdk/hooks.hh>
#include <sdk/sdk.hh>

#include <modules/backtrack.hh>

using namespace sdk;

class bf_write;

namespace send_datagram {

std::unique_ptr<hooks::HookFunction<NetChannel, 0>> send_datagram_hook;

#if doghook_platform_windows()
u32 __fastcall hooked_send_datagram(NetChannel *channel, void *, bf_write *datagram)
#else
u32 hooked_send_datagram(NetChannel *channel, bf_write *datagram)
#endif
{
    auto in_state    = channel->in_reliable_state();
    auto in_sequence = channel->in_sequence();

    // TODO: Call out to backtrack
    backtrack::add_latency_to_netchannel(channel);

    auto ret = send_datagram_hook->call_original<u32>(datagram);

    channel->in_reliable_state() = in_state;
    channel->in_sequence()       = in_sequence;

    return ret;
}

void level_init() {
    assert(send_datagram_hook == nullptr);
    send_datagram_hook = std::make_unique<hooks::HookFunction<NetChannel, 0>>(IFace<Engine>()->net_channel_info(), 46, 47, 47, reinterpret_cast<void *>(&hooked_send_datagram));
}

void level_shutdown() {
    send_datagram_hook.reset();
    send_datagram_hook = nullptr;
}

} // namespace send_datagram
