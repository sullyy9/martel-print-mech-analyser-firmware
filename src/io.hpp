#pragma once

#include <cstdint>

/*------------------------------------------------------------------------------------------------*/
// types
/*------------------------------------------------------------------------------------------------*/

namespace io {

enum class LEDColour: uint32_t {
    None = 0b111,
    Red = 0b011,
    Green = 0b101,
    Blue = 0b110,
};

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

namespace io {

auto init() -> void;

auto monoled_1_on() -> void;
auto monoled_2_on() -> void;

auto monoled_1_off() -> void;
auto monoled_2_off() -> void;

auto rgb_led_set(LEDColour colour) -> void;

auto button_is_pressed() -> bool;

auto paper_in() -> void;
auto paper_out() -> void;

auto platen_in() -> void;
auto platen_out() -> void;

}

/*------------------------------------------------------------------------------------------------*/
