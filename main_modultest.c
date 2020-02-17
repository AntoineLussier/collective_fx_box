/*
 * Template file to create a live audio processing program with portaudio.
 *
 * Compile with:
 *  gcc main_template.c lib/*.c -Ilib -lm -lportaudio -o main_template
 *
 * Run with:
 *  ./main_template
*/

/* System includes. */
#include <stdlib.h>     /* malloc, free */
#include <stdio.h>      /* printf, fprintf, getchar, stderr */    

/* Include all portaudio functions. */
#include "portaudio.h"

/* Define global audio parameters, used to setup portaudio. */
#define SAMPLE_RATE         44100
#define FRAMES_PER_BUFFER   1024
#define NUMBER_OF_CHANNELS  2


//== Program-specific includes. ==
// This is where you include the specific headers needed by your program...
#include "sinosc.h"

//== Program-specific parameters. ==
// This is where you define the specific parameters needed by your program...
#define CARRIER 250
#define RATIO 0.502
#define INDEX 20

/* The DSP structure contains all needed audio processing "objects". */
struct DSP {
    // This is where you declare the specific processing structures
    // needed by your program... Each declaration should have the form:

    // struct delay *delayline[NUMBER_OF_CHANNELS];

    // Which means a "multi-channel" pointer to the processing structure.
    struct sinosc *car[NUMBER_OF_CHANNELS];
    struct sinosc *mod[NUMBER_OF_CHANNELS];
    struct sinosc *lfo[NUMBER_OF_CHANNELS];
};

/* This function allocates memory and intializes all dsp structures. */
struct DSP * dsp_init() {
    int i;
    struct DSP *dsp = malloc(sizeof(struct DSP));   /* Memory allocation for DSP structure. */
    for (i = 0; i < NUMBER_OF_CHANNELS; i++) {
        // This is where you setup the specific processing structures needed by your program,
        // using the provided xxx_init functions. Something like:

        // dsp->delayline[i] = delay_init(DELTIME, SAMPLE_RATE);
        dsp->car[i] = sinosc_init(CARRIER, SAMPLE_RATE);
        dsp->mod[i] = sinosc_init(CARRIER * RATIO, SAMPLE_RATE);
        dsp->lfo[i] = sinosc_init(1+i*0.01, SAMPLE_RATE);
    }
    return dsp;
}

/* This function releases memory used by all dsp structures. */
void dsp_delete(struct DSP *dsp) {
    int i;
    for (i = 0; i < NUMBER_OF_CHANNELS; i++) {
        // This is where you release the memory used by the specific processing structure
        // used in the program. Something like:

        // delay_delete(dsp->delayline[i]);
        sinosc_delete(dsp->car[i]);
        sinosc_delete(dsp->mod[i]);
        sinosc_delete(dsp->lfo[i]);

    }
    free(dsp);
}

/* This function does the actual processing chain. */
void dsp_process(const float *in, float *out, unsigned long framesPerBuffer, struct DSP *dsp) {
    unsigned int i, j, index;   /* Variables used to compute the index of samples in input/output arrays. */

    // Add any variables useful to your processing logic here...
    float modval, modamp;
    float modfreq = CARRIER * RATIO * INDEX;

    for (i=0; i<framesPerBuffer; i++) {             /* For each sample frame in a buffer size... */
        for (j=0; j<NUMBER_OF_CHANNELS; j++) {      /* For each channel in a frame... */
            index = i * NUMBER_OF_CHANNELS + j;     /* Compute the index of the sample in the arrays... */

            // This is where you want to put your processing logic... A simple thru is:
            // out[index] = in[index];
            modamp = (sinosc_process(dsp->lfo[j]) * 0.5 + 0.5) * modfreq;
            modval = sinosc_process(dsp->mod[j]) * modamp;
            sinosc_set_freq(dsp->car[j], CARRIER + modval);
            out[index] = sinosc_process(dsp->car[j]) * 0.2;
        }
    }
}

/**********************************************************************************************
 *
 * You shouldn't need to edit anything below here !
 *
 *********************************************************************************************/

static int callback(const void *inputBuffer, void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags, void *userData) 
{
    const float *in = (const float *)inputBuffer;
    float *out = (float *)outputBuffer;
    /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;

    struct DSP *dsp = (struct DSP *) userData;

    if (inputBuffer == NULL) { return paAbort; }

    dsp_process(in, out, framesPerBuffer, dsp);
    
    return paContinue;
}

int paErrorCheck(PaError err)
{
    if (err != paNoError) {
        Pa_Terminate();
        fprintf(stderr,  Pa_GetErrorText(err));
        return 1;
    }
    return 0;
}

int paDefaultDeviceCheck(PaDeviceIndex device, char *direction)
{
    if (device == paNoDevice) {
        Pa_Terminate();
        fprintf(stderr, "Error: No default %s device.\n", direction);
        return 1;
    }
    return 0;
}

int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;

    struct DSP *dsp = dsp_init();
    
    err = Pa_Initialize();
    if (paErrorCheck(err)) { return -1; }

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (paDefaultDeviceCheck(inputParameters.device, "input")) { return -1; }

    inputParameters.channelCount = NUMBER_OF_CHANNELS;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (paDefaultDeviceCheck(outputParameters.device, "output")) { return -1; }

    outputParameters.channelCount = NUMBER_OF_CHANNELS;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, &inputParameters, &outputParameters, SAMPLE_RATE,
                        FRAMES_PER_BUFFER, paNoFlag, callback, (void *)dsp);
    if (paErrorCheck(err)) { return -1; }

    err = Pa_StartStream(stream);
    if (paErrorCheck(err)) { return -1; }

    printf("Hit ENTER to stop program.\n");
    getchar();

    err = Pa_CloseStream(stream);
    if (paErrorCheck(err)) { return -1; }

    dsp_delete(dsp);

    Pa_Terminate();

    return 0;
}