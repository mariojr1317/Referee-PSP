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

    // Fijar directorio de trabajo
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
    
    // Inicializar Motor de Audio
    initAudio();
    reproducirAmbiente(1); // Arranca el sonido del estadio en bucle

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
    SceCtrlData pad;

    Image* imgArbitro = loadPNG("assets/referee_main.png");
    Image* imgBox = loadPNG("assets/box_selected.png");
    Image* imgIconoNueva = loadPNG("assets/icon_nueva_partida.png");

    int seleccionMenu = 0;
    int totalOpciones = 6;

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);

        startFrame();

        if (imgArbitro) {
            drawImage(imgArbitro, 180, 0, 300, 272);
        } else {
            drawRect(180, 20, 280, 230, 0xFF005500); 
        }

        int posX = 15 + (seleccionMenu * 75);
        if (imgBox) {
            drawImage(imgBox, posX, 110, 70, 70);
        } else {
            drawRect(posX, 110, 70, 70, 0xFF00FFFF); 
        }

        if (imgIconoNueva) {
            drawImage(imgIconoNueva, 20, 115, 60, 60);
        } else {
            drawRect(20, 115, 60, 60, 0xFFFFFFFF); 
        }

        endFrame();

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
            sonarSilbato();
            sceKernelDelayThread(300000);
        }
        else if (pad.Buttons & PSP_CTRL_TRIANGLE) {
            // Presiona TRIÁNGULO para cambiar al segundo ambiente de estadio
            reproducirAmbiente(2);
            sceKernelDelayThread(200000);
        }
        else if (pad.Buttons & PSP_CTRL_SQUARE) {
            // Presiona CUADRADO para sacar tarjeta
            sonarTarjeta();
            sceKernelDelayThread(200000);
        }
    }

    freeAudio();
    freeImage(imgArbitro);
    freeImage(imgBox);
    freeImage(imgIconoNueva);

    return 0;
}
