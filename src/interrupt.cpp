////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    interrupt.cpp
/// @brief   Interrupt module.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string_view>

#include "xintc.h"

#include "interrupt.hpp"

/*------------------------------------------------------------------------------------------------*/

namespace {

constexpr uint16_t CONTROLLER_DEVICE_ID = XPAR_INTC_0_DEVICE_ID;
constinit XIntc controller{};

}

/*------------------------------------------------------------------------------------------------*/

auto interrupt::init() -> Status {
    if(XIntc_Initialize(&controller, CONTROLLER_DEVICE_ID) != XST_SUCCESS) {
        return Status::InitFailure;
    }

    if(XIntc_SelfTest(&controller) != XST_SUCCESS) {
        return Status::SelfTestFailure;
    }

    return Status::Ok;
}

/*------------------------------------------------------------------------------------------------*/

auto interrupt::enable(Interrupt interrupt, XInterruptHandler callback, void* callback_ref) -> Status {

    if(XIntc_Connect(&controller, interrupt, (XInterruptHandler)callback, callback_ref) != XST_SUCCESS) {
        return Status::InitFailure;
    }

    if(XIntc_Start(&controller, XIN_REAL_MODE) != XST_SUCCESS) {
        return Status::InitFailure;
    }

    XIntc_Enable(&controller, interrupt);
    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XIntc_InterruptHandler,
                                 &controller);
    Xil_ExceptionEnable();

    return Status::Ok;
}

/*------------------------------------------------------------------------------------------------*/

auto interrupt::acknowledge(Interrupt interrupt) -> void {
    XIntc_Acknowledge(&controller, interrupt);
}

/*------------------------------------------------------------------------------------------------*/
// Error handling.
/*------------------------------------------------------------------------------------------------*/

auto interrupt::status_message(Status status) -> std::string_view {
    using enum interrupt::Status;

    switch(status) {
        case Ok: return "Ok";
        case InitFailure: return "Init failed";
        case SelfTestFailure: return "Self test failed";

        default: return "Unknown";
    }
}

/*------------------------------------------------------------------------------------------------*/
