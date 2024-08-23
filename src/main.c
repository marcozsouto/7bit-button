/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS 30
#define DEBOUNCE_DELAY_MS 50

/* The devicetree node identifier for the "led0" alias. */

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

volatile uint32_t button_press_count = 0;
struct gpio_callback button_cb_data;
static struct k_timer debounce_timer;
static bool debounce_active = false;


static const struct gpio_dt_spec led_a = GPIO_DT_SPEC_GET(DT_ALIAS(leda), gpios);
static const struct gpio_dt_spec led_b = GPIO_DT_SPEC_GET(DT_ALIAS(ledb), gpios);
static const struct gpio_dt_spec led_c = GPIO_DT_SPEC_GET(DT_ALIAS(ledc), gpios);
static const struct gpio_dt_spec led_d = GPIO_DT_SPEC_GET(DT_ALIAS(ledd), gpios);
static const struct gpio_dt_spec led_e = GPIO_DT_SPEC_GET(DT_ALIAS(lede), gpios);
static const struct gpio_dt_spec led_f = GPIO_DT_SPEC_GET(DT_ALIAS(ledf), gpios);
static const struct gpio_dt_spec led_g = GPIO_DT_SPEC_GET(DT_ALIAS(ledg), gpios);

uint8_t segment_map[] = {
//    abcdefg
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b01110111, // A
    0b01111100, // b
    0b00111001, // C
    0b01011110, // d
    0b01111001, // E
    0b01110001  // F
};

int set_led(const struct gpio_dt_spec* pin) {
	int ret = gpio_pin_configure_dt(pin, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printf("error led config\n");
		return 0;
	}

	return 1;
}

void button_pressed_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (!debounce_active) {  // Only proceed if debounce isn't active
        debounce_active = true;
        k_timer_start(&debounce_timer, K_MSEC(DEBOUNCE_DELAY_MS), K_NO_WAIT);
    }
}
void debounce_timer_handler(struct k_timer *timer_id)
{
    if (gpio_pin_get_dt(&button) > 0) {
		if (button_press_count >= 15) {
			button_press_count = 0;
		} else {
			button_press_count++;
		}

		printf("Button pressed %d times\n", button_press_count);
    }
    debounce_active = false;
}


void display_character(uint8_t character) {
	gpio_pin_set_dt(&led_a, character & 0b00000001);
    gpio_pin_set_dt(&led_b, character & 0b00000010);
    gpio_pin_set_dt(&led_c, character & 0b00000100);
    gpio_pin_set_dt(&led_d, character & 0b00001000);
    gpio_pin_set_dt(&led_e, character & 0b00010000);
    gpio_pin_set_dt(&led_f, character & 0b00100000);
    gpio_pin_set_dt(&led_g, character & 0b01000000);
}

int main(void)
{
    int ret;

    if (!device_is_ready(button.port)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

	set_led(&led_a);
	set_led(&led_b);
	set_led(&led_c);
	set_led(&led_d);
	set_led(&led_e);
	set_led(&led_f);
	set_led(&led_g);

	gpio_init_callback(&button_cb_data, button_pressed_callback, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

	k_timer_init(&debounce_timer, debounce_timer_handler, NULL);

	while (1) {
		display_character(segment_map[button_press_count]);

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
