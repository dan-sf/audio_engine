#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "platform.h"
#include "notes.h"

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

void generate_sine(SineState *audio_data, int32 len) {
    int16 *sample_write = audio_data->buffer;
    int16 sample_value;
    float32 angle_add = TWO_PI * tone_hz / samples_per_second;

    int32 bytes_per_sample = sizeof(uint16) * CHANNELS;
    int32 total_samples = len / bytes_per_sample;

    for (int32 sample_index = 0; sample_index < total_samples; sample_index++) {
        float32 sval = tone_volume * sinf(audio_data->theta);
        audio_data->theta += angle_add;
        if (audio_data->theta > TWO_PI) {
            audio_data->theta -= TWO_PI;
        }
        sample_value = (int16) sval;

        // Write the sample_value to the buffer for each channel
        for (int32 channel = 0; channel < CHANNELS; channel++) {
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

void generate_triangle(TriangleState *audio_state, int32 len) {
    int16 *sample_write = audio_state->buffer;

    int32 bytes_per_sample = sizeof(uint16) * CHANNELS;
    int32 total_samples = len / bytes_per_sample;

    int32 period = samples_per_second / tone_hz;
    int32 half_period = period / 2;
    int32 volume_steps_per_sample = 2 * tone_volume / half_period;
    int16 sample_value = audio_state->level;
    for (int32 sample_index = 0; sample_index < total_samples; sample_index++) {

        // Create the triangle wav
        if (sample_value <= -tone_volume) audio_state->sign = 1;
        if (sample_value >= tone_volume) audio_state->sign = -1;
        sample_value += audio_state->sign * volume_steps_per_sample;

        // Write the sample_value to the buffer for each channel
        for (int32 channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
    audio_state->level = sample_value;
}

typedef struct {
    float32 level;
    int16 *buffer;
} SawtoothState;

SawtoothState get_sawtooth_state(int16 *buffer) {
    SawtoothState result;
    result.level = 0;
    result.buffer = buffer;
    return result;
}

void generate_sawtooth(SawtoothState *audio_state, int32 len) {
    int16 *sample_write = audio_state->buffer;

    int32 bytes_per_sample = sizeof(uint16) * CHANNELS;
    int32 total_samples = len / bytes_per_sample;

    int32 period = samples_per_second / tone_hz;
    int32 half_period = period / 2;
    int32 volume_steps_per_sample = 2*tone_volume / period;
    int16 sample_value = audio_state->level;
    for (int32 sample_index = 0; sample_index < total_samples; sample_index++) {

        if (sample_value >= tone_volume) sample_value = -tone_volume;
        sample_value += volume_steps_per_sample;

        // Write the sample_value to the buffer for each channel
        for (int32 channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
    audio_state->level = sample_value;
}

typedef struct {
    float32 level;
    int32 last;
    int32 sign;
    int16 *buffer;
} SquareState;

SquareState get_square_state(int16 *buffer) {
    SquareState result;
    result.level = 0;
    result.last = 0;
    result.sign = 1;
    result.buffer = buffer;
    return result;
}

void generate_square(SquareState *audio_state, int32 len) {
    int16 *sample_write = audio_state->buffer;

    int32 bytes_per_sample = sizeof(uint16) * CHANNELS;
    int32 total_samples = len / bytes_per_sample;

    int32 period = samples_per_second / tone_hz;
    int32 half_period = period / 2;
    int16 sample_value;
    int32 sample_index;
    for (sample_index = 0; sample_index < total_samples; sample_index++) {

        if ((sample_index + audio_state->last) % half_period == 0) {
            audio_state->sign *= -1;
        }

        sample_value = audio_state->sign * tone_volume;

        // Write the sample_value to the buffer for each channel
        for (int32 channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
    audio_state->last = (sample_index + audio_state->last) % half_period;
}

typedef struct {
    void *user_data;
    SineState sine_data;
    TriangleState triangle_data;
    SquareState square_data;
    SawtoothState sawtooth_data;
    enum WaveType {SIN, TRI, SQU, SAW} wave_type;
} AudioData;


// @Todo: need to come up with a good solution for a general callback function
void audio_callback(void *userdata, Uint8 *stream, int32 len) {
    AudioData *audio_data = (AudioData *) userdata;
    switch (audio_data->wave_type) {
#if 0
        case SIN:
            generate_sine(&audio_data->sine_data, len);
            memcpy(stream, (uint8 *) audio_data->sine_data.buffer, len);
            break;
        case TRI:
            generate_triangle(&audio_data->triangle_data, len);
            memcpy(stream, (uint8 *) audio_data->triangle_data.buffer, len);
            break;
        case SQU:
            generate_square(&audio_data->square_data, len);
            memcpy(stream, (uint8 *) audio_data->square_data.buffer, len);
            break;
        case SAW:
            generate_sawtooth(&audio_data->sawtooth_data, len);
            memcpy(stream, (uint8 *) audio_data->sawtooth_data.buffer, len);
            break;
#endif
        default:
            break;
    }
}

int32 main(int32 argc, char* argv[]){
    printf("Playing a wave.\n");

    int32 bytes_per_sample = sizeof(Uint16) * CHANNELS;
    int32 bytes_to_write = samples_per_second * bytes_per_sample; // number of bytes for a second of audio

    void *sound_buffer = malloc(bytes_to_write);
    Sint16 *sample_write = (Sint16 *)sound_buffer;
    Sint16 *sample_start = sample_write;

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec wanted_spec;
    SDL_AudioSpec obtained_spec;

    SineState sine_audio_data = get_sine_state(sample_start);
    TriangleState triangle_audio_data = get_triangle_state(sample_start);
    SawtoothState sawtooth_audio_data = get_sawtooth_state(sample_start);
    SquareState square_audio_data = get_square_state(sample_start);

    AudioData audio_data;
    audio_data.sine_data = sine_audio_data;
    audio_data.triangle_data = triangle_audio_data;
    audio_data.square_data = square_audio_data;
    audio_data.sawtooth_data = sawtooth_audio_data;

    wanted_spec.freq = samples_per_second;
    wanted_spec.format = AUDIO_S16;
    wanted_spec.channels = CHANNELS;
    wanted_spec.samples = 4096;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = &audio_data;
    audio_data.wave_type = SIN; // @Update: the wave_type should be initialized to a better default

    // Open the audio device
    int32 iscapture = 0;
    int32 allowed_changes = 0;
    const char* device_name = SDL_GetAudioDeviceName(0, iscapture);
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(device_name, iscapture, &wanted_spec,
                                                   &obtained_spec, allowed_changes);
    if (device == 0) {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return 1;
    }

    // @TODO: Work on getting visuals up and running

    SDL_Window *window;
    SDL_Renderer *renderer; // @Question: Do I need a renderer to draw to the window?

    // @Note: Use default renderer for now, in the future we may want to switch to something else
    if (SDL_CreateWindowAndRenderer(1400, 800, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 1;
    }
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_SetWindowTitle(window, "Audio Engine");

    // Check that the window was successfully created
    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_ShowWindow(window);

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    SDL_Rect white_key;
    white_key.x = 50; white_key.y = 50; white_key.w = 50; white_key.h = 50;
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderFillRect(renderer, &white_key);

    SDL_Rect black_key;
    black_key.x = 150; black_key.y = 50; black_key.w = 50; black_key.h = 50;
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
    SDL_RenderFillRect(renderer, &black_key);

    SDL_RenderPresent(renderer);

    // Start playing
    SDL_PauseAudioDevice(device, 0);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        running = false;
                        break;

                    // Wave types
                    case SDLK_w: // Sine
                        printf("Playing a sine\n");
                        audio_data.wave_type = SIN;
                        break;
                    case SDLK_e: // Triangle
                        printf("Playing a triangle\n");
                        audio_data.wave_type = TRI;
                        break;
                    case SDLK_r: // Square
                        printf("Playing a square\n");
                        audio_data.wave_type = SQU;
                        break;
                    case SDLK_t: // Sawtooth
                        printf("Playing a sawtooth\n");
                        audio_data.wave_type = SAW;
                        break;

                    // C major scale
                    case SDLK_a:
                        tone_hz = get_frequency("C4");
                        break;
                    case SDLK_s:
                        tone_hz = get_frequency("D4");
                        break;
                    case SDLK_d:
                        tone_hz = get_frequency("E4");
                        break;
                    case SDLK_f:
                        tone_hz = get_frequency("F4");
                        break;
                    case SDLK_g:
                        tone_hz = get_frequency("G4");
                        break;
                    case SDLK_h:
                        tone_hz = get_frequency("A4");
                        break;
                    case SDLK_j:
                        tone_hz = get_frequency("B4");
                        break;
                    case SDLK_k:
                        tone_hz = get_frequency("C5");
                        break;

                    default:
                        break;
                }
            }

            if (event.type == SDL_KEYUP && event.key.repeat == 0) {
                switch (event.key.keysym.sym) {
                    // C major scale
                    case SDLK_a:
                    case SDLK_s:
                    case SDLK_d:
                    case SDLK_f:
                    case SDLK_g:
                    case SDLK_h:
                    case SDLK_j:
                    case SDLK_k:
                        tone_hz = 0.0;
                        break;
                    default:
                        break;
                }
            }
        }
        SDL_Delay(50);
    }

    // Shut everything down
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(device);
    free(sound_buffer);
    SDL_Quit();
}
