#include <cstdint>

#include "xil_printf.h"
#include "xil_types.h"
#include "xllfifo.h"
#include "xparameters.h"

#include "interrupt.hpp"
#include "io.hpp"


//////////////////////////////////////////////////

enum class MechAction : uint8_t {
    Advance,
    Reverse,
    BurnLineStart,
    BurnLineStop,
};

namespace {

// Get device IDs from xparameters.h
constexpr uint32_t BURN_BUFFER_ID = XPAR_BURN_BUFFER_DEVICE_ID;

constexpr uint32_t HEAD_WIDTH = 384;
constexpr uint32_t HEAD_BYTES = (HEAD_WIDTH / 8);
constexpr uint32_t HEAD_WORDS = (HEAD_BYTES / 4);

}

//////////////////////////////////////////////////

namespace {

XLlFifo burn_buffer;

constexpr uint32_t ACTION_BUFFER_SIZE = 1024;
volatile MechAction action_buffer[ACTION_BUFFER_SIZE];
volatile uint32_t action_buffer_in_ptr = 0;
volatile uint32_t action_buffer_out_ptr = 0;
volatile uint32_t action_buffer_count = 0;

MechAction action_next;
bool action_complete = true;

}

void motor_advance_isr(void* CallbackRef);
void motor_reverse_isr(void* CallbackRef);
void head_active_start_isr(void* CallbackRef);
void head_active_end_isr(void* CallbackRef);

//////////////////////////////////////////////////

auto main() -> int {

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("Entered function main\r\n");

    //////////////////////////////////////////////////
    // Setup LED and button controllers 
    io::init();

    //////////////////////////////////////////////////
    // Setup burn buffer FIFO.
    XLlFifo_Config* fifo_config = XLlFfio_LookupConfig(BURN_BUFFER_ID);
    XLlFifo_CfgInitialize(&burn_buffer, fifo_config, fifo_config->BaseAddress);

    XLlFifo_Status(&burn_buffer);
    XLlFifo_IntClear(&burn_buffer, 0xFFFFFFFF);
    XLlFifo_Status(&burn_buffer);

    //////////////////////////////////////////////////
    // Setup interrupt controller.
    xil_printf("Setup interrupts - ");
    const auto status = interrupt::init();
    xil_printf("%s\r\n", interrupt::status_message(status).data());

    //////////////////////////////////////////////////
    // Setup interrupts.

    interrupt::enable(interrupt::MotorAdvance, (XInterruptHandler)motor_advance_isr);
    interrupt::enable(interrupt::MotorReverse, (XInterruptHandler)motor_reverse_isr);
    interrupt::enable(interrupt::HeadActiveStart, (XInterruptHandler)head_active_start_isr);
    interrupt::enable(interrupt::HeadActiveEnd, (XInterruptHandler)head_active_end_isr);

    //////////

    xil_printf("Start main loop :)\r\n");
    while(true) {
        if(io::button_is_pressed()) {
            io::monoled_1_on();
            io::monoled_2_on();
        } else {
            io::monoled_1_off();
            io::monoled_2_off();
        }

        //////////////////////////////////////////////////
        // Motor advance / reverse.
        if(action_buffer_count > ACTION_BUFFER_SIZE) {
            xil_printf("!!!!overflow!!!!\r\n");
        }

        if(action_complete) {
            if(action_buffer_out_ptr == action_buffer_in_ptr) {
                continue;
            }

            action_next = action_buffer[action_buffer_out_ptr];
            action_buffer_out_ptr = (action_buffer_out_ptr + 1) % ACTION_BUFFER_SIZE;
            action_buffer_count--;

            action_complete = false;
        }

        if(action_next == MechAction::Advance) {
            print("ADV\r\n");
            action_complete = true;

        } else if(action_next == MechAction::Reverse) {
            print("REV\r\n");
            action_complete = true;

        } else if(action_next == MechAction::BurnLineStart) {
            action_complete = true;

        } else if(action_next == MechAction::BurnLineStop) {

            const uint32_t words = XLlFifo_iRxGetLen(&burn_buffer) / 4;

            if(words > HEAD_WORDS) {
                xil_printf("%u LN:", words);

                for(uint32_t i = 0; i < HEAD_WORDS; i++) {
                    const uint32_t burn_line = XLlFifo_RxGetWord(&burn_buffer);
                    outbyte(static_cast<uint8_t>(burn_line >> 0 & 0xFF));
                    outbyte(static_cast<uint8_t>(burn_line >> 8 & 0xFF));
                    outbyte(static_cast<uint8_t>(burn_line >> 16 & 0xFF));
                    outbyte(static_cast<uint8_t>(burn_line >> 24 & 0xFF));
                }
                xil_printf("\r\n");
                action_complete = true;
            }
        } else {
            xil_printf("Unknown action\r\n");
        }
    }
}

//////////////////////////////////////////////////

void motor_advance_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::MotorAdvance);

    action_buffer[action_buffer_in_ptr] = MechAction::Advance;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void motor_reverse_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::MotorReverse);

    action_buffer[action_buffer_in_ptr] = MechAction::Reverse;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void head_active_start_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::HeadActiveStart);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStart;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void head_active_end_isr([[maybe_unused]] void* CallbackRef) {
    interrupt::acknowledge(interrupt::HeadActiveEnd);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStop;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}
