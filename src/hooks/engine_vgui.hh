#pragma once

namespace paint_hook {

void notify_begin_draw();
void wait_for_end_draw();

void init_all();
void shutdown_all();
} // namespace paint_hook
