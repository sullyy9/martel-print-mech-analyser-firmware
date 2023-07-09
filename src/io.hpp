#include <cstdint>

namespace io {

auto init() -> void;

auto monoled_1_on() -> void;
auto monoled_2_on() -> void;

auto monoled_1_off() -> void;
auto monoled_2_off() -> void;

auto button_is_pressed() -> bool;

}
