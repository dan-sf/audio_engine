#include <stdio.h>
#include <SDL2/SDL.h>
#include "platform.h"
#include <stdbool.h>

#define SECONDS 6
#define CHANNELS 2
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

float32 samples_per_second = 44100.0;
float32 tone_hz = 440.0;
float32 tone_volume = 5000;

int32 IS_SINE = 1;

typedef struct {
    void (*generate)();
    void *state;
    // ??? state_info;
} GenerateAudio;


#define SINE

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

    for (int32 sample_index = 0; sample_index < total_samples; sample_index++) {
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
    for (int sample_index = 0; sample_index < total_samples; sample_index++) {

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

typedef struct {
    void *user_data;
    SineState sine_data;
    TriangleState triangle_data;
} AD;

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

void generate_sawtooth(SawtoothState *audio_state, int len) {
    int16 *sample_write = audio_state->buffer;

    int bytes_per_sample = sizeof(uint16) * CHANNELS;
    int total_samples = len / bytes_per_sample;

    int period = samples_per_second / tone_hz;
    int half_period = period / 2;
    int volume_steps_per_sample = 2*tone_volume / period;
    int16 sample_value = audio_state->level;
    for (int sample_index = 0; sample_index < total_samples; sample_index++) {

        if (sample_value >= tone_volume) sample_value = -tone_volume;
        sample_value += volume_steps_per_sample;

        // Write the sample_value to the buffer for each channel
        for (int channel = 0; channel < CHANNELS; channel++) {
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

void generate_square(SquareState *audio_state, int len) {
    int16 *sample_write = audio_state->buffer;

    int bytes_per_sample = sizeof(uint16) * CHANNELS;
    int total_samples = len / bytes_per_sample;

    int period = samples_per_second / tone_hz;
    int half_period = period / 2;
    int16 sample_value;
    int sample_index;
    for (sample_index = 0; sample_index < total_samples; sample_index++) {

        if ((sample_index + audio_state->last) % half_period == 0) {
            audio_state->sign *= -1;
        }

        sample_value = audio_state->sign * tone_volume;

        // Write the sample_value to the buffer for each channel
        for (int channel = 0; channel < CHANNELS; channel++) {
            *sample_write++ = sample_value;
        }
    }
    audio_state->last = (sample_index + audio_state->last) % half_period;
}

// @Todo: need to come up with a good solution for a general callback function


#ifdef SINEEEEE
void audio_callback(void *userdata, Uint8 *stream, int len) {
    SineState *audio_data = (SineState *) userdata;

    generate_sine(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}
#endif

void sine_audio_callback(void *userdata, Uint8 *stream, int len) {
    AD *audio_data = (AD *) userdata;
    if (IS_SINE) {
        //SineState *sine_data = (SineState *) audio_data->sine_data;

        generate_sine(&audio_data->sine_data, len);
        memcpy(stream, (uint8 *) audio_data->sine_data.buffer, len);
    } else {
        //printf("here tri\n");
        //TriangleState *triangle_data = (TriangleState *) audio_data->triangle_data;

        generate_triangle(&audio_data->triangle_data, len);
        memcpy(stream, (uint8 *) audio_data->triangle_data.buffer, len);
    }
}

#ifdef TRIANGLE
void audio_callback(void *userdata, Uint8 *stream, int len) {
    TriangleState *audio_data = (TriangleState *) userdata;

    generate_triangle(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}
#endif

void triangle_audio_callback(void *userdata, Uint8 *stream, int len) {
    TriangleState *audio_data = (TriangleState *) userdata;

    generate_triangle(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}

#ifdef SAWTOOTH
void audio_callback(void *userdata, Uint8 *stream, int len) {
    SawtoothState *audio_data = (SawtoothState *) userdata;

    generate_sawtooth(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}
#endif

#ifdef SQUARE
void audio_callback(void *userdata, Uint8 *stream, int len) {
    SquareState *audio_data = (SquareState *) userdata;

    generate_square(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}
#endif

void square_audio_callback(void *userdata, Uint8 *stream, int len) {
    SquareState *audio_data = (SquareState *) userdata;

    generate_square(audio_data, len);
    memcpy(stream, (uint8 *) audio_data->buffer, len);
}

void (*ac)(void *userdata, Uint8 *stream, int len);

void audio_callback(void *userdata, Uint8 *stream, int len) {
    ac(userdata, stream, len);
}

int main(int argc, char* argv[]){
    ac = &sine_audio_callback;
    printf("Playing a wave.\n");

    int bytes_per_sample = sizeof(Uint16) * CHANNELS;
    int bytes_to_write = samples_per_second * bytes_per_sample; // number of bytes for a second of audio

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

#ifdef SINE
    SineState audio_data = get_sine_state(sample_start);
#endif
#ifdef TRIANGLE
    TriangleState audio_data = get_triangle_state(sample_start);
#endif
#ifdef SAWTOOTH
    SawtoothState audio_data = get_sawtooth_state(sample_start);
#endif
#ifdef SQUARE
    SquareState audio_data = get_square_state(sample_start);
#endif

    SineState sine_audio_data = get_sine_state(sample_start);
    TriangleState triangle_audio_data = get_triangle_state(sample_start);
    SawtoothState sawtooth_audio_data = get_sawtooth_state(sample_start);
    SquareState square_audio_data = get_square_state(sample_start);

    AD ad;
    ad.sine_data = sine_audio_data;
    ad.triangle_data = triangle_audio_data;

    wanted_spec.freq = samples_per_second;
    wanted_spec.format = AUDIO_S16;
    wanted_spec.channels = CHANNELS;
    wanted_spec.samples = 4096;
    wanted_spec.callback = ac; //audio_callback;
    wanted_spec.userdata = &ad; //&audio_data;

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

    // @TODO: Work on getting visuals up and running

    // SDL_Window *window;
    // SDL_Renderer *renderer;
    // SDL_Surface *surface;
    // SDL_Texture *texture;
    // SDL_Event event;

    // if (SDL_CreateWindowAndRenderer(320, 240, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    //     return 1;
    // }

    // // Create an application window with the following settings:
    // SDL_Window *window;
    // window = SDL_CreateWindow(
    //     "An SDL2 window",                  // window title
    //     0,           // initial x position
    //     0,           // initial y position
    //     640,                               // width, in pixels
    //     480,                               // height, in pixels
    //     SDL_WINDOW_OPENGL//|SDL_WINDOW_FULLSCREEN                  // flags - see below
    //     //SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN                  // flags - see below
    // );

    // // Check that the window was successfully created
    // if (window == NULL) {
    //     // In the case that the window could not be made...
    //     printf("Could not create window: %s\n", SDL_GetError());
    //     return 1;
    // }
    // SDL_ShowWindow(window);

    // Start playing
    SDL_PauseAudioDevice(device, 0);


    enum WaveType {SIN, TRI, SQU} wave_type;
    wave_type = SIN;
    // Wait for SECONDS number of seconds
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            running = false;
                            break;
                        case SDLK_w: // Sine
                            printf("Playing a sine\n");
                            IS_SINE = 1;
                            wave_type = SIN;
                            obtained_spec.callback = sine_audio_callback;
                            obtained_spec.userdata = &sine_audio_data;
                            wanted_spec.callback = sine_audio_callback;
                            wanted_spec.userdata = &sine_audio_data;
                            ac = &sine_audio_callback;
                            break;
                        case SDLK_e: // Triangle
                            IS_SINE = 0;
                            printf("Playing a triangle\n");
                            wave_type = TRI;
                            obtained_spec.callback = triangle_audio_callback;
                            obtained_spec.userdata = &triangle_audio_data;
                            wanted_spec.callback = triangle_audio_callback;
                            wanted_spec.userdata = &triangle_audio_data;
                            ac = &triangle_audio_callback;
                            break;
                        case SDLK_r: // Square
                            printf("Playing a square\n");
                            wave_type = SQU;
                            obtained_spec.callback = square_audio_callback;
                            ac = &square_audio_callback;
                            break;
                        default:
                            break;
                    }
                // case SDL_KEYUP:
                //     //SDL_KeyboardEvent k = event.key;
                //     // printf("%s\n", SDL_GetKeyName(event.key.keysym.sym));
                //     break;
                default:
                    break;
            }

            // Update the sound output here
        }
        SDL_Delay(100);
    }

    // Shut everything down
    // SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(device);
    free(sound_buffer);
    SDL_Quit();
}
