#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include "audio.h"

PSP_MODULE_INFO("ArbitroProPSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define COLOR_WHITE   0xFFFFFFFF
#define COLOR_GREEN   0xFF00FF00
#define COLOR_YELLOW  0xFF00FFFF
#define COLOR_RED     0xFF0000FF
#define COLOR_CYAN    0xFFFFFF00
#define COLOR_GRAY    0xFF888888
#define COLOR_MAGENTA 0xFFFF00FF

int exit_callback(int arg1, int arg2, void *common) {
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

// Configuración de opciones del Menú Principal
const char* opcionesMenu[6] = {
    "NUEVA PARTIDA", "CARRERA", "DESAFIOS", "MULTIJUGADOR", "OPCIONES", "CREDITOS"
};

const char* descripcionesMenu[6] = {
    "Inicia un partido rapido y conviertete en el mejor arbitro.",
    "Comienza tu trayectoria profesional desde ligas locales hasta el Mundial.",
    "Supera retos extremos de decision rapida y VAR bajo alta presion.",
    "Compite en pruebas de reglamento contra otro arbitro via Ad-Hoc.",
    "Ajusta los graficos 2D, volumen del silbato y configuracion de botones.",
    "Desarrolladores y creditos del proyecto Homebrew."
};

void DibujarEncabezado() {
    pspDebugScreenSetTextColor(COLOR_WHITE);
    pspDebugScreenPrintf(" PSP  |  SIMULADOR DE ARBITRO DE FUTBOL         1/1  00:00 [III]\n");
    pspDebugScreenSetTextColor(COLOR_CYAN);
    pspDebugScreenPrintf("==================================================================\n\n");
}

int main(void) {
    SetupCallbacks();
    pspDebugScreenInit();
    
    // Fondo oscuro mate estilo UI moderna
    pspDebugScreenSetBackColor(0xFF111111);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    SceCtrlData pad;

    int estadoJuego = 0; // 0: Menú Principal, 1: En Partido
    int seleccionMenu = 0;
    int totalOpciones = 6;

    while (1) {
        pspDebugScreenClear();
        pspDebugScreenSetXY(0, 0);
        sceCtrlReadBufferPositive(&pad, 1);

        if (estadoJuego == 0) {
            // RENDERIZADO DEL MENÚ
            DibujarEncabezado();

            pspDebugScreenSetTextColor(COLOR_WHITE);
            pspDebugScreenPrintf("  SELECCIONA MODO:\n\n ");

            // Carrusel horizontal de cajas
            for (int i = 0; i < totalOpciones; i++) {
                if (i == seleccionMenu) {
                    pspDebugScreenSetTextColor(COLOR_YELLOW);
                    pspDebugScreenPrintf("[> %s <] ", opcionesMenu[i]);
                } else {
                    pspDebugScreenSetTextColor(COLOR_GRAY);
                    pspDebugScreenPrintf("  %s   ", opcionesMenu[i]);
                }
            }

            pspDebugScreenPrintf("\n\n\n");
            pspDebugScreenSetTextColor(COLOR_CYAN);
            pspDebugScreenPrintf("------------------------------------------------------------------\n");
            pspDebugScreenSetTextColor(COLOR_WHITE);
            pspDebugScreenPrintf(" (i) %s\n", descripcionesMenu[seleccionMenu]);
            pspDebugScreenSetTextColor(COLOR_CYAN);
            pspDebugScreenPrintf("------------------------------------------------------------------\n\n");

            pspDebugScreenSetTextColor(COLOR_GREEN);
            pspDebugScreenPrintf(" [PAD IZD/DER] NAVEGAR   |   [X] ACEPTAR   |   [O] ATRAS\n");

            // NAVEGACIÓN D-PAD
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
                if (seleccionMenu == 0) {
                    estadoJuego = 1; // Salta a partido
                }
                sceKernelDelayThread(300000);
            }
        }
        else if (estadoJuego == 1) {
            // PANTALLA PROVISIONAL DE PARTIDO
            DibujarEncabezado();
            
            pspDebugScreenSetTextColor(COLOR_YELLOW);
            pspDebugScreenPrintf(" PARTIDO EN CURSO (MODO 2D)\n\n");
            
            pspDebugScreenSetTextColor(COLOR_WHITE);
            pspDebugScreenPrintf(" Alianza FC  0 - 0  CD FAS\n\n");
            
            pspDebugScreenSetTextColor(COLOR_GREEN);
            pspDebugScreenPrintf(" [Presiona O para regresar al Menu Principal]\n");

            if (pad.Buttons & PSP_CTRL_CIRCLE) {
                sonarMenu();
                estadoJuego = 0;
                sceKernelDelayThread(300000);
            }
        }

        sceDisplayWaitVblankStart();
    }

    return 0;
}
