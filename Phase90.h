#ifndef PHASE90_H
#define PHASE90_H

// #define MIN_W 1500
// #define MAX_W 15000

#include <cmath>

#include "LFO_LUT.h"

// // Triangle function with period 2pi and range [-1,1]
// float triangle(float x) {
//     // Keep the argument within the range [0, 2*pi]
//     float value = fmodf(x, 2 * M_PI);
//     if (value < M_PI) {
//         return 2 * (value / M_PI) - 1;  // Rising edge
//     } else {
//         return 1 - 2 * ((value - M_PI) / M_PI);  // Falling edge
//     }
// }

float linear_interpolate(float x0, float y0, float x1, float y1, float x) {
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

// Function to get the value from the phase using the lookup table
float lut_waveform(float phase) {
    // Map the phase to an index in the lookup table
    float index = (phase / (2 * M_PI)) * 226;

    // Find two indices for interpolation
    int index0 = (int)index;
    int index1 = index0 + 1;

    if (index1 >= 226) index1 = 0;  // Wrap around for periodicity

    // Perform linear interpolation
    float x0 = index0;
    float y0 = lfo_lut[index0];
    float x1 = index1;
    float y1 = lfo_lut[index1];

    return linear_interpolate(x0, y0, x1, y1, index);
}

class LFO {
public:
    void Init(float sampleRate) {
        this->sampleRate = sampleRate;
    }

    void setRate(float rate) {
        currentRate = rate;
        updateIncrement();
    }

    void updateIncrement() {
        increment = (currentRate / sampleRate) * (2.0f * M_PI);
    }

    float getNextSample() {
        phase += increment;
        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI; // Wrap phase
        }
        // return (triangle(phase) + 1.0f) / 2.0f; // Range: [0, 1]
        return (lut_waveform(phase)); 
    }

private:
    float currentRate = 1.0f; // Default rate
    float phase = 0.0f; // Current phase
    float increment = 0.0f; // Increment per sample
    float sampleRate = 48000.0f;
};


/*
Digital emulation of MXR Phase 90 (no feedback resistor)
Uses the bilinear transform of transfer function ((s-wc)/(s+wc))^4 + 1
No prewarping is used
*/
class Phase90 {
public:
    Phase90() : lfo() {}

    void Init(float sampleRate) {
        this->sampleRate = sampleRate;
        lfo.Init(sampleRate);
    }

    void setRate(float potValue) {
        // rate range: 0.1 - 10 Hz
        float rate = 0.1f + potValue * (10.0f - 0.1f);
        lfo.setRate(rate);
    }

    float processSample(float x) {
        float wc = lfo.getNextSample();
        // float lfoValue = lfo.getNextSample();
        // float wc = MIN_W + lfoValue * (MAX_W - MIN_W);

        float a1 = (2.0f - wc/sampleRate) / (2.0f + wc/sampleRate);

        float c[4]; // Temporary array to hold coeffs resulting from 4 allpass filters
        c[0] = a1*a1*a1*a1;
        c[1] = -4.0f * a1*a1*a1;
        c[2] = 6.0f * a1*a1;
        c[3] = -4.0f * a1;

        // IO relation implementation:
        // input x is x[n] and x_delay[0] is x[n-1], etc.
        float y;

        y = 
            (
            c[0]*x + 
            c[1]*x_delay[0] + 
            c[2]*x_delay[1] + 
            c[3]*x_delay[2] + 
            x_delay[3]
            )
            - (
            c[3]*y_delay[0] + 
            c[2]*y_delay[1] + 
            c[1]*y_delay[2] + 
            c[0]*y_delay[3]);


        // update delayed samples
        x_delay[3] = x_delay[2];
        x_delay[2] = x_delay[1];
        x_delay[1] = x_delay[0];
        x_delay[0] = x;

        y_delay[3] = y_delay[2];
        y_delay[2] = y_delay[1];
        y_delay[1] = y_delay[0];
        y_delay[0] = y;
        
        // if (y > 1) return 1.0;
        // else if (y < -1) return -1.0;
        // else return y;

        return (0.5*x + 0.5*y);
    }

private:
    float max_w_inv;
    float min_w_inv;
    float x_delay[4];
    float y_delay[4];
    float sampleRate;
    LFO lfo; 
};

#endif // PHASE90_H
