#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "cambios2.h"

INT(*aQueGrupo) (INT);
INT(*inicioCambios) (INT, HANDLE, CHAR*);
INT(*inicioCambiosHijo) (INT, HANDLE, CHAR*);
VOID(*pon_error)(CHAR*);

DWORD WINAPI Zacarias(LPVOID param);
DWORD WINAPI Alumnos(LPVOID param);

struct global {
    HANDLE mtxLib;
    HANDLE mem;
    HANDLE SZ;
    LPCH refM;
    HANDLE alarma;
    HINSTANCE lib;
    HANDLE SHijos;
    HANDLE cambios;

    HANDLE Z;
    HANDLE alumnos[32];
};

global glob;

void fin();

int main(int argc, char* argv[]) {
    int vel, valor, cont = 0;
    LARGE_INTEGER time;

    glob.mtxLib = NULL;
    glob.mem = NULL;
    glob.refM = NULL;
    glob.alarma = NULL;
    glob.lib = NULL;
    glob.Z = NULL;
    glob.SHijos = NULL;

    if (argc == 2) {
        //MECANISMOS DE SINCRONIZACION
        glob.mtxLib = CreateMutex(NULL, FALSE, "mtxLib");
        if (glob.mtxLib == NULL) {
			printf("Error al crear el mutex\r\n");
            fin();
            exit(1);
        }

        glob.SHijos = CreateSemaphore(NULL, 0, 1, "SHijos");
        if (glob.SHijos == NULL) {
			printf("Error al crear el semaforo\r\n");
            fin();
            exit(1);
        }

        glob.SZ = CreateSemaphore(NULL, 0, 1, "SZ");
        if (glob.SZ == NULL) {
			printf("Error al crear el semaforo\r\n");
            fin();
            exit(1);
        }

        glob.cambios = CreateSemaphore(NULL, 1, 1, "cambios");
        if (glob.cambios == NULL) {
			printf("Error al crear el semaforo\r\n");
            fin();
            exit(1);
        }

        //CREAMOS LA MEMORIA COMPARTIDA
        glob.mem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(char) * 80 + sizeof(int) * 3 + sizeof(DWORD), "memoria");
        if (glob.mem == NULL) {
            fin();
            exit(1);
        }

        glob.refM = (LPCH)MapViewOfFile(glob.mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3 + sizeof(DWORD));
        if (glob.refM == NULL) {
            fin();
            exit(1);
        }

        //CREAMOS LA ALARMA Y /ESTABLECEMOS EL TIEMPO
        glob.alarma = CreateWaitableTimer(NULL, TRUE, NULL);
        if (glob.alarma == NULL) {
            fin();
            exit(1);
        }

        vel = atoi(argv[1]);
        if (vel <= 0) {
            vel = 0;
            time.QuadPart = -200000000LL;
        }
        else {
            time.QuadPart = -300000000LL;
        }

        if (!SetWaitableTimer(glob.alarma, &time, 0, NULL, NULL, 0)) {
            fin();
            exit(1);
        }

        //CARGAMOS LA LIBRERIA
        glob.lib = LoadLibrary("cambios2.dll");

        if (glob.lib == NULL) {
            fin();
            exit(1);
        }

        //FUNCIONES SIN ARGUMENTOS
        FARPROC finCambios = GetProcAddress(glob.lib, "finCambios");
        if (finCambios == NULL) {
            fin();
            exit(1);
        }

        FARPROC refrescar = GetProcAddress(glob.lib, "refrescar");
        if (refrescar == NULL) {
            fin();
            exit(1);
        }

        //FUNCIONES CON ARGUMENTOS
        inicioCambios = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(glob.lib, "inicioCambios");
        if (inicioCambios == NULL) {
            fin();
            exit(1);
        }

        valor = inicioCambios(vel, glob.mtxLib, glob.refM);
        if (valor == -1) {
            printf("Error al iniciar los cambios\r\n");
            fflush(stdout);
            fin();
            exit(1);
        }

		//COPIAMOS LA VELOCIDAD A LA MEMORIA COMPARTIDA Y PONEMOS EL CONTADOR A 0
        *((int*)&(glob.refM[84])) = 0;
        *((int*)&(glob.refM[88])) = vel;
        refrescar();

        //CREAMOS UN HILO
        DWORD ZID;
        glob.Z = CreateThread(NULL, 0, Zacarias, NULL, 0, &ZID);
        if (glob.Z == NULL) {
            fin();
            exit(1);
        }

        WaitForSingleObject(glob.SZ, INFINITE);
        *((DWORD*)&(glob.refM[92])) = ZID;
        refrescar();

        DWORD alumnosID[32];
        for (int i = 0; i < 32; i++) {
            glob.alumnos[i] = CreateThread(NULL, 0, Alumnos, LPVOID(i), 0, &alumnosID[i]);
            if (glob.alumnos[i] == NULL) {
                fin();
                exit(1);
            }
            WaitForSingleObject(glob.SHijos, INFINITE);
        }

        ReleaseSemaphore(glob.SHijos, 1, NULL);

        WaitForSingleObject(glob.alarma, INFINITE);
        WaitForSingleObject(glob.cambios, INFINITE);

        PostThreadMessage(ZID, 1000, NULL, NULL);

        for (int j = 0; j < 32; j++) {
            PostThreadMessage(alumnosID[j], 1000, NULL, NULL);
        }

        WaitForSingleObject(glob.Z, INFINITE);
        WaitForMultipleObjects(32, glob.alumnos, TRUE, INFINITE);

        valor = finCambios();
        if (valor == -1) {
            printf("Error al finalizar los cambios\r\n");
            fflush(stdout);
            fin();
            exit(1);
        }

        fin();

        return 0;
    }
    else {
        printf("ERROR\r\n");
        printf("Ejemplo de uso: (nombre ejecutable) (velocidad)\r\n");
        fflush(stdout);
        return 1;
    }

    return 0;
}

