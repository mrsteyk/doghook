#pragma once

namespace hex {
static constexpr auto in_range(char x, char a, char b) {
    return (x >= a && x <= b);
}

static constexpr auto get_bits(char x) {
    if (in_range((x & (~0x20)), 'A', 'F')) {
        return (x & (~0x20)) - 'A' + 0xa;
    } else if (in_range(x, '0', '9')) {
        return x - '0';
    } else {
        return 0;
    }
}

static constexpr auto byte(const char *x) {
    return get_bits(x[0]) << 4 | get_bits(x[1]);
}
static constexpr auto word(const char *x) {
    return byte(x) << 8 | byte(x + 2);
}
static constexpr auto dword(const char *x) {
    return word(x) << 16 | word(x + 4);
}
} // namespace hex
