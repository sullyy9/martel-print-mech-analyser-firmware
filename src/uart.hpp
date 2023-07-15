////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    uart.hpp
/// @brief   Module for handling reading and writing from/to the uart peripheral.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

/*------------------------------------------------------------------------------------------------*/
// Error handling.
/*------------------------------------------------------------------------------------------------*/

namespace uart {

enum class Error : uint32_t {
    None,
    InitFailure,

};

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

namespace uart {

auto init() -> std::optional<Error>;

auto write(uint8_t byte) -> void;
auto write(std::span<const uint8_t> data) -> void;
auto write(std::span<const char> data) -> void;

auto free() -> uint32_t;

auto error_message(Error error) -> std::string_view;

}

/*------------------------------------------------------------------------------------------------*/
