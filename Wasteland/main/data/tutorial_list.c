#include "data/tutorial_list.h"
#include <stdio.h>

static const tutorial_entry_t s_tutorials[] = {
    { "A look at Amazing WS2812D 4-Pin LEDs!",                    "s4t1PC_Bwgo" },
    { "Tap to Trigger: RFID + ESP32 + OLED Tutorial = AWESOME",   "7sqpxrMZuvo" },
    { "DIY wearable electronics on the go - here's what you need","BkbkwV_rxpU" },
    { "Engineer Scissors at Tinker Tailor",                       "Qal_MCykJJE" },
    { "Stuck in a creative rut? 9 wearable tech project starters","oSROYoP5HN0" },
    { "Phone Controls This Tiny LED Matrix Board",                "8e2qXAOdp-A" },
    { "Soldering POWER to your wearable project!",                "kWeSXx36g0U" },
    { "ESP32 OLED Project You Can Finish Today!",                 "wiu2lC0JfTA" },
    { "Color Detecting Jewelry: The Future of Fashion Tech",      "JGI9nlzN0mI" },
    { "Learn how to Solder an RGB LED neopixel ring",             "J-xa1br6J2A" },
    { "Makers on the Go: Tips for Building Anywhere!",            "pSXmlVV5ekM" },
    { "Sparkle Up Your Fashion Tech with ESP32 and WS2812b Lights!","Wf8HES75ies" },
    { "Tired of 'blinking LED' projects? Try this instead (Software)","byPnBiiqz9o" },
    { "Can You Make a Magic Sleeve With This Fabric?",            "VN8haRtufno" },
    { "The BEST Soldering Tools (and why you need them)",         "8DsUmoiebYo" },
    { "Make BUBBLE WRAP FABRIC, upcycle!",                        "DQYRnMUzXR8" },
    { "This Hat Helps You Avoid Collisions with Sound and Vibration","IrGlImGeCoY" },
    { "Build This Genius To-Do List Gadget (ePaper + ESP32)",     "dYDikwG_Oho" },
    { "Secret night time writing lamp",                           "SuxPk5NV71w" },
    { "DIY Phone-Controlled LED Wearables With ESP32",            "j6XMTufDMRg" },
};

static const int s_tutorial_count = sizeof(s_tutorials) / sizeof(s_tutorials[0]);

int tutorial_list_count(void)
{
    return s_tutorial_count;
}

const tutorial_entry_t *tutorial_list_get(int index)
{
    if (index < 0 || index >= s_tutorial_count) {
        return NULL;
    }
    return &s_tutorials[index];
}