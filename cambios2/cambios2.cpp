#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "cambios2.h"

//   C:\Users\Usuario\source\repos\cambios2\Debug\cambios2.exe

INT(*aQueGrupo) (INT);
VOID(*ponError) (CHAR*);
INT(*inicioCambios) (INT, HANDLE, CHAR*);
INT(*inicioCambiosHijo) (INT, HANDLE, CHAR*);

void finalizar();

int main(int argc, char* argv[]) {
    int vel;
    LARGE_INTEGER time;
    

    HANDLE alarma = CreateWaitableTimer(NULL, TRUE, NULL);
    if (alarma == NULL) {
        printf("Error al crear la alarma (%d)\n", GetLastError());
        return 1;
    }

    if (argc < 2 || argc > 2) {
        vel = 0;
        time.QuadPart = -200000000LL;
    }
    else {
        vel = atoi(argv[1]);
        if (vel <= 0) {
            vel = 0;
            time.QuadPart = -200000000LL;
        }
        else {
            time.QuadPart = -300000000LL;
        }
    }

    if (!SetWaitableTimer(alarma, &time, 0, NULL, NULL, 0)) {
        printf("Error al establecer la alarma (%d)\n", GetLastError());
        return 2;
    }
    

    //CARGAMOS LIBRERIA
    HINSTANCE lib = LoadLibrary("cambios2.dll");

    if (lib == NULL) {
        printf("No se pudo cargar la libreria\r\n");
        fflush(stdout);
        exit(1);
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

    ///////////////////////////////////////////////
    if (WaitForSingleObject(alarma, INFINITE) != WAIT_OBJECT_0)
        printf("Error con la alarma (%d)\n", GetLastError());
    ///////////////////////////////////////////////

    //LIBREAMOS LIBRERIA
    if (!FreeLibrary(lib)) {
        printf("Error al liberar la libreria\r\n");
        fflush(stdout);
        exit(1);
    }

    return 0;
}

void finalizar() {
	printf("Finalizando...\r\n");
	fflush(stdout);
}