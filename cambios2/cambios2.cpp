#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "cambios2.h"

//   C:\Users\Usuario\source\repos\cambios2\Debug\cambios2.exe

INT(*aQueGrupo) (INT);
INT(*inicioCambios) (INT, HANDLE, CHAR*);
INT(*inicioCambiosHijo) (INT, HANDLE, CHAR*);

DWORD WINAPI Zacarias (LPVOID param);
DWORD WINAPI Alumnos (LPVOID param);

int main(int argc, char* argv[]) {
    int vel;
    LARGE_INTEGER time;

    //VARIABLES PARA LA CREACIÓN DEL PROCESO HIJO
    CHAR szPath[MAX_PATH];
    STARTUPINFO info;
    PROCESS_INFORMATION pInfo;

    if (argc == 4) {
        int valor;
        GetModuleFileName(NULL, szPath, MAX_PATH);
        ZeroMemory(&info, sizeof(info));
        info.cb = sizeof(info);
        ZeroMemory(&pInfo, sizeof(pInfo));

		//MECANISMOS DE SINCRONIZACION
		HANDLE mtxLib = CreateMutex(NULL, FALSE, "mtxLib");
		if (mtxLib == NULL) {
			printf("Error al crear el mutex (%d)\n", GetLastError());
            exit(1);
		}

		//CREAMOS LA MEMORIA COMPARTIDA
        HANDLE mem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(char)*80 + sizeof(int)*3, "memoria");
		if (mem == NULL) {
			printf("Error al crear la memoria compartida (%d)\n", GetLastError());
            exit(1);
		}

		LPCH refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3);
		if (refM == NULL) {
			printf("Error al mapear la memoria compartida (%d)\n", GetLastError());
            exit(1);
		}

		//CREAMOS LA ALARMA Y /ESTABLECEMOS EL TIEMPO
        HANDLE alarma = CreateWaitableTimer(NULL, TRUE, NULL);
        if (alarma == NULL) {
            printf("Error al crear la alarma (%d)\n", GetLastError());
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

        if (!SetWaitableTimer(alarma, &time, 0, NULL, NULL, 0)) {
            printf("Error al establecer la alarma (%d)\n", GetLastError());
            exit(1);
        }

		//CARGAMOS LA LIBRERIA
        HINSTANCE lib = LoadLibrary("cambios2.dll");

        if (lib == NULL) {
            printf("No se pudo cargar la libreria\r\n");
            fflush(stdout);
            exit(1);
        } 

        //FUNCIONES SIN ARGUMENTOS
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
        inicioCambios = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambios");
        if (inicioCambios == NULL) {
            printf("No se ha podido cargar la funcion\r\n");
            fflush(stdout);
            exit(1);
        }   

		//COPIAMOS LA VELOCIDAD A LA MEMORIA COMPARTIDA
		refM = refM + sizeof(char) * 80 + sizeof(int)*2;
		CopyMemory(refM, &vel, sizeof(vel));
        refM = refM - sizeof(char) * 80 - sizeof(int) * 2;

		valor = inicioCambios(vel, mtxLib, refM);
		if (valor == -1) {
			printf("Error al iniciar los cambios\r\n");
			fflush(stdout);
            exit(1);
        }

        //CREAMOS UN HILO
		HANDLE Z = CreateThread(NULL, 0, Zacarias, NULL, 0, NULL);
        if (Z == NULL) {
            printf("Error al crear el hilo (%d)\r\n", GetLastError());
            fflush(stdout);
            exit(1);
        }

        //CREAMOS EL PROCESO HIJO
        if (!CreateProcess(szPath, argv[3], NULL, NULL, FALSE, 0, NULL, NULL, &info, &pInfo)) {
            printf("Error al crear el hijo (%d)\r\n", GetLastError());
            fflush(stdout);
            exit(1);
        }

        ///////////////////////////////////////////////
        if (WaitForSingleObject(alarma, INFINITE) != WAIT_OBJECT_0)
            printf("Error con la alarma (%d)\n", GetLastError());
        ///////////////////////////////////////////////

        //ESPERAMOS A QUE EL PROCESO HIJO TERMINE
        WaitForSingleObject(Z, INFINITE);
		CloseHandle(Z);

        WaitForSingleObject(pInfo.hProcess, INFINITE);
        CloseHandle(pInfo.hThread);
        CloseHandle(pInfo.hProcess);

        //LIBREAMOS LIBRERIA
        FreeLibrary(lib);	   

		//LIBERAMOS MEMORIA COMPARTIDA
		UnmapViewOfFile(refM);
		CloseHandle(mem);

		//LIBERAMOS EL MUTEX
		CloseHandle(mtxLib);

        return 0;
	} //----------------------------------------------------------------------------------- FUNCION DEL PROCESO HIJO
    else if (argc == 1 && _tcscmp(argv[0], _T("h")) == 0) {
        int velH, val;
        
		//INICIAMOS LA MEMORIA COMPARTIDA
		HANDLE mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
		if (mem == NULL) {
			printf("Error al abrir la memoria compartida (%d)\n", GetLastError());
			fflush(stdout);
            exit(1);
		}

		LPCH refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3);
		if (refM == NULL) {
			printf("Error al mapear la memoria compartida (%d)\n", GetLastError());
			fflush(stdout);
            exit(1);
		}

		refM = refM + sizeof(char) * 80 + sizeof(int) * 2;
        velH = *refM;
		refM = refM - sizeof(char) * 80 - sizeof(int) * 2;

        //ABRIMOS EL MUTEX PARA LA LIBRERIA
		HANDLE mtxLib = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mtxLib");
		if (mtxLib == NULL) {
			printf("Error al abrir el mutex (%d)\n", GetLastError());
			fflush(stdout);
			exit(1);
		}

		//MECANISMO DE SINCRONIZACION
		HANDLE event = CreateEvent(NULL, FALSE, FALSE, "CreacionHijos");

		//CARGAMOS LA LIBRERIA
        HINSTANCE lib = LoadLibrary("cambios2.dll");

        if (lib == NULL) {
            printf("No se pudo cargar la libreria\r\n");
            fflush(stdout);
            exit(1);
        }

		//FUNCIONES SIN ARGUMENTOS
        FARPROC fnCambios2 = GetProcAddress(lib, "fncambios2");
        if (fnCambios2 == NULL) {
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

		//FUNCIONES CON ARGUMENTOS
        inicioCambiosHijo = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambiosHijo");
        if (inicioCambiosHijo == NULL) {
            printf("No se ha podido cargar la funcion\r\n");
            fflush(stdout);
            exit(1);
        }

        aQueGrupo = (INT(*) (INT)) GetProcAddress(lib, "aQuEGrupo");
        if (aQueGrupo == NULL) {
            printf("No se ha podido cargar la funcion\r\n");
            fflush(stdout);
            exit(1);
        }

		val = inicioCambiosHijo(0, mtxLib, refM);

        HANDLE alumnos[32];

        for (int i = 0; i < 32; i++) {
			alumnos[i] = CreateThread(NULL, 0, Alumnos, LPVOID(i), 0, NULL);
			if (alumnos[i] == NULL) {
				printf("Error al crear el hilo (%d)\r\n", GetLastError());
				fflush(stdout);
				exit(1);
			}
        }

        FreeLibrary(lib);

		//LIBERAMOS MEMORIA COMPARTIDA
		UnmapViewOfFile(refM);
		CloseHandle(mem);

        return 0;
    }
    else {
		printf("ERROR\r\n");
		printf("Ejemplo de uso: (nombre ejecutable) (velocidad) p h\r\n");
		fflush(stdout);
		return 1;
    }
}

