////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    mech.cpp
/// @brief   Module for handling the print mechanism emulator.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstdint>

#include "xllfifo.h"
#include "xparameters.h"

#include "interrupt.hpp"
#include "mech.hpp"

/*------------------------------------------------------------------------------------------------*/

namespace {

constexpr uint32_t BURN_BUFFER_ID = XPAR_BURN_BUFFER_DEVICE_ID;

XLlFifo burn_buffer;

auto action_buffer = std::array<volatile mech::Action, 1024>{};
volatile uint32_t action_buffer_in_ptr = 0;
volatile uint32_t action_buffer_out_ptr = 0;
volatile uint32_t action_buffer_count = 0;

void motor_advance_isr(void* CallbackRef);
void motor_reverse_isr(void* CallbackRef);
void head_active_start_isr(void* CallbackRef);
void head_active_end_isr(void* CallbackRef);

}

/*------------------------------------------------------------------------------------------------*/

auto mech::init() -> void {

    XLlFifo_Config* fifo_config = XLlFfio_LookupConfig(BURN_BUFFER_ID);
    XLlFifo_CfgInitialize(&burn_buffer, fifo_config, fifo_config->BaseAddress);

    XLlFifo_Status(&burn_buffer);
    XLlFifo_IntClear(&burn_buffer, 0xFFFFFFFF);
    XLlFifo_Status(&burn_buffer);

    interrupt::enable(interrupt::MotorAdvance, (XInterruptHandler)motor_advance_isr, nullptr);
    interrupt::enable(interrupt::MotorReverse, (XInterruptHandler)motor_reverse_isr, nullptr);
    interrupt::enable(interrupt::HeadActiveStart,
                      (XInterruptHandler)head_active_start_isr,
                      nullptr);
    interrupt::enable(interrupt::HeadActiveEnd, (XInterruptHandler)head_active_end_isr, nullptr);
}

/*------------------------------------------------------------------------------------------------*/

auto mech::clear() -> void {
    const uint32_t words = XLlFifo_iRxGetLen(&burn_buffer);
    if(words < HEAD_WORDS) {
        return;
    }

    while(XLlFifo_iRxGetLen(&burn_buffer) > 0) {
        XLlFifo_RxGetWord(&burn_buffer);
    }

    action_buffer_in_ptr = 0;
    action_buffer_out_ptr = 0;
    action_buffer_count = 0;
}

/*------------------------------------------------------------------------------------------------*/

auto mech::get_next_action() -> std::optional<Action> {
    if(action_buffer_out_ptr == action_buffer_in_ptr) {
        return std::nullopt;
    }

    const auto next_action = action_buffer[action_buffer_out_ptr];
    action_buffer_out_ptr = (action_buffer_out_ptr + 1) % action_buffer.size();
    action_buffer_count--;

    return next_action;
}

/*------------------------------------------------------------------------------------------------*/

auto mech::read_burn_line() -> std::optional<std::array<uint8_t, HEAD_BYTES>> {
    const uint32_t words = XLlFifo_iRxGetLen(&burn_buffer) / 4;
    if(words < HEAD_WORDS) {
        return std::nullopt;
    }

    std::array<uint8_t, HEAD_BYTES> burn_line{};
    for(uint32_t i = 0; i < HEAD_BYTES; i += 4) {
        const auto word = XLlFifo_RxGetWord(&burn_buffer);

        burn_line[i + 0] = static_cast<uint8_t>((word >> 0) & 0xFF);
        burn_line[i + 1] = static_cast<uint8_t>((word >> 8) & 0xFF);
        burn_line[i + 2] = static_cast<uint8_t>((word >> 16) & 0xFF);
        burn_line[i + 3] = static_cast<uint8_t>((word >> 24) & 0xFF);
    }

    return burn_line;
}

/*------------------------------------------------------------------------------------------------*/

namespace {

void motor_advance_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::MotorAdvance);

    action_buffer[action_buffer_in_ptr] = mech::Action::Advance;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer.size();
    action_buffer_count++;
}

void motor_reverse_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::MotorReverse);

    action_buffer[action_buffer_in_ptr] = mech::Action::Reverse;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer.size();
    action_buffer_count++;
}

void head_active_start_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::HeadActiveStart);

    action_buffer[action_buffer_in_ptr] = mech::Action::BurnLineStart;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer.size();
    action_buffer_count++;
}

void head_active_end_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::HeadActiveEnd);

    action_buffer[action_buffer_in_ptr] = mech::Action::BurnLineStop;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer.size();
    action_buffer_count++;
}

}
