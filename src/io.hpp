#pragma once

#include <cstdint>

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

namespace io {

auto init() -> void;

auto monoled_1_on() -> void;
auto monoled_2_on() -> void;

auto monoled_1_off() -> void;
auto monoled_2_off() -> void;

auto button_is_pressed() -> bool;

auto paper_in() -> void;
auto paper_out() -> void;

auto platen_in() -> void;
auto platen_out() -> void;

}

/*------------------------------------------------------------------------------------------------*/
