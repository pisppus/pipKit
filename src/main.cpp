#include <Arduino.h>
#include <SPIFFS.h>
#include <math.h>
#include <pipKit.hpp>

using namespace pipgui;

GUI ui;

Button btnNext(2, Pullup);
Button btnPrev(4, Pullup);

static inline uint32_t rgb565(uint16_t c)
{
  uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
  uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
  uint8_t b5 = (uint8_t)(c & 0x1F);
  uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
  uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
  uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
  return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
}

ButtonVisualState settingsBtnState{true, 0, 255, true, false, 0};

static const uint16_t ICON_DOT_WHITE[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static const uint16_t ICON_DOT_GRAY[4] = {0x8410, 0x8410, 0x8410, 0x8410};

float g_graphPhase = 0.0f;
uint32_t g_lastGraphUpdateMs = 0;
uint32_t g_lastClockSecond = 0;
uint8_t g_batteryLevel = 100;
bool g_batteryDirDown = true;
uint8_t g_progressValue = 0;
bool g_progressDirDown = false;
uint32_t g_lastProgressUpdateMs = 0;

int16_t g_graphV1 = 0;
int16_t g_graphV2 = 0;
int16_t g_graphV3 = 0;

ToggleSwitchState g_toggleState{false, 0, 0};
bool g_errorDemoActive = false;
uint32_t g_settingsLoadingUntil = 0;

static const uint8_t g_dotsCount = 15;
uint8_t g_dotsActive = 0;
uint8_t g_dotsPrev = 0;
bool g_dotsAnimate = false;
int8_t g_dotsDir = 0;
uint32_t g_dotsAnimStartMs = 0;
static const uint32_t g_dotsAnimDurMs = 240;

static const char *const g_drumOptionsH[] = {"Off", "5 min", "10 min", "30 min", "1 hr"};
static const uint8_t g_drumCountH = 5;
static const char *const g_drumOptionsV[] = {"Small", "Medium", "Large"};
static const uint8_t g_drumCountV = 3;

uint8_t g_drumSelectedH = 0;
uint8_t g_drumPrevH = 0;
bool g_drumAnimateH = false;
uint32_t g_drumAnimStartMs = 0;
static const uint32_t g_drumAnimDurMs = 280;

uint8_t g_drumSelectedV = 0;
uint8_t g_drumPrevV = 0;
bool g_drumAnimateV = false;

float g_blurPhase = 0.0f;
uint32_t g_blurLastUpdateMs = 0;

static void runBootAnimation(GUI &ui, Button &next, Button &prev, BootAnimation anim, uint32_t durationMs, const String &title, const String &subtitle)
{
  (void)next;
  (void)prev;
  ui.showLogo(title, subtitle, anim, 0xFFFF, 0x0000, durationMs);
  const uint32_t start = millis();
  while ((millis() - start) < durationMs)
  {
    ui.loop();
  }
}

void screenPrimitivesDemo(GUI &ui);
void screenGradientsDemo(GUI &ui);
void screenFontWeightDemo(GUI &ui);
void screenCircleDemo(GUI &ui);

void screenMain(GUI &ui)
{
  uint16_t bg565 = ui.rgb(0, 0, 0);
  ui.clear(bg565);

  ui.setTextStyle(H1);
  ui.drawText("Main menu", -1, -1, ui.rgb(255, 255, 255), bg565, AlignCenter);

  const int16_t ix = 12;
  const int16_t iy = 12;
  const uint16_t is = 48;
  const uint8_t level = g_batteryLevel;
  const uint16_t fill565 = (level <= 20) ? ui.rgb(255, 40, 40) : ui.rgb(80, 255, 120);
  const uint32_t fill888 = rgb565(fill565);
  const uint32_t bg888 = rgb565(bg565);

  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer2)
      .color(fill565)
      .bgColor(bg565);

  const int16_t cutX = (int16_t)(ix + (int16_t)((uint32_t)is * (uint32_t)level) / 100u);
  ui.fillRect()
      .pos(cutX, iy)
      .size((int16_t)(ix + (int16_t)is - cutX), (int16_t)is)
      .color(bg565);

  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer0)
      .color(0xFFFF)
      .bgColor(bg565);
  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer1)
      .color(0xFFFF)
      .bgColor(bg565);
  ui.drawIcon()
      .pos(70, 12)
      .size(48)
      .icon(warning_layer0)
      .color(0xFF00)
      .bgColor(bg565);
}

void screenSettings(GUI &ui)
{
  uint16_t bg565 = ui.rgb(0, 31, 31);
  ui.clear(bg565);

  ui.setTextStyle(H1);
  ui.drawText("Settings menu", -1, 80, rgb565(0xFFFF), bg565, AlignCenter);

  const String label = settingsBtnState.loading ? String("Saving") : String("Show modal");

  ui.drawButton()
      .label(label)
      .pos(60, 20)
      .size(120, 44)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(10)
      .state(settingsBtnState);
}

void screenGlowDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.drawGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg565)
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900);

  ui.drawGlowCircle()
      .pos(70, 175)
      .radius(22)
      .fillColor(ui.rgb(80, 255, 120))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(200);

  ui.drawGlowRect()
      .pos(center, 70)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400);

  ui.drawGlowRect()
      .pos(140, 175)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg565)
      .glowSize(16)
      .glowStrength(180);

  ui.setTextStyle(H2);
  ui.drawText("Glow demo", -1, 22, rgb565(0xFFFF), bg565, AlignCenter);
  ui.setTextStyle(Body);
  ui.drawText("REC / shapes", -1, 44, ui.rgb(200, 200, 200), bg565, AlignCenter);
}

void screenBlurDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  uint16_t w = ui.screenWidth();
  int16_t statusH = 20; // как в configureStatusBar
  int16_t bandY = statusH;
  int16_t bandH = 80;
  float t = g_blurPhase;

  // Движущиеся прямоугольники под полосой
  for (int i = 0; i < 3; ++i)
  {
    float tt = t * (0.8f + 0.3f * i);
    int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));
    int16_t cy = (int16_t)(bandY + 15 + i * 22 + cosf(tt * 1.3f) * 8.0f);
    int16_t rw = 60 + i * 20;
    int16_t rh = 18;
    uint32_t col;

    if (i == 0)
      col = ui.rgb(255, 80, 80);
    else if (i == 1)
      col = ui.rgb(80, 255, 120);
    else
      col = ui.rgb(80, 255, 140);

    ui.fillRect()
        .pos((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
        .size(rw, rh)
        .color(col);
  }

  // Блюр-полоска сверху движущихся фигур
  ui.drawBlur()
      .pos(0, bandY)
      .size((int16_t)w, bandH)
      .radius(10)
      .direction(TopDown)
      .gradient(false)
      .materialStrength(160)
      .noiseAmount(40)
      .materialColor(ui.rgb(10, 10, 10));

  // Вторая строка фигур без блюра — для наглядного сравнения
  for (int i = 0; i < 3; ++i)
  {
    float tt = t * (0.8f + 0.3f * i);
    int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));
    int16_t cy = (int16_t)(bandY + bandH + 10 + i * 18);
    int16_t rw = 60 + i * 20;
    int16_t rh = 16;
    uint32_t col;

    if (i == 0)
      col = ui.rgb(255, 80, 80);
    else if (i == 1)
      col = ui.rgb(80, 255, 120);
    else
      col = ui.rgb(80, 255, 140);

    ui.fillRect()
        .pos((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
        .size(rw, rh)
        .color(col);
  }

  ui.setTextStyle(H2);
  ui.drawText("Blur strip", -1, (int16_t)(bandY + bandH + 60), rgb565(0xFFFF), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("Next: change screen", -1, (int16_t)(bandY + bandH + 80), ui.rgb(160, 160, 160), bg565, AlignCenter);
  ui.drawText("Prev: back / OK", -1, (int16_t)(bandY + bandH + 96), ui.rgb(160, 160, 160), bg565, AlignCenter);
}

void screenGraph(GUI &ui)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 280, 170, 13, LeftToRight, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphSmall(GUI &ui)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 160, 80, 10, LeftToRight, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(true);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphTall(GUI &ui)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 160, 180, 10, RightToLeft, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(true);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphOsc(GUI &ui)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 200, 170, 13, Oscilloscope, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(false);

  float tBase = g_graphPhase;
  float slow = sinf(tBase * 0.05f);
  float offset = slow * 40.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(tBase * 0.3f) * 20.0f;
  float t = tBase;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  ui.drawGraphLine(1, (int16_t)v2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, (int16_t)v3, ui.rgb(100, 160, 255), -110, 110);
}

