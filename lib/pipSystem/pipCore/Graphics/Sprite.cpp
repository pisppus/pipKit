#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Platforms/GuiDisplay.hpp>
#include <cstdlib>
#include <cstring>
#include <algorithm>

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
        _clipX = _clipY = _clipW = _clipH = 0;
    }

    void Sprite::clipNormalize()
    {
        if (_w <= 0 || _h <= 0)
        {
            _clipX = _clipY = _clipW = _clipH = 0;
            return;
        }

        int16_t x1 = std::max<int16_t>(0, _clipX);
        int16_t y1 = std::max<int16_t>(0, _clipY);
        int16_t x2 = std::min<int16_t>(_w, _clipX + std::max<int16_t>(0, _clipW));
        int16_t y2 = std::min<int16_t>(_h, _clipY + std::max<int16_t>(0, _clipH));

        _clipX = x1;
        _clipY = y1;
        _clipW = std::max<int16_t>(0, x2 - x1);
        _clipH = std::max<int16_t>(0, y2 - y1);
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
        return (x >= _clipX && y >= _clipY && x < (_clipX + _clipW) && y < (_clipY + _clipH));
    }

    void Sprite::pushSprite(Sprite *dst, int16_t x, int16_t y) const
    {
        if (!dst || !_buf || !dst->_buf)
            return;

        int16_t dstX1 = std::max<int16_t>(x, dst->_clipX);
        int16_t dstY1 = std::max<int16_t>(y, dst->_clipY);
        int16_t dstX2 = std::min<int16_t>(x + _w, dst->_clipX + dst->_clipW);
        int16_t dstY2 = std::min<int16_t>(y + _h, dst->_clipY + dst->_clipH);

        int16_t copyW = dstX2 - dstX1;
        int16_t copyH = dstY2 - dstY1;

        if (copyW <= 0 || copyH <= 0)
            return;

        int16_t srcX = dstX1 - x;
        int16_t srcY = dstY1 - y;

        const uint16_t *srcPtr = _buf + (size_t)srcY * _w + srcX;
        uint16_t *dstPtr = dst->_buf + (size_t)dstY1 * dst->_w + dstX1;

        const size_t bytesPerLine = copyW * sizeof(uint16_t);
        const size_t srcStride = _w;
        const size_t dstStride = dst->_w;

        for (int16_t yy = 0; yy < copyH; ++yy)
        {
            memcpy(dstPtr, srcPtr, bytesPerLine);
            srcPtr += srcStride;
            dstPtr += dstStride;
        }
    }

    void Sprite::writeToDisplay(GuiDisplay &display, int16_t x, int16_t y, int16_t w, int16_t h) const
    {
        if (!_buf || w <= 0 || h <= 0)
            return;

        int16_t x1 = std::max<int16_t>(x, _clipX);
        int16_t y1 = std::max<int16_t>(y, _clipY);
        int16_t x2 = std::min<int16_t>(x + w, _clipX + _clipW);
        int16_t y2 = std::min<int16_t>(y + h, _clipY + _clipH);

        int16_t clippedW = x2 - x1;
        int16_t clippedH = y2 - y1;

        if (clippedW <= 0 || clippedH <= 0)
            return;

        const uint16_t *pixels = _buf + (size_t)y1 * _w + x1;

        display.writeRect565(x1, y1, clippedW, clippedH, pixels, _w);
    }
}