DWORD WINAPI Zacarias (LPVOID param) {

    return 0;
}

DWORD WINAPI Alumnos (LPVOID param) {
    int posi = (int)param;
    int aux;
    char identificador;

    char grupo1[] = {'A', 'B', 'C', 'D', 'a', 'b', 'c', 'd'};
    char grupo2[] = { 'E', 'F', 'G', 'H', 'e', 'f', 'g', 'h'};
    char grupo3[] = { 'I', 'J', 'L', 'M', 'i', 'j', 'l', 'm'};
    char grupo4[] = { 'N', 'O', 'P', 'R', 'n', 'o', 'p', 'r'};

	HANDLE ev = OpenEvent(EVENT_ALL_ACCESS, FALSE, "CreacionHijos");

    //INICIAMOS LA MEMORIA COMPARTIDA
    HANDLE mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
    if (mem == NULL) {
        printf("Error al abrir la memoria compartida (%d)\n", GetLastError());
        fflush(stdout);
        exit(1);
    }

    LPCH refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3);
    if (refM == NULL) {
        printf("Error al mapear la memoria compartida (%d)\n", GetLastError());
        fflush(stdout);
        exit(1);
    }

    //ASIGNACION INICIAL
    if (posi < 8) {
		identificador = grupo1[posi];

        if (posi == 7) {

        }
        //INICNIAMOS EL EVENTO

	}
	else if (8 <= posi < 16) {
        aux = posi - 8;
		identificador = grupo2[aux];
        posi += 2;

        if (posi == 17) {

        }
	}
	else if (16 <= posi < 24) {
		aux = posi - 16;
		identificador = grupo3[aux];
		posi += 4;

		if (posi == 27) {

		}
	}
	else {
		aux = posi - 24;
		identificador = grupo4[aux];
		posi += 6;

		if (posi == 37) {

		}
	}

	UnmapViewOfFile(refM);
	CloseHandle(mem);

    return 0;
}