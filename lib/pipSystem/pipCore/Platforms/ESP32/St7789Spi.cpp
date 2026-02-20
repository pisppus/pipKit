#include <Arduino.h>
#include <pipCore/Platforms/ESP32/St7789Spi.hpp>

#include <string.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <esp_rom_sys.h>
#include <esp_heap_caps.h>

#if __has_include(<soc/spi_periph.h>)
#include <soc/spi_periph.h>
#endif

namespace pipcore
{
    // Anonymous namespace for internal linkage - callback must be defined before use
    namespace
    {
        constexpr size_t DmaBufferBytes = 8192;

        inline bool isPinValid(int8_t pin)
        {
            return pin >= 0;
        }

#if __has_include(<soc/spi_periph.h>)
        inline int8_t getSpi2IomuxMosi()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spid_iomux_pin;
#else
            return -1;
#endif
        }

        inline int8_t getSpi2IomuxSclk()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spiclk_iomux_pin;
#else
            return -1;
#endif
        }

        inline int8_t getSpi2IomuxCs0()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spics0_iomux_pin;
#else
            return -1;
#endif
        }
#endif

        int8_t resolveDefaultMosi()
        {
#if __has_include(<soc/spi_periph.h>)
            {
                int8_t pin = getSpi2IomuxMosi();
                if (isPinValid(pin))
                    return pin;
            }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
            return 13;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
            return 11;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
            return 11;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
            return 7;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
            return 7;
#else
            return -1;
#endif
        }

        int8_t resolveDefaultSclk()
        {
#if __has_include(<soc/spi_periph.h>)
            {
                int8_t pin = getSpi2IomuxSclk();
                if (isPinValid(pin))
                    return pin;
            }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
            return 14;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
            return 12;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
            return 12;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
            return 6;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
            return 6;
#else
            return -1;
#endif
        }

        int8_t resolveDefaultCs()
        {
#if __has_include(<soc/spi_periph.h>)
            {
                int8_t pin = getSpi2IomuxCs0();
                if (isPinValid(pin))
                    return pin;
            }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
            return 15;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
            return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
            return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
            return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
            return 10;
#else
            return -1;
#endif
        }
    }

    Esp32St7789Spi::Esp32St7789Spi(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint32_t hz)
        : _pinMosi(mosi),
          _pinSclk(sclk),
          _pinCs(cs),
          _pinDc(dc),
          _pinRst(rst),
          _hz(hz ? hz : 80000000U),
          _spiHandle(nullptr),
          _dmaBufSize(DmaBufferBytes),
          _dmaBuf{nullptr, nullptr},
          _trans{nullptr, nullptr},
          _transInFlight{false, false},
          _dmaNext(0),
          _dmaInflight(0),
          _initialized(false)
    {
        // Resolve default pins if not specified
        if (!isPinValid(_pinMosi))
            _pinMosi = resolveDefaultMosi();
        if (!isPinValid(_pinSclk))
            _pinSclk = resolveDefaultSclk();
        if (!isPinValid(_pinCs))
            _pinCs = resolveDefaultCs();
    }

    Esp32St7789Spi::~Esp32St7789Spi()
    {
        deinit();
    }

    bool Esp32St7789Spi::init()
    {
        if (_initialized)
            return true;

        if (!isPinValid(_pinDc))
            return false;

        if (!initSpi())
            return false;

        // Configure GPIO for DC and RST
        gpio_config_t io{};
        io.intr_type = GPIO_INTR_DISABLE;
        io.mode = GPIO_MODE_OUTPUT;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.pull_up_en = GPIO_PULLUP_DISABLE;

        io.pin_bit_mask = (1ULL << static_cast<uint8_t>(_pinDc));
        if (isPinValid(_pinRst))
            io.pin_bit_mask |= (1ULL << static_cast<uint8_t>(_pinRst));

        gpio_config(&io);

        _initialized = true;
        return true;
    }

    void Esp32St7789Spi::deinit()
    {
        // Note: This can be called even if _initialized is false
        // (e.g., during failed init), so we check pointers, not the flag

        if (_spiHandle)
        {
            // Flush any pending transactions before cleanup
            flush();
            spi_bus_remove_device(static_cast<spi_device_handle_t>(_spiHandle));
            spi_bus_free(SPI2_HOST);
            _spiHandle = nullptr;
        }

        for (int i = 0; i < 2; ++i)
        {
            if (_dmaBuf[i])
            {
                heap_caps_free(_dmaBuf[i]);
                _dmaBuf[i] = nullptr;
            }

            if (_trans[i])
            {
                heap_caps_free(_trans[i]);
                _trans[i] = nullptr;
            }

            _transInFlight[i] = false;
        }

        _dmaNext = 0;
        _dmaInflight = 0;
        _initialized = false;
    }

    bool Esp32St7789Spi::initSpi()
    {
        if (_spiHandle)
            return true;

        spi_bus_config_t bus{};
        bus.mosi_io_num = _pinMosi;
        bus.miso_io_num = -1;  // Not used for ST7789
        bus.sclk_io_num = _pinSclk;
        bus.quadwp_io_num = -1;
        bus.quadhd_io_num = -1;
        bus.max_transfer_sz = static_cast<int>(DmaBufferBytes);

        esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
        if (err != ESP_OK)
            return false;

        spi_device_interface_config_t dev{};
        dev.mode = 3;
        dev.clock_speed_hz = static_cast<int>(_hz);
        dev.spics_io_num = isPinValid(_pinCs) ? _pinCs : -1;
        dev.queue_size = 2;
         dev.flags = 0;
        dev.cs_ena_pretrans = 1;
        dev.cs_ena_posttrans = 1;
        dev.pre_cb = nullptr;

        spi_device_handle_t h = nullptr;
        err = spi_bus_add_device(SPI2_HOST, &dev, &h);
        if (err != ESP_OK)
        {
            spi_bus_free(SPI2_HOST);
            return false;
        }

        _spiHandle = h;

        // Allocate DMA buffers
        for (int i = 0; i < 2; ++i)
        {
            _dmaBuf[i] = static_cast<uint8_t *>(heap_caps_aligned_alloc(4, _dmaBufSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
            if (!_dmaBuf[i])
            {
                // Partial allocation failure - cleanup and return error
                deinit();
                return false;
            }
        }

        // Allocate persistent transactions for queued DMA transfers.
        // IMPORTANT: spi_device_queue_trans() requires the spi_transaction_t
        // to stay valid until spi_device_get_trans_result() returns it.
        for (int i = 0; i < 2; ++i)
        {
            _trans[i] = static_cast<spi_transaction_t *>(heap_caps_calloc(1, sizeof(spi_transaction_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
            if (!_trans[i])
            {
                deinit();
                return false;
            }
            _transInFlight[i] = false;
        }

        _dmaNext = 0;
        _dmaInflight = 0;

        return true;
    }

    void Esp32St7789Spi::setDc(bool level)
    {
        gpio_set_level(static_cast<gpio_num_t>(_pinDc), level ? 1 : 0);
    }

    void Esp32St7789Spi::setRst(bool level)
    {
        if (isPinValid(_pinRst))
            gpio_set_level(static_cast<gpio_num_t>(_pinRst), level ? 1 : 0);
    }

    void Esp32St7789Spi::delayMs(uint32_t ms)
    {
        esp_rom_delay_us(ms * 1000U);
    }

    void Esp32St7789Spi::write(const void *data, size_t len)
    {
        if (!len || !_spiHandle)
            return;

        flushQueued();

        // Data phase
        setDc(true);

        spi_transaction_t t{};
        t.length = static_cast<int>(len * 8U);
        t.tx_buffer = data;

        esp_err_t err = spi_device_polling_transmit(static_cast<spi_device_handle_t>(_spiHandle), &t);
        if (err != ESP_OK) {
            Serial.printf("[ST7789-SPI] write error: %d\n", err);
        }
    }

    void Esp32St7789Spi::writeCommand(uint8_t cmd)
    {
        if (!_spiHandle)
            return;

        flushQueued();

        // Command phase
        setDc(false);

        spi_transaction_t t{};
        t.flags = SPI_TRANS_USE_TXDATA;
        t.length = 8;
        t.tx_data[0] = cmd;

        esp_err_t err = spi_device_polling_transmit(static_cast<spi_device_handle_t>(_spiHandle), &t);
        if (err != ESP_OK) {
            Serial.printf("[ST7789-SPI] writeCommand(0x%02X) error: %d\n", cmd, err);
        }
    }

    void Esp32St7789Spi::writePixels(const void *data, size_t len)
    {
        if (!len || !_spiHandle)
            return;

        // Pixel data phase (always data)
        setDc(true);

        // Safety check: ensure DMA buffers are allocated
        if (!_dmaBuf[0] || !_dmaBuf[1] || !_trans[0] || !_trans[1])
            return;

        const uint8_t *p = static_cast<const uint8_t *>(data);
        size_t remaining = len;
        while (remaining)
        {
            // Wait until we have a free slot (both DMA buffer and transaction object)
            while (_dmaInflight >= 2)
                waitQueued();

            int slot = _dmaNext & 1;
            // If the selected slot is still in-flight, wait for completion.
            while (_transInFlight[slot])
                waitQueued();
            _dmaNext = (_dmaNext + 1) & 1;

            const size_t n = remaining > _dmaBufSize ? _dmaBufSize : remaining;

            // Copy to DMA buffer
            memcpy(_dmaBuf[slot], p, n);

            // Setup persistent transaction
            spi_transaction_t *t = _trans[slot];
            memset(t, 0, sizeof(*t));
            t->length = static_cast<int>(n * 8U);
            t->tx_buffer = _dmaBuf[slot];

            spi_device_queue_trans(static_cast<spi_device_handle_t>(_spiHandle), t, portMAX_DELAY);
            _transInFlight[slot] = true;
            ++_dmaInflight;

            p += n;
            remaining -= n;
        }
    }

    void Esp32St7789Spi::flush()
    {
        flushQueued();
    }

    void Esp32St7789Spi::waitQueued()
    {
        if (_dmaInflight <= 0 || !_spiHandle)
            return;

        spi_transaction_t *r = nullptr;
        spi_device_get_trans_result(static_cast<spi_device_handle_t>(_spiHandle), &r, portMAX_DELAY);

        if (r == _trans[0])
            _transInFlight[0] = false;
        else if (r == _trans[1])
            _transInFlight[1] = false;

        --_dmaInflight;
    }

    void Esp32St7789Spi::flushQueued()
    {
        while (_dmaInflight > 0)
            waitQueued();
    }
}
