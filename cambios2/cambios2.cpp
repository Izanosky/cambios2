#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "cambios2.h"

INT(*aQueGrupo) (INT);
INT(*inicioCambios) (INT, HANDLE, CHAR*);
INT(*inicioCambiosHijo) (INT, HANDLE, CHAR*);

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

    HANDLE Z;
    HANDLE alumnos[32];
};

global glob;

void fin();

int main(int argc, char* argv[]) {
    int vel;
    LARGE_INTEGER time;
    if (argc == 2) {
        int valor, cont = 0;

        glob.mtxLib = NULL;
        glob.mem = NULL;
        glob.refM = NULL;
        glob.alarma = NULL;
        glob.lib = NULL;
        glob.Z = NULL;
        glob.SHijos = NULL;

        //MECANISMOS DE SINCRONIZACION
        glob.mtxLib = CreateMutex(NULL, FALSE, "mtxLib");
        if (glob.mtxLib == NULL) {
            fin();
            exit(1);
        }

        glob.SHijos = CreateSemaphoreA(NULL, 0, 1, "SHijos");
        if (glob.SHijos == NULL) {
            fin();
            exit(1);
        }

        glob.SZ = CreateSemaphore(NULL, 0, 1, "SZ");
        if (glob.SZ == NULL) {
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

        //METEMOS LA FUNCION REFRESCAR
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
            exit(1);
        }

        //COPIAMOS LA VELOCIDAD A LA MEMORIA COMPARTIDA
        glob.refM = glob.refM + sizeof(char) * 80 + sizeof(int);
        CopyMemory(glob.refM, &cont, sizeof(cont));
        glob.refM = glob.refM + sizeof(int);
        CopyMemory(glob.refM, &vel, sizeof(vel));
        glob.refM = glob.refM - sizeof(char) * 80 - sizeof(int) * 2;
        refrescar();

        //CREAMOS UN HILO
        DWORD ZID;
        glob.Z = CreateThread(NULL, 0, Zacarias, NULL, 0, &ZID);
        if (glob.Z == NULL) {
            fin();
            exit(1);
        }

        //ESPERAMOS A QUE ZACARIAS SE HAYA CREADO
        WaitForSingleObject(glob.SZ, INFINITE);
        glob.refM = glob.refM + sizeof(char) * 80 + sizeof(int) * 3;
        CopyMemory(glob.refM, &ZID, sizeof(ZID));
        glob.refM = glob.refM - sizeof(char) * 80 - sizeof(int) * 3;
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

        WaitForSingleObject(glob.alarma, INFINITE);

        PostThreadMessage(ZID, 1000, NULL, NULL);
        for (int j = 0; j < 32; j++) {
            PostThreadMessage(alumnosID[j], 1000, NULL, NULL);
        }

        WaitForMultipleObjects(32, glob.alumnos, TRUE, INFINITE);
        WaitForSingleObject(glob.Z, INFINITE);

        fin();

        return 0;
    }
    else {
        printf("ERROR\r\n");
        printf("Ejemplo de uso: (nombre ejecutable) (velocidad)\r\n");
        fflush(stdout);
        return 1;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI Alumnos(LPVOID param) {
    int posi = (int)param, val, velH;
    char identificador, grupoA;
    char vacio = ' ';
    DWORD ZID;
    HANDLE SHijos = NULL, mem = NULL, mtxHijos = NULL;
    LPCH refM = NULL;
    HINSTANCE lib = NULL;

    MSG msg;
    PeekMessage(&msg, NULL, 1000, 1000, PM_NOREMOVE);

    char grupo[] = { 'A', 'B', 'C', 'D', 'a', 'b', 'c', 'd',
                     'E', 'F', 'G', 'H', 'e', 'f', 'g', 'h',
                     'I', 'J', 'L', 'M', 'i', 'j', 'l', 'm',
                     'N', 'O', 'P', 'R', 'n', 'o', 'p', 'r' };

    //ABRIMOS EL SEMAFORO
    SHijos = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SHijos");
    if (SHijos == NULL) {
        exit(1);
    }

    //ABRIMOS LA LIBRERIA
    lib = LoadLibrary("cambios2.dll");
    if (lib == NULL) {
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

    //FUNCIONES SIN ARGUMENTOS
    FARPROC fnCambios2 = GetProcAddress(lib, "fncambios2");
    if (fnCambios2 == NULL) {
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

    FARPROC aQuEGrupo = GetProcAddress(lib, "aQuEGrupo");
    if (aQuEGrupo == NULL) {
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

    //ABRIMOS EL MUTEX DE LA LIBRERIA
    mtxHijos = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mtxLib");
    if (mtxHijos == NULL) {
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
    refM = refM + sizeof(char) * 80 + sizeof(int) * 2;
    velH = *(int*)refM;
    refM = refM - sizeof(char) * 80 - sizeof(int) * 2;

    val = inicioCambiosHijo(velH, mtxHijos, refM);
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
        if (mtxHijos != NULL) {
            CloseHandle(mtxHijos);
        }
        exit(1);
    }

    //ALMACENAMOS EL ID DE ZACARIAS
    refM = refM + sizeof(char) * 80 + sizeof(int) * 3;
    ZID = *(DWORD*)refM;
    refM = refM - sizeof(char) * 80 - sizeof(int) * 3;

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
        if (GetMessage(&msg, NULL, 1000, 1000) > 0) {
            //BORRAMOS TODOS LOS RECURSOS
            if (SHijos != NULL) {
                CloseHandle(SHijos);
            }
            if (lib != NULL) {
                FreeLibrary(lib);
            }
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
            return 0;
        }
    }
}

DWORD WINAPI Zacarias(LPVOID param) {
    MSG msg;
    HANDLE semZ = NULL;
    HANDLE mem = NULL;
    LPCH refM = NULL;
    DWORD ZID;

    PeekMessage(&msg, NULL, 1000, 1050, PM_NOREMOVE);

    semZ = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "SZ");
    if (semZ == NULL) {
        exit(1);
    }

    //ABRIMOS LA MEMORIA COMPPARTIDA
    mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
    if (mem == NULL) {
        if (semZ != NULL) {
            CloseHandle(semZ);
        }
        exit(1);
    }
    refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3 + sizeof(DWORD));
    if (refM == NULL) {
        if (semZ != NULL) {
            CloseHandle(semZ);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        exit(1);
    }

    ReleaseSemaphore(semZ, 1, NULL);

    for (int i = 0; i < 32; i++) {
        GetMessage(&msg, NULL, 1050, 1050);
    }

    if (GetMessage(&msg, NULL, 1000, 1050) > 0) {
        if (semZ != NULL) {
            CloseHandle(semZ);
        }
        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        return 0;
    }

    while (TRUE) {

    }
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

    WaitForSingleObject(glob.Z, INFINITE);
    WaitForMultipleObjects(32, glob.alumnos, TRUE, INFINITE);

    if (glob.Z != NULL) {
        CloseHandle(glob.Z);
    }

    for (int i = 0; i < 32; i++) {
        if (glob.alumnos[i] != NULL) {
            CloseHandle(glob.alumnos[i]);
        }
    }
}