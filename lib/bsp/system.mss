
 PARAMETER VERSION = 2.2.0


BEGIN OS
 PARAMETER OS_NAME = standalone
 PARAMETER OS_VER = 8.0
 PARAMETER PROC_INSTANCE = microblaze_0
 PARAMETER stdin = axi_uartlite_0
 PARAMETER stdout = axi_uartlite_0
END


BEGIN PROCESSOR
 PARAMETER DRIVER_NAME = cpu
 PARAMETER DRIVER_VER = 2.16
 PARAMETER HW_INSTANCE = microblaze_0
END


BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_gpio_buttons
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_gpio_led
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_gpio_paper_sensor_control
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_gpio_pins
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_gpio_platen_sensor_control
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = spi
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_quad_spi_flash
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = spi
 PARAMETER DRIVER_VER = 4.9
 PARAMETER HW_INSTANCE = axi_spi_thermistor
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 3.7
 PARAMETER HW_INSTANCE = axi_uartlite_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = llfifo
 PARAMETER DRIVER_VER = 5.5
 PARAMETER HW_INSTANCE = burn_buffer
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = intc
 PARAMETER DRIVER_VER = 3.15
 PARAMETER HW_INSTANCE = microblaze_0_axi_intc
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 4.8
 PARAMETER HW_INSTANCE = microblaze_0_local_memory_dlmb_bram_if_cntlr
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 4.8
 PARAMETER HW_INSTANCE = microblaze_0_local_memory_ilmb_bram_if_cntlr
END


