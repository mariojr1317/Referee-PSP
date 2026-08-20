#include "audio.h"
#include <pspkernel.h>
#include <pspaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int audioThreadId = -1;
static int trackActual = 0;
static bool audioRunning = true;

// Buscador de archivos de audio
static FILE* abrirAudio(const char* filename) {
    if (!filename) return NULL;
    FILE* fp = fopen(filename, "rb");
    if (fp) return fp;

    char buf[512];
    snprintf(buf, sizeof(buf), "./%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    snprintf(buf, sizeof(buf), "ArbitroPSP/%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    snprintf(buf, sizeof(buf), "ms0:/PSP/GAME/ArbitroPSP/%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    snprintf(buf, sizeof(buf), "disc0:/PSP_GAME/USRDIR/%s", filename);
    fp = fopen(buf, "rb");
    if (fp) return fp;

    if (strncmp(filename, "assets/", 7) == 0) {
        snprintf(buf, sizeof(buf), "%s", filename + 7);
        fp = fopen(buf, "rb");
        if (fp) return fp;
    }

    return NULL;
}

// Estructuras para parsear encabezados WAV reales
struct WAVHeader {
    char riff[4];
    unsigned int size;
    char wave[4];
};

struct ChunkHeader {
    char id[4];
    unsigned int size;
};

struct FormatChunk {
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
};

// Reproductor WAV con conversión Mono->Estéreo automática
void reproducirWAV(const char* filename) {
    FILE* fp = abrirAudio(filename);
    if (!fp) return;

    WAVHeader wavHeader;
    if (fread(&wavHeader, 1, sizeof(WAVHeader), fp) < sizeof(WAVHeader)) {
        fclose(fp);
        return;
    }

    if (memcmp(wavHeader.riff, "RIFF", 4) != 0 || memcmp(wavHeader.wave, "WAVE", 4) != 0) {
        fclose(fp);
        return;
    }

    FormatChunk fmt = {0};
    unsigned int dataSize = 0;

    ChunkHeader chunk;
    while (fread(&chunk, 1, sizeof(ChunkHeader), fp) == sizeof(ChunkHeader)) {
        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            fread(&fmt, 1, sizeof(FormatChunk), fp);
            if (chunk.size > sizeof(FormatChunk)) {
                fseek(fp, chunk.size - sizeof(FormatChunk), SEEK_CUR);
            }
        } else if (memcmp(chunk.id, "data", 4) == 0) {
            dataSize = chunk.size;
            break;
        } else {
            fseek(fp, chunk.size, SEEK_CUR);
        }
    }

    if (dataSize == 0 || fmt.bitsPerSample != 16) {
        fclose(fp);
        return;
    }

    int canal = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
    if (canal < 0) {
        fclose(fp);
        return;
    }

    short readBuffer[512 * 2];
    short stereoBuffer[512 * 2];
    unsigned int leidosTotal = 0;

    while (leidosTotal < dataSize && audioRunning) {
        int aLeer = (fmt.numChannels == 1) ? 512 : 1024;
        size_t leidos = fread(readBuffer, sizeof(short), aLeer, fp);
        if (leidos <= 0) break;

        leidosTotal += leidos * sizeof(short);

        if (fmt.numChannels == 1) {
            for (size_t i = 0; i < leidos; i++) {
                stereoBuffer[i * 2]     = readBuffer[i];
                stereoBuffer[i * 2 + 1] = readBuffer[i];
            }
            sceAudioOutputPannedBlocking(canal, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, stereoBuffer);
        } else {
            sceAudioOutputPannedBlocking(canal, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, readBuffer);
        }
    }

    sceAudioChRelease(canal);
    fclose(fp);
}

// Hilo de reproducción de sonido de ambiente (acepta .wav para mejor compatibilidad)
static int AudioThread(SceSize args, void *argp) {
    while (audioRunning) {
        if (trackActual == 0) {
            sceKernelDelayThread(50000);
            continue;
        }

        const char* archivoTrack = (trackActual == 1) ? "assets/ambiente_estadio.wav" : "assets/ambiente_estadio2.wav";
        FILE* fp = abrirAudio(archivoTrack);
        if (!fp) {
            // Intentar extensión .mp3 si no encuentra .wav
            archivoTrack = (trackActual == 1) ? "assets/ambiente_estadio.mp3" : "assets/ambiente_estadio2.mp3";
            fp = abrirAudio(archivoTrack);
        }

        if (fp) {
            fclose(fp);
            reproducirWAV(archivoTrack);
        } else {
            sceKernelDelayThread(100000);
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
