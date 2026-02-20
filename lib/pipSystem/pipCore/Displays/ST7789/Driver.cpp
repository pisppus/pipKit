#include <pipCore/Displays/ST7789/Driver.hpp>
#include <string.h>

namespace pipcore
{
    namespace
    {
        constexpr uint8_t CmdSWRESET = 0x01;
        constexpr uint8_t CmdSLPOUT = 0x11;
        constexpr uint8_t CmdNORON = 0x13;
        constexpr uint8_t CmdINVOFF = 0x20;
        constexpr uint8_t CmdINVON = 0x21;
        constexpr uint8_t CmdDISPON = 0x29;
        constexpr uint8_t CmdCASET = 0x2A;
        constexpr uint8_t CmdRASET = 0x2B;
        constexpr uint8_t CmdRAMWR = 0x2C;
        constexpr uint8_t CmdMADCTL = 0x36;
        constexpr uint8_t CmdCOLMOD = 0x3A;
        constexpr uint8_t Colmod16bpp = 0x55;
        constexpr uint8_t MadctlMY = 0x80;
        constexpr uint8_t MadctlMX = 0x40;
        constexpr uint8_t MadctlMV = 0x20;
        constexpr uint8_t MadctlBGR = 0x08;

        inline uint16_t bswap16(uint16_t v)
        {
            return static_cast<uint16_t>(__builtin_bswap16(v));
        }
    }

    bool ST7789::configure(ISt7789Transport *transport,
                           uint16_t width,
                           uint16_t height,
                           uint8_t order,
                           bool invert,
                           bool swap,
                           int16_t xOffset,
                           int16_t yOffset)
    {
        if (!transport)
            return false;
        if (width == 0 || height == 0)
            return false;
        if (xOffset < 0 || yOffset < 0)
            return false;

        _transport = transport;
        _width = width;
        _height = height;
        _physWidth = width;
        _physHeight = height;
        _xStart = xOffset;
        _yStart = yOffset;
        _xOffsetCfg = xOffset;
        _yOffsetCfg = yOffset;
        _order = (order == 1) ? 1 : 0;
        _invert = invert;
        _swap = swap;
        _initialized = false;

        return true;
    }

    void ST7789::hardReset()
    {
        if (!_transport)
            return;

        _transport->setRst(0);
        _transport->delayMs(20);
        _transport->setRst(1);
        _transport->delayMs(150);
    }

    bool ST7789::begin(uint8_t rotation)
    {
        if (!_transport)
        {
            return false;
        }
        if (_width == 0 || _height == 0)
        {
            return false;
        }

        if (!_transport->init())
        {
            return false;
        }

        hardReset();

        _transport->writeCommand(CmdSWRESET);
        _transport->delayMs(150);

        _transport->writeCommand(CmdSLPOUT);
        _transport->delayMs(200);

        _transport->writeCommand(CmdCOLMOD);
        {
            uint8_t v = Colmod16bpp;
            _transport->write(&v, 1);
        }
        _transport->delayMs(10);

        _transport->writeCommand(_invert ? CmdINVON : CmdINVOFF);
        _transport->delayMs(10);

        _transport->writeCommand(CmdNORON);
        _transport->delayMs(10);

        setRotationInternal(rotation);

        _transport->writeCommand(CmdDISPON);
        _transport->delayMs(100);

        _initialized = true;
        return true;
    }

    void ST7789::setInversion(bool enabled)
    {
        _invert = enabled;
        if (!_transport || !_initialized)
            return;

        _transport->writeCommand(_invert ? CmdINVON : CmdINVOFF);
        _transport->delayMs(10);
    }

    void ST7789::setRotationInternal(uint8_t rotation)
    {
        _rotation = rotation & 3U;

        uint8_t madctl = (_order == 1) ? MadctlBGR : 0;

        switch (_rotation)
        {
        case 0: // Портрет 0°
            madctl = (_order == 1) ? MadctlBGR : 0;
            _width = _physWidth;
            _height = _physHeight;
            _xStart = _xOffsetCfg;
            _yStart = _yOffsetCfg;
            break;

        case 1: // Альбом 90°
            madctl = (uint8_t)(MadctlMX | MadctlMV | ((_order == 1) ? MadctlBGR : 0));
            _width = _physHeight;
            _height = _physWidth;
            _xStart = _yOffsetCfg;
            _yStart = _xOffsetCfg;
            break;

        case 2: // Портрет 180°
            madctl = (uint8_t)(MadctlMX | MadctlMY | ((_order == 1) ? MadctlBGR : 0));
            _width = _physWidth;
            _height = _physHeight;
            _xStart = _xOffsetCfg;
            _yStart = _yOffsetCfg;
            break;

        case 3: // Альбом 270°
            madctl = (uint8_t)(MadctlMV | MadctlMY | ((_order == 1) ? MadctlBGR : 0));
            _width = _physHeight;
            _height = _physWidth;
            _xStart = _yOffsetCfg;
            _yStart = _xOffsetCfg;
            break;
        }

        _transport->writeCommand(CmdMADCTL);
        _transport->write(&madctl, 1);
    }

