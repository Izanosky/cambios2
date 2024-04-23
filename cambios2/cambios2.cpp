#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "cambios2.h"

INT(*aQueGrupo) (INT);
VOID(*ponError) (CHAR*);
INT(*inicioCambios) (INT, HANDLE, CHAR*);
INT(*inicioCambiosHijo) (INT, HANDLE, CHAR*);

void fin();

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType)
    {

    case CTRL_C_EVENT:
        fin();
        return TRUE;

    case CTRL_CLOSE_EVENT:
        printf("Ctrl-Close event\n\n");
        return TRUE;


    case CTRL_BREAK_EVENT:
        printf("Ctrl-Break event\n\n");
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        printf("Ctrl-Logoff event\n\n");
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        printf("Ctrl-Shutdown event\n\n");
        return FALSE;

    default:
        return FALSE;
    }
}


int main(int argc, char* argv[]) {
    int vel;
    
    if (argc < 2 || argc > 2) {
        vel = 0;
        //alarm(20);
    }
    else {
        vel = atoi(argv[1]);
        if (vel <= 0) {
            vel = 0;
            //alarm(20);
        }
        else {
            //alarm(30);
        }
    }

    //CARGAMOS LIBRERIA
    HINSTANCE lib = LoadLibrary("cambios2.dll");

    if (lib == NULL) {
        printf("No se pudo cargar la libreria\r\n");
        fflush(stdout);
        exit(1);
    }
    else {
        printf("Libreria cargada\r\n");
        fflush(stdout);
    }

    //------------------------------------

    //FUNCIONES SIN ARGUMENTOS
    FARPROC fnCambios = GetProcAddress(lib, "fncambios2");
    if (fnCambios == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    FARPROC incrementarCuenta = GetProcAddress(lib, "incrementarCuenta");
    if (incrementarCuenta == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    FARPROC refrescar = GetProcAddress(lib, "refrescar");
    if (refrescar == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    FARPROC finCambios = GetProcAddress(lib, "finCambios");
    if (finCambios == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    //FUNCIONES CON ARGUMENTOS
    aQueGrupo = (INT(*) (INT)) GetProcAddress(lib, "aQuEGrupo");
    if (aQueGrupo == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    ponError = (VOID(*) (CHAR*)) GetProcAddress(lib, "pon_error");
    if (ponError == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    inicioCambios = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambios");
    if (inicioCambios == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    inicioCambiosHijo = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambiosHijo");
    if (inicioCambiosHijo == NULL) {
        printf("No se ha podido cargar la funcion\r\n");
        fflush(stdout);
        exit(1);
    }

    //LIBREAMOS LIBRERIA
    if (!FreeLibrary(lib)) {
        printf("Error al liberar la libreria\r\n");
        fflush(stdout);
        exit(1);
    }


    return 0;
}

void fin() {
	
}