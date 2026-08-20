#ifndef AUDIO_H
#define AUDIO_H

// Inicialización y limpieza del motor de audio
void initAudio();
void freeAudio();

// Reproducción de Efectos de Sonido (Archivos .WAV)
void reproducirWAV(const char* filename);

// Controles del Sonido de Ambiente de Estadio (.MP3)
// track = 1 -> "assets/ambiente_estadio.mp3"
// track = 2 -> "assets/ambiente_estadio2.mp3"
void reproducirAmbiente(int track);
void detenerAmbiente();

// Atajos para los sonidos del juego
void sonarSilbato();
void sonarMenu();
void sonarTarjeta();

#endif
