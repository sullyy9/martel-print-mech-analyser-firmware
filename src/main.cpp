#include <cstdint>

#include "xgpio.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xintc.h"
#include "xllfifo.h"
#include "xparameters.h"

//////////////////////////////////////////////////

// Get device IDs from xparameters.h
static constexpr uint32_t BUTTON_GPIO_ID{XPAR_AXI_GPIO_BUTTONS_DEVICE_ID};
static constexpr uint32_t LED_GPIO_ID{XPAR_AXI_GPIO_LED_DEVICE_ID};

#define BURN_BUFFER_ID XPAR_BURN_BUFFER_DEVICE_ID

#define BTN_CHANNEL 1
#define LED_CHANNEL 1

#define MOTOR_CHANNEL 1

#define BTN_MASK 0b1111
#define LED_MASK 0b1111

#define INTC_DEVICE_ID XPAR_INTC_0_DEVICE_ID

#define MOTOR_ADV_INT_ID (XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_ADVANCE_TICK_INTR)
#define MOTOR_REV_INT_ID (XPAR_MICROBLAZE_0_AXI_INTC_STEPPER_MOTOR_LINE_REVERSE_TICK_INTR)
#define HEAD_ACTIVE_START_INT_ID \
    (XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_START_TICK_INTR)
#define HEAD_ACTIVE_END_INT_ID (XPAR_MICROBLAZE_0_AXI_INTC_THERMAL_HEAD_HEAD_ACTIVE_END_TICK_INTR)

#define HEAD_WIDTH 384
#define HEAD_BYTES (384 / 8)
#define HEAD_WORDS (HEAD_BYTES / 4)

enum class MechAction : uint8_t {
    Advance,
    Reverse,
    BurnLineStart,
    BurnLineStop,
};

//////////////////////////////////////////////////

static XIntc InterruptController;
static XGpio led_device, btn_device;

static XLlFifo burn_buffer;

static volatile bool motor_advanced = false;
static volatile bool motor_reversed = false;

static constexpr uint32_t action_buffer_size = 1024;
static volatile MechAction action_buffer[action_buffer_size];
static volatile uint32_t action_buffer_in_ptr = 0;
static volatile uint32_t action_buffer_out_ptr = 0;
static volatile uint32_t action_buffer_count = 0;

static MechAction action_next;
static bool action_complete = true;

int SetUpInterruptSystem(XIntc* XIntcInstancePtr, uint8_t id, XInterruptHandler handler);
void motor_advance_isr(void* CallbackRef);
void motor_reverse_isr(void* CallbackRef);
void head_active_start_isr(void* CallbackRef);
void head_active_end_isr(void* CallbackRef);

//////////////////////////////////////////////////

int SetUpInterruptSystem(XIntc* XIntcInstancePtr,
                         const uint8_t interrupt_id,
                         XInterruptHandler handler_callback) {
    int Status;

    Status = XIntc_Connect(XIntcInstancePtr,
                           interrupt_id,
                           (XInterruptHandler)handler_callback,
                           (void*)0);
    if(Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    Status = XIntc_Start(XIntcInstancePtr, XIN_REAL_MODE);
    if(Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XIntc_Enable(XIntcInstancePtr, interrupt_id);
    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XIntc_InterruptHandler,
                                 XIntcInstancePtr);
    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

//////////////////////////////////////////////////

int main() {
    XGpio_Config* cfg_ptr;
    u32 data;

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("Entered function main\r\n");

    // Initialize LED Device
    cfg_ptr = XGpio_LookupConfig(LED_GPIO_ID);
    XGpio_CfgInitialize(&led_device, cfg_ptr, cfg_ptr->BaseAddress);
    XGpio_SetDataDirection(&led_device, LED_CHANNEL, 0x00);

    // Initialize Button Device
    cfg_ptr = XGpio_LookupConfig(BUTTON_GPIO_ID);
    XGpio_CfgInitialize(&btn_device, cfg_ptr, cfg_ptr->BaseAddress);
    XGpio_SetDataDirection(&btn_device, BTN_CHANNEL, BTN_MASK);

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

    if(XIntc_Initialize(&InterruptController, INTC_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("Interrupt init failed\r\n");
    }

    if(XIntc_SelfTest(&InterruptController) != XST_SUCCESS) {
        xil_printf("Interrupt self-test failed\r\n");
    }

    //////////////////////////////////////////////////
    // Setup interrupts.
    if(SetUpInterruptSystem(&InterruptController,
                            MOTOR_ADV_INT_ID,
                            (XInterruptHandler)motor_advance_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&InterruptController,
                            MOTOR_REV_INT_ID,
                            (XInterruptHandler)motor_reverse_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&InterruptController,
                            HEAD_ACTIVE_START_INT_ID,
                            (XInterruptHandler)head_active_start_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    if(SetUpInterruptSystem(&InterruptController,
                            HEAD_ACTIVE_END_INT_ID,
                            (XInterruptHandler)head_active_end_isr)
       != XST_SUCCESS) {
        xil_printf("Interrupt setup failed\r\n");
    }

    //////////

    xil_printf("Start main loop :)\r\n");
    while(true) {
        data = XGpio_DiscreteRead(&btn_device, BTN_CHANNEL);
        data &= BTN_MASK;
        if(data != 0) {
            data = LED_MASK;
        } else {
            data = 0;
        }
        XGpio_DiscreteWrite(&led_device, LED_CHANNEL, data);

        //////////////////////////////////////////////////
        // Motor advance / reverse.
        if(action_buffer_count > action_buffer_size) {
            xil_printf("!!!!overflow!!!!\r\n");
        }

        if(action_complete) {
            if(action_buffer_out_ptr == action_buffer_in_ptr) {
                continue;
            }

            action_next = action_buffer[action_buffer_out_ptr];
            action_buffer_out_ptr = (action_buffer_out_ptr + 1) % action_buffer_size;
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
                    outbyte(burn_line >> 0 & 0xFF);
                    outbyte(burn_line >> 8 & 0xFF);
                    outbyte(burn_line >> 16 & 0xFF);
                    outbyte(burn_line >> 24 & 0xFF);
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

    XIntc_Acknowledge(&InterruptController, MOTOR_ADV_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::Advance;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer_size;
    action_buffer_count++;

    motor_advanced = true;
}

void motor_reverse_isr([[maybe_unused]] void* CallbackRef) {

    XIntc_Acknowledge(&InterruptController, MOTOR_REV_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::Reverse;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer_size;
    action_buffer_count++;

    motor_reversed = true;
}

void head_active_start_isr([[maybe_unused]] void* CallbackRef) {
    XIntc_Acknowledge(&InterruptController, HEAD_ACTIVE_START_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStart;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer_size;
    action_buffer_count++;
}

void head_active_end_isr([[maybe_unused]] void* CallbackRef) {
    XIntc_Acknowledge(&InterruptController, HEAD_ACTIVE_END_INT_ID);

    action_buffer[action_buffer_in_ptr] = MechAction::BurnLineStop;
    action_buffer_in_ptr = (action_buffer_in_ptr + 1) % action_buffer_size;
    action_buffer_count++;
}