DWORD WINAPI Zacarias(LPVOID param) {
    MSG msg1, msg2;
    HANDLE semZ = NULL, cambios = NULL;
    int tipo1, tipo2, grupos1, grupos2;
    DWORD pid1, pid2, waitResult;
    int gC1, gA1, gC2, gA2;

    PeekMessage(&msg1, NULL, 1000, 1050, PM_REMOVE);

    semZ = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SZ");
    if (semZ == NULL) {
        exit(1);
    }

    cambios = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "cambios");
    if (cambios == NULL) {
        CloseHandle(semZ);
        exit(1);
    }

    ReleaseSemaphore(semZ, 1, NULL);

    for (int i = 0; i < 32; i++) {
        GetMessage(&msg1, NULL, 1050, 1050);
    }

    while (TRUE) {
        //WaitMessage();
        if (PeekMessage(&msg1, NULL, 1000, 1000, PM_REMOVE)) {
            if (semZ != NULL) {
                CloseHandle(semZ);
            }
            if (cambios != NULL) {
                CloseHandle(cambios);
            }
            exit(0);
        }
        else if (PeekMessage(&msg1, NULL, 1010, 1050, PM_REMOVE)) {
            grupos1 = msg1.wParam;
            pid1 = msg1.lParam;
            gA1 = grupos1 / 10;
            gC1 = grupos1 % 10;
            tipo1 = 1000 + gC1 * 10 + gA1;
            if (PeekMessage(&msg2, NULL, tipo1, tipo1, PM_REMOVE)) {
                waitResult = WaitForSingleObject(cambios, 0L);
                switch (waitResult){
                case WAIT_OBJECT_0:
                    grupos2 = msg2.wParam;
                    pid2 = msg2.lParam;
                    gA2 = grupos2 / 10;
                    gC2 = grupos2 % 10;

                    tipo1 = 1100 + gC1 * 10 + gA1;
                    tipo2 = 1100 + gC2 * 10 + gA2;

                    PostThreadMessage(pid1, tipo1, WPARAM(pid2), LPARAM(1));
                    PostThreadMessage(pid2, tipo2, WPARAM(pid1), LPARAM(1));

                    for (int i = 0; i < 2; i++) {
                        GetMessage(&msg1, NULL, 1050, 1050);
                    }
                    ReleaseSemaphore(cambios, 1, NULL);
                    break;
                case WAIT_TIMEOUT:
                    if (PeekMessage(&msg1, NULL, 1000, 1000, PM_REMOVE)) {
                        if (semZ != NULL) {
                            CloseHandle(semZ);
                        }
                        if (cambios != NULL) {
                            CloseHandle(cambios);
                        }
                        exit(0);
                    }
                }             
            }
            else {
                gA1 = grupos1 / 10;
                gC1 = grupos1 % 10;
                tipo1 = 1100 + gC1 * 10 + gA1;
                PostThreadMessage(pid1, tipo1, NULL, LPARAM(0));
            }
        }
    }

    return 0;
}

