#ifndef PHASE90_H
#define PHASE90_H

#define MIN_W 2030
// #define MIN_W 1500
#define MAX_W 10150
// #define MAX_W 15000

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>

// Triangle function with period 2pi and range [-1,1]
float triangle(float x) {
    // Keep the argument within the range [0, 2*pi]
    float value = fmodf(x, 2 * M_PI);
    if (value < M_PI) {
        return 2 * (value / M_PI) - 1;  // Rising edge
    } else {
        return 1 - 2 * ((value - M_PI) / M_PI);  // Falling edge
    }
}

class LFO {
public:
    void setRate(float rate) {
        currentRate = rate;
        updateIncrement();
    }

    void setSampleRate(float sampleRate) {
        this->sampleRate = sampleRate;
    }

    void updateIncrement() {
        increment = (currentRate / sampleRate) * (2.0f * M_PI);
    }

    float getNextSample() {
        phase += increment;
        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI; // Wrap phase
        }
        return (triangle(phase) + 1.0f) / 2.0f; // Range: [0, 1]
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
        lfo.setSampleRate(sampleRate);
        max_w_inv = 1.0f / MAX_W;
        min_w_inv = 1.0f / MIN_W;
    }

    void setRate(float potValue) {
        // rate range: 0.1 - 10 Hz
        float rate = 0.1f + potValue * (10.0f - 0.1f);
        lfo.setRate(rate);
    }

    float processSample(float x) {
        float lfoValue = lfo.getNextSample();
        // modulate 1/wc by lfo, not wc since R is modulated in real circuit.
        float w_inv = min_w_inv + lfoValue * (max_w_inv - min_w_inv);
        // float wc = 1.0f / w_inv;
        float wc = MIN_W + lfoValue * (MAX_W - MIN_W);


        // float a1 = 2.0f - wc/sampleRate;
        // float a2 = 2.0f + wc/sampleRate;

        // float c[5]; // temporary array to hold coeffs resulting from 4 allpass filters
        // c[0] = a1*a1*a1*a1;
        // c[1] = -4.0f * a1*a1*a1 * a2;
        // c[2] = 6.0f * a1*a1 * a2*a2;
        // c[3] = -4.0f * a1 * a2*a2*a2;
        // c[4] = a2*a2*a2*a2;


        float a1 = (2.0f - wc/sampleRate) / (2.0f + wc/sampleRate);

        float c[4]; // temporary array to hold coeffs resulting from 4 allpass filters
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
