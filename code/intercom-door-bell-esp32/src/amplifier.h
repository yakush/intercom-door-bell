#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include <mutex>

// #include "esp_log.h"
// #include <math.h>

#define SAMPLE_RATE (44100)
// #define DMA_BUF_LEN (32)
// #define DMA_NUM_BUF (2)
#define DMA_BUF_LEN (64)
#define DMA_NUM_BUF (8)
#define I2S_NUM (0)

#define WAVE_FREQ_HZ (235.0f)
#define TWOPI (6.28318531f)
#define PHASE_INC (TWOPI * WAVE_FREQ_HZ / SAMPLE_RATE)

// forward
static void audio_task(void *userData);

struct __attribute__((packed)) I2S_Frame
{
    int16_t channel1;
    int16_t channel2;

    I2S_Frame(int v = 0)
    {
        channel1 = channel2 = v;
    }

    I2S_Frame(int ch1, int ch2)
    {
        channel1 = ch1;
        channel2 = ch2;
    }
};

typedef void (*i2s_data_cb_t)(I2S_Frame *data, int32_t len);

class Amplifier_Class
{
private:
    unsigned int _freq;
    i2s_port_t _port;
    i2s_config_t _i2s_config;
    i2s_pin_config_t _pin_config;

    i2s_data_cb_t dataCB;
    I2S_Frame out_buf[DMA_BUF_LEN];

public:
    Amplifier_Class()
    {
    }

    bool begin(
        int port,
        int pin_bck,
        int pin_lrck,
        int pin_data,
        unsigned int freq,
        i2s_data_cb_t dataCallback)
    {
        this->dataCB = dataCallback;
        this->_freq = freq;
        this->_port = (i2s_port_t)port;

        _i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = freq,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            //.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
            .dma_buf_count = DMA_NUM_BUF,
            .dma_buf_len = DMA_BUF_LEN // Interrupt level 1
        };

        _pin_config = {
            .bck_io_num = pin_bck,    // this is BCK pin
            .ws_io_num = pin_lrck,    // this is LRCK pin
            .data_out_num = pin_data, // this is DATA output pin
            .data_in_num = -1         // Not used
        };

        bool ok = true;
        ok = ok && i2s_driver_install((i2s_port_t)_port, &_i2s_config, 0, NULL) == ESP_OK;
        ok = ok && i2s_set_pin(_port, &_pin_config) == ESP_OK;
        ok = ok && i2s_set_sample_rates(_port, freq) == ESP_OK;
        // return ok;

        // Highest possible priority for realtime audio task
        xTaskCreate(audio_task, "audio", 1024, NULL, configMAX_PRIORITIES - 1, NULL);

        return ok;
    }

    void run()
    {
        // TODO: remember last bytes_written and adjust how many to request?
        if (this->dataCB == NULL)
        {
            return;
        }

        this->dataCB(this->out_buf, DMA_BUF_LEN);

        // Write with max delay. We want to push buffers as fast as we
        // can into DMA memory. If DMA memory isn't transmitted yet this
        // will yield the task until the interrupt fires when DMA buffer has
        // space again. If we aren't keeping up with the real-time deadline,
        // audio will glitch and the task will completely consume the CPU,
        // not allowing any task switching interrupts to be processed.
        size_t bytes_written;
        i2s_write(_port, out_buf, sizeof(out_buf), &bytes_written, (TickType_t)portMAX_DELAY);
        // You could put a taskYIELD() here to ensure other tasks always have a chance to run.
        // delay(1);
        taskYIELD();
    }
};

Amplifier_Class Amplifier;

static void audio_task(void *userData)
{
    while (1)
    {
        Amplifier.run();
    }
}