    void ST7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        if (!_transport)
            return;

        if (x1 < x0 || y1 < y0)
            return;

        if (_width == 0 || _height == 0)
            return;

        if (x0 >= _width || y0 >= _height)
            return;

        if (x1 >= _width)
            x1 = static_cast<uint16_t>(_width - 1U);
        if (y1 >= _height)
            y1 = static_cast<uint16_t>(_height - 1U);

        const int32_t xs32 = static_cast<int32_t>(x0) + static_cast<int32_t>(_xStart);
        const int32_t xe32 = static_cast<int32_t>(x1) + static_cast<int32_t>(_xStart);
        const int32_t ys32 = static_cast<int32_t>(y0) + static_cast<int32_t>(_yStart);
        const int32_t ye32 = static_cast<int32_t>(y1) + static_cast<int32_t>(_yStart);

        if (xs32 < 0 || ys32 < 0)
            return;

        const uint16_t xs = static_cast<uint16_t>(xs32);
        const uint16_t xe = static_cast<uint16_t>(xe32);
        const uint16_t ys = static_cast<uint16_t>(ys32);
        const uint16_t ye = static_cast<uint16_t>(ye32);

        uint8_t buf[4];

        _transport->writeCommand(CmdCASET);
        buf[0] = static_cast<uint8_t>(xs >> 8);
        buf[1] = static_cast<uint8_t>(xs & 0xFF);
        buf[2] = static_cast<uint8_t>(xe >> 8);
        buf[3] = static_cast<uint8_t>(xe & 0xFF);
        _transport->write(buf, sizeof(buf));

        _transport->writeCommand(CmdRASET);
        buf[0] = static_cast<uint8_t>(ys >> 8);
        buf[1] = static_cast<uint8_t>(ys & 0xFF);
        buf[2] = static_cast<uint8_t>(ye >> 8);
        buf[3] = static_cast<uint8_t>(ye & 0xFF);
        _transport->write(buf, sizeof(buf));

        _transport->writeCommand(CmdRAMWR);
    }

    void ST7789::writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes)
    {
        if (!_transport || !pixelCount)
            return;

        if (!swapBytes)
        {
            constexpr size_t ChunkPixels = 1024;

            const uint16_t *p = pixels;
            size_t remaining = pixelCount;
            while (remaining)
            {
                const size_t n = remaining > ChunkPixels ? ChunkPixels : remaining;
                _transport->writePixels(p, n * sizeof(uint16_t));
                p += n;
                remaining -= n;
            }
            return;
        }

        constexpr size_t SwapChunkPixels = 256;
        uint16_t tmp[SwapChunkPixels];

        const uint16_t *p = pixels;
        size_t remaining = pixelCount;
        while (remaining)
        {
            const size_t n = remaining > SwapChunkPixels ? SwapChunkPixels : remaining;
            for (size_t i = 0; i < n; ++i)
                tmp[i] = bswap16(p[i]);
            _transport->writePixels(tmp, n * sizeof(uint16_t));
            p += n;
            remaining -= n;
        }
    }

    void ST7789::fillScreen565(uint16_t color565, bool swapBytes)
    {
        if (!_transport)
            return;

        setAddrWindow(0, 0, static_cast<uint16_t>(_width - 1U), static_cast<uint16_t>(_height - 1U));

        constexpr size_t ChunkPixels = 256;
        uint16_t tmp[ChunkPixels];

        const uint16_t v = swapBytes ? bswap16(color565) : color565;
        for (size_t i = 0; i < ChunkPixels; ++i)
            tmp[i] = v;

        const size_t total = static_cast<size_t>(_width) * static_cast<size_t>(_height);
        size_t remaining = total;
        while (remaining)
        {
            const size_t n = remaining > ChunkPixels ? ChunkPixels : remaining;
            _transport->writePixels(tmp, n * sizeof(uint16_t));
            remaining -= n;
        }

        _transport->flush();
    }

    void ST7789::writeCmd(uint8_t cmd)
    {
        if (_transport)
            _transport->writeCommand(cmd);
    }

    void ST7789::writeData(const void *data, size_t len)
    {
        if (_transport)
            _transport->write(data, len);
    }

    void ST7789::writeDataQueued(const void *data, size_t len)
    {
        if (_transport)
            _transport->writePixels(data, len);
    }
}