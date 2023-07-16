////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    protocol.hpp
/// @brief   Module for enccoding and decoding data.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <optional>

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

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

namespace protocol {

auto process_byte(uint8_t byte) -> std::optional<Command>;

}

/*------------------------------------------------------------------------------------------------*/
