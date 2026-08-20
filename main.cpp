#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "audio.h"
#include "graphics.h"

PSP_MODULE_INFO("ArbitroProPSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

int exit_callback(int arg1, int arg2, void *common) {
    freeAudio();
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
    return thid;
}

int main(int argc, char* argv[]) {
    SetupCallbacks();

    // Intentar cambiar al directorio donde reside el EBOOT
    if (argc > 0 && argv[0]) {
        char dir[256];
        strncpy(dir, argv[0], sizeof(dir));
        char* lastSlash = strrchr(dir, '/');
        if (lastSlash) {
            *lastSlash = '\0';
            sceIoChdir(dir);
        }
    }

    initGraphics();
    initAudio();

    // Intentar cargar imágenes
    Image* imgArbitro     = loadPNG("assets/referee_main.png");
    Image* imgBox         = loadPNG("assets/box_selected.png");
    Image* imgIconoNueva  = loadPNG("assets/icon_nueva_partida.png");

    // Iniciar sonido de estadio
    reproducirAmbiente(1);

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
    SceCtrlData pad;

    int seleccionMenu = 0;
    int totalOpciones = 5;

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);

        startFrame();

        // 1. Fondo de Campo de Fútbol (Verde césped)
        drawRect(0, 0, 480, 272, 0xFF1E5B22);

        // 2. Líneas del campo decorativas
        drawRect(238, 0, 4, 272, 0x80FFFFFF); // Línea central
        drawRect(10, 10, 460, 252, 0x40FFFFFF); // Borde campo

        // 3. Área del Árbitro (Derecha)
        if (imgArbitro) {
            drawImage(imgArbitro, 260, 20, 200, 230);
        } else {
            // Silueta / Recuadro de Árbitro si no hay PNG
            drawRect(280, 40, 160, 200, 0xFF111111);
            drawRect(290, 50, 140, 180, 0xFF333333);
            drawRect(340, 60, 40, 40, 0xFF00D7FF); // Cabeza / Silueta
        }

        // 4. Menú de Opciones (Abajo / Centro)
        for (int i = 0; i < totalOpciones; i++) {
            int posX = 20 + (i * 50);
            int posY = 200;

            if (i == seleccionMenu) {
                // Marco Marco Seleccionado (Amarillo)
                if (imgBox) {
                    drawImage(imgBox, posX - 4, posY - 4, 48, 48);
                } else {
                    drawRect(posX - 4, posY - 4, 48, 48, 0xFF00D7FF);
                }
            }

            // Botón base
            drawRect(posX, posY, 40, 40, 0xFF444444);

            // Icono de la primera opción
            if (i == 0 && imgIconoNueva) {
                drawImage(imgIconoNueva, posX + 4, posY + 4, 32, 32);
            } else {
                // Indicador de color para cada opción
                unsigned int colores[] = { 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFFFF00, 0xFFFF00FF };
                drawRect(posX + 8, posY + 8, 24, 24, colores[i % 5]);
            }
        }

        endFrame();

        // Controles y Efectos de Sonido
        if (pad.Buttons & PSP_CTRL_LEFT) {
            seleccionMenu--;
            if (seleccionMenu < 0) seleccionMenu = totalOpciones - 1;
            sonarMenu();
            sceKernelDelayThread(150000);
        }
        else if (pad.Buttons & PSP_CTRL_RIGHT) {
            seleccionMenu++;
            if (seleccionMenu >= totalOpciones) seleccionMenu = 0;
            sonarMenu();
            sceKernelDelayThread(150000);
        }
        else if (pad.Buttons & PSP_CTRL_CROSS) {
            sonarSilbato(); // Botón X -> Silbato
            sceKernelDelayThread(300000);
        }
        else if (pad.Buttons & PSP_CTRL_TRIANGLE) {
            reproducirAmbiente(2); // Botón Triángulo -> Estadio 2
            sceKernelDelayThread(200000);
        }
        else if (pad.Buttons & PSP_CTRL_SQUARE) {
            sonarTarjeta(); // Botón Cuadrado -> Tarjeta
            sceKernelDelayThread(200000);
        }
    }

    freeAudio();
    freeImage(imgArbitro);
    freeImage(imgBox);
    freeImage(imgIconoNueva);

    return 0;
}
