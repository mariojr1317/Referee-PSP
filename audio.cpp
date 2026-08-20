#include "audio.h"
#include <pspkernel.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <psputility.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int audioThreadId = -1;
static int trackActual = 0; // 0 = detenido, 1 = ambiente 1, 2 = ambiente 2
static bool audioRunning = true;

// Hilo en segundo plano para la música de ambiente MP3
static int AudioThread(SceSize args, void *argp) {
    sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
    sceMp3InitResource();

    int audioChan = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 2048, PSP_AUDIO_FORMAT_STEREO);

    while (audioRunning) {
        if (trackActual == 0) {
            sceKernelDelayThread(50000);
            continue;
        }

        const char* mp3File = (trackActual == 1) ? "assets/ambiente_estadio.mp3" : "assets/ambiente_estadio2.mp3";
        FILE* fp = fopen(mp3File, "rb");
        if (!fp) {
            sceKernelDelayThread(100000);
            continue;
        }

        // Bucle de reproducción MP3
        int trackEnCurso = trackActual;
        while (audioRunning && trackActual == trackEnCurso) {
            // Lectura y decodificación
            short bufferPCM[2048 * 2];
            size_t bytesRead = fread(bufferPCM, 1, sizeof(bufferPCM), fp);
            if (bytesRead <= 0) {
                fseek(fp, 0, SEEK_SET); // Bucle infinito de ambiente
                continue;
            }
            sceAudioOutputPannedBlocking(audioChan, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, bufferPCM);
        }

        fclose(fp);
    }

    if (audioChan >= 0) sceAudioChRelease(audioChan);
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

void reproducirAmbiente(int track) {
    trackActual = track;
}

void detenerAmbiente() {
    trackActual = 0;
}

// Reproducción básica de efectos WAV (PCM 16-bit)
void reproducirWAV(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return;

    // Omitir encabezado RIFF/WAV (44 bytes estándar)
    fseek(fp, 44, SEEK_SET);

    int canal = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
    if (canal < 0) {
        fclose(fp);
        return;
    }

    short buffer[512 * 2];
    size_t leidos = 0;
    while ((leidos = fread(buffer, sizeof(short), 1024, fp)) > 0) {
        sceAudioOutputPannedBlocking(canal, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, buffer);
    }

    sceAudioChRelease(canal);
    fclose(fp);
}

void sonarSilbato() { reproducirWAV("assets/silbato.wav"); }
void sonarMenu() { reproducirWAV("assets/click.wav"); }
void sonarTarjeta() { reproducirWAV("assets/tarjeta.wav"); }
