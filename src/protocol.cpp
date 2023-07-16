////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    protocol.cpp
/// @brief   Module for enccoding and decoding data.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>

#include "protocol.hpp"

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
        if(state == State::Idle && byte == FRAME_START) {
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

    return std::nullopt;
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
