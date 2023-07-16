#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "interrupt.hpp"
#include "io.hpp"
#include "mech.hpp"
#include "protocol.hpp"
#include "thermistor.hpp"
#include "uart.hpp"

using namespace std::literals;

/*------------------------------------------------------------------------------------------------*/

namespace {

constinit bool record{false};

}

/*------------------------------------------------------------------------------------------------*/

auto main() -> int {

    io::init();

    io::paper_out();
    io::platen_out();
    io::monoled_1_off();
    io::monoled_2_off();
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

    uart::write("\r\n"sv);
    uart::write("\r\n"sv);
    uart::write("\r\n"sv);
    uart::write("--------------------------------------------------\r\n"sv);
    uart::write("Martel Print Mech Analyser\r\n"sv);
    uart::write("--------------------------------------------------\r\n"sv);

    mech::init();
    thermistor::init();

    thermistor::set_temp(25);

    //////////////////////////////////////////////////

    uart::write("Startup complete\r\n"sv);
    std::optional<mech::Action> action_next{};

    while(true) {

        // Handle button press.
        if(io::button_is_pressed()) {
            io::monoled_1_on();
            io::monoled_2_on();
        } else {
            io::monoled_1_off();
            io::monoled_2_off();
        }

        // Read received bytes and process until a command is found.
        std::optional<protocol::Command> command{};
        while(uart::received() > 0) {
            if(command = protocol::process_byte(uart::read()); command) {
                break;
            }
        }

        // Handle the command if one was found.
        if(command) {
            using enum protocol::Command;
            switch(command.value()) {
                case Unrecognised: uart::write("Unrecognised command\r\n"sv); break;
                case FrameError: uart::write("Frame error\r\n"sv); break;

                case Poll: uart::write(0x06); break;

                case SetPaperIn: io::paper_in(); break;
                case SetPaperOut: io::paper_out(); break;

                case SetPlatenIn: io::platen_in(); break;
                case SetPlatenOut: io::platen_out(); break;

                case RecordingStart: record = true; break;
                case RecordingStop: record = false; break;
            }
        }

        // Process mech events.
        if(record) {
            if(!action_next) {
                action_next = mech::get_next_action();
            }

            if(action_next == mech::Action::Advance) {
                uart::write("ADV\r\n"sv);
                action_next.reset();

            } else if(action_next == mech::Action::Reverse) {
                uart::write("REV\r\n"sv);
                action_next.reset();

            } else if(action_next == mech::Action::BurnLineStart) {
                action_next.reset();

            } else if(action_next == mech::Action::BurnLineStop) {
                const auto burn_line = mech::read_burn_line();

                if(!burn_line) {
                    uart::write("Error: expected burn line but none was available.\r\n"sv);
                } else {
                    uart::write("LN:"sv);
                    uart::write(burn_line.value());
                    uart::write("\r\n"sv);
                }
                action_next.reset();
            }
        }
    }
}
