////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    mech.hpp
/// @brief   Module for handling the print mechanism emulator.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <array>
#include <cstdint>
#include <optional>

/*------------------------------------------------------------------------------------------------*/

namespace mech {

enum class Action : uint8_t {
    Advance,
    Reverse,
    BurnLineStart,
    BurnLineStop,
};

constexpr uint32_t HEAD_WIDTH = 384;
constexpr uint32_t HEAD_BYTES = (HEAD_WIDTH / 8);
constexpr uint32_t HEAD_WORDS = (HEAD_BYTES / 4);

}

/*------------------------------------------------------------------------------------------------*/

namespace mech {

auto init() -> void;

auto clear() -> void;

auto get_next_action() -> std::optional<Action>;

auto read_burn_line() -> std::optional<std::array<uint8_t, HEAD_BYTES>>;

}

/*------------------------------------------------------------------------------------------------*/