DWORD WINAPI Alumnos(LPVOID param) {
    DWORD ZID;
    int posi = (int)param, val, velH, grupoC, tipo, aux;
    char identificador, grupoA, vacio = ' ';
    HANDLE SHijos = NULL, mem = NULL, mtxLib = NULL;
    LPCH refM = NULL;
    HINSTANCE lib = NULL;
    MSG msg;
    char grupo[] = { 'A', 'B', 'C', 'D', 'a', 'b', 'c', 'd',
                     'E', 'F', 'G', 'H', 'e', 'f', 'g', 'h',
                     'I', 'J', 'L', 'M', 'i', 'j', 'l', 'm',
                     'N', 'O', 'P', 'R', 'n', 'o', 'p', 'r' };

    PeekMessage(&msg, NULL, 1000, 2000, PM_NOREMOVE);

    //ABRIMOS EL SEMAFORO
    SHijos = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SHijos");
    if (SHijos == NULL) {
        exit(1);
    }

    //ABRIMOS LA MEMORIA COMPARTIDA
    mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
    if (mem == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }
    refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3 + sizeof(DWORD));
    if (refM == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    //ABRIMOS LA LIBRERIA
    lib = LoadLibrary("cambios2.dll");
    if (lib == NULL) {
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    //FUNCIONES SIN ARGUMENTOS
    FARPROC incrementarCuenta = GetProcAddress(lib, "incrementarCuenta");
    if (incrementarCuenta == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    FARPROC refrescar = GetProcAddress(lib, "refrescar");
    if (refrescar == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    //FUNCIONES CON ARGUMENTOS
    inicioCambiosHijo = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambiosHijo");
    if (inicioCambiosHijo == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    aQueGrupo = (INT(*) (INT)) GetProcAddress(lib, "aQuEGrupo");
    if (aQueGrupo == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    //ABRIMOS EL MUTEX DE LA LIBRERIA
    mtxLib = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mtxLib");
    if (mtxLib == NULL) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        exit(1);
    }

    //OBTENEMOS LA VELOCIDAD
    velH = *((int*)&(glob.refM[88]));

    //ALMACENAMOS EL ID DE ZACARIAS
    ZID = *((DWORD*)&(glob.refM[92]));

    val = inicioCambiosHijo(velH, mtxLib, refM);
    if (val == -1) {
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (SHijos != NULL) {
            CloseHandle(SHijos);
        }
        if (mtxLib != NULL) {
            CloseHandle(mtxLib);
        }
        exit(1);
    }

    //ASIGNACION INICIAL
    if (posi < 8) {
        identificador = grupo[posi];
        grupoA = 1;

        refM = refM + posi * 2;
        CopyMemory(refM, &identificador, sizeof(identificador));
        refM = refM + 1;
        CopyMemory(refM, &grupoA, sizeof(grupoA));

        if (posi == 7) {
            for (int j = 0; j < 2; j++) {
                refM = refM + 1;
                CopyMemory(refM, &vacio, sizeof(vacio));
                refM = refM + 1;
                CopyMemory(refM, &grupoA, sizeof(grupoA));
            }
            refM = refM - 4;
        }

        refM = refM - posi * 2 - 1;
        refrescar();
        ReleaseSemaphore(SHijos, 1, NULL);
    }
    else if (8 <= posi && posi < 16) {
        identificador = grupo[posi];
        posi = posi + 2;
        grupoA = 2;

        refM = refM + posi * 2;
        CopyMemory(refM, &identificador, sizeof(identificador));
        refM = refM + 1;
        CopyMemory(refM, &grupoA, sizeof(grupoA));

        if (posi == 17) {
            for (int j = 0; j < 2; j++) {
                refM = refM + 1;
                CopyMemory(refM, &vacio, sizeof(vacio));
                refM = refM + 1;
                CopyMemory(refM, &grupoA, sizeof(grupoA));
            }
            refM = refM - 4;
        }

        refM = refM - posi * 2 - 1;
        refrescar();
        ReleaseSemaphore(SHijos, 1, NULL);
    }
    else if (16 <= posi && posi < 24) {
        identificador = grupo[posi];
        posi = posi + 4;
        grupoA = 3;

        refM = refM + posi * 2;
        CopyMemory(refM, &identificador, sizeof(identificador));
        refM = refM + 1;
        CopyMemory(refM, &grupoA, sizeof(grupoA));
        if (posi == 27) {
            for (int j = 0; j < 2; j++) {
                refM = refM + 1;
                CopyMemory(refM, &vacio, sizeof(vacio));
                refM = refM + 1;
                CopyMemory(refM, &grupoA, sizeof(grupoA));
            }
            refM = refM - 4;
        }

        refM = refM - posi * 2 - 1;
        refrescar();
        ReleaseSemaphore(SHijos, 1, NULL);
    }
    else {
        identificador = grupo[posi];
        posi = posi + 6;
        grupoA = 4;

        refM = refM + posi * 2;
        CopyMemory(refM, &identificador, sizeof(identificador));
        refM = refM + 1;
        CopyMemory(refM, &grupoA, sizeof(grupoA));

        if (posi == 37) {
            for (int j = 0; j < 2; j++) {
                refM = refM + 1;
                CopyMemory(refM, &vacio, sizeof(vacio));
                refM = refM + 1;
                CopyMemory(refM, &grupoA, sizeof(grupoA));
            }
            refM = refM - 4;
        }

        refM = refM - posi * 2 - 1;
        refrescar();
        ReleaseSemaphore(SHijos, 1, NULL);
    }

    PostThreadMessage(ZID, 1050, NULL, NULL);

    while (TRUE) {
        grupoC = aQueGrupo((int)grupoA);
        if (grupoC != grupoA) {
            tipo = 1000 + grupoA * 10 + grupoC;
            PostThreadMessage(ZID, tipo, WPARAM(grupoA * 10 + grupoC), LPARAM(GetCurrentThreadId()));

            //ESPERAMOS LA CONFIRMACION
            WaitMessage();
            if (PeekMessage(&msg, NULL, 1100 + grupoC * 10 + grupoA, 1100 + grupoC * 10 + grupoA, PM_REMOVE)) {
                if (msg.lParam == 1) {
                    //COMPROBACION PARA QUE AMBOS PROCESOS VAYAN JUNTOS
                    PostThreadMessage(msg.wParam, 1150, NULL, NULL);
                    if (GetMessage(&msg, NULL, 1150, 1150) > 0){
                        WaitForSingleObject(SHijos, INFINITE);
                        for (int i = 10 * (grupoC - 1); i < 10 * grupoC; i++) {
                            if (refM[i * 2] == 32) {
                                refM[posi * 2] = 32;

                                refM[i * 2] = identificador;
                                refM[i * 2 + 1] = grupoC;

                                posi = i;

                                grupoA = grupoC;

                                incrementarCuenta();

                                aux = *((int*)&(glob.refM[84]));
                                aux += 1;
                                *((int*)&(glob.refM[84])) = aux;

                                refrescar();
                                break;
                            }
                        }
                        ReleaseSemaphore(SHijos, 1, NULL);

                        PostThreadMessage(ZID, 1050, NULL, NULL);

                        if (PeekMessage(&msg, NULL, 1000, 1000, PM_REMOVE)) {
                            //CERRAMOS TODO
                            if (lib != NULL) {
                                FreeLibrary(lib);
                            }
                            if (refM != NULL) {
                                UnmapViewOfFile(refM);
                            }
                            if (mem != NULL) {
                                CloseHandle(mem);
                            }
                            if (SHijos != NULL) {
                                CloseHandle(SHijos);
                            }
                            if (mtxLib != NULL) {
                                CloseHandle(mtxLib);
                            }
                            return 0;
                        }
                    }
                }
            }
            else if (PeekMessage(&msg, NULL, 1000, 1000, PM_REMOVE)) {
                //CERRAMOS TODO
                if (lib != NULL) {
                    FreeLibrary(lib);
                }
                if (refM != NULL) {
                    UnmapViewOfFile(refM);
                }
                if (mem != NULL) {
                    CloseHandle(mem);
                }
                if (SHijos != NULL) {
                    CloseHandle(SHijos);
                }
                if (mtxLib != NULL) {
                    CloseHandle(mtxLib);
                }
                return 0;
            }

        }
    }

    return 0;
}

void fin() {
    if (glob.mtxLib != NULL) {
        CloseHandle(glob.mtxLib);
    }
    if (glob.SZ != NULL) {
        CloseHandle(glob.SZ);
    }
    if (glob.mem != NULL) {
        CloseHandle(glob.mem);
    }
    if (glob.refM != NULL) {
        UnmapViewOfFile(glob.refM);
    }
    if (glob.alarma != NULL) {
        CloseHandle(glob.alarma);
    }
    if (glob.lib != NULL) {
        FreeLibrary(glob.lib);
    }
    if (glob.SHijos != NULL) {
        CloseHandle(glob.SHijos);
    }
    if (glob.cambios != NULL) {
        CloseHandle(glob.cambios);
    }

    if (glob.Z != NULL) {
        CloseHandle(glob.Z);
    }

    for (int i = 0; i < 32; i++) {
        if (glob.alumnos[i] != NULL) {
            CloseHandle(glob.alumnos[i]);
        }
    }
}