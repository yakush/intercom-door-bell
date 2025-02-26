#pragma once
#include <Arduino.h>

// forward
static void bellDetector_task(void *userData);
typedef void (*bellDetector_pressed_cb)();
typedef void (*bellDetector_released_cb)();

const int BELL_DETECTOR_AFTER_ON_TIME = 200;

class BellDetector_Class
{
private:
    int pin;
    bellDetector_pressed_cb cb_pressed;
    bellDetector_released_cb cb_released;

    bool state = false;
    unsigned long prevDetectionTime = 0;

public:
    BellDetector_Class()
    {
    }

    void begin(
        int pin,
        bellDetector_pressed_cb cb_pressed,
        bellDetector_released_cb cb_released)
    {
        this->pin = pin;
        this->cb_pressed = cb_pressed;
        this->cb_released = cb_released;

        pinMode(pin, INPUT);

        xTaskCreate(bellDetector_task, "bellDetector_task", 1024, NULL, 1, NULL);
    }

    void run()
    {
        int value = digitalRead(pin);
        if (value == LOW)
        {
            // detection
            if (state == false)
            {
                // changed state to on - call callback
                if (cb_pressed != NULL)
                {
                    cb_pressed();
                }
            }
            state = true;
            prevDetectionTime = millis();
        }
        else
        {
            // no detection
            if (state == true)
            {
                if (millis() - prevDetectionTime > BELL_DETECTOR_AFTER_ON_TIME)
                {
                    // after some time with no detection - unset state.
                    if (cb_released != NULL)
                    {
                        cb_released();
                    }
                    state = false;
                }
            }
        }

        taskYIELD();
    }
};

BellDetector_Class BellDetector;

static void bellDetector_task(void *userData)
{
    while (1)
    {
        BellDetector.run();
    }
}
