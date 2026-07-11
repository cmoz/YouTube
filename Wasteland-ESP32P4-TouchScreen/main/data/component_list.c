#include "data/component_list.h"
#include "esp_random.h"
#include <stddef.h>

/* Starter list — swap/extend freely, this is just seed data. */
static const component_entry_t s_components[] = {
    { "ESP32 DevKit",          "a dual-core WiFi & Bluetooth microcontroller",  COMPONENT_CAT_MICROCONTROLLER },
    { "Arduino Nano",          "a compact ATmega328P microcontroller board",    COMPONENT_CAT_MICROCONTROLLER },
    { "HC-SR04 Ultrasonic",    "an ultrasonic distance sensor",                 COMPONENT_CAT_SENSOR },
    { "MFRC522 RFID",          "a 13.56MHz RFID reader",                        COMPONENT_CAT_SENSOR },
    { "DHT22 Sensor",          "a temperature and humidity sensor",             COMPONENT_CAT_SENSOR },
    { "Flex Sensor",           "a bend-sensitive resistive sensor",             COMPONENT_CAT_SENSOR },
    { "WS2812B LED Strip",     "an addressable RGB LED strip",                  COMPONENT_CAT_OUTPUT },
    { "SSD1306 OLED",          "a small monochrome OLED display",               COMPONENT_CAT_OUTPUT },
    { "Vibration Motor",       "a small haptic vibration motor",                COMPONENT_CAT_OUTPUT },
    { "Piezo Buzzer",          "a piezo buzzer",                                COMPONENT_CAT_OUTPUT },
    { "Conductive Thread",     "conductive thread for e-textiles",              COMPONENT_CAT_FABRIC },
    { "Snap Button Connector", "a conductive snap connector",                   COMPONENT_CAT_FABRIC },
    { "Conductive Fabric",     "a conductive woven fabric patch",               COMPONENT_CAT_FABRIC },
    { "LiPo Battery",          "a rechargeable lithium-polymer battery",        COMPONENT_CAT_POWER },
    { "Coin Cell Holder",      "a small coin-cell battery holder",              COMPONENT_CAT_POWER },
    { "Rotary Encoder",        "a rotary encoder with push button",             COMPONENT_CAT_OTHER },
    { "Push Button",           "a momentary tactile push button",               COMPONENT_CAT_OTHER },
    { "10K Potentiometer",     "a rotary variable resistor",                    COMPONENT_CAT_OTHER },
};

static const char *s_category_labels[COMPONENT_CAT_COUNT] = {
    "MC",
    "SENSOR",
    "OUTPUT",
    "FABRIC",
    "POWER",
    "OTHER",
};

static const int s_component_count = sizeof(s_components) / sizeof(s_components[0]);

int component_list_count(void)
{
    return s_component_count;
}

const component_entry_t *component_list_get(int index)
{
    if (index < 0 || index >= s_component_count) {
        return NULL;
    }
    return &s_components[index];
}

int component_list_random_in_category(component_category_t category, int exclude_index)
{
    int candidates[sizeof(s_components) / sizeof(s_components[0])];
    int candidate_count = 0;

    for (int i = 0; i < s_component_count; i++) {
        if (s_components[i].category == category && i != exclude_index) {
            candidates[candidate_count++] = i;
        }
    }

    if (candidate_count == 0) {
        /* only the excluded entry exists in this category -- keep it */
        for (int i = 0; i < s_component_count; i++) {
            if (s_components[i].category == category) {
                return i;
            }
        }
        return -1;
    }

    return candidates[esp_random() % candidate_count];
}

const char *component_category_label(component_category_t category)
{
    if (category < 0 || category >= COMPONENT_CAT_COUNT) {
        return "";
    }
    return s_category_labels[category];
}