void screenProgressDemo(GUI &ui)
{
  ui.clear(0x000000);

  ui.drawProgressBar()
      .pos(center, 60)
      .size(200, 10)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .anim(Indeterminate);

  ui.drawProgressBar()
      .pos(center, 74)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .anim(Indeterminate);

  ui.drawProgressBar()
      .pos(center, 88)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .radius(6)
      .anim(None);

  ui.drawCircularProgressBar()
      .pos(50, 165)
      .radius(22)
      .thickness(8)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .anim(Indeterminate);

  ui.drawCircularProgressBar()
      .pos(105, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  ui.drawCircularProgressBar()
      .pos(160, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .anim(Shimmer);

  ui.drawCircularProgressBar()
      .pos(215, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 200, 120))
      .anim(Shimmer);
}

void screenProgressTextDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Progress with text", -1, 24, ui.rgb(220, 220, 220), bg565, AlignCenter);

  uint32_t base = ui.rgb(20, 20, 20);
  uint32_t fill1 = ui.rgb(0, 122, 255);
  uint32_t fill2 = ui.rgb(255, 59, 48);

  // Simple percent centered
  ui.drawProgressBar()
      .pos(center, 70)
      .size(200, 14)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill1)
      .radius(7)
      .anim(None);
  ui.drawProgressPercent(center, 70, 200, 14, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), base, 0);

  // Left custom label + right percent
  ui.drawProgressBar()
      .pos(center, 92)
      .size(200, 14)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill2)
      .radius(7)
      .anim(Shimmer);
  ui.drawProgressText(center, 92, 200, 14, String("Uploading"), AlignLeft, ui.rgb(255, 255, 255), base, 0);
  ui.drawProgressPercent(center, 92, 200, 14, g_progressValue, AlignRight, ui.rgb(200, 200, 200), 0x000000, 0);

  // Circular progress with text in the middle area
  ui.drawCircularProgressBar()
      .pos(center, 160)
      .radius(30)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill1)
      .anim(None);
  ui.drawProgressPercent((int16_t)(center - 30), 130, 60, 60, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), bg565, 0);
}

void screenPrimitivesDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  uint32_t w = ui.screenWidth();

  ui.setTextStyle(Caption);
  ui.drawText("Primitives", -1, 8, ui.rgb(220, 220, 220), bg565, AlignCenter);

  ui.fillCircle()
      .pos(50, 55)
      .radius(18)
      .color(ui.rgb(0, 87, 250));
  ui.drawCircle()
      .pos(50, 55)
      .radius(18)
      .color(ui.rgb(255, 255, 255));
  ui.fillEllipse()
      .pos(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 0, 72));
  ui.drawEllipse()
      .pos(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 255, 255));
  ui.fillTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(0, 200, 120));
  ui.drawTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(255, 255, 255));
  ui.fillSquircle()
      .pos(70, 135)
      .radius(26)
      .color(ui.rgb(255, 128, 0));
  ui.drawSquircle()
      .pos(70, 135)
      .radius(26)
      .color(ui.rgb(255, 255, 255));

  // Прямоугольник с разными радиусами углов для демонстрации per-corner скругления
  ui.fillRect()
      .pos(210, 165)
      .size(80, 40)
      .color(bg565);
  ui.drawGlowRect()
      .pos(250, 185)
      .size(80, 40)
      .radius(12)
      .fillColor(ui.rgb(40, 120, 255))
      .bgColor(bg565)
      .glowSize(10)
      .glowStrength(180);
  ui.drawCircle()
      .pos(170, 135)
      .radius(28)
      .color(ui.rgb(60, 60, 60));
  ui.drawArc()
      .pos(170, 135)
      .radius(28)
      .startDeg(-90.0f)
      .endDeg(90.0f)
      .color(ui.rgb(80, 255, 120));
  ui.drawArc()
      .pos(170, 135)
      .radius(22)
      .startDeg(90.0f)
      .endDeg(270.0f)
      .color(ui.rgb(100, 160, 255));
  ui.drawLine()
      .from(12, 90)
      .to(118, 160)
      .color(ui.rgb(255, 255, 255));
  ui.drawLine()
      .from(12, 160)
      .to(118, 90)
      .color(ui.rgb(255, 255, 255));
  ui.drawLine()
      .from(12, 125)
      .to(118, 126)
      .color(ui.rgb(255, 255, 255));
  ui.drawArc()
      .pos(250, 135)
      .radius(14)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255));
  ui.drawArc()
      .pos(250, 135)
      .radius(10)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(80, 255, 120));

  // Прямоугольник с разными радиусами углов для демонстрации per-corner скругления
  ui.fillRect()
      .pos(10, 180)
      .size(80, 40)
      .color(ui.rgb(30, 30, 30))
      .radius({16, 4, 16, 4});

  ui.drawLine()
      .from(10, 225)
      .to((int16_t)(w - 10), 225)
      .color(ui.rgb(200, 200, 200));

  ui.drawText("Next: change screen", -1, 235, ui.rgb(160, 160, 160), bg565, AlignCenter);
}

void screenGradientsDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  ui.setTextStyle(Caption);
  ui.drawText("Gradients / Alpha", -1, 8, ui.rgb(220, 220, 220), bg565, AlignCenter);

  char buf[64];
  uint32_t tStart, tElapsed;

  tStart = micros();
  ui.gradientVertical()
      .pos(10, 30)
      .size(140, 60)
      .topColor(ui.rgb(255, 0, 72))
      .bottomColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "V:%luus", tElapsed);
  ui.drawText(buf, 80, 65, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientHorizontal()
      .pos(170, 30)
      .size(140, 60)
      .leftColor(ui.rgb(255, 128, 0))
      .rightColor(ui.rgb(80, 255, 120));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "H:%luus", tElapsed);
  ui.drawText(buf, 240, 65, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientCorners()
      .pos(10, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 0, 72))
      .topRightColor(ui.rgb(0, 87, 250))
      .bottomLeftColor(ui.rgb(80, 255, 120))
      .bottomRightColor(ui.rgb(255, 128, 0));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "C4:%luus", tElapsed);
  ui.drawText(buf, 80, 135, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientDiagonal()
      .pos(170, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 255, 255))
      .bottomRightColor(ui.rgb(40, 40, 40));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "D:%luus", tElapsed);
  ui.drawText(buf, 240, 135, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientRadial()
      .pos(10, 170)
      .size(140, 60)
      .center(80, 200)
      .radius(48)
      .innerColor(ui.rgb(255, 255, 255))
      .outerColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "R:%luus", tElapsed);
  ui.drawText(buf, 80, 205, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .topColor(ui.rgb(20, 20, 20))
      .bottomColor(ui.rgb(20, 20, 20));
  tElapsed = micros() - tStart;

  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .topColor(ui.rgb(20, 20, 20))
      .bottomColor(ui.rgb(60, 60, 60));
  snprintf(buf, sizeof(buf), "V2:%luus", micros() - tStart);
  ui.drawText(buf, 240, 205, ui.rgb(255, 255, 255), bg565, AlignCenter);
}

void screenListMenuDemo(GUI &ui)
{
  ui.clear(0x0000);
  ui.updateList(7);
}

void screenListMenuPlainDemo(GUI &ui)
{
  ui.clear(0x0000);
  ui.updateList(10);
}

void screenTileMenuDemo(GUI &ui)
{
  ui.renderTileMenu(8);
}

void screenTileMenuLayoutDemo(GUI &ui)
{
  ui.renderTileMenu(11);
}

void screenTileMenu4ColsDemo(GUI &ui)
{
  ui.renderTileMenu(12);
}

void screenToggleSwitchDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .state(g_toggleState)
      .activeColor(ui.rgb(21, 180, 110));

  ui.setTextStyle(H2);
  ui.drawText("ToggleSwitch", -1, 24, ui.rgb(220, 220, 220), bg565, AlignCenter);
}

void screenScrollDotsDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText("Scroll dots", -1, 24, ui.rgb(220, 220, 220), bg565, AlignCenter);
  ui.drawText("15 dots (tapering)", -1, 48, ui.rgb(120, 120, 120), bg565, AlignCenter);
}

void screenDrumRollDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText("Drum roll", -1, 24, ui.rgb(220, 220, 220), bg565, AlignCenter);
  ui.drawText("Next: H  Prev: V", -1, 48, ui.rgb(120, 120, 120), bg565, AlignCenter);

  uint32_t now = millis();
  if (g_drumAnimateH && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateH = false;
  if (g_drumAnimateV && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateV = false;

  float progressH = 1.0f;
  if (g_drumAnimateH && g_drumAnimDurMs > 0)
  {
    uint32_t el = now - g_drumAnimStartMs;
    progressH = (el >= g_drumAnimDurMs) ? 1.0f : (float)el / (float)g_drumAnimDurMs;
  }

  float progressV = 1.0f;
  if (g_drumAnimateV && g_drumAnimDurMs > 0)
  {
    uint32_t el = now - g_drumAnimStartMs;
    progressV = (el >= g_drumAnimDurMs) ? 1.0f : (float)el / (float)g_drumAnimDurMs;
  }

  String optsH[5] = {String(g_drumOptionsH[0]), String(g_drumOptionsH[1]), String(g_drumOptionsH[2]), String(g_drumOptionsH[3]), String(g_drumOptionsH[4])};
  ui.drawDrumRollHorizontal(0, 75, ui.screenWidth(), 28, optsH, g_drumCountH, g_drumSelectedH, g_drumPrevH, progressH, ui.rgb(255, 255, 255), ui.rgb(8, 8, 8), 18);

  String optsV[3] = {String(g_drumOptionsV[0]), String(g_drumOptionsV[1]), String(g_drumOptionsV[2])};
  ui.drawDrumRollVertical(ui.screenWidth() - 80, 120, 70, 90, optsV, g_drumCountV, g_drumSelectedV, g_drumPrevV, progressV, ui.rgb(200, 200, 200), ui.rgb(8, 8, 8), 14);
}

void screenErrorOverlayDemo(GUI &ui)
{
  ui.clear(ui.rgb(8, 8, 8));
  ui.showError("Error demo", "Something went wrong", Crash, "OK");
}

void screenWarningOverlayDemo(GUI &ui)
{
  ui.clear(ui.rgb(8, 8, 8));
  ui.showError("Warning demo", "Check settings", Warning, "OK");
}

void screenDitherDemo(GUI &ui)
{
  ui.clear(ui.rgb(8, 8, 8));
}

void screenDitherBlueDemo(GUI &ui)
{
  ui.clear(ui.rgb(8, 8, 20));
}

void screenDitherTemporalDemo(GUI &ui)
{
  ui.clear(ui.rgb(20, 8, 8));
}

void screenDitherCubesDemo(GUI &ui)
{
  ui.clear(ui.rgb(8, 20, 8));
}

void screenDitherCompareGradDemo(GUI &ui)
{
  // Left: direct 16-bit (RGB565). Right: 24-bit approximated with BNSD
  // BNSD is now always enabled

  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  int16_t w = (int16_t)ui.screenWidth();
  int16_t h = (int16_t)ui.screenHeight();
  int16_t cw = (int16_t)(w / 2 - 24);
  int16_t ch = (int16_t)(h - 100);

  if (cw < 32)
    cw = 32;
  if (ch < 32)
    ch = 32;

  int16_t leftX = 12;
  int16_t rightX = (int16_t)(w / 2 + 12);
  int16_t topY = 44;
  uint32_t now = millis();

  // smooth per-channel sinusoidal animation (avoids hue-discontinuities)
  static bool frozen = false;
  static uint8_t frozen_r = 0, frozen_g = 0, frozen_b = 0;

  if (btnPrev.wasPressed())
  {
    frozen = !frozen; // toggle freeze state on PREV press
    if (frozen)
    {
      // capture current color into the frozen sample
      float tcap = (float)now / 1000.0f;
      frozen_r = (uint8_t)((sinf(tcap * 0.9f) * 0.5f + 0.5f) * 255.0f);
      frozen_g = (uint8_t)((sinf(tcap * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);
      frozen_b = (uint8_t)((sinf(tcap * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
    }
  }

  uint8_t r, g, b;
  if (frozen)
  {
    r = frozen_r;
    g = frozen_g;
    b = frozen_b;
  }
  else
  {
    float t = (float)now / 1000.0f;
    r = (uint8_t)((sinf(t * 0.9f) * 0.5f + 0.5f) * 255.0f);
    g = (uint8_t)((sinf(t * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);
    b = (uint8_t)((sinf(t * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
  }

  // Draw left: 16-bit direct
  ui.fillRect()
      .pos(leftX, topY)
      .size(cw, ch)
      .color(ui.rgb(r, g, b));

  // Draw right: direct RGB565
  ui.fillRect()
      .pos(rightX, topY)
      .size(cw, ch)
      .color(ui.rgb(r, g, b));

  // Static sample square (for quick inspection) + status
  int16_t sampleX = (int16_t)(w - 64);
  int16_t sampleY = (int16_t)(topY + 4);

  ui.fillRect()
      .pos(sampleX - 1, sampleY - 1)
      .size(50, 50)
      .color(ui.rgb(40, 40, 40)); // border
  ui.fillRect()
      .pos(sampleX, sampleY)
      .size(48, 48)
      .color(ui.rgb(r, g, b));

  ui.setTextStyle(Caption);
  ui.drawText(String(frozen ? "Frozen (PREV)" : "Live (PREV to freeze)"), (int16_t)(sampleX + 24), (int16_t)(sampleY + 56), ui.rgb(200, 200, 200), bg565, AlignCenter);
  ui.drawText("16-bit: RGB565", leftX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg565, AlignCenter);
  ui.drawText("24-bit: FRC BlueNoise+gamma", rightX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg565, AlignCenter);

  char buf[64];
  snprintf(buf, sizeof(buf), "HEX: #%02X%02X%02X", r, g, b);
  ui.drawText(String(buf), (int16_t)(w / 2), (int16_t)(topY + ch + 8), ui.rgb(160, 160, 160), bg565, AlignCenter);
  ui.drawText("RGB565 steps per channel: R=32 G=64 B=32", (int16_t)(w / 2), (int16_t)(topY + ch + 26), ui.rgb(160, 160, 160), bg565, AlignCenter);
  ui.drawText("24-bit per channel: 256 (visualized via FRC)", (int16_t)(w / 2), (int16_t)(topY + ch + 44), ui.rgb(160, 160, 160), bg565, AlignCenter);

  uint8_t r5 = (uint8_t)(r >> 3);
  uint8_t g6 = (uint8_t)(g >> 2);
  uint8_t b5 = (uint8_t)(b >> 3);
  snprintf(buf, sizeof(buf), "RGB565: R5=%02u G6=%02u B5=%02u", r5, g6, b5);
  ui.drawText(String(buf), (int16_t)(w / 2), (int16_t)(topY + ch + 62), ui.rgb(160, 160, 160), bg565, AlignCenter);
}

void updateGraphDemo()
{
  float t = g_graphPhase;
  float slow = sinf(t * 0.05f);
  float offset = slow * 40.0f;
  float amp1 = 40.0f + slow * 20.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(t * 0.3f) * 20.0f;

  float v1 = offset + sinf(t) * amp1;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  g_graphV1 = (int16_t)v1;
  g_graphV2 = (int16_t)v2;
  g_graphV3 = (int16_t)v3;

  g_graphPhase += 0.12f;
  if (g_graphPhase > 1000.0f)
    g_graphPhase -= 1000.0f;
}

void screenFontCompareDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  int16_t w = (int16_t)ui.screenWidth();
  int16_t h = (int16_t)ui.screenHeight();

  ui.setTextStyle(H2);
  ui.drawText("PSDF font demo", (int16_t)(w / 2), 20, ui.rgb(220, 220, 220), bg565, AlignCenter);

  const String sample = "The quick brown fox";

  // Left: PSDF (size + weight variations)
  const uint16_t sizes[4] = {18, 24, 36, 48};
  const uint16_t weights[4] = {400, 500, 600, 700};
  int16_t y0 = 50;
  int16_t spacing = 42;

  for (int i = 0; i < 4; ++i)
  {
    ui.setFontSize(sizes[i]);
    int16_t ty = (int16_t)(y0 + i * spacing);
    // label + sample
    ui.drawText(String("PSDF ") + String(sizes[i]) + "px", 8, ty, ui.rgb(255, 255, 255), bg565, AlignLeft);
    ui.drawText(sample, 8, (int16_t)(ty + 18), ui.rgb(200, 200, 200), bg565, AlignLeft);
  }
}

void screenFontWeightDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  int16_t w = (int16_t)ui.screenWidth();

  ui.setTextStyle(H2);
  ui.drawText("Font Weight Test", (int16_t)(w / 2), 10, ui.rgb(255, 255, 0), bg565, AlignCenter);

  const String sample = "The quick brown fox";
  const uint16_t weights[] = {100, 300, 400, 500, 600, 700, 900};
  const char *labels[] = {"Thin 100", "Light 300", "Regular 400", "Medium 500", "SemiBold 600", "Bold 700", "Black 900"};

  int16_t y = 35;
  int16_t spacing = 38;

  for (int i = 0; i < 7; ++i)
  {
    ui.setFontSize(12);
    ui.setFontWeight(400);
    ui.drawText(String(labels[i]), 8, y, ui.rgb(150, 150, 150), bg565, AlignLeft);

    ui.setFontSize(20);
    ui.setFontWeight(weights[i]);
    ui.drawText(sample, 8, (int16_t)(y + 14), ui.rgb(255, 255, 255), bg565, AlignLeft);

    y += spacing;
  }
}

// ===== DEDICATED PRIMITIVE TEST SCREENS =====

void screenTestCircles(GUI &ui)
{
  uint16_t bg565 = ui.rgb(20, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Circle Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fillCircle + drawCircle (AA)", -1, 28, ui.rgb(180, 180, 200), bg565, AlignCenter);

  // Row 1: Small circles (1px to 8px)
  ui.drawText("r=1-4", 25, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(25, 58).radius(1).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 58).radius(1).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 65).radius(2).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 65).radius(2).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 75).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 75).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 88).radius(4).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 88).radius(4).color(ui.rgb(255, 255, 255));

  // Row 2: Medium circles (6px to 15px)
  ui.drawText("r=6-15", 80, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(65, 70).radius(6).color(ui.rgb(255, 0, 72));
  ui.drawCircle().pos(65, 70).radius(6).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(95, 70).radius(9).color(ui.rgb(80, 255, 120));
  ui.drawCircle().pos(95, 70).radius(9).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 105).radius(12).color(ui.rgb(255, 128, 0));
  ui.drawCircle().pos(80, 105).radius(12).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 145).radius(15).color(ui.rgb(180, 80, 255));
  ui.drawCircle().pos(80, 145).radius(15).color(ui.rgb(255, 255, 255));

  // Row 3: Large circles (18px to 35px)
  ui.drawText("r=18-35", 155, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(145, 75).radius(18).color(ui.rgb(0, 200, 200));
  ui.drawCircle().pos(145, 75).radius(18).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(195, 75).radius(22).color(ui.rgb(200, 200, 80));
  ui.drawCircle().pos(195, 75).radius(22).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(145, 130).radius(25).color(ui.rgb(255, 100, 100));
  ui.drawCircle().pos(145, 130).radius(25).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(195, 135).radius(30).color(ui.rgb(100, 150, 255));
  ui.drawCircle().pos(195, 135).radius(30).color(ui.rgb(255, 255, 255));

  // Overlapping circles test (AA edge quality)
  ui.fillCircle().pos(40, 185).radius(18).color(ui.rgb(0, 87, 250));
  ui.fillCircle().pos(60, 195).radius(18).color(ui.rgb(255, 0, 72));
  ui.fillCircle().pos(50, 175).radius(18).color(ui.rgb(80, 255, 120));

  // Draw-only circles (outlines)
  ui.drawCircle().pos(110, 185).radius(15).color(ui.rgb(255, 255, 255));
  ui.drawCircle().pos(110, 185).radius(12).color(ui.rgb(200, 200, 200));
  ui.drawCircle().pos(110, 185).radius(8).color(ui.rgb(150, 150, 150));

  ui.drawCircle().pos(160, 195).radius(20).color(ui.rgb(255, 128, 0));
  ui.drawCircle().pos(210, 185).radius(25).color(ui.rgb(0, 200, 200));

  ui.drawText("Check AA edges on overlaps!", -1, 240, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestRoundRects(GUI &ui)
{
  uint16_t bg565 = ui.rgb(20, 28, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("RoundRect Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fill/draw RoundRect (1 & 4 radius)", -1, 28, ui.rgb(180, 200, 180), bg565, AlignCenter);

  // Single radius variants
  ui.drawText("Single radius:", 60, 45, ui.rgb(180, 220, 180), bg565, AlignCenter);

  ui.fillRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(0, 87, 250));
  ui.drawRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 0, 72));
  ui.drawRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(80, 255, 120));
  ui.drawRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 128, 0));
  ui.drawRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 255, 255));

  // 4-radius variants (asymmetric corners)
  ui.drawText("4-corner radii:", 60, 110, ui.rgb(220, 180, 180), bg565, AlignCenter);

  // TL=2, TR=8, BR=2, BL=8
  ui.fillRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(180, 80, 255));
  ui.drawRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(255, 255, 255));

  // TL=12, TR=4, BR=12, BL=4
  ui.fillRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(0, 200, 200));
  ui.drawRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(255, 255, 255));

  // TL=3, TR=10, BR=18, BL=6
  ui.fillRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(200, 200, 80));
  ui.drawRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(255, 255, 255));

  // Draw-only round rects
  ui.drawText("drawRoundRect only:", 70, 185, ui.rgb(200, 200, 200), bg565, AlignCenter);

  ui.drawRect().pos(20, 200).size(45, 35).radius({6}).color(ui.rgb(255, 255, 255));
  ui.drawRect().pos(75, 205).size(50, 40).radius({10}).color(ui.rgb(0, 255, 255));
  ui.drawRect().pos(135, 200).size(55, 45).radius({5, 15, 5, 15}).color(ui.rgb(255, 200, 0));
  ui.drawRect().pos(200, 205).size(50, 40).radius({15, 5, 15, 5}).color(ui.rgb(255, 100, 200));

  ui.drawText("Check corner AA quality!", -1, 260, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestEllipses(GUI &ui)
{
  uint16_t bg565 = ui.rgb(28, 20, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Ellipse Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fillEllipse + drawEllipse (Wu-style AA)", -1, 28, ui.rgb(200, 180, 180), bg565, AlignCenter);

  // Various aspect ratios
  ui.drawText("rx > ry (wide):", 70, 45, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(40, 70).radiusX(15).radiusY(8).color(ui.rgb(0, 87, 250));
  ui.drawEllipse().pos(40, 70).radiusX(15).radiusY(8).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(100, 70).radiusX(25).radiusY(10).color(ui.rgb(255, 0, 72));
  ui.drawEllipse().pos(100, 70).radiusX(25).radiusY(10).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(170, 70).radiusX(35).radiusY(12).color(ui.rgb(80, 255, 120));
  ui.drawEllipse().pos(170, 70).radiusX(35).radiusY(12).color(ui.rgb(255, 255, 255));

  ui.drawText("rx < ry (tall):", 70, 95, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(40, 120).radiusX(8).radiusY(15).color(ui.rgb(255, 128, 0));
  ui.drawEllipse().pos(40, 120).radiusX(8).radiusY(15).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(100, 120).radiusX(10).radiusY(25).color(ui.rgb(180, 80, 255));
  ui.drawEllipse().pos(100, 120).radiusX(10).radiusY(25).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(170, 120).radiusX(12).radiusY(35).color(ui.rgb(0, 200, 200));
  ui.drawEllipse().pos(170, 120).radiusX(12).radiusY(35).color(ui.rgb(255, 255, 255));

  // Near-circular
  ui.drawText("Near-circular:", 70, 165, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(50, 190).radiusX(18).radiusY(16).color(ui.rgb(200, 200, 80));
  ui.drawEllipse().pos(50, 190).radiusX(18).radiusY(16).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(120, 190).radiusX(22).radiusY(20).color(ui.rgb(100, 255, 100));
  ui.drawEllipse().pos(120, 190).radiusX(22).radiusY(20).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(190, 190).radiusX(28).radiusY(25).color(ui.rgb(255, 100, 100));
  ui.drawEllipse().pos(190, 190).radiusX(28).radiusY(25).color(ui.rgb(255, 255, 255));

  // Small ellipses (edge case test)
  ui.fillEllipse().pos(30, 235).radiusX(3).radiusY(5).color(ui.rgb(255, 200, 0));
  ui.drawEllipse().pos(30, 235).radiusX(3).radiusY(5).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(50, 235).radiusX(5).radiusY(3).color(ui.rgb(0, 255, 200));
  ui.drawEllipse().pos(50, 235).radiusX(5).radiusY(3).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(70, 235).radiusX(4).radiusY(4).color(ui.rgb(255, 100, 200));
  ui.drawEllipse().pos(70, 235).radiusX(4).radiusY(4).color(ui.rgb(255, 255, 255));

  ui.drawText("Check Wu-style AA on edges!", -1, 260, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestTriangles(GUI &ui)
{
  uint16_t bg565 = ui.rgb(20, 24, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Triangle Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fillTriangle (4x subpixel AA)", -1, 28, ui.rgb(180, 200, 220), bg565, AlignCenter);

  // Small triangles (5-15px height)
  ui.drawText("Small (5-15px):", 70, 45, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(255, 255, 255));

  // Medium triangles
  ui.drawText("Medium (20-40px):", 80, 95, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(255, 255, 255));

  // Flat/thin triangles (stress test for AA)
  ui.drawText("Flat/thin (AA stress):", 90, 170, ui.rgb(220, 180, 180), bg565, AlignCenter);

  // Very flat triangle
  ui.fillTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 255, 255));

  // Thin triangle
  ui.fillTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(255, 255, 255));

  // Long thin triangle
  ui.fillTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(255, 255, 255));

  // Overlapping triangles (check blend quality)
  ui.fillTriangle().point0(30, 235).point1(60, 275).point2(0, 275).radius(0).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 250).point1(75, 290).point2(15, 290).radius(0).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 250).point1(45, 290).point2(-15, 290).radius(0).color(ui.rgb(80, 255, 120));

  ui.drawText("Check coverage-based AA edges!", -1, 305, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestRoundTriangles(GUI &ui)
{
  uint16_t bg565 = ui.rgb(24, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("RoundTriangle Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fill/draw RoundTriangle (SDF AA)", -1, 28, ui.rgb(200, 180, 220), bg565, AlignCenter);

  // Small rounded triangles (radius 3-6)
  ui.drawText("Small (r=3-6):", 70, 45, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(255, 255, 255));

  // Medium rounded triangles (radius 8-12)
  ui.drawText("Medium (r=8-12):", 80, 95, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(255, 255, 255));

  // Different triangle shapes with rounding
  ui.drawText("Various shapes (r=10):", 90, 170, ui.rgb(220, 180, 180), bg565, AlignCenter);

  // Equilateral-ish
  ui.fillTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 255, 255));

  // Wide flat
  ui.fillTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(255, 255, 255));

  // Tall narrow
  ui.fillTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(255, 255, 255));

  // Overlapping rounded triangles (blend test)
  ui.fillTriangle().point0(30, 270).point1(60, 310).point2(0, 310).radius(8).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 280).point1(75, 320).point2(15, 320).radius(8).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 280).point1(45, 320).point2(-15, 320).radius(8).color(ui.rgb(80, 255, 120));

  ui.drawText("Check SDF AA on rounded corners!", -1, 335, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestSquircles(GUI &ui)
{
  uint16_t bg565 = ui.rgb(18, 18, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Squircle Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fillSquircle + drawSquircle (AA + aaGamma)", -1, 28, ui.rgb(200, 200, 220), bg565, AlignCenter);

  ui.fillSquircle().pos(50, 80).radius(18).color(ui.rgb(0, 87, 250));
  ui.drawSquircle().pos(50, 80).radius(18).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 0, 72));
  ui.drawSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(200, 80).radius(34).color(ui.rgb(80, 255, 120));
  ui.drawSquircle().pos(200, 80).radius(34).color(ui.rgb(255, 255, 255));

  ui.drawText("Small r=6..10 (edge case):", 95, 128, ui.rgb(200, 180, 160), bg565, AlignCenter);
  ui.fillSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 128, 0));
  ui.drawSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(55, 150).radius(8).color(ui.rgb(180, 80, 255));
  ui.drawSquircle().pos(55, 150).radius(8).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(85, 150).radius(10).color(ui.rgb(0, 200, 200));
  ui.drawSquircle().pos(85, 150).radius(10).color(ui.rgb(255, 255, 255));

  ui.drawText("Overlay check (blending):", 170, 128, ui.rgb(200, 180, 160), bg565, AlignCenter);
  ui.fillSquircle().pos(160, 160).radius(24).color(ui.rgb(0, 87, 250));
  ui.fillSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 0, 72));
  ui.fillSquircle().pos(140, 170).radius(24).color(ui.rgb(80, 255, 120));

  ui.drawSquircle().pos(160, 160).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(140, 170).radius(24).color(ui.rgb(255, 255, 255));

  ui.drawText("Check AA edges (no dark halos)", -1, 305, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestArcsAndLines(GUI &ui)
{
  uint16_t bg565 = ui.rgb(28, 24, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Arc & Line Test", -1, 6, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("drawArc + drawLine (AA)", -1, 28, ui.rgb(220, 200, 180), bg565, AlignCenter);

  // Arcs with different angles
  ui.drawText("Arcs (various angles):", 85, 45, ui.rgb(220, 200, 180), bg565, AlignCenter);

  // Quarter arcs
  ui.drawArc().pos(40, 75).radius(18).startDeg(-90).endDeg(0).color(ui.rgb(0, 87, 250));
  ui.drawArc().pos(80, 75).radius(18).startDeg(0).endDeg(90).color(ui.rgb(255, 0, 72));
  ui.drawArc().pos(120, 75).radius(18).startDeg(90).endDeg(180).color(ui.rgb(80, 255, 120));
  ui.drawArc().pos(160, 75).radius(18).startDeg(180).endDeg(270).color(ui.rgb(255, 128, 0));

  // Half arcs
  ui.drawArc().pos(70, 125).radius(22).startDeg(-90).endDeg(90).color(ui.rgb(180, 80, 255));
  ui.drawArc().pos(130, 125).radius(22).startDeg(90).endDeg(270).color(ui.rgb(0, 200, 200));

  // Full circle arc (outline)
  ui.drawArc().pos(180, 125).radius(20).startDeg(0).endDeg(360).color(ui.rgb(200, 200, 80));

  // Small arcs
  ui.drawArc().pos(200, 75).radius(8).startDeg(45).endDeg(135).color(ui.rgb(255, 100, 100));
  ui.drawArc().pos(215, 75).radius(6).startDeg(200).endDeg(340).color(ui.rgb(100, 255, 200));

  // Lines at various angles
  ui.drawText("Lines (AA quality):", 80, 155, ui.rgb(220, 200, 180), bg565, AlignCenter);

  // Horizontal/Vertical
  ui.drawLine().from(20, 175).to(80, 175).color(ui.rgb(255, 255, 255));
  ui.drawLine().from(50, 165).to(50, 185).color(ui.rgb(200, 200, 200));

  // 45-degree diagonals
  ui.drawLine().from(100, 170).to(140, 210).color(ui.rgb(255, 100, 100));
  ui.drawLine().from(140, 170).to(100, 210).color(ui.rgb(100, 255, 100));

  // Steep angles
  ui.drawLine().from(160, 170).to(165, 220).color(ui.rgb(100, 100, 255));
  ui.drawLine().from(180, 170).to(200, 190).color(ui.rgb(255, 200, 100));

  // Long diagonal
  ui.drawLine().from(20, 230).to(100, 280).color(ui.rgb(0, 200, 255));

  // Line thickness test via overlapping
  ui.drawLine().from(140, 240).to(220, 300).color(ui.rgb(255, 255, 255));

  // Arc pie-slices
  ui.drawText("Pie arcs:", 200, 155, ui.rgb(220, 200, 180), bg565, AlignCenter);
  ui.drawArc().pos(200, 200).radius(25).startDeg(-30).endDeg(30).color(ui.rgb(255, 128, 0));
  ui.drawArc().pos(200, 200).radius(20).startDeg(60).endDeg(120).color(ui.rgb(0, 255, 128));
  ui.drawArc().pos(200, 200).radius(15).startDeg(150).endDeg(210).color(ui.rgb(255, 0, 255));

  ui.drawText("Check sqrt_fraction AA!", -1, 305, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenTestAllPrimitivesGrid(GUI &ui)
{
  uint16_t bg565 = ui.rgb(16, 16, 16);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("All Primitives Grid", -1, 4, ui.rgb(255, 255, 255), bg565, AlignCenter);

  // Grid of all primitives for quick comparison - using fluent API
  const int16_t cols = 4;
  const int16_t rows = 5;
  const int16_t cellW = 60;
  const int16_t cellH = 58;
  const int16_t startX = 0;
  const int16_t startY = 28;

  uint16_t colors[] = {
      ui.rgb(0, 87, 250),    // blue
      ui.rgb(255, 0, 72),    // red
      ui.rgb(80, 255, 120),  // green
      ui.rgb(255, 128, 0),   // orange
      ui.rgb(180, 80, 255),  // purple
      ui.rgb(0, 200, 200),   // cyan
      ui.rgb(200, 200, 80),  // yellow
      ui.rgb(255, 100, 100), // pink
  };

  int colorIdx = 0;

  for (int row = 0; row < rows; row++)
  {
    for (int col = 0; col < cols; col++)
    {
      int16_t x = startX + col * cellW + cellW / 2;
      int16_t y = startY + row * cellH + cellH / 2;
      uint16_t c = colors[colorIdx % 8];
      uint16_t white = ui.rgb(255, 255, 255);
      colorIdx++;

      int type = (row * cols + col) % 8;

      if (type == 0) // filled circle
      {
        ui.fillCircle().pos(x, y).radius(14).color(c);
        ui.drawCircle().pos(x, y).radius(14).color(white);
      }
      else if (type == 1) // round rect
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(white);
      }
      else if (type == 2) // ellipse
      {
        ui.fillEllipse().pos(x, y).radiusX(16).radiusY(10).color(c);
        ui.drawEllipse().pos(x, y).radiusX(16).radiusY(10).color(white);
      }
      else if (type == 3) // triangle
      {
        ui.fillTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(c);
        ui.drawTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(white);
      }
      else if (type == 4) // arc
      {
        ui.drawArc().pos(x, y).radius(14).startDeg(0).endDeg(270).color(c);
      }
      else if (type == 5) // 4-radius round rect
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(white);
      }
      else if (type == 6) // tall ellipse
      {
        ui.fillEllipse().pos(x, y).radiusX(8).radiusY(16).color(c);
        ui.drawEllipse().pos(x, y).radiusX(8).radiusY(16).color(white);
      }
      else if (type == 7) // flat triangle
      {
        ui.fillTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(c);
        ui.drawTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(white);
      }
    }
  }

  ui.drawText("All primitives with AA comparison", -1, 318, ui.rgb(200, 200, 100), bg565, AlignCenter);
}

void screenCircleDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(16, 16, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Circle / RoundRect", -1, 8, ui.rgb(255, 255, 255), bg565, AlignCenter);
  ui.setTextStyle(Caption);
  ui.drawText("fillCircle + fillRoundRect", -1, 28, ui.rgb(160, 160, 200), bg565, AlignCenter);

  // Top row: circle + 2 filled round-rects
  ui.fillCircle()
      .pos(48, 78)
      .radius(22)
      .color(ui.rgb(0, 87, 250));

  ui.drawCircle()
      .pos(48, 78)
      .radius(22)
      .color(ui.rgb(255, 255, 255));

  ui.fillRect()
      .pos(88, 52)
      .size(140, 44)
      .color(ui.rgb(80, 255, 120))
      .radius({12});

  ui.fillRect()
      .pos(88, 104)
      .size(140, 44)
      .color(ui.rgb(255, 128, 0))
      .radius({20, 6, 20, 6});

  // Middle row: outlined round-rects (via drawRect fluent)
  ui.drawRect()
      .pos(12, 160)
      .size(100, 44)
      .color(ui.rgb(255, 255, 255))
      .radius({12});

  ui.drawRect()
      .pos(12, 214)
      .size(100, 44)
      .color(ui.rgb(255, 255, 255))
      .radius({20, 6, 20, 6});

  // Right side: fillEllipse + drawArc + drawLine
  ui.fillEllipse()
      .pos(175, 190)
      .radiusX(46)
      .radiusY(20)
      .color(ui.rgb(255, 0, 72));

  ui.drawArc()
      .pos(175, 250)
      .radius(24)
      .startDeg(-90.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(120, 270)
      .to(230, 300)
      .color(ui.rgb(255, 255, 255));

  ui.setTextStyle(Caption);
  ui.drawText("fillCircle", 48, 108, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("fillRoundRect r=12", 158, 46, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("fillRoundRect {20,6,20,6}", 158, 98, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("drawRoundRect r=12", 62, 154, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("drawRoundRect {20,6,20,6}", 62, 208, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("fillEllipse", 175, 216, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("drawArc", 175, 276, ui.rgb(180, 180, 200), bg565, AlignCenter);
  ui.drawText("drawLine", 175, 304, ui.rgb(180, 180, 200), bg565, AlignCenter);

  // Info text
  ui.setTextStyle(Caption);
  ui.drawText("Next/Prev: change screen", -1, 235, ui.rgb(120, 120, 140), bg565, AlignCenter);
}

void screenAutoTextColorDemo(GUI &ui)
{
  uint16_t bg565 = ui.rgb(32, 32, 32);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText("Auto Text Color Test", -1, 4, ui.rgb(255, 255, 255), bg565, AlignCenter);

  // Test colors with different brightness
  uint16_t testColors[] = {
      ui.rgb(255, 255, 255), // white - should get black text
      ui.rgb(200, 200, 200), // light gray
      ui.rgb(128, 128, 128), // mid gray
      ui.rgb(64, 64, 64),    // dark gray
      ui.rgb(0, 0, 0),       // black - should get white text
      ui.rgb(255, 0, 0),     // red
      ui.rgb(0, 255, 0),     // green
      ui.rgb(0, 0, 255),     // blue
      ui.rgb(255, 255, 0),   // yellow
      ui.rgb(0, 255, 255),   // cyan
      ui.rgb(255, 0, 255),   // magenta
      ui.rgb(255, 128, 0),   // orange
  };

  const char *labels[] = {
      "White", "Light", "Mid", "Dark", "Black",
      "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "Orange"};

  int cols = 3;
  int rows = 4;
  int16_t cellW = 76;
  int16_t cellH = 70;
  int16_t startX = 6;
  int16_t startY = 32;

  for (int i = 0; i < 12; i++)
  {
    int col = i % cols;
    int row = i / cols;
    int16_t x = startX + col * cellW;
    int16_t y = startY + row * cellH;

    uint16_t bg = testColors[i];
    uint16_t fg = detail::autoTextColor(bg, 128);

    // Fill background
    ui.fillRect().pos(x, y).size(cellW - 4, cellH - 4).color(bg);

    // Draw text with auto-selected color
    ui.setTextStyle(Body);
    ui.drawText(labels[i], x + (cellW - 4) / 2, y + (cellH - 4) / 2 - 6, fg, bg, AlignCenter);
  }

  ui.setTextStyle(Caption);
  ui.drawText("Text color auto-selected based on bg", -1, 310, ui.rgb(180, 180, 180), bg565, AlignCenter);
}

void setup()
{
  Serial.begin(115200);
  SPIFFS.begin(true);

  ui.configureDisplay()
      .pins({11, 12, 10, 9, 14})
      .hz(80000000)
      .size(240, 320)
      .apply();

  ui.begin(3, 0);
  ui.configureStatusBar(false, 0x0000, 20, Top);
  ui.setStatusBarStyle(StatusBarStyleBlurGradient);
  ui.setStatusBarText("pipGUI", "00:00", "");
  ui.setStatusBarBattery(100, Bar);
  ui.setBacklightPin(4);

  btnNext.begin();
  btnPrev.begin();

  ui.configureTextStyles(24, 18, 14, 12);
  ui.logoSizesPx(36, 24);

  runBootAnimation(ui, btnNext, btnPrev, FadeIn, 900, "PISPPUS", "Fade in");

  ui.regScreens({
      {0, screenMain},
      {1, screenSettings},
      {2, screenGlowDemo},
      {3, screenGraph},
      {4, screenGraphSmall},
      {5, screenGraphTall},
      {6, screenProgressDemo},
      {28, screenProgressTextDemo},
      {7, screenListMenuDemo},
      {8, screenTileMenuDemo},
      {9, screenGraphOsc},
      {10, screenListMenuPlainDemo},
      {11, screenTileMenuLayoutDemo},
      {12, screenTileMenu4ColsDemo},
      {13, screenToggleSwitchDemo},
      {14, screenScrollDotsDemo},
      {15, screenErrorOverlayDemo},
      {16, screenWarningOverlayDemo},
      {17, screenBlurDemo},
      {18, screenDitherDemo},
      {19, screenDitherBlueDemo},
      {20, screenDitherTemporalDemo},
      {21, screenDitherCubesDemo},
      {22, screenDitherCompareGradDemo},
      {23, screenPrimitivesDemo},
      {24, screenFontCompareDemo},
      {25, screenGradientsDemo},
      {26, screenFontWeightDemo},
      {27, screenDrumRollDemo},
      {29, screenCircleDemo},
      {30, screenTestCircles},
      {31, screenTestRoundRects},
      {32, screenTestEllipses},
      {33, screenTestTriangles},
      {34, screenTestArcsAndLines},
      {35, screenTestAllPrimitivesGrid},
      {36, screenTestRoundTriangles},
      {37, screenTestSquircles},
      {38, screenAutoTextColorDemo},
  });

  ui.configureList(7, {
                              {warning_layer0, "Main screen", "Простой экран", 0},
                              {error_layer0, "Settings", "Кнопка и уведомление", 1},
                              {warning_layer0, "Glow demo", "Фигуры со свечением", 2},
                              {error_layer0, "Graph demo", "Сетка и три линии", 3},
                              {error_layer0, "Graph small", "Маленький авто-масштаб", 4},
                              {warning_layer0, "Graph tall", "Высокий график справа", 5},
                              {warning_layer0, "Graph osc", "Цифровой осциллограф", 9},
                              {error_layer0, "Progress demo", "Прогресс-бар с анимациями", 6},
                              {warning_layer0, "Progress + text", "Новые стили прогресса", 28},
                              {error_layer0, "Tile menu", "Плиточное меню (grid)", 8},
                              {warning_layer0, "Tile layout", "Кастомная раскладка", 11},
                              {error_layer0, "Tile 4 cols", "4 плитки в ряд", 12},
                              {warning_layer0, "List menu 2", "Текст + активная плашка", 10},
                              {warning_layer0, "ToggleSwitch", "Переключатель", 13},
                              {error_layer0, "Scroll dots", "Индикатор страниц", 14},
                              {error_layer0, "Error", "Экран ошибки", 15},
                              {warning_layer0, "Warning", "Экран предупреждения", 16},
                              {error_layer0, "Blur strip", "Блюр-полоска", 17},
                              {warning_layer0, "Dither demo", "Gamma + BlueNoise", 18},
                              {error_layer0, "Dither Blue", "BlueNoise only", 19},
                              {warning_layer0, "Dither Temporal", "Temporal FRC", 20},
                              {error_layer0, "Dither Cubes", "Two cubes + avg (16/24)", 21},
                              {warning_layer0, "Dither Compare", "Compare 16-bit vs 24-bit", 22},
                              {error_layer0, "Primitives", "Circle / Arc / Ellipse / Triangle / Squircle", 23},
                              {warning_layer0, "Gradients", "All gradient types", 25},
                              {error_layer0, "Font compare", "PSDF", 24},
                              {warning_layer0, "Font weights", "Test all weights", 26},
                              {error_layer0, "Drum roll", "Horizontal + vertical picker", 27},
                              {warning_layer0, "Circle AA", "Gupta-Sproull optimized", 29},
                              {error_layer0, "Test: Circles", "fillCircle + drawCircle", 30},
                              {warning_layer0, "Test: RoundRects", "1 & 4 radius variants", 31},
                              {error_layer0, "Test: Ellipses", "Wu-style AA ellipses", 32},
                              {warning_layer0, "Test: Triangles", "4x subpixel AA triangles", 33},
                              {error_layer0, "Test: Arcs+Lines", "sqrt_fraction AA", 34},
                              {warning_layer0, "Test: All Grid", "All primitives comparison", 35},
                              {error_layer0, "Test: RoundTriangles", "SDF AA rounded triangles", 36},
                              {warning_layer0, "Test: Squircles", "fill/draw squircle AA", 37},
                              {error_layer0, "Auto text color", "BT.709 luminance test", 38},
                          })
      .parent(0)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .cardSize(310, 50);

  ui.configureList(10, {
                               {warning_layer0, "Back", "К списку карточек", 7},
                               {error_layer0, "Main screen", "Простой экран", 0},
                               {warning_layer0, "Settings", "Кнопка и уведомление", 1},
                               {warning_layer0, "Glow demo", "Фигуры со свечением", 2},
                               {error_layer0, "Graph osc", "Цифровой осциллограф", 9},
                               {error_layer0, "Tile menu", "Плиточное меню (grid)", 8},
                               {warning_layer0, "ToggleSwitch", "Переключатель", 13},
                               {error_layer0, "Dither demo", "Gamma + BlueNoise", 18},
                               {warning_layer0, "Dither Cubes", "Two cubes + avg (16/24)", 21},
                               {error_layer0, "Dither Compare", "Compare 16-bit vs 24-bit", 22},
                               {warning_layer0, "Font compare", "PSDF", 24},
                           })
      .parent(7)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(12)
      .cardSize(310, 34)
      .mode(Plain);

  ui.configureTileMenu(8, 0, {
                                 {"Main", "Главный экран", 0},
                                 {"Settings", "Настройки", 1},
                                 {"Glow demo", "Фигуры со свечением", 2},
                                 {"Graph", "Графики", 3},
                                 {"Toggle", "Switch", 13},
                             });

  ui.setTileMenuStyle(
      8, ui.rgb(8, 8, 8),
      ui.rgb(21, 54, 140),
      13, 10, 2, 100,
      70, 0, 0, 0,
      TextSubtitle);

  ui.configureTileMenu(11, 7, {
                                  {"Back", "", 7},
                                  {"Small 1", "", 0},
                                  {"Small 2", "", 1},
                                  {"Big", "Main", 2},
                              });

  ui.setTileMenuStyle(
      11, ui.rgb(8, 8, 8),
      ui.rgb(21, 54, 140),
      13, 10, 2, 0,
      0, 0, 0, 0,
      TextOnly);

  ui.setTileMenuLayout(11, 2, 3, {
                                     {0, 0, 2, 1},
                                     {0, 1, 1, 1},
                                     {0, 2, 1, 1},
                                     {1, 1, 1, 2},
                                 });

  ui.configureTileMenu(12, 7, {
                                  {"Back", "", 7},
                                  {"1", "", 0},
                                  {"2", "", 1},
                                  {"3", "", 2},
                                  {"4", "", 3},
                                  {"5", "", 4},
                                  {"6", "", 5},
                                  {"7", "", 6},
                                  {"8", "", 9},
                              });

  ui.setTileMenuStyle(
      12, ui.rgb(8, 8, 8),
      ui.rgb(21, 54, 140),
      13, 8, 4, 0,
      0, 0, 0, 0,
      TextOnly);

  ui.setScreen(7);
}

void loop()
{
  ui.loopWithInput(btnNext, btnPrev);

  uint32_t nowMs = millis();
  uint32_t sec = nowMs / 1000U;
  static uint32_t lastMinute = 0xFFFFFFFF;
  uint32_t minute = sec / 60U;

  if (minute != lastMinute)
  {
    lastMinute = minute;
    uint8_t m = minute % 60U;
    uint8_t h = (minute / 60U) % 24U;
    char buf[6];
    snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)h, (unsigned)m);
    ui.setStatusBarText("pipGUI", String(buf), "");
    if (!ui.notificationActive())
      ui.updateStatusBar();
  }

  // Demo: show a toast notification at intervals (cycles through messages, with icon)
  static uint32_t lastToastMs = 0;
  const uint32_t toastIntervalMs = 12000;
  if (!ui.toastActive() && !ui.notificationActive() && nowMs - lastToastMs >= toastIntervalMs)
  {
    lastToastMs = nowMs;
    static const char *const toastMessages[] = {
        "Settings saved",
        "Connected",
        "Sync complete",
        "Ready",
        "pipGUI",
    };
    static const uint8_t toastCount = sizeof(toastMessages) / sizeof(toastMessages[0]);
    uint8_t idx = (uint8_t)((nowMs / toastIntervalMs) % (uint32_t)toastCount);
    bool fromTop = (idx & 1u) != 0;
    IconId iconId = fromTop ? warning_layer0 : error_layer0;

    ui.showToast()
        .text(String(toastMessages[idx]))
        .duration(2500)
        .fromTop(fromTop)
        .icon(iconId, 20)
        .show();
  }

  static uint32_t lastBatteryMs = 0;
  if (nowMs - lastBatteryMs >= 250)
  {
    lastBatteryMs = nowMs;
    if (g_batteryDirDown)
    {
      if (g_batteryLevel > 0)
        g_batteryLevel--;
      else
        g_batteryDirDown = false;
    }
    else
    {
      if (g_batteryLevel < 100)
        g_batteryLevel++;
      else
        g_batteryDirDown = true;
    }
    ui.setStatusBarBattery(g_batteryLevel, Bar);
    if (!ui.notificationActive())
      ui.updateStatusBar();
  }

  bool nextPressed = btnNext.wasPressed();
  bool prevPressed = btnPrev.wasPressed();
  uint8_t cur = ui.currentScreen();

  if (cur == 2 && !ui.notificationActive())
  {
    static uint32_t lastGlowRedrawMs = 0;
    if (nowMs - lastGlowRedrawMs >= 30)
    {
      lastGlowRedrawMs = nowMs;
      uint16_t bg565 = ui.rgb(10, 10, 10);

      ui.updateGlowCircle()
          .pos(70, 95)
          .radius(28)
          .fillColor(ui.rgb(255, 40, 40))
          .bgColor(bg565)
          .glowSize(20)
          .glowStrength(240)
          .anim(Pulse)
          .pulsePeriodMs(900);

      ui.updateGlowRect()
          .pos(center, 70)
          .size(150, 78)
          .radius(18)
          .fillColor(ui.rgb(80, 150, 255))
          .bgColor(bg565)
          .glowSize(18)
          .glowStrength(220)
          .anim(Pulse)
          .pulsePeriodMs(1400);

      ui.updateGlowRect()
          .pos(140, 175)
          .size(150, 78)
          .radius(18)
          .fillColor(ui.rgb(255, 180, 0))
          .bgColor(bg565)
          .glowSize(16)
          .glowStrength(180);
    }
  }

  if (cur == 17 && !ui.notificationActive())
  {
    uint32_t now = millis();
    if (now - g_blurLastUpdateMs >= 1)
    {
      g_blurLastUpdateMs = now;
      g_blurPhase += 0.10f;
      if (g_blurPhase > 1000.0f)
        g_blurPhase -= 1000.0f;

      uint16_t bg565 = ui.rgb(10, 10, 10);
      uint32_t w = ui.screenWidth();
      int16_t statusH = 20;
      int16_t bandY = statusH;
      int16_t bandH = 80;

      ui.fillRect()
          .pos(0, bandY)
          .size((int16_t)w, bandH)
          .color(bg565);

      float t = g_blurPhase;
      for (int i = 0; i < 3; ++i)
      {
        float tt = t * (0.8f + 0.3f * i);
        int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));
        int16_t cy = (int16_t)(bandY + 15 + i * 22 + cosf(tt * 1.3f) * 8.0f);
        int16_t rw = 60 + i * 20;
        int16_t rh = 18;
        uint32_t col;

        if (i == 0)
          col = ui.rgb(255, 80, 80);
        else if (i == 1)
          col = ui.rgb(80, 255, 120);
        else
          col = ui.rgb(80, 255, 140);

        ui.fillRect()
            .pos((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
            .size(rw, rh)
            .color(col);
      }

      static bool s_row2Inited = false;
      static int16_t s_row2PrevX[3];
      static int16_t s_row2PrevY[3];
      static int16_t s_row2PrevW[3];
      static int16_t s_row2PrevH[3];

      if (s_row2Inited)
      {
        for (int i = 0; i < 3; ++i)
          ui.fillRect()
              .pos(s_row2PrevX[i], s_row2PrevY[i])
              .size(s_row2PrevW[i], s_row2PrevH[i])
              .color(bg565);
      }

      for (int i = 0; i < 3; ++i)
      {
        float tt = t * (0.8f + 0.3f * i);
        int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));
        int16_t cy = (int16_t)(bandY + bandH + 10 + i * 18);
        int16_t rw = 60 + i * 20;
        int16_t rh = 16;
        uint32_t col;

        if (i == 0)
          col = ui.rgb(255, 80, 80);
        else if (i == 1)
          col = ui.rgb(80, 255, 120);
        else
          col = ui.rgb(80, 255, 140);

        int16_t x0 = (int16_t)(cx - rw / 2);
        int16_t y0 = (int16_t)(cy - rh / 2);

        s_row2PrevX[i] = x0;
        s_row2PrevY[i] = y0;
        s_row2PrevW[i] = rw;
        s_row2PrevH[i] = rh;

        ui.fillRect()
            .pos(x0, y0)
            .size(rw, rh)
            .color(col);
      }
      s_row2Inited = true;

      ui.updateBlur()
          .pos(0, bandY)
          .size((int16_t)w, bandH)
          .radius(10)
          .direction(TopDown)
          .gradient(false)
          .materialStrength(160)
          .noiseAmount(40)
          .materialColor(ui.rgb(10, 10, 10));
    }
  }

  if ((cur == 3 || cur == 4 || cur == 5 || cur == 9) && !ui.notificationActive())
  {
    uint32_t now = millis();
    if (now - g_lastGraphUpdateMs >= 30)
    {
      g_lastGraphUpdateMs = now;
      updateGraphDemo();

      if (cur == 3 || cur == 4 || cur == 5)
      {
        ui.updateGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
        ui.updateGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
        ui.updateGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
      }
      else
      {
        ui.updateGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
        ui.updateGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
      }
    }
  }

  if (cur == 6 && !ui.notificationActive())
  {
    uint32_t now = millis();
    if (now - g_lastProgressUpdateMs >= 30)
    {
      g_lastProgressUpdateMs = now;

      if (g_progressDirDown)
      {
        if (g_progressValue > 0)
          g_progressValue--;
        else
          g_progressDirDown = false;
      }
      else
      {
        if (g_progressValue < 100)
          g_progressValue++;
        else
          g_progressDirDown = true;
      }

      ui.updateProgressBar()
          .pos(center, 60)
          .size(200, 10)
          .value(0)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(0, 87, 250))
          .radius(6)
          .anim(Indeterminate)
          .doFlush(false);

      ui.updateProgressBar()
          .pos(center, 74)
          .size(200, 10)
          .value(g_progressValue)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(255, 0, 72))
          .radius(6)
          .anim(None)
          .doFlush(false);

      ui.updateProgressBar()
          .pos(center, 88)
          .size(200, 10)
          .value(g_progressValue)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(255, 128, 0))
          .radius(6)
          .anim(Shimmer)
          .doFlush(false);

      ui.updateCircularProgressBar()
          .pos(50, 165)
          .radius(22)
          .thickness(8)
          .value(0)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(0, 87, 250))
          .anim(Indeterminate)
          .doFlush(false);

      ui.updateCircularProgressBar()
          .pos(105, 165)
          .radius(22)
          .thickness(8)
          .value(g_progressValue)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(255, 0, 72))
          .anim(None)
          .doFlush(false);

      ui.updateCircularProgressBar()
          .pos(160, 165)
          .radius(22)
          .thickness(8)
          .value(g_progressValue)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(255, 128, 0))
          .anim(Shimmer)
          .doFlush(false);

      ui.updateCircularProgressBar()
          .pos(215, 165)
          .radius(22)
          .thickness(8)
          .value(g_progressValue)
          .baseColor(ui.rgb(10, 10, 10))
          .fillColor(ui.rgb(0, 200, 120))
          .anim(Shimmer)
          .doFlush(true);
    }
  }

  if (cur == 1 && !ui.notificationActive())
  {
    ui.updateButtonPress(settingsBtnState, btnPrev.isDown());
    uint32_t now = millis();

    if (settingsBtnState.loading && g_settingsLoadingUntil != 0 && now >= g_settingsLoadingUntil)
    {
      settingsBtnState.loading = false;
      g_settingsLoadingUntil = 0;
    }

    if (prevPressed)
    {
      if (!settingsBtnState.loading)
      {
        settingsBtnState.loading = true;
        g_settingsLoadingUntil = now + 1500U;
        ui.showNotification("Settings", "Saving settings...", "OK", 0, Normal);
      }
    }

    const String label = settingsBtnState.loading ? String("Saving") : String("Show modal");
    ui.updateButton()
        .label(label)
        .pos(60, 20)
        .size(120, 44)
        .baseColor(ui.rgb(40, 150, 255))
        .radius(10)
        .state(settingsBtnState);
  }

  if ((cur == 7 || cur == 10) && !ui.notificationActive())
  {
    ui.listInput(cur)
        .nextPressed(nextPressed)
        .prevPressed(prevPressed)
        .nextDown(btnNext.isDown())
        .prevDown(btnPrev.isDown());
  }
  else if ((cur == 8 || cur == 11 || cur == 12) && !ui.notificationActive())
  {
    ui.handleTileMenuInput(cur, nextPressed, prevPressed, btnNext.isDown(), btnPrev.isDown());
  }
  else if (cur == 13 && !ui.notificationActive())
  {
    bool prevVal = g_toggleState.value;
    bool changed = ui.updateToggleSwitch(g_toggleState, nextPressed);

    if (prevPressed)
      ui.setScreen(7);

    if (changed)
    {
      uint16_t bg565 = ui.rgb(10, 10, 10);
      uint16_t active = ui.rgb(21, 180, 110);

      ui.updateToggleSwitch()
          .pos(center, 150)
          .size(78, 36)
          .state(g_toggleState)
          .activeColor(active);

      if (prevVal != g_toggleState.value)
      {
        ui.setTextStyle(Body);
        const char *s = g_toggleState.value ? "ON" : "OFF";
        ui.updateText(String(s), -1, 105, ui.rgb(200, 200, 200), bg565, AlignCenter);
      }
    }
  }
  else if (cur == 14 && !ui.notificationActive())
  {
    if (nextPressed || prevPressed)
    {
      uint8_t prev = g_dotsActive;
      if (nextPressed)
      {
        g_dotsActive = (uint8_t)((g_dotsActive + 1) % g_dotsCount);
        g_dotsDir = 1;
      }
      else
      {
        g_dotsActive = (uint8_t)((g_dotsActive + g_dotsCount - 1) % g_dotsCount);
        g_dotsDir = -1;
      }
      g_dotsPrev = prev;
      g_dotsAnimate = true;
      g_dotsAnimStartMs = millis();
    }

    if (g_dotsAnimate)
    {
      uint32_t now = millis();
      if (now - g_dotsAnimStartMs >= g_dotsAnimDurMs)
        g_dotsAnimate = false;

      float p = 1.0f;
      if (g_dotsAnimDurMs > 0)
      {
        uint32_t el = now - g_dotsAnimStartMs;
        if (el >= g_dotsAnimDurMs)
          p = 1.0f;
        else
          p = (float)el / (float)g_dotsAnimDurMs;
      }

      ui.updateScrollDots()
          .pos(center, 150)
          .count(g_dotsCount)
          .activeIndex(g_dotsActive)
          .prevIndex(g_dotsPrev)
          .animProgress(p)
          .animate(g_dotsAnimate)
          .animDirection(g_dotsDir)
          .activeColor(ui.rgb(0, 87, 250))
          .inactiveColor(ui.rgb(60, 60, 60))
          .dotRadius(3)
          .spacing(14)
          .activeWidth(18);
    }
  }
  else if (cur == 27 && !ui.notificationActive())
  {
    if (nextPressed)
    {
      g_drumPrevH = g_drumSelectedH;
      g_drumSelectedH = (uint8_t)((g_drumSelectedH + 1) % g_drumCountH);
      g_drumAnimateH = true;
      g_drumAnimStartMs = millis();
    }
    else if (prevPressed)
    {
      g_drumPrevH = g_drumSelectedH;
      g_drumSelectedH = (uint8_t)((g_drumSelectedH + g_drumCountH - 1) % g_drumCountH);
      g_drumAnimateH = true;
      g_drumAnimStartMs = millis();
    }
    if (prevPressed)
    {
      g_drumPrevV = g_drumSelectedV;
      g_drumSelectedV = (uint8_t)((g_drumSelectedV + 1) % g_drumCountV);
      g_drumAnimateV = true;
      g_drumAnimStartMs = millis();
    }
    ui.requestRedraw();
  }
  else if ((cur == 15 || cur == 16) && !ui.notificationActive())
  {
    if (!ui.errorActive())
    {
      if (prevPressed)
      {
        ui.setScreen(7);
      }
      else if (nextPressed)
      {
        if (cur == 15)
        {
          ui.showError("Failed to create screen sprite", "Code: 0xFAISPRT", Crash, "OK");
          ui.showError("LittleFS mount failed!", "Code: 0xLFS_MNT", Crash, "OK");
          ui.showError("Sprite creation failed!", "Code: 0xSPR_MEM", Crash, "OK");
        }
        else
        {
          ui.showError("No profile found!", "Code:0xNOPROF", Warning, "OK");
          ui.showError("Network is unstable", "Code:0xNETWARN", Warning, "OK");
          ui.showError("Low battery warning", "Code:0xLOWBATT", Warning, "OK");
        }
      }
    }
  }
  else if (!ui.notificationActive())
  {
    if (nextPressed)
      ui.nextScreen();
  }

  if (ui.errorActive())
  {
    ui.setErrorButtonDown(btnPrev.isDown());
  }
  else if (ui.notificationActive())
  {
    ui.setNotificationButtonDown(btnPrev.isDown());
  }
}
