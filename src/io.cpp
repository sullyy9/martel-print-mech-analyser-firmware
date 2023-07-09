#include <cstdint>

#include "xgpio.h"
#include "xparameters.h"

#include "io.hpp"

//////////////////////////////////////////////////

namespace {

enum class Direction : uint8_t { Input, Output };

struct GPIOConfig {
    uint16_t id;
    uint8_t channel;
    Direction direction;
    uint32_t mask;
};

constexpr auto MONOLED_1_CONFIG = GPIOConfig{
    .id = XPAR_AXI_GPIO_LED_DEVICE_ID,
    .channel = 1,
    .direction = Direction::Output,
    .mask = 0b01,
};

constexpr auto MONOLED_2_CONFIG = GPIOConfig{
    .id = XPAR_AXI_GPIO_LED_DEVICE_ID,
    .channel = 1,
    .direction = Direction::Output,
    .mask = 0b10,
};

constexpr auto BUTTON_CONFIG = GPIOConfig{
    .id = XPAR_AXI_GPIO_BUTTONS_DEVICE_ID,
    .channel = 1,
    .direction = Direction::Input,
    .mask = 0b1,
};

XGpio monoled_1 = {};
XGpio monoled_2 = {};
XGpio button = {};

auto gpio_init(const GPIOConfig& config) -> XGpio;

auto gpio_set_high(XGpio instance, const GPIOConfig& config) -> void;
auto gpio_set_low(XGpio instance, const GPIOConfig& config) -> void;

auto gpio_is_high(XGpio instance, const GPIOConfig& config) -> bool;
auto gpio_is_low(XGpio instance, const GPIOConfig& config) -> bool;

}

//////////////////////////////////////////////////

auto io::init() -> void {
    monoled_1 = gpio_init(MONOLED_1_CONFIG);
    monoled_2 = gpio_init(MONOLED_2_CONFIG);
    button = gpio_init(BUTTON_CONFIG);
}

auto io::monoled_1_on() -> void {
    gpio_set_high(monoled_1, MONOLED_1_CONFIG);
}

auto io::monoled_2_on() -> void {
    gpio_set_high(monoled_2, MONOLED_2_CONFIG);
}

auto io::monoled_1_off() -> void {
    gpio_set_low(monoled_1, MONOLED_1_CONFIG);
}

auto io::monoled_2_off() -> void {
    gpio_set_low(monoled_2, MONOLED_2_CONFIG);
}

auto io::button_is_pressed() -> bool {
    return gpio_is_high(button, BUTTON_CONFIG);
}

//////////////////////////////////////////////////

namespace {

auto gpio_init(const GPIOConfig& config) -> XGpio {
    XGpio_Config* xgpio_config = XGpio_LookupConfig(config.id);

    XGpio instance = {};
    XGpio_CfgInitialize(&instance, xgpio_config, xgpio_config->BaseAddress);

    const auto value = XGpio_GetDataDirection(&instance, config.channel);
    if(config.direction == Direction::Output) {
        XGpio_SetDataDirection(&instance, config.channel, value & ~config.mask);
    } else {
        XGpio_SetDataDirection(&instance, config.channel, value | config.mask);
    }

    return instance;
}

auto gpio_set_high(XGpio instance, const GPIOConfig& config) -> void {
    if(config.direction != Direction::Output) {
        return;
    }
    XGpio_DiscreteSet(&instance, config.channel, config.mask);
}

auto gpio_set_low(XGpio instance, const GPIOConfig& config) -> void {
    if(config.direction != Direction::Output) {
        return;
    }
    XGpio_DiscreteClear(&instance, config.channel, config.mask);
}

auto gpio_is_high(XGpio instance, const GPIOConfig& config) -> bool {
    return XGpio_DiscreteRead(&instance, config.channel) != 0;
}

auto gpio_is_low(XGpio instance, const GPIOConfig& config) -> bool {
    return XGpio_DiscreteRead(&instance, config.channel) == 0;
}

}