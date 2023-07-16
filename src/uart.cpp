////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    uart.hpp
/// @brief   Module for handling reading and writing from/to the uart peripheral.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>

#include "xparameters.h"
#include "xuartlite.h"

#include "interrupt.hpp"
#include "uart.hpp"

/*------------------------------------------------------------------------------------------------*/
// private variables
/*------------------------------------------------------------------------------------------------*/

namespace {

constexpr uint16_t DEVICE_ID = XPAR_UARTLITE_0_DEVICE_ID;
XUartLite uart_instance;

auto tx_buffer = std::array<volatile uint8_t, 1024>{};
volatile uint32_t tx_buffer_in_ptr = 0;
volatile uint32_t tx_buffer_out_ptr = 0;
volatile uint32_t tx_buffer_count = 0;

auto rx_buffer = std::array<volatile uint8_t, 1024>{};
volatile uint32_t rx_buffer_in_ptr = 0;
volatile uint32_t rx_buffer_out_ptr = 0;
volatile uint32_t rx_buffer_count = 0;

}

/*------------------------------------------------------------------------------------------------*/
// private functions
/*------------------------------------------------------------------------------------------------*/

namespace {

auto receive_isr(XUartLite* instance, uint32_t bytes) -> void;
auto transmit_isr(XUartLite* instance, uint32_t bytes) -> void;

auto start_transmission() -> void;

}

/*------------------------------------------------------------------------------------------------*/
// public functions
/*------------------------------------------------------------------------------------------------*/

auto uart::init() -> std::optional<Error> {
    XUartLite_Config* config = XUartLite_LookupConfig(DEVICE_ID);

    if(XUartLite_CfgInitialize(&uart_instance, config, config->RegBaseAddr) != XST_SUCCESS) {
        return Error::InitFailure;
    }

    interrupt::enable(interrupt::Interrupt::Uart,
                      (XInterruptHandler)XUartLite_InterruptHandler,
                      &uart_instance);

    XUartLite_SetSendHandler(&uart_instance, (XUartLite_Handler)transmit_isr, &uart_instance);
    XUartLite_SetRecvHandler(&uart_instance, (XUartLite_Handler)receive_isr, &uart_instance);

    XUartLite_EnableInterrupt(&uart_instance);

    // Setup next RX request.
    XUartLite_Recv(&uart_instance, (uint8_t*)&rx_buffer[rx_buffer_in_ptr], 1);

    return std::nullopt;
}

/*------------------------------------------------------------------------------------------------*/

auto uart::write(const uint8_t byte) -> void {
    tx_buffer[tx_buffer_in_ptr] = byte;
    tx_buffer_in_ptr = (tx_buffer_in_ptr + 1) % tx_buffer.size();
    tx_buffer_count++;

    if(XUartLite_IsSending(&uart_instance) == 0) {
        start_transmission();
    }
}

/*------------------------------------------------------------------------------------------------*/

auto uart::write(std::span<const uint8_t> data) -> void {
    for(const auto byte : data) {
        tx_buffer[tx_buffer_in_ptr] = byte;
        tx_buffer_in_ptr = (tx_buffer_in_ptr + 1) % tx_buffer.size();
        tx_buffer_count++;
    }

    if(XUartLite_IsSending(&uart_instance) == 0) {
        start_transmission();
    }
}

/*------------------------------------------------------------------------------------------------*/

auto uart::write(std::span<const char> data) -> void {
    for(const auto byte : data) {
        tx_buffer[tx_buffer_in_ptr] = byte;
        tx_buffer_in_ptr = (tx_buffer_in_ptr + 1) % tx_buffer.size();
        tx_buffer_count++;
    }

    if(XUartLite_IsSending(&uart_instance) == 0) {
        start_transmission();
    }
}

/*------------------------------------------------------------------------------------------------*/

auto uart::read() -> uint8_t {
    const auto byte = rx_buffer[rx_buffer_out_ptr];
    rx_buffer_out_ptr = (rx_buffer_out_ptr + 1) % rx_buffer.size();
    rx_buffer_count--;

    return byte;
}

/*------------------------------------------------------------------------------------------------*/

auto uart::free() -> uint32_t {
    return tx_buffer.size() - tx_buffer_count;
}

/*------------------------------------------------------------------------------------------------*/

auto uart::received() -> uint32_t {
    return rx_buffer_count;
}

/*------------------------------------------------------------------------------------------------*/

auto uart::error_message(Error error) -> std::string_view {
    using enum Error;

    switch(error) {
        case None: return "None";
        case InitFailure: return "Init failure";
        default: return "Unknown";
    }
}

/*------------------------------------------------------------------------------------------------*/
// private functions
/*------------------------------------------------------------------------------------------------*/

namespace {

auto receive_isr([[maybe_unused]] XUartLite* instance, [[maybe_unused]] uint32_t bytes) -> void {
    interrupt::acknowledge(interrupt::Interrupt::Uart);

    // const uint32_t bytes_until_overflow = rx_buffer.size() - rx_buffer_in_ptr;
    // if(bytes > bytes_until_overflow) {
    //     XUartLite_Recv(&uart_instance,
    //                    (uint8_t*)&rx_buffer[rx_buffer_in_ptr],
    //                    bytes_until_overflow);

    //     rx_buffer_in_ptr = (rx_buffer_in_ptr + bytes_until_overflow) % rx_buffer.size();
    //     rx_buffer_count += bytes_until_overflow;
    //     bytes -= bytes_until_overflow;
    // }

    // XUartLite_Recv(&uart_instance, (uint8_t*)&rx_buffer[rx_buffer_in_ptr], bytes);
    // rx_buffer_in_ptr = (rx_buffer_in_ptr + bytes) % rx_buffer.size();
    // rx_buffer_count += bytes;

    rx_buffer_in_ptr = (rx_buffer_in_ptr + 1) % rx_buffer.size();
    rx_buffer_count++;

    // Setup next RX request.
    XUartLite_Recv(&uart_instance, (uint8_t*)&rx_buffer[rx_buffer_in_ptr], 1);
}

/*------------------------------------------------------------------------------------------------*/

auto transmit_isr([[maybe_unused]] XUartLite* instance, uint32_t bytes) -> void {
    interrupt::acknowledge(interrupt::Interrupt::Uart);

    tx_buffer_out_ptr = (tx_buffer_out_ptr + bytes) % tx_buffer.size();
    tx_buffer_count -= bytes;

    if(tx_buffer_count != 0) {
        start_transmission();
    }
}

/*------------------------------------------------------------------------------------------------*/

auto start_transmission() -> void {
    auto bytes = tx_buffer_count;

    if(bytes > (tx_buffer.size() - tx_buffer_out_ptr)) {
        bytes = (tx_buffer.size() - tx_buffer_out_ptr);
    }

    XUartLite_Send(&uart_instance, (uint8_t*)&tx_buffer[tx_buffer_out_ptr], bytes);
}

}

/*------------------------------------------------------------------------------------------------*/
