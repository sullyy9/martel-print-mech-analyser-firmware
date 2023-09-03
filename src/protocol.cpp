////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    protocol.cpp
/// @brief   Module for enccoding and decoding data.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>

#include "protocol.hpp"
#include "uart.hpp"

/*------------------------------------------------------------------------------------------------*/
// types
/*------------------------------------------------------------------------------------------------*/

enum class State {
    Idle,
    Processing,
};

/*------------------------------------------------------------------------------------------------*/
// private variables
/*------------------------------------------------------------------------------------------------*/

namespace {

constexpr uint8_t FRAME_START = 0x02;
constexpr uint8_t FRAME_END = 0x03;
constexpr uint8_t ESCAPE = 0x1B;

constinit State state = State::Idle;

constinit bool escape_next = false;

constinit std::array<uint8_t, 64> cmd_buffer{};
constinit uint32_t cmd_buffer_in_ptr{};

}

/*------------------------------------------------------------------------------------------------*/
// private functions
/*------------------------------------------------------------------------------------------------*/

namespace {

auto process_command() -> std::optional<protocol::Command>;

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

auto protocol::process_byte(uint8_t byte) -> std::optional<Command> {

    if(state == State::Idle) {
        if(byte == FRAME_START) {
            cmd_buffer_in_ptr = 0;
            state = State::Processing;
        }
        return std::nullopt;
    }

    // state == Processing.
    if(byte == FRAME_START && !escape_next) {
        state = State::Idle;
        return Command::FrameError;
    }

    if(byte == FRAME_END && !escape_next) {
        state = State::Idle;
        return process_command();
    }

    if(byte == ESCAPE && !escape_next) {
        escape_next = true;
        return std::nullopt;
    }

    cmd_buffer[cmd_buffer_in_ptr++] = byte;
    escape_next = false;

    return std::nullopt;
}

/*------------------------------------------------------------------------------------------------*/

auto protocol::send_response(Response response, std::optional<const std::span<const uint8_t>> data)
    -> void {
    if(response == Response::Acknowledge) {
        // while(uart::free() < 5);
        uart::write(std::array<uint8_t, 5>{FRAME_START, 0x06, FRAME_END, '\r', '\n'});
        return;
    }

    if(response == Response::MotorAdvance) {
        // while(uart::free() < 5);
        uart::write(std::array<uint8_t, 5>{FRAME_START, 'F', FRAME_END, '\r', '\n'});
        return;
    }

    if(response == Response::MotorReverse) {
        // while(uart::free() < 5);
        uart::write(std::array<uint8_t, 5>{FRAME_START, 'B', FRAME_END, '\r', '\n'});
        return;
    }

    if(response == Response::BurnLine && data) {
        // while(uart::free() < (data.value().size() + 5));
        uart::write(std::array<uint8_t, 2>{FRAME_START, 'U'});

        for(const auto byte : data.value()) {
            if(byte == FRAME_START || byte == FRAME_END || byte == ESCAPE) {
                uart::write(std::array{ESCAPE, byte});
            } else {
                uart::write(byte);
            }
        }

        uart::write(std::array<uint8_t, 3>{FRAME_END, '\r', '\n'});
        return;
    }
}

/*------------------------------------------------------------------------------------------------*/
// private functions
/*------------------------------------------------------------------------------------------------*/

namespace {

auto process_command() -> std::optional<protocol::Command> {
    switch(cmd_buffer.front()) {
        case 'P': return protocol::Command::Poll;

        case 'A': return protocol::Command::SetPaperIn;
        case 'a': return protocol::Command::SetPaperOut;

        case 'L': return protocol::Command::SetPlatenIn;
        case 'l': return protocol::Command::SetPlatenOut;

        case 'R': return protocol::Command::RecordingStart;
        case 'r': return protocol::Command::RecordingStop;

        default: return protocol::Command::Unrecognised;
    }
}

}

/*------------------------------------------------------------------------------------------------*/
