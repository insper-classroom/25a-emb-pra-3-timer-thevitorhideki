/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#define SOUND_SPEED_US 0.0343

const int ECHO_PIN = 2;
const int TRIGGER_PIN = 3;

volatile bool timer_fired = false;
volatile uint64_t fall_time = 0;
volatile uint64_t rise_time = 0;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;

    // Can return a value here in us to fire in the future
    return 0;
}

void echo_callback(uint gpio, uint32_t events) {
    if (events == GPIO_IRQ_EDGE_FALL) {
        fall_time = to_us_since_boot(get_absolute_time());
    } else if (events == GPIO_IRQ_EDGE_RISE) {
        rise_time = to_us_since_boot(get_absolute_time());
    }
}

int main() {
    stdio_init_all();

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(
        ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &echo_callback);

    alarm_id_t alarm;

    datetime_t t = {
        .year = 2020,
        .month = 01,
        .day = 13,
        .dotw = 3, // 0 is Sunday, so 3 is Wednesday
        .hour = 11,
        .min = 20,
        .sec = 00};

    rtc_init();
    rtc_set_datetime(&t);

    char caracter = getchar();

    while (true) {
        if (caracter == 'Y' || caracter == 254) {
            fall_time = 0;
            rise_time = 0;

            gpio_put(TRIGGER_PIN, 1);
            sleep_ms(5);
            gpio_put(TRIGGER_PIN, 0);

            alarm = add_alarm_in_ms(100, alarm_callback, NULL, false);

            if (!alarm) {
                printf("Failed to add timer\n");
            }

            while (fall_time == 0 && !timer_fired) {
            }

            if (timer_fired) {
                printf("Falha\n");
                cancel_alarm(alarm);
                timer_fired = 0;
            } else {
                float distance = (fall_time - rise_time) * SOUND_SPEED_US / 2;

                datetime_t t = {0};
                rtc_get_datetime(&t);
                char datetime_buf[256];
                char *datetime_str = &datetime_buf[0];
                datetime_to_str(datetime_str, sizeof(datetime_buf), &t);

                printf("%s - %f cm\n", datetime_str, distance);
                cancel_alarm(alarm);
            }

            caracter = getchar_timeout_us(100);
        } else {
            printf("Parado\n");
            caracter = getchar();
        }
    }
}
