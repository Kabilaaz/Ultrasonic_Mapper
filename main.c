#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "web_server.h"
#include "nvs_flash.h"

#define TIMEOUT 15000
#define STEP_COUNT 2
#define STEP_PIN GPIO_NUM_4
#define DIR_PIN GPIO_NUM_2  

#define US1_TRIG GPIO_NUM_16
#define US2_TRIG GPIO_NUM_5
#define US3_TRIG GPIO_NUM_19
#define US4_TRIG GPIO_NUM_27
#define US5_TRIG GPIO_NUM_25
#define US6_TRIG GPIO_NUM_32

#define US1_ECHO GPIO_NUM_17
#define US2_ECHO GPIO_NUM_18
#define US3_ECHO GPIO_NUM_21
#define US4_ECHO GPIO_NUM_26
#define US5_ECHO GPIO_NUM_33
#define US6_ECHO GPIO_NUM_35

volatile uint16_t us1[50], us2[50], us3[50], us4[50], us5[50], us6[50];
volatile uint16_t us1Process[50], us2Process[50], us3Process[50], us4Process[50], us5Process[50], us6Process[50];

volatile uint8_t currStep = 0;
volatile uint8_t stepperDir = 0;

volatile uint8_t frontComp = 0;
volatile uint8_t backComp = 0;

QueueHandle_t motorQueue, front, back;
TaskHandle_t frontHandle, backHandle, motorHandle;

uint16_t read_ultrasonic(gpio_num_t trig, gpio_num_t echo) {
    gpio_set_level(trig, 0);
    esp_rom_delay_us(2);
    gpio_set_level(trig, 1);
    esp_rom_delay_us(10);
    gpio_set_level(trig, 0);

    uint32_t waitStart = esp_timer_get_time();
    while(gpio_get_level(echo) == 0) {
        if((esp_timer_get_time() - waitStart) > 2000) return 300; 
    }

    uint32_t echoStart = esp_timer_get_time();
    while(gpio_get_level(echo) == 1) {
        if((esp_timer_get_time() - echoStart) > TIMEOUT) return 300; 
    }

    uint32_t time_us = esp_timer_get_time() - echoStart;
    
    uint16_t distance_cm = (uint16_t)(time_us / 58);
    
    ESP_LOGI("DATA", "%d", distance_cm);

    return distance_cm;
}

void stepMotor(void *pvParameters) {
    uint8_t cmd;
    uint8_t start = 1;
    while (1) {
        if (xQueueReceive(motorQueue, &cmd, portMAX_DELAY)) {
            if (cmd == 1) {
                gpio_set_level(DIR_PIN, stepperDir);

                for(uint16_t i = 0; i < STEP_COUNT; i++) {
                    gpio_set_level(STEP_PIN, 1);
                    esp_rom_delay_us(5000);
                    gpio_set_level(STEP_PIN, 0);
                    esp_rom_delay_us(5000);
                }

                us1Process[currStep] = us1[currStep];
                us2Process[currStep] = us2[currStep];
                us3Process[currStep] = us3[currStep];
                us4Process[currStep] = us4[currStep];
                us5Process[currStep] = us5[currStep];
                us6Process[currStep] = us6[currStep];

                currStep++;
                if (currStep >= 50) {
                    currStep = 0;
                    stepperDir = !stepperDir;
                }
                xQueueSend(front, &start, 0);
                xQueueSend(back, &start, 0);
            }
        }
    }
}

void frontTask(void *pvParameters) {
    uint8_t cmd;
    uint8_t sendMotor = 1;

    while (1) {
        if(xQueueReceive(front, &cmd, portMAX_DELAY)) {
            if(cmd == 1) {
                us1[currStep] = read_ultrasonic(US1_TRIG, US1_ECHO);
                us2[currStep] = read_ultrasonic(US2_TRIG, US2_ECHO);
                us3[currStep] = read_ultrasonic(US3_TRIG, US3_ECHO);

                if (frontComp == 0 && backComp == 1) {
                    backComp = 0;
                    xQueueSend(motorQueue, &sendMotor, 0);
                } else if (backComp == 0) {
                    frontComp = 1;
                }
            }
        }
    }
}

void backTask(void *pvParameters) {
    uint8_t cmd;
    uint8_t sendMotor = 1;

    while (1) {
        if(xQueueReceive(back, &cmd, portMAX_DELAY)) {
            if (cmd == 1) {
                us4[currStep] = read_ultrasonic(US4_TRIG, US4_ECHO);
                us5[currStep] = read_ultrasonic(US5_TRIG, US5_ECHO);
                us6[currStep] = read_ultrasonic(US6_TRIG, US6_ECHO);

                if (frontComp == 1 && backComp == 0) {
                    frontComp = 0;
                    xQueueSend(motorQueue, &sendMotor, 0);
                } else if (frontComp == 0) {
                    backComp = 1;
                }
            }
        }
    }
}

void configuration(void) {
    static const char *TAG_CONF = "CONFIGURATION";
    uint8_t start = 1;

    gpio_config_t pinConfig = {
        .pin_bit_mask = (1ULL << STEP_PIN) | (1ULL << DIR_PIN) | (1ULL << US1_TRIG) | (1ULL << US2_TRIG) | (1ULL << US3_TRIG) | (1ULL << US4_TRIG) | (1ULL << US5_TRIG) | (1ULL << US6_TRIG),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&pinConfig));

    gpio_config_t echoPinConfig = {
        .pin_bit_mask = (1ULL << US1_ECHO) | (1ULL << US2_ECHO) | (1ULL << US3_ECHO) | (1ULL << US4_ECHO) | (1ULL << US5_ECHO) | (1ULL << US6_ECHO),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&echoPinConfig));

    ESP_LOGI(TAG_CONF, "Pin configuration complete");

    xQueueSend(front, &start, 0);
    xQueueSend(back, &start, 0);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_web_server();

    motorQueue = xQueueCreate(10, sizeof(uint8_t));
    front = xQueueCreate(10, sizeof(uint8_t));
    back = xQueueCreate(10, sizeof(uint8_t));

    xTaskCreate(frontTask, "Front Task", 4*1024, NULL, 5, &frontHandle);
    xTaskCreate(backTask, "Back Task", 4*1024, NULL, 5, &backHandle);
    
    xTaskCreate(stepMotor, "Motor Task", 4*1024, NULL, 6, &motorHandle); 

    configuration();
}
