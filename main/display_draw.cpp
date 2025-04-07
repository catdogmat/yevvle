#include "display.h"

std::vector<std::pair<uint8_t, uint8_t>> calcEllipse(int x, int y, uint8_t width, uint8_t height)
{
    int x1 = -width, y1 = 0; // II quadrant from bottom left to top right
    int e2 = height, dx = (1 + 2 * x1) * e2 * e2; // error increment
    int dy = x1 * x1, err = dx + dy; // error of 1 step

    std::vector<std::pair<uint8_t, uint8_t>> lines;
    lines.reserve(height);

    do {
        e2 = 2 * err;

        if (e2 >= dx) {
            x1++;
            err += dx += 2 * height * height;
        } // x1 step

        if (e2 <= dy) {
            // Whenever we advance to next line we print the previous line
            lines.emplace_back(y1, -x1);
            y1++;
            err += dy += 2 * width * width;
        } // y1 step
    } while (x1 <= 0);

    // -> finish tip of ellipse ?
    // display.drawLine(x, y - y1, x, y + y1, on);
    return lines;
}

void Display::drawEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on)
{
    for (auto& [y1, x1] : calcEllipse(x, y, width, height)) {
        drawLine(x + x1, y - y1, x - x1, y - y1, on);
        drawLine(x + x1, y + y1, x - x1, y + y1, on);
    }
}

void Display::drawEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t color)
{
    auto elipse1 = calcEllipse(x, y, width1, height);
    auto elipse2 = calcEllipse(x, y, width2, height);

    for (auto i = 0; i < elipse1.size(); i++) {
        auto& [y11, x11] = elipse1[i];
        auto& [y12, x12] = elipse2[i];
        // ESP_LOGE("", "%d %d %d %d", y11, y12, x11, x12);
        if (big) {
            drawLine(x - x11, y - y11, x + x12, y - y11, color);
            drawLine(x - x11, y + y11, x + x12, y + y11, color);
        } else {
            drawLine(x + x11, y - y11, x + x12, y - y11, color);
            drawLine(x + x11, y + y11, x + x12, y + y11, color);
        }
    }
}

void Display::drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off)
{
  if (p < 25)
    drawEllipseDifference(x, y, r * (25 - p) / 25, r, r, false, off);
  else if (p < 50)
    drawEllipseDifference(x, y, r * (p - 25) / 25, r, r, true, off);
  else if (p < 75)
    drawEllipseDifference(x, y, r, r * (75 - p) / 25, r, true, off);
  else
    drawEllipseDifference(x, y, r, r * (p - 75) / 25, r, false, off);
}

void Display::drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off) 
{
  //drawEllipseDifference(x, y, radius * (25 - p) / 25, radius, radius, radius, on);
  
  startWrite();
  fillCircleHelper(x, y, radius, p < 50 ? 2 : 1, 0, on);
  fillCircleHelper(x, y, radius, p < 50 ? 1 : 2, 0, off);
  endWrite();
  
  if (p < 25)
    drawEllipse(x, y, radius * (25 - p) / 25, radius, on);
  else if (p < 50)
    drawEllipse(x, y, radius * (p - 25) / 25, radius, off);
  else if (p < 75)
    drawEllipse(x, y, radius * (75 - p) / 25, radius, off);
  else
    drawEllipse(x, y, radius * (p - 75) / 25, radius, on);
}