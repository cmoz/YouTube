// Cabin Altimeter
// ESP32 (30-pin) + SSD1306 OLED (0x3C) + AHT20 (0x38) + BMP280 (0x77 / 0x76)
// All devices share one I2C bus: SDA = GPIO21, SCL = GPIO22.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>

// ---- Configuration --------------------------------------------------------
#define BMP280_ADDR      0x77      // set to 0x76 if your I2C scan shows that
#define OLED_ADDR        0x3C
#define SDA_PIN          21
#define SCL_PIN          22

#define SCREEN_W         128
#define SCREEN_H         64

#define SAMPLE_MS        2000UL    // one pressure sample every 2 s
#define SEA_LEVEL_HPA    1013.25f  // standard atmosphere

// Flight graph: each column = average altitude over COL_MS.
// 125 plot columns x 5 s = ~10 min of history (60000 would give ~2 h)
#define COL_MS           5000UL
#define PLOT_W           (SCREEN_W - 3)   // interior columns x = 1..125
#define HIST_LEN         (PLOT_W - 1)     // finished columns; +1 live column
#define MIN_SPAN_FT      400.0f           // minimum vertical span of graph

// Vertical speed window: RATE_N samples x SAMPLE_MS = 60 s
#define RATE_N           31

// Graph area (bottom of screen)
#define GRAPH_Y0         34
#define GRAPH_Y1         63

// ---- Globals --------------------------------------------------------------
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
Adafruit_BMP280  bmp(&Wire);
Adafruit_AHTX0   aht;

float    history[HIST_LEN];   // altitude in feet, one per finished column
uint16_t histHead  = 0;       // next write index
uint16_t histCount = 0;       // valid samples

float    accSum = 0;          // live (in-progress) column accumulator
uint16_t accN   = 0;
unsigned long colStart = 0;

float    rateBuf[RATE_N];     // last 60 s of altitude, for ft/min
uint16_t rateHead = 0, rateCount = 0;

bool ahtOk = false;
unsigned long lastSample = 0;

// ---- Helpers --------------------------------------------------------------
// Standard pressure-altitude formula, returns feet
static float pressureToFeet(float hPa) {
  float meters = 44330.0f * (1.0f - powf(hPa / SEA_LEVEL_HPA, 0.1903f));
  return meters * 3.28084f;
}

static void pushSample(float hPa) {
  history[histHead] = hPa;
  histHead = (histHead + 1) % HIST_LEN;
  if (histCount < HIST_LEN) histCount++;
}

// i = 0 is oldest sample
static float getSample(uint16_t i) {
  uint16_t start = (histHead + HIST_LEN - histCount) % HIST_LEN;
  return history[(start + i) % HIST_LEN];
}

static void fatal(const char *msg) {
  Serial.println(msg);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
  while (true) delay(1000);
}

static void drawGraph() {
  // Frame sits fully on-screen: x 0..126, y 33..63 (the last pixel row/col can be hidden by bezels)
  display.drawRect(0, GRAPH_Y0 - 1, SCREEN_W - 1, GRAPH_Y1 - GRAPH_Y0 + 2, SSD1306_WHITE);
  // Finished columns + the live (in-progress) column at the right edge
  uint16_t n = histCount + (accN ? 1 : 0);
  if (n == 0) return;
  auto pt = [&](uint16_t i) -> float {
    return (i < histCount) ? getSample(i) : accSum / accN;
  };

  float mn = pt(0), mx = mn;
  for (uint16_t i = 1; i < n; i++) {
    float v = pt(i);
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  // Minimum span so cabin-pressure wobble doesn't look like a climb
  if (mx - mn < MIN_SPAN_FT) {
    float mid = (mx + mn) * 0.5f;
    mn = mid - MIN_SPAN_FT * 0.5f;
    mx = mid + MIN_SPAN_FT * 0.5f;
  }

  const int top = GRAPH_Y0 + 1, bot = GRAPH_Y1 - 1;
  int prevX = 0, prevY = 0;
  for (uint16_t i = 0; i < n; i++) {
    // Right-align: newest column at x = PLOT_W (125)
    int x = PLOT_W - (n - 1) + i;
    int y = bot - (int)((pt(i) - mn) / (mx - mn) * (bot - top));
    if (i > 0) display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
    else       display.drawPixel(x, y, SSD1306_WHITE);
    prevX = x;
    prevY = y;
  }
}

// Vertical speed in ft/min over the last ~60 s. Returns false until window is full.
static bool verticalRate(float ft, float &fpm) {
  rateBuf[rateHead] = ft;
  rateHead = (rateHead + 1) % RATE_N;
  if (rateCount < RATE_N) rateCount++;
  if (rateCount < RATE_N) return false;
  float oldest = rateBuf[rateHead];  // next slot to overwrite = oldest sample
  fpm = (ft - oldest) / ((RATE_N - 1) * SAMPLE_MS / 60000.0f);
  return true;
}

static void render(float hPa, float tempC, float rh, bool haveRate, float fpm) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Cabin altitude, large
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.printf("%5.0f ft", pressureToFeet(hPa));

  // Pressure / temp / humidity, small
  display.setTextSize(1);
  display.setCursor(0, 18);
  display.printf("%.1f hPa", hPa);
  display.setCursor(66, 18);
  if (haveRate) display.printf("%+5.0f fpm", roundf(fpm / 10.0f) * 10.0f);
  else          display.print("  -- fpm");
  display.setCursor(0, 26);
  if (ahtOk) display.printf("%.1fC  %.0f%%RH", tempC, rh);
  else       display.printf("%.1fC  --%%RH", tempC);

  drawGraph();
  display.display();
}

// ---- Arduino entry points -------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Cabin Altimeter");
  display.display();

  if (!bmp.begin(BMP280_ADDR)) {
    fatal("BMP280 not found.\nCheck wiring and\nBMP280_ADDR (0x76/77)");
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,    // temperature
                  Adafruit_BMP280::SAMPLING_X16,   // pressure
                  Adafruit_BMP280::FILTER_X4,
                  Adafruit_BMP280::STANDBY_MS_500);

  ahtOk = aht.begin();
  if (!ahtOk) Serial.println("AHT20 not found, humidity disabled");

  lastSample = millis() - SAMPLE_MS;  // sample immediately
  colStart = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;

  float hPa   = bmp.readPressure() / 100.0f;
  float tempC = bmp.readTemperature();
  float rh    = 0;

  if (ahtOk) {
    sensors_event_t h, t;
    aht.getEvent(&h, &t);
    rh    = h.relative_humidity;
    tempC = t.temperature;
  }

  if (isnan(hPa) || hPa < 300.0f || hPa > 1100.0f) {
    Serial.println("Bad pressure reading, skipped");
    return;
  }

  float ft = pressureToFeet(hPa);

  // Accumulate into the live graph column; close it every COL_MS
  accSum += ft;
  accN++;
  if (now - colStart >= COL_MS) {
    pushSample(accSum / accN);
    accSum = 0;
    accN = 0;
    colStart = now;
  }

  float fpm = 0;
  bool haveRate = verticalRate(ft, fpm);

  render(hPa, tempC, rh, haveRate, fpm);
  Serial.printf("%.2f hPa  %.0f ft  %+.0f fpm  %.1f C  %.0f %%RH\n",
                hPa, ft, haveRate ? fpm : 0.0f, tempC, rh);
}
