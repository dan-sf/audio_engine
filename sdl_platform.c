#include <stdio.h>
#include <SDL2/SDL.h>
#include "platform.h"

#define SECONDS 6
#define CHANNELS 2
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

float32 samples_per_second = 44100.0;
float32 tone_hz = 440.0;
float32 tone_volume = 5000;


typedef struct {
    float32 theta;
    int16 *buffer;
} SineState;

SineState get_sine_state(int16 *buffer) {
    SineState result;
    result.theta = 0;
    result.buffer = buffer;
    return result;
}

void generate_sine(SineState *audio_data, int len) {
    int16 *sample_write = audio_data->buffer;
    int16 sample_value;
    float32 angle_add = TWO_PI * tone_hz / samples_per_second;

    int bytes_per_sample = sizeof(uint16) * CHANNELS;
    int total_samples = len / bytes_per_sample;

    for (int32 sample_index = 0; sample_index <= total_samples; sample_index++) {
        float32 sval = tone_volume * sinf(audio_data->theta);
        audio_data->theta += angle_add;
        if (audio_data->theta > TWO_PI) {
            audio_data->theta -= TWO_PI;
        }
        sample_value = (int16) sval;

        // Write the sample_value to the buffer for each channel
        for (int channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
}

typedef struct {
    float32 level;
    int32 sign;
    int16 *buffer;
} TriangleState;

TriangleState get_triangle_state(int16 *buffer) {
    TriangleState result;
    result.level = 0;
    result.sign = 1;
    result.buffer = buffer;
    return result;
}

void generate_triangle(TriangleState *audio_state, int len) {
    int16 *sample_write = audio_state->buffer;

    int bytes_per_sample = sizeof(uint16) * CHANNELS;
    int total_samples = len / bytes_per_sample;

    int period = samples_per_second / tone_hz;
    int half_period = period / 2;
    int volume_steps_per_sample = 2 * tone_volume / half_period;
    int16 sample_value = audio_state->level;
    for (int sample_index = 0; sample_index <= total_samples; sample_index++) {

        // Create the triangle wav
        if (sample_value <= -tone_volume) audio_state->sign = 1;
        if (sample_value >= tone_volume) audio_state->sign = -1;
        sample_value += audio_state->sign * volume_steps_per_sample;

        // Write the sample_value to the buffer for each channel
        for (int channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
    audio_state->level = sample_value;
}

// typedef struct {
//     float32 level;
//     int16 *buffer;
// } SawtoothState;
// 
// SawtoothState get_sawtooth_state() {
//     SawtoothState result = {0, 0};
//     return result;
// }
// 
// void generate_sawtooth(SawtoothState *audio_data, int len) {
//     int16 *sample_write = audio_data->buffer;
//     int16 sample_value;
//     float32 angle_add = TWO_PI * tone_hz / samples_per_second;
// 
//     int bytes_per_sample = sizeof(uint16) * CHANNELS;
//     int total_samples = len / bytes_per_sample;
// 
//     for (int32 sample_index = 0; sample_index <= total_samples; sample_index++) {
//         float32 sval = tone_volume * sinf(audio_data->theta);
//         audio_data->theta += angle_add;
//         if (audio_data->theta > TWO_PI) {
//             audio_data->theta -= TWO_PI;
//         }
//         sample_value = (int16) sval;
// 
//         // Write the sample_value to the buffer for each channel
//         for (int channel = 0; channel < CHANNELS; channel++) {
//             *sample_write++ = sample_value;
//         }
//     }
// 
// }



// void audio_callback(void *userdata, Uint8 *stream, int len) {
//     SineState *audio_data = (SineState *) userdata;
// 
//     generate_sine(audio_data, len);
//     memcpy(stream, (uint8 *) audio_data->buffer, len);
// }

void audio_callback(void *userdata, Uint8 *stream, int len) {
    TriangleState *audio_data = (TriangleState *) userdata;

    generate_triangle(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}


int main(int argc, char* argv[]){
    printf("Playing a wave.\n");

    int bytes_per_sample = sizeof(Uint16) * CHANNELS;
    int bytes_to_write = samples_per_second * bytes_per_sample; // number of bytes for a second of audio

    void *sound_buffer = malloc(bytes_to_write);
    Sint16 *sample_write = (Sint16 *)sound_buffer;
    Sint16 *sample_start = sample_write;

    // Init SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec wanted_spec;
    SDL_AudioSpec obtained_spec;

    SineState audio_data = get_sine_state(sample_start);

    wanted_spec.freq = samples_per_second;
    wanted_spec.format = AUDIO_S16;
    wanted_spec.channels = CHANNELS;
    wanted_spec.samples = 4096;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = &audio_data;

    // Open the audio device
    int iscapture = 0;
    int allowed_changes = 0;
    const char* device_name = SDL_GetAudioDeviceName(0, iscapture);
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(device_name, iscapture, &wanted_spec,
                                                   &obtained_spec, allowed_changes);
    if (device == 0) {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return 1;
    }

    // Start playing
    SDL_PauseAudioDevice(device, 0);

    // Wait for SECONDS number of seconds
    SDL_Delay(SECONDS * 1000);

    // Shut everything down
    SDL_CloseAudioDevice(device);
    free(sound_buffer);
    SDL_Quit();
}
