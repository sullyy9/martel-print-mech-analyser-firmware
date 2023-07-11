////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author  Ryan Sullivan (ryansullivan@googlemail.com)
///
/// @file    thermistor.cpp
/// @brief   Module for reading and writing the digital potentiometer acting as the
///          thermistor.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>

#include "xspi.h"

#include "thermistor.hpp"

/*------------------------------------------------------------------------------------------------*/

namespace {

constexpr uint16_t DEVICE_ID = XPAR_AXI_SPI_THERMISTOR_DEVICE_ID;
XSpi spi{};

}

/*------------------------------------------------------------------------------------------------*/

auto thermistor::init() -> void {
    auto* const spi_config = XSpi_LookupConfig(DEVICE_ID);
    XSpi_CfgInitialize(&spi, spi_config, spi_config->BaseAddress);

    XSpi_SetOptions(&spi, XSP_MASTER_OPTION);

    XSpi_Start(&spi);
}

/*------------------------------------------------------------------------------------------------*/

auto thermistor::set_temp(const int32_t temp) -> void {
    uint8_t out = 127;
    XSpi_SetSlaveSelect(&spi, 1);
    XSpi_Transfer(&spi, &out, nullptr, 1);
}

/*------------------------------------------------------------------------------------------------*/

