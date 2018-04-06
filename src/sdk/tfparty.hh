#pragma once

namespace tf {
namespace party {

void get_tf_party();

bool in_queue();

void request_queue(int type);

void request_leave_queue(int type);

} // namespace party
namespace gc {

void get_gc();

bool is_connect_to_match_server();

void abandon();

} // namespace gc

} // namespace tf
