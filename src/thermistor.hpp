////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    thermistor.hpp
/// @brief   Module for reading and writing the digital potentiometer acting as the 
///          thermistor.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

/*------------------------------------------------------------------------------------------------*/

namespace thermistor {

auto init() -> void;

auto set_temp(int32_t temp) -> void;

}

/*------------------------------------------------------------------------------------------------*/
