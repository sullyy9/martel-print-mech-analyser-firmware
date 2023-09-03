////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    protocol.hpp
/// @brief   Module for enccoding and decoding data.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <optional>
#include <span>

/*------------------------------------------------------------------------------------------------*/
// types
/*------------------------------------------------------------------------------------------------*/

namespace protocol {

enum class Command : uint32_t {
    Unrecognised,
    FrameError,

    Poll,

    SetPaperIn,
    SetPaperOut,

    SetPlatenIn,
    SetPlatenOut,

    RecordingStart,
    RecordingStop,
};

enum class Response : uint32_t {
    Acknowledge,

    MotorAdvance,
    MotorReverse,
    BurnLine,
};

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

namespace protocol {

auto process_byte(uint8_t byte) -> std::optional<Command>;

auto send_response(Response response, std::optional<const std::span<const uint8_t>> data) -> void;

}

/*------------------------------------------------------------------------------------------------*/
