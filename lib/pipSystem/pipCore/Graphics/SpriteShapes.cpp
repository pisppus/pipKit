#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Platforms/GuiDisplay.hpp>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace pipcore
{

    void Sprite::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565)
    {
        if (!_buf || _clipW == 0 || _clipH == 0)
            return;

        if (y0 == y1)
        {
            if (x1 < x0)
                std::swap(x0, x1);
            fillRect(x0, y0, x1 - x0 + 1, 1, color565);
            return;
        }
        if (x0 == x1)
        {
            if (y1 < y0)
                std::swap(y0, y1);
            fillRect(x0, y0, 1, y1 - y0 + 1, color565);
            return;
        }

        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep)
        {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        int32_t dx = x1 - x0;
        int32_t dy = abs(y1 - y0);
        int32_t yStep = (y1 >= y0) ? 1 : -1;

        float invLenSq255 = 255.0f / (float)(dx * dx + dy * dy);

        int32_t err = 0;
        int32_t limit = (dx - 1) / 2;
        int32_t y = y0;
        const uint16_t fg = color565;

        auto plotAlpha = [&](int px, int py, int alpha)
        {
            if (alpha <= 0)
                return;
            if ((uint16_t)(px - _clipX) >= (uint16_t)_clipW)
                return;
            if ((uint16_t)(py - _clipY) >= (uint16_t)_clipH)
                return;

            uint16_t *p = _buf + (size_t)py * _w + px;
            uint16_t bg = __builtin_bswap16(*p);
            *p = __builtin_bswap16(Sprite::blend565(bg, fg, (uint8_t)alpha));
        };

        if (steep)
        {
            for (int x = x0; x <= x1; ++x)
            {
                int e0 = err;
                int e1 = err - dx;
                int e2 = err + dx;

                int a0 = (int)(255.0f - (e0 * e0) * invLenSq255);
                int a1 = (int)(255.0f - (e1 * e1) * invLenSq255);
                int a2 = (int)(255.0f - (e2 * e2) * invLenSq255);

                plotAlpha(y, x, a0);
                plotAlpha(y + yStep, x, a1);
                plotAlpha(y - yStep, x, a2);

                int next_err = err + dy;
                if (next_err > limit)
                {
                    y += yStep;
                    err = next_err - dx;
                }
                else
                {
                    err = next_err;
                }
            }
        }
        else
        {
            for (int x = x0; x <= x1; ++x)
            {
                int e0 = err;
                int e1 = err - dx;
                int e2 = err + dx;

                int a0 = (int)(255.0f - (e0 * e0) * invLenSq255);
                int a1 = (int)(255.0f - (e1 * e1) * invLenSq255);
                int a2 = (int)(255.0f - (e2 * e2) * invLenSq255);

                plotAlpha(x, y, a0);
                plotAlpha(x, y + yStep, a1);
                plotAlpha(x, y - yStep, a2);

                int next_err = err + dy;
                if (next_err > limit)
                {
                    y += yStep;
                    err = next_err - dx;
                }
                else
                {
                    err = next_err;
                }
            }
        }
    }

    void Sprite::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565)
    {
        if (w <= 0 || h <= 0)
            return;

        fillRect(x, y, w, 1, color565);
        fillRect(x, y + h - 1, w, 1, color565);

        if (h > 2)
        {
            fillRect(x, y + 1, 1, h - 2, color565);
            fillRect(x + w - 1, y + 1, 1, h - 2, color565);
        }
    }

    void Sprite::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565)
    {
        if (!_buf || w <= 0 || h <= 0)
            return;

        if (radius <= 0)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

        int16_t maxR = (w < h ? w : h) >> 1;
        if (radius > maxR)
            radius = maxR;

        fillRect(x, y + radius, w, h - (radius << 1), color565);

        uint16_t fg = color565;
        int32_t r2 = radius * radius;

        int16_t leftCx = x + radius;
        int16_t rightCx = x + w - radius - 1;
        int16_t topCy = y + radius;
        int16_t botCy = y + h - radius - 1;

        auto drawAA = [&](int16_t px, int16_t py, uint8_t alpha)
        {
            if ((uint16_t)(px - _clipX) < (uint16_t)_clipW &&
                (uint16_t)(py - _clipY) < (uint16_t)_clipH)
            {
                uint16_t *p = _buf + (size_t)py * _w + px;
                uint16_t bg = __builtin_bswap16(*p);
                *p = __builtin_bswap16(Sprite::blend565(bg, fg, alpha));
            }
        };

        for (int16_t dy = 0; dy < radius; dy++)
        {
            float fy = (float)(radius - dy);
            float xf = sqrtf((float)r2 - fy * fy);
            int16_t xi = (int16_t)xf;

            int16_t xOffset = radius - xi;
            int16_t lineWidth = w - (xOffset << 1);

            if (lineWidth > 0)
            {
                fillRect(x + xOffset, y + dy, lineWidth, 1, color565);
                fillRect(x + xOffset, y + h - 1 - dy, lineWidth, 1, color565);
            }

            uint8_t alpha = (uint8_t)((xf - (float)xi) * 255.0f);
            if (alpha > 0)
            {
                int16_t axL = leftCx - xi - 1;
                int16_t axR = rightCx + xi + 1;
                drawAA(axL, topCy - dy, alpha);
                drawAA(axR, topCy - dy, alpha);
                drawAA(axL, botCy + dy, alpha);
                drawAA(axR, botCy + dy, alpha);
            }
        }
    }

    void Sprite::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565)
    {
        if (w <= 0 || h <= 0)
            return;
        if (radius <= 0)
        {
            drawRect(x, y, w, h, color565);
            return;
        }

        int16_t r = radius;
        if (r * 2 > w)
            r = static_cast<int16_t>(w / 2);
        if (r * 2 > h)
            r = static_cast<int16_t>(h / 2);
        if (r <= 0)
        {
            drawRect(x, y, w, h, color565);
            return;
        }

        fillRect(x + r, y, w - r * 2, 1, color565);
        fillRect(x + r, y + h - 1, w - r * 2, 1, color565);
        fillRect(x, y + r, 1, h - r * 2, color565);
        fillRect(x + w - 1, y + r, 1, h - r * 2, color565);

        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t px = 0;
        int16_t py = r;

        int16_t xl = x + r;
        int16_t xr = x + w - r - 1;
        int16_t yt = y + r;
        int16_t yb = y + h - r - 1;

        while (px < py)
        {
            if (f >= 0)
            {
                py--;
                ddF_y += 2;
                f += ddF_y;
            }
            px++;
            ddF_x += 2;
            f += ddF_x;

            drawPixel(xr + px, yt - py, color565);
            drawPixel(xl - px, yt - py, color565);
            drawPixel(xr + px, yb + py, color565);
            drawPixel(xl - px, yb + py, color565);

            drawPixel(xr + py, yt - px, color565);
            drawPixel(xl - py, yt - px, color565);
            drawPixel(xr + py, yb + px, color565);
            drawPixel(xl - py, yb + px, color565);
        }
    }

    void Sprite::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color565)
    {
        if (!_buf || r <= 0)
            return;

        const uint16_t fg = color565;
        const float rf = (float)r;
        const float rrF = rf * rf;

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if ((uint16_t)(px - _clipX) < (uint16_t)_clipW &&
                (uint16_t)(py - _clipY) < (uint16_t)_clipH)
            {
                uint16_t *p = _buf + (size_t)py * _w + px;
                uint16_t bg = __builtin_bswap16(*p);
                *p = __builtin_bswap16(Sprite::blend565(bg, fg, a));
            }
        };

        for (int16_t dy = -r; dy <= r; ++dy)
        {
            float yf = (float)dy;
            float xf = sqrtf(rrF - yf * yf);
            int16_t xi = (int16_t)xf;

            if (xi >= 0)
            {
                fillRect(x - xi, y + dy, xi * 2 + 1, 1, color565);
            }

            uint8_t a = (uint8_t)((xf - (float)xi) * 255.0f);
            if (a > 0)
            {
                blendPixel(x + xi + 1, y + dy, a);
                blendPixel(x - xi - 1, y + dy, a);
            }
        }

        for (int16_t dx = -r; dx <= r; ++dx)
        {
            float xf = (float)dx;
            float yf = sqrtf(rrF - xf * xf);
            int16_t yi = (int16_t)yf;
            uint8_t a = (uint8_t)((yf - (float)yi) * 255.0f);

            if (a > 0)
            {
                blendPixel(x + dx, y + yi + 1, a);
                blendPixel(x + dx, y - yi - 1, a);
            }
        }
    }

}