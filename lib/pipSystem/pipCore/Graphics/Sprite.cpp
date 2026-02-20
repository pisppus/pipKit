#include <pipCore/Graphics/Sprite.hpp>

#include <pipCore/Platforms/GuiDisplay.hpp>

#include <stdlib.h>
#include <string.h>

namespace pipcore
{
    Sprite::~Sprite()
    {
        deleteSprite();
    }

    bool Sprite::createSprite(int16_t w, int16_t h)
    {
        deleteSprite();

        if (w <= 0 || h <= 0)
            return false;

        const size_t pixels = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (pixels == 0 || pixels > (SIZE_MAX / sizeof(uint16_t)))
            return false;

        _buf = static_cast<uint16_t *>(malloc(pixels * sizeof(uint16_t)));
        if (!_buf)
            return false;

        _w = w;
        _h = h;
        _clipX = 0;
        _clipY = 0;
        _clipW = w;
        _clipH = h;

        return true;
    }

    void Sprite::deleteSprite()
    {
        if (_buf)
        {
            free(_buf);
            _buf = nullptr;
        }
        _w = 0;
        _h = 0;
        _clipX = 0;
        _clipY = 0;
        _clipW = 0;
        _clipH = 0;
    }

    void Sprite::clipNormalize()
    {
        if (_w <= 0 || _h <= 0)
        {
            _clipX = 0;
            _clipY = 0;
            _clipW = 0;
            _clipH = 0;
            return;
        }

        if (_clipW < 0)
            _clipW = 0;
        if (_clipH < 0)
            _clipH = 0;

        if (_clipX < 0)
        {
            _clipW = static_cast<int16_t>(_clipW + _clipX);
            _clipX = 0;
        }
        if (_clipY < 0)
        {
            _clipH = static_cast<int16_t>(_clipH + _clipY);
            _clipY = 0;
        }

        if (_clipX + _clipW > _w)
            _clipW = static_cast<int16_t>(_w - _clipX);
        if (_clipY + _clipH > _h)
            _clipH = static_cast<int16_t>(_h - _clipY);

        if (_clipW < 0)
            _clipW = 0;
        if (_clipH < 0)
            _clipH = 0;
    }

    void Sprite::setClipRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        _clipX = x;
        _clipY = y;
        _clipW = w;
        _clipH = h;
        clipNormalize();
    }

    void Sprite::getClipRect(int32_t *x, int32_t *y, int32_t *w, int32_t *h) const
    {
        if (x)
            *x = _clipX;
        if (y)
            *y = _clipY;
        if (w)
            *w = _clipW;
        if (h)
            *h = _clipH;
    }

    bool Sprite::clipTest(int16_t x, int16_t y) const
    {
        if (x < _clipX || y < _clipY)
            return false;
        if (x >= _clipX + _clipW)
            return false;
        if (y >= _clipY + _clipH)
            return false;
        return true;
    }

    void Sprite::pushSprite(Sprite *dst, int16_t x, int16_t y) const
    {
        if (!dst)
            return;
        if (!_buf || !dst->_buf)
            return;

        const int16_t srcW = _w;
        const int16_t srcH = _h;
        const int16_t dstW = dst->_w;
        const int16_t dstH = dst->_h;

        int16_t sx0 = 0;
        int16_t sy0 = 0;
        int16_t dx0 = x;
        int16_t dy0 = y;

        int16_t w = srcW;
        int16_t h = srcH;

        if (dx0 < 0)
        {
            sx0 = static_cast<int16_t>(-dx0);
            w = static_cast<int16_t>(w + dx0);
            dx0 = 0;
        }
        if (dy0 < 0)
        {
            sy0 = static_cast<int16_t>(-dy0);
            h = static_cast<int16_t>(h + dy0);
            dy0 = 0;
        }

        if (dx0 + w > dstW)
            w = static_cast<int16_t>(dstW - dx0);
        if (dy0 + h > dstH)
            h = static_cast<int16_t>(dstH - dy0);

        int16_t clipX = dst->_clipX;
        int16_t clipY = dst->_clipY;
        int16_t clipW = dst->_clipW;
        int16_t clipH = dst->_clipH;
        if (clipW < 0)
            clipW = 0;
        if (clipH < 0)
            clipH = 0;

        int16_t clipX2 = static_cast<int16_t>(clipX + clipW);
        int16_t clipY2 = static_cast<int16_t>(clipY + clipH);

        if (dx0 < clipX)
        {
            int16_t d = static_cast<int16_t>(clipX - dx0);
            sx0 = static_cast<int16_t>(sx0 + d);
            w = static_cast<int16_t>(w - d);
            dx0 = clipX;
        }
        if (dy0 < clipY)
        {
            int16_t d = static_cast<int16_t>(clipY - dy0);
            sy0 = static_cast<int16_t>(sy0 + d);
            h = static_cast<int16_t>(h - d);
            dy0 = clipY;
        }
        if (dx0 + w > clipX2)
            w = static_cast<int16_t>(clipX2 - dx0);
        if (dy0 + h > clipY2)
            h = static_cast<int16_t>(clipY2 - dy0);

        if (w <= 0 || h <= 0)
            return;

        const size_t srcStride = static_cast<size_t>(srcW);
        const size_t dstStride = static_cast<size_t>(dstW);

        for (int16_t yy = 0; yy < h; ++yy)
        {
            const uint16_t *s = _buf + static_cast<size_t>(sy0 + yy) * srcStride + static_cast<size_t>(sx0);
            uint16_t *d = dst->_buf + static_cast<size_t>(dy0 + yy) * dstStride + static_cast<size_t>(dx0);
            memcpy(d, s, static_cast<size_t>(w) * sizeof(uint16_t));
        }
    }

    void Sprite::writeToDisplay(GuiDisplay &display, int16_t x, int16_t y, int16_t w, int16_t h) const
    {
        if (!_buf)
            return;
        if (w <= 0 || h <= 0)
            return;

        int16_t x0 = x;
        int16_t y0 = y;
        int16_t x1 = x + w - 1;
        int16_t y1 = y + h - 1;

        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= _w)
            x1 = static_cast<int16_t>(_w - 1);
        if (y1 >= _h)
            y1 = static_cast<int16_t>(_h - 1);

        if (x0 > x1 || y0 > y1)
            return;

        int16_t clippedW = static_cast<int16_t>(x1 - x0 + 1);
        int16_t clippedH = static_cast<int16_t>(y1 - y0 + 1);

        const uint16_t *pixels = _buf + static_cast<size_t>(y0) * static_cast<size_t>(_w) + static_cast<size_t>(x0);

        display.writeRect565(x0, y0, clippedW, clippedH, pixels, _w);
    }
}
