/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define DISPLAY_LED_NODE DT_ALIAS(backlight0)

static const struct gpio_dt_spec display_led = GPIO_DT_SPEC_GET(DISPLAY_LED_NODE, gpios);

static uint32_t count;

int main(void)
{
	if (!gpio_is_ready_dt(&display_led)) {
		return 0;
	}
	int ret = gpio_pin_configure_dt(&display_led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
	ret = gpio_pin_set_dt(&display_led, 1);
		if (ret < 0) {
			return 0;
		}

	char count_str[70] = {0};
	const struct device *display_dev;
	lv_obj_t *aht30_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	aht30_label = lv_label_create(lv_scr_act());
	lv_obj_align(aht30_label, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	const struct device *const aht30 = DEVICE_DT_GET_ONE(aosong_aht20);

	if (!device_is_ready(aht30)) {
		printf("Device %s is not ready\n", aht30->name);
		return 0;
	}

	while (1) {
		if ((count % 100) == 0U) {
			int rc = sensor_sample_fetch(aht30);

			if (rc != 0) {
				printf("Sensor fetch failed: %d\n", rc);
				break;
			}

			struct sensor_value temperature;
			struct sensor_value humidity;

			rc = sensor_channel_get(aht30, SENSOR_CHAN_AMBIENT_TEMP,
						&temperature);
			if (rc == 0) {
				rc = sensor_channel_get(aht30, SENSOR_CHAN_HUMIDITY,
							&humidity);
			}
			if (rc != 0) {
				printf("get failed: %d\n", rc);
				break;
			}
			sprintf(count_str, "Temperature:%f \nHumidity:%f%%", sensor_value_to_double(&temperature), sensor_value_to_double(&humidity));
			lv_label_set_text(aht30_label, count_str);
		}
		lv_task_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}