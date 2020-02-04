#include <zephyr.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_EE06_LOG_LEVEL
LOG_MODULE_REGISTER(INIT);

#include "init.h"
#include "gpio.h"
#include "spi_config.h"
#include "i2c_config.h"
#include "ads124s08.h"


void init_board() {
    LOG_INF("Initialising board");

	if (NULL == get_GPIO_device())
	{
		LOG_ERR("Unable to initialize GPIO device");
		k_fatal_halt(1);
	}

	if (NULL == get_I2C_device())
	{
		LOG_ERR("Unable to initialize I2C device");
		k_fatal_halt(2);
	}
	if (NULL == get_SPI_device())
	{
		LOG_ERR("Unable to initialize SPI device");
		k_fatal_halt(3);
	}
	ADS124S08_init();
	ADS124S08_begin();
}