#pragma once
#include <Arduino.h>
#include <math.h>

struct __attribute__((packed)) WaveGenerator_Frame
{
    int16_t channel1;
    int16_t channel2;

    WaveGenerator_Frame(int16_t v = 0)
    {
        channel1 = channel2 = v;
    }

    WaveGenerator_Frame(int ch1, int ch2)
    {
        channel1 = ch1;
        channel2 = ch2;
    }
};

std::mutex waveGenerator_mutex;

class WaveGenerator
{
private:
    int bitrate = 44100;
    float deltaTime;
    int32_t frame = 0;

    float _amplitude = 10000.0; // -32,768 to 32,767
    unsigned int _freq = 440;
    bool _playing = true;

public:
    void amplitude(float amplitude)
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        _amplitude = amplitude;
    }
    float amplitude()
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        return _amplitude;
    }
    void freq(int32_t freq)
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        _freq = freq;
    }
    int32_t freq()
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        return _freq;
    }
    void playing(bool playing)
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        _playing = playing;
    }
    bool playing()
    {
        std::lock_guard<std::mutex> lck(waveGenerator_mutex);
        return _playing;
    }

public:
    WaveGenerator(int bitrate)
    {
        this->bitrate = bitrate;
        this->deltaTime = 1.0 / bitrate;
    }

    void generate(WaveGenerator_Frame *buffer, size_t len)
    {
        auto playing = this->playing();
        auto freq = this->freq();
        auto amplitude = this->amplitude();

        if (!playing)
        {
            for (int sample = 0; sample < len; sample++)
            {
                float output = 0;
                buffer[sample].channel1 = output;
                buffer[sample].channel2 = output;
            }
            return;
        }

        static float pi_2 = PI * 2.0;
        static float pi_1_2 = PI * 1.0 / 2.0;
        static float pi_3_2 = PI * 3.0 / 2.0;

        // fill the channel data
        for (int sample = 0; sample < len; sample++)
        {
            float output = 0;
            float time = frame * this->deltaTime;

            float angle = (pi_2 * freq * (time));

            // saw tooth wave:
            angle = pi_2 * (angle / pi_2 - floorf(angle / pi_2));
            output += angle < PI ? (angle - pi_1_2) : (pi_3_2 - angle);

            // normalize
            output = amplitude * (output / PI);

            buffer[sample].channel1 = output;
            buffer[sample].channel2 = output;

            this->frame += 1;
        }

        // to prevent watchdog
        delay(1);
    }
};