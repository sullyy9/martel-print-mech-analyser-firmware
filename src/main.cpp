#include <array>
#include <cstdint>
#include <optional>

#include "xil_printf.h"

#include "interrupt.hpp"
#include "io.hpp"
#include "mech.hpp"

/*------------------------------------------------------------------------------------------------*/

auto main() -> int {

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("Entered function main\r\n");

    const auto status = interrupt::init();
    xil_printf("Setup interrupts - %s\r\n", interrupt::status_message(status).data());

    io::init();
    mech::init();

    //////////////////////////////////////////////////
    xil_printf("Start main loop :)\r\n");

    std::optional<mech::Action> action_next {};

    while(true) {
        if(io::button_is_pressed()) {
            io::monoled_1_on();
            io::monoled_2_on();
        } else {
            io::monoled_1_off();
            io::monoled_2_off();
        }

        if(!action_next) {
            action_next = mech::get_next_action();
        }

        if(action_next == mech::Action::Advance) {
            print("ADV\r\n");
            action_next.reset();

        } else if(action_next == mech::Action::Reverse) {
            print("REV\r\n");
            action_next.reset();

        } else if(action_next == mech::Action::BurnLineStart) {
            action_next.reset();

        } else if(action_next == mech::Action::BurnLineStop) {
            const auto burn_line = mech::read_burn_line();

            if(!burn_line) {
                xil_printf("Error: expected burn line but none was available.");
            } else {
                xil_printf("%u LN:", burn_line->size());
                for(const auto word : burn_line.value()) {
                    outbyte(static_cast<uint8_t>(word >> 0 & 0xFF));
                    outbyte(static_cast<uint8_t>(word >> 8 & 0xFF));
                    outbyte(static_cast<uint8_t>(word >> 16 & 0xFF));
                    outbyte(static_cast<uint8_t>(word >> 24 & 0xFF));
                }
                xil_printf("\r\n");
            }
            action_next.reset();
        }
    }
}
