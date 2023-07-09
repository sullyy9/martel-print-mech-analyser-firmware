#include <cstdint>

#include "xgpio.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xintc.h"
#include "xllfifo.h"
#include "xparameters.h"

//////////////////////////////////////////////////

enum class MechAction : uint8_t {
    Advance,
    Reverse,
    BurnLineStart,
    BurnLineStop,
};

namespace {

// Get device IDs from xparameters.h
constexpr uint32_t BUTTON_GPIO_ID = XPAR_AXI_GPIO_BUTTONS_DEVICE_ID;
constexpr uint32_t LED_GPIO_ID = XPAR_AXI_GPIO_LED_DEVICE_ID;

constexpr uint32_t BURN_BUFFER_ID = XPAR_BURN_BUFFER_DEVICE_ID;

constexpr uint32_t BTN_CHANNEL = 1;
constexpr uint32_t LED_CHANNEL = 1;

constexpr uint32_t BUTTON_MASK = 0b1;
constexpr uint32_t LED_MASK = 0b11;

constexpr uint16_t INTC_DEVICE_ID = XPAR_INTC_0_DEVICE_ID;

constexpr uint8_t MOTOR_ADV_INT_ID =
    XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_ADVANCE_TICK_INTR;
constexpr uint8_t MOTOR_REV_INT_ID =
    XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_REVERSE_TICK_INTR;

constexpr uint8_t HEAD_ACTIVE_START_INT_ID =
    XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_START_TICK_INTR;

constexpr uint8_t HEAD_ACTIVE_END_INT_ID =
    XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_END_TICK_INTR;

constexpr uint32_t HEAD_WIDTH = 384;
constexpr uint32_t HEAD_BYTES = (HEAD_WIDTH / 8);
constexpr uint32_t HEAD_WORDS = (HEAD_BYTES / 4);

}

//////////////////////////////////////////////////

namespace {

XIntc interrupt_controller;
XGpio led_device, btn_device;

XLlFifo burn_buffer;

constexpr uint32_t ACTION_BUFFER_SIZE = 1024;
volatile MechAction action_buffer[ACTION_BUFFER_SIZE];
volatile uint32_t action_buffer_in_ptr = 0;
volatile uint32_t action_buffer_out_ptr = 0;
volatile uint32_t action_buffer_count = 0;

MechAction action_next;
bool action_complete = true;

}

auto SetUpInterruptSystem(XIntc* controller, uint8_t interrupt_id, XInterruptHandler callback)
    -> int32_t;
void motor_advance_isr(void* CallbackRef);
void motor_reverse_isr(void* CallbackRef);
void head_active_start_isr(void* CallbackRef);
void head_active_end_isr(void* CallbackRef);

//////////////////////////////////////////////////

auto SetUpInterruptSystem(XIntc* controller, const uint8_t interrupt_id, XInterruptHandler callback)
    -> int32_t {

    if(XIntc_Connect(controller, interrupt_id, (XInterruptHandler)callback, nullptr)
       != XST_SUCCESS) {
        return XST_FAILURE;
    }

    if(XIntc_Start(controller, XIN_REAL_MODE) != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XIntc_Enable(controller, interrupt_id);
    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XIntc_InterruptHandler,
                                 controller);
    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

//////////////////////////////////////////////////

auto main() -> int {

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("Entered function main\r\n");

    // Initialize LED Device
    XGpio_Config* cfg_ptr = nullptr;
    cfg_ptr = XGpio_LookupConfig(LED_GPIO_ID);
    XGpio_CfgInitialize(&led_device, cfg_ptr, cfg_ptr->BaseAddress);
    XGpio_SetDataDirection(&led_device, LED_CHANNEL, 0x00);

    // Initialize Button Device
    cfg_ptr = XGpio_LookupConfig(BUTTON_GPIO_ID);
    XGpio_CfgInitialize(&btn_device, cfg_ptr, cfg_ptr->BaseAddress);
    XGpio_SetDataDirection(&btn_device, BTN_CHANNEL, BUTTON_MASK);

    //////////////////////////////////////////////////
    // Setup burn buffer FIFO.
    XLlFifo_Config* fifo_config = XLlFfio_LookupConfig(BURN_BUFFER_ID);
    XLlFifo_CfgInitialize(&burn_buffer, fifo_config, fifo_config->BaseAddress);

    XLlFifo_Status(&burn_buffer);
    XLlFifo_IntClear(&burn_buffer, 0xFFFFFFFF);
    XLlFifo_Status(&burn_buffer);

    //////////////////////////////////////////////////
    // Setup interrupt controller.
    xil_printf("Setup interrupts O_O\r\n");

    if(XIntc_Initialize(&interrupt_controller, INTC_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("Interrupt init failed\r\n");
    }

    if(XIntc_SelfTest(&interrupt_controller) != XST_SUCCESS) {
        xil_printf("Interrupt self-test failed\r\n");
    }

    //////////////////////////////////////////////////
    // Setup interrupts.
    if(SetUpInterruptSystem(&interrupt_controller,
                            MOTOR_ADV_INT_ID,
                            (XInterruptHandler)motor_advance_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&interrupt_controller,
                            MOTOR_REV_INT_ID,
                            (XInterruptHandler)motor_reverse_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&interrupt_controller,
                            HEAD_ACTIVE_START_INT_ID,
                            (XInterruptHandler)head_active_start_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&interrupt_controller,
                            HEAD_ACTIVE_END_INT_ID,
                            (XInterruptHandler)head_active_end_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    //////////

    xil_printf("Start main loop :)\r\n");
    while(true) {
        uint32_t data = XGpio_DiscreteRead(&btn_device, BTN_CHANNEL);
        data &= BUTTON_MASK;
        if(data != 0) {
            data = LED_MASK;
        } else {
            data = 0;
        }
        XGpio_DiscreteWrite(&led_device, LED_CHANNEL, data);

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

    XIntc_Acknowledge(&interrupt_controller, MOTOR_ADV_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::Advance;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void motor_reverse_isr([[maybe_unused]] void* CallbackRef) {

    XIntc_Acknowledge(&interrupt_controller, MOTOR_REV_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::Reverse;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void head_active_start_isr([[maybe_unused]] void* CallbackRef) {
    XIntc_Acknowledge(&interrupt_controller, HEAD_ACTIVE_START_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStart;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}

void head_active_end_isr([[maybe_unused]] void* CallbackRef) {
    XIntc_Acknowledge(&interrupt_controller, HEAD_ACTIVE_END_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStop;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % ACTION_BUFFER_SIZE;
    action_buffer_count++;
}
