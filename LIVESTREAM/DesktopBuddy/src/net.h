#pragma once
#include <Arduino.h>

#define MAX_STOCKS 8

struct Stock {
  char symbol[12];
  char name[22];
  float price;
  float change;               // % up or down since yesterday's close
  float buyBelow, sellAbove;  // his own targets from the phone page, 0 = not set
  bool valid;                 // we have a price
};

void netBegin();                     // connect to the saved WiFi (or carry on offline)
// Phone WiFi setup - tick(status) is called constantly to redraw; return false to cancel
bool netWifiSetup(bool (*tick)(const char *status));
void netLoop();                      // call every loop (runs the phone web page)
int  netGetStocks(Stock *out);       // copy of the latest prices, returns how many
// Latest weather: WMO weather code, day or night, temperature in C. False until we have it
bool netGetWeather(int &code, bool &isDay, float &tempC);
bool netOnline();
String netAddress();                 // IP address for the phone page
