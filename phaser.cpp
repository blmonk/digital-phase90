#include <math.h>
#include <stdio.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "i2s.h"
#include "pico/stdlib.h"

#include "Phase90.h"
#include <cmath>
#include <climits>

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
const uint LED_PIN = PICO_DEFAULT_LED_PIN;
#endif

static __attribute__((aligned(8))) pio_i2s i2s;

bool led_flag;
Phase90 phaser;

// static void process_audio(const int32_t* input, int32_t* output, size_t num_frames) {
//     // Just copy the input to the output
//     for (size_t i = 0; i < num_frames; i++) {
// 
//         output[i*2] = phaser.processSample(input[i*2]);
//         // output[i*2] = input[i*2];
//         output[i*2 + 1] = output[i*2];
//         
//     }
// }

static void process_audio(const int32_t* input, int32_t* output, size_t num_frames) {
    // The maximum possible value for int32_t
    const float MAX_INT32 = 2147483647.0f;

    for (size_t i = 0; i < num_frames; i++) {

        // Convert to float in the range of -1 to 1
        float input_sample = static_cast<float>(input[i*2]) / MAX_INT32;

        // Process the sample with the phaser
        float processed_sample = phaser.processSample(input_sample);

        // Convert the processed sample back to int32_t, scaling it back to the original range
        output[i*2] = static_cast<int32_t>(processed_sample * MAX_INT32);
        output[i*2 + 1] = output[i*2];
    }
}

// static void process_audio(const int32_t* input, int32_t* output, size_t num_frames) {
//     // Just copy the input to the output, converting to float before processing
//     for (size_t i = 0; i < num_frames; i++) {
// 
//         // Convert to float and normalize to the range [-1.0, 1.0]
//         float input_sample = static_cast<float>(input[i*2]) / INT32_MAX;
//         
//         // Process the normalized sample
//         float processed_sample = phaser.processSample(input_sample);
// 
//         // Clip the processed sample to the range [-1.0, 1.0]
//         processed_sample = std::max(-1.0f, std::min(1.0f, processed_sample));
// 
//         // Round the processed sample and convert back to int32_t
//         output[i*2] = static_cast<int32_t>(std::round(processed_sample * INT32_MAX));
//         output[i*2 + 1] = output[i*2];
//     }
// }

static void dma_i2s_in_handler(void) {
    /* We're double buffering using chained TCBs. By checking which buffer the
     * DMA is currently reading from, we can identify which buffer it has just
     * finished reading (the completion of which has triggered this interrupt).
     */
    if (*(int32_t**)dma_hw->ch[i2s.dma_ch_in_ctrl].read_addr == i2s.input_buffer) {
        // It is inputting to the second buffer so we can overwrite the first
        process_audio(i2s.input_buffer, i2s.output_buffer, AUDIO_BUFFER_FRAMES);
    } else {
        // It is currently inputting the first buffer, so we write to the second
        process_audio(&i2s.input_buffer[STEREO_BUFFER_SIZE], &i2s.output_buffer[STEREO_BUFFER_SIZE], AUDIO_BUFFER_FRAMES);
    }
    dma_hw->ints0 = 1u << i2s.dma_ch_in_data;  // clear the IRQ
}

int main() {
    // Set a 132.000 MHz system clock to more evenly divide the audio frequencies
    // 132 MHz - 32 bit
    // 129.6 MHz - 24 bit
    // set_sys_clock_khz(129600, true);
    set_sys_clock_khz(132000, true);
    stdio_init_all();

    // printf("System Clock: %lu\n", clock_get_hz(clk_sys));

    // Init GPIO LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    phaser.Init(48000.0f);
    phaser.setRate(0.1f);

    i2s_program_start_synched(pio0, &i2s_config_default, dma_i2s_in_handler, &i2s);

    // puts("i2s_example started.");



    // Blink the LED so we know we started everything correctly.
    while (true) {
        // gpio_put(LED_PIN, 1);
        // sleep_ms(1000);
        // gpio_put(LED_PIN, 0);
        // sleep_ms(1000);
        // gpio_put(LED_PIN, led_flag);
    }

    return 0;
}