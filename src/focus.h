#pragma once

enum focus_flags {
    focus_flag_none = 0,
    focus_flag_can_interact = (1 << 0),
    focus_flag_can_scroll = (1 << 1)
};
