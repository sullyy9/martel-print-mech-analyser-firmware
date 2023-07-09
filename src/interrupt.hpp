////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    interrupt.hpp
/// @brief   Interrupt module.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <string_view>

#include "xintc.h"

/*------------------------------------------------------------------------------------------------*/
// error handling.
/*------------------------------------------------------------------------------------------------*/

namespace interrupt {

enum class Status : uint32_t {
    Ok,
    InitFailure,
    SelfTestFailure,

};

}

/*------------------------------------------------------------------------------------------------*/
// public types.
/*------------------------------------------------------------------------------------------------*/

namespace interrupt {

enum Interrupt : uint8_t {
    MotorAdvance = XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_ADVANCE_TICK_INTR,
    MotorReverse = XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_REVERSE_TICK_INTR,
    HeadActiveStart = XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_START_TICK_INTR,
    HeadActiveEnd = XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_END_TICK_INTR,

};

using enum Interrupt;

}

/*------------------------------------------------------------------------------------------------*/
// module public function definitions.
/*------------------------------------------------------------------------------------------------*/

namespace interrupt {

auto init() -> Status;

auto enable(Interrupt interrupt, XInterruptHandler callback) -> Status;

auto acknowledge(Interrupt interrupt) -> void;

auto status_message(Status status) -> std::string_view;

}

/*------------------------------------------------------------------------------------------------*/
