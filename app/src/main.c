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
LV_FONT_DECLARE(monocraft);

static uint32_t count;
static uint8_t palette = LV_PALETTE_RED;

lv_obj_t *temp_label;
lv_obj_t *humi_label;

static const struct gpio_dt_spec display_led = GPIO_DT_SPEC_GET(DISPLAY_LED_NODE, gpios);

#define BUTTON0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(BUTTON0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	if(palette<LV_PALETTE_GREY) {
		palette++;
	} else {
		palette = LV_PALETTE_RED;
	}
	lv_obj_set_style_text_color(temp_label, lv_palette_main(palette), 0);
	lv_obj_set_style_text_color(humi_label, lv_palette_main(palette), 0);
}

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
	
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	char count_str[10] = {0};
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	temp_label = lv_label_create(lv_scr_act());
	lv_obj_align(temp_label, LV_ALIGN_TOP_MID, 0, 10);
	humi_label = lv_label_create(lv_scr_act());
	lv_obj_align(humi_label, LV_ALIGN_BOTTOM_MID, 0, -10);
	lv_obj_set_style_text_font(temp_label, &monocraft, 0);
	lv_obj_set_style_text_font(humi_label, &monocraft, 0);
	lv_obj_set_style_text_color(temp_label, lv_palette_main(palette), 0);
	lv_obj_set_style_text_color(humi_label, lv_palette_main(palette), 0);
	lv_disp_set_bg_color(NULL, lv_color_make(0x40,0x40,0x40));

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
			sprintf(count_str, "%2.2f°C", sensor_value_to_double(&temperature));
			lv_label_set_text(temp_label, count_str);
			sprintf(count_str, "%2.2f%%", sensor_value_to_double(&humidity));
			lv_label_set_text(humi_label, count_str);
		}
		lv_task_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}