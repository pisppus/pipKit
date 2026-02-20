#include <pipCore/Displays/ST7789/Display.hpp>
#include <string.h>

namespace pipcore
{
    ST7789Display::~ST7789Display()
    {
    }

    void ST7789Display::writeRect565(int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     const uint16_t *pixels,
                                     int32_t stridePixels)
    {
        if (!pixels)
            return;
        if (w <= 0 || h <= 0)
            return;
        if (stridePixels < w)
            return;

        const int16_t dispW = (int16_t)_drv.width();
        const int16_t dispH = (int16_t)_drv.height();
        if (dispW <= 0 || dispH <= 0)
            return;

        int16_t x0 = x;
        int16_t y0 = y;
        int16_t x1 = x + w - 1;
        int16_t y1 = y + h - 1;

        if (x1 < 0 || y1 < 0 || x0 >= dispW || y0 >= dispH)
            return;

        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= dispW)
            x1 = static_cast<int16_t>(dispW - 1);
        if (y1 >= dispH)
            y1 = static_cast<int16_t>(dispH - 1);

        int16_t clippedW = static_cast<int16_t>(x1 - x0 + 1);
        int16_t clippedH = static_cast<int16_t>(y1 - y0 + 1);

        int16_t skipX = static_cast<int16_t>(x0 - x);
        int16_t skipY = static_cast<int16_t>(y0 - y);
        pixels += static_cast<size_t>(skipY) * static_cast<size_t>(stridePixels) + static_cast<size_t>(skipX);

        _drv.setAddrWindow(static_cast<uint16_t>(x0),
                           static_cast<uint16_t>(y0),
                           static_cast<uint16_t>(x0 + clippedW - 1),
                           static_cast<uint16_t>(y0 + clippedH - 1));

        for (int16_t yy = 0; yy < clippedH; ++yy)
        {
            const uint16_t *row = pixels + static_cast<size_t>(yy) * static_cast<size_t>(stridePixels);
            _drv.writePixels565(row, static_cast<size_t>(clippedW), _drv.swapBytes());
        }
    }
}
