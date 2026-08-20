#include "audio.h"
#include <pspkernel.h>
#include <pspaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int audioThreadId = -1;
static int trackActual = 0;
static bool audioRunning = true;

static FILE* abrirAudio(const char* filename) {
    if (!filename) return NULL;
    FILE* fp = fopen(filename, "rb");
    if (fp) return fp;

    char buf[512];
    snprintf(buf, sizeof(buf), "./%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    snprintf(buf, sizeof(buf), "ms0:/PSP/GAME/ArbitroPSP/%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    if (strncmp(filename, "assets/", 7) == 0) {
        snprintf(buf, sizeof(buf), "%s", filename + 7);
        fp = fopen(buf, "rb");
        if (fp) return fp;
    }

    return NULL;
}

void reproducirWAV(const char* filename) {
    FILE* fp = abrirAudio(filename);
    if (!fp) return;

    fseek(fp, 44, SEEK_SET);

    int canal = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
    if (canal < 0) {
        fclose(fp);
        return;
    }

    short buffer[512 * 2];
    size_t leidos = 0;
    while ((leidos = fread(buffer, sizeof(short), 1024, fp)) > 0 && audioRunning) {
        sceAudioOutputPannedBlocking(canal, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, buffer);
    }

    sceAudioChRelease(canal);
    fclose(fp);
}

static int AudioThread(SceSize args, void *argp) {
    while (audioRunning) {
        if (trackActual == 0) {
            sceKernelDelayThread(100000);
            continue;
        }

        const char* archivoTrack = (trackActual == 1) ? "assets/ambiente_estadio.wav" : "assets/ambiente_estadio2.wav";
        FILE* fp = abrirAudio(archivoTrack);
        if (fp) {
            fclose(fp);
            reproducirWAV(archivoTrack);
        } else {
            sceKernelDelayThread(200000);
        }
    }
    return 0;
}

void initAudio() {
    audioRunning = true;
    audioThreadId = sceKernelCreateThread("audio_ambient_thread", AudioThread, 0x12, 0x10000, 0, NULL);
    if (audioThreadId >= 0) {
        sceKernelStartThread(audioThreadId, 0, NULL);
    }
}

void freeAudio() {
    audioRunning = false;
    if (audioThreadId >= 0) {
        sceKernelWaitThreadEnd(audioThreadId, NULL);
        sceKernelDeleteThread(audioThreadId);
    }
}

void reproducirAmbiente(int track) { trackActual = track; }
void detenerAmbiente() { trackActual = 0; }

void sonarSilbato() { reproducirWAV("assets/silbato.wav"); }
void sonarMenu() { reproducirWAV("assets/click.wav"); }
void sonarTarjeta() { reproducirWAV("assets/tarjeta.wav"); }
