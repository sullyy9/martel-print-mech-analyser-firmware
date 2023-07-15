#include <array>
#include <cstdint>
#include <optional>

#include "interrupt.hpp"
#include "io.hpp"
#include "mech.hpp"
#include "thermistor.hpp"
#include "uart.hpp"

/*------------------------------------------------------------------------------------------------*/

auto main() -> int {

    io::init();
    io::rgb_led_set(io::LEDColour::Green);

    if(const auto status = interrupt::init(); status != interrupt::Status::Ok) {
        while(true) {
            io::rgb_led_set(io::LEDColour::Red);
        }
    }

    if(const auto error = uart::init(); error) {
        while(true) {
            io::rgb_led_set(io::LEDColour::Red);
        }
    }

    uart::write("\r\n");
    uart::write("\r\n");
    uart::write("\r\n");
    uart::write("--------------------------------------------------\r\n");
    uart::write("Martel Print Mech Analyser\r\n");
    uart::write("--------------------------------------------------\r\n");

    mech::init();
    thermistor::init();

    thermistor::set_temp(25);

    //////////////////////////////////////////////////
    // xil_printf("Start main loop :)\r\n");

    uart::write("Startup complete\r\n");
    std::optional<mech::Action> action_next{};

    while(true) {
        if(io::button_is_pressed()) {
            io::monoled_1_on();
            io::monoled_2_on();
            io::paper_out();
            io::platen_out();
        } else {
            io::monoled_1_off();
            io::monoled_2_off();
            io::paper_in();
            io::platen_in();
        }

        if(!action_next) {
            action_next = mech::get_next_action();
        }

        if(action_next == mech::Action::Advance) {
            uart::write("ADV\r\n");
            action_next.reset();

        } else if(action_next == mech::Action::Reverse) {
            uart::write("REV\r\n");
            action_next.reset();

        } else if(action_next == mech::Action::BurnLineStart) {
            action_next.reset();

        } else if(action_next == mech::Action::BurnLineStop) {
            const auto burn_line = mech::read_burn_line();

            if(!burn_line) {
                uart::write("Error: expected burn line but none was available.");
            } else {
                uart::write("LN:");
                for(const auto word : burn_line.value()) {
                    uart::write(static_cast<uint8_t>(word >> 0 & 0xFF));
                    uart::write(static_cast<uint8_t>(word >> 8 & 0xFF));
                    uart::write(static_cast<uint8_t>(word >> 16 & 0xFF));
                    uart::write(static_cast<uint8_t>(word >> 24 & 0xFF));
                }
                uart::write("\r\n");
            }
            action_next.reset();
        }
    }
}
