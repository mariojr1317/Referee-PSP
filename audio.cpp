#include "audio.h"
#include <pspaudio.h>
#include <pspkernel.h>
#include <math.h>

void emitirPitido(int frecuenciaHz, int duracionMs) {
    int canal = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
    if (canal < 0) return;

    int sampleRate = 44100;
    int totalMuestras = (sampleRate * duracionMs) / 1000;
    short buffer[512 * 2];

    double fase = 0.0;
    double incremento = 2.0 * 3.14159265358979323846 * frecuenciaHz / sampleRate;

    int enviadas = 0;
    while (enviadas < totalMuestras) {
        int tamanoBloque = (totalMuestras - enviadas > 512) ? 512 : (totalMuestras - enviadas);
        for (int i = 0; i < tamanoBloque; i++) {
            short valor = (sin(fase) > 0) ? 10000 : -10000;
            buffer[i * 2]     = valor;
            buffer[i * 2 + 1] = valor;
            fase += incremento;
        }
        sceAudioOutputPannedBlocking(canal, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, buffer);
        enviadas += tamanoBloque;
    }

    sceAudioChRelease(canal);
}

void sonarSilbato() { emitirPitido(1800, 250); }
void sonarError() { emitirPitido(300, 400); }
void sonarMenu() { emitirPitido(800, 50); }
void sonarFinJuego() {
    emitirPitido(1500, 200);
    sceKernelDelayThread(100000);
    emitirPitido(1500, 500);
}
