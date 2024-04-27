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

struct global {
    HANDLE mtxLib;
    HANDLE mem;
    LPCH refM;
    HANDLE alarma;
	HINSTANCE lib;
    HANDLE Z;
    CHAR szPath[MAX_PATH];
    STARTUPINFO info;
    PROCESS_INFORMATION pInfo;
    HANDLE hProcess;
};

global glob;

void fin();

int main(int argc, char* argv[]) {
    int vel;
    LARGE_INTEGER time;

    //VARIABLES PARA LA CREACIÓN DEL PROCESO HIJO
    

    if (argc == 4) {      
        int valor, cont = 0;
        GetModuleFileName(NULL, glob.szPath, MAX_PATH);
        ZeroMemory(&glob.info, sizeof(glob.info));
        glob.info.cb = sizeof(glob.info);
        ZeroMemory(&glob.pInfo, sizeof(glob.pInfo));

		glob.mtxLib = NULL;
		glob.mem = NULL;
		glob.refM = NULL;
		glob.alarma = NULL;
		glob.lib = NULL;
		glob.Z = NULL;
		glob.pInfo.hProcess = NULL;

		//MECANISMOS DE SINCRONIZACION
		glob.mtxLib = CreateMutex(NULL, FALSE, "mtxLib");
		if (glob.mtxLib == NULL) {
            fin();
            exit(1);
		}

		//CREAMOS LA MEMORIA COMPARTIDA
        glob.mem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(char)*80 + sizeof(int)*3 + sizeof(DWORD), "memoria");
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

        //FUNCIONES CON ARGUMENTOS
        inicioCambios = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(glob.lib, "inicioCambios");
        if (inicioCambios == NULL) {
            fin();
            exit(1);
        }   

		//COPIAMOS LA VELOCIDAD A LA MEMORIA COMPARTIDA
		glob.refM = glob.refM + sizeof(char) * 80 + sizeof(int);
        CopyMemory(glob.refM, &cont, sizeof(cont));
        glob.refM = glob.refM + sizeof(int);
		CopyMemory(glob.refM, &vel, sizeof(vel));
        glob.refM = glob.refM - sizeof(char) * 80 - sizeof(int) * 2;

		valor = inicioCambios(vel, glob.mtxLib, glob.refM);
		if (valor == -1) {
			printf("Error al iniciar los cambios\r\n");
			fflush(stdout);
            exit(1);
        }

        //CREAMOS UN HILO
        DWORD ZID;
		glob.Z = CreateThread(NULL, 0, Zacarias, NULL, 0, &ZID);
        if (glob.Z == NULL) {
            fin();
            exit(1);
        }

        glob.refM = glob.refM + sizeof(char) * 80 + sizeof(int) * 3;
        CopyMemory(glob.refM, &ZID, sizeof(ZID));
        glob.refM = glob.refM - sizeof(char) * 80 - sizeof(int) * 3;

        //CREAMOS EL PROCESO HIJO
        if (!CreateProcess(glob.szPath, argv[3], NULL, NULL, FALSE, 0, NULL, NULL, &glob.info, &glob.pInfo)) {
            fin();
            exit(1);
        }

		glob.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, glob.pInfo.dwProcessId);
		if (glob.hProcess == NULL) {
            PostThreadMessage(ZID, 1000, NULL, NULL);
            PostThreadMessage(glob.pInfo.dwProcessId, 1000, NULL, NULL);

            fin();
			exit(1);
		}

        WaitForSingleObject(glob.alarma, INFINITE);

		PostThreadMessage(ZID, 1000, NULL, NULL);
        PostThreadMessage(GetProcessId(glob.hProcess), 1000, NULL, NULL);

        fin();

        return 0;
	} //----------------------------------------------------------------------------------- FUNCION DEL PROCESO HIJO
    else if (argc == 1 && _tcscmp(argv[0], _T("h")) == 0) {
        int velH, val;
		HANDLE mem = NULL, mtxLib = NULL, mutexHijos = NULL;
		LPCH refM = NULL;
		HINSTANCE lib = NULL;
        
        MSG msg;
        PeekMessage(&msg, NULL, 1000, 1000, PM_NOREMOVE);
        
		//INICIAMOS LA MEMORIA COMPARTIDA
		mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
		if (mem == NULL) {
            exit(1);
		}

		refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3 + +sizeof(DWORD));
		if (refM == NULL) {
            if (mem != NULL) {
				CloseHandle(mem);
            }
            exit(1);
		}

		refM = refM + sizeof(char) * 80 + sizeof(int) * 2;
        velH = *refM;
		refM = refM - sizeof(char) * 80 - sizeof(int) * 2;

        //ABRIMOS EL MUTEX PARA LA LIBRERIA
		mtxLib = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mtxLib");
		if (mtxLib == NULL) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
			exit(1);
		}

		//MECANISMO DE SINCRONIZACION
		mutexHijos = CreateMutex(NULL, FALSE, "mtxHijos");
		if (mutexHijos == NULL) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
			if (mtxLib != NULL) {
				CloseHandle(mtxLib);
			}
			exit(1);
		}

		//CARGAMOS LA LIBRERIA
        lib = LoadLibrary("cambios2.dll");

        if (lib == NULL) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
            if (mtxLib != NULL) {
                CloseHandle(mtxLib);
            }
			if (mutexHijos != NULL) {
				CloseHandle(mutexHijos);
			}
            exit(1);
        }

		//FUNCIONES SIN ARGUMENTOS
        FARPROC fnCambios2 = GetProcAddress(lib, "fncambios2");
        if (fnCambios2 == NULL) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
            if (mtxLib != NULL) {
                CloseHandle(mtxLib);
            }
            if (mutexHijos != NULL) {
                CloseHandle(mutexHijos);
            }
			if (lib != NULL) {
				FreeLibrary(lib);
			}
            exit(1);
        }

		//FUNCIONES CON ARGUMENTOS
        inicioCambiosHijo = (INT(*) (INT, HANDLE, CHAR*)) GetProcAddress(lib, "inicioCambiosHijo");
        if (inicioCambiosHijo == NULL) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
            if (mtxLib != NULL) {
                CloseHandle(mtxLib);
            }
            if (mutexHijos != NULL) {
                CloseHandle(mutexHijos);
            }
            if (lib != NULL) {
                FreeLibrary(lib);
            }
            exit(1);
        }

		val = inicioCambiosHijo(velH, mtxLib, refM);
        if (val == -1) {
            if (refM != NULL) {
                UnmapViewOfFile(refM);
            }
            if (mem != NULL) {
                CloseHandle(mem);
            }
            if (mtxLib != NULL) {
                CloseHandle(mtxLib);
            }
            if (mutexHijos != NULL) {
                CloseHandle(mutexHijos);
            }
            if (lib != NULL) {
                FreeLibrary(lib);
            }
            exit(1);
        }

        HANDLE alumnos[32];
        for (int j = 0; j < 32; j++) {
			alumnos[j] = NULL;
        }

		DWORD alumnosID[32];

        for (int i = 0; i < 32; i++) {		
			alumnos[i] = CreateThread(NULL, 0, Alumnos, LPVOID(i), 0, &alumnosID[i]);
			if (alumnos[i] == NULL) {
                if (refM != NULL) {
                    UnmapViewOfFile(refM);
                }
                if (mem != NULL) {
                    CloseHandle(mem);
                }
                if (mtxLib != NULL) {
                    CloseHandle(mtxLib);
                }
                if (mutexHijos != NULL) {
                    CloseHandle(mutexHijos);
                }
                if (lib != NULL) {
                    FreeLibrary(lib);
                }
                for (int j = 0; j < 32; j++) {
					if (alumnos[j] != NULL) {
						CloseHandle(alumnos[j]);
					}
                }
				exit(1);
			}
        }

        if (GetMessage(&msg, NULL, 1000, 1000) > 0) {
			printf("Mensaje recibido, soy el proceso hijo\r\n");
            for (int j = 0; j < 32; j++) {
				PostThreadMessage(alumnosID[j], 1000, NULL, NULL);
            }
        }

		WaitForMultipleObjects(32, alumnos, TRUE, INFINITE);

        if (refM != NULL) {
            UnmapViewOfFile(refM);
        }
        if (mem != NULL) {
            CloseHandle(mem);
        }
        if (mtxLib != NULL) {
            CloseHandle(mtxLib);
        }
        if (mutexHijos != NULL) {
            CloseHandle(mutexHijos);
        }
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        for (int j = 0; j < 32; j++) {
            if (alumnos[j] != NULL) {
                CloseHandle(alumnos[j]);
            }
        }

        return 0;
    }
    else {
		printf("ERROR\r\n");
		printf("Ejemplo de uso: (nombre ejecutable) (velocidad) p h\r\n");
		fflush(stdout);
		return 1;
    }
}

DWORD WINAPI Alumnos (LPVOID param) {
    int posi = (int)param;
    char identificador, grupoA;
    char vacio = ' ';
	DWORD ZID;
	HANDLE mutexHijos = NULL, mem = NULL;
	LPCH refM = NULL;
	HINSTANCE lib = NULL;

    MSG msg;
    PeekMessage(&msg, NULL, 1000, 1000, PM_NOREMOVE);

    char grupo[] = { 'A', 'B', 'C', 'D', 'a', 'b', 'c', 'd',
                     'E', 'F', 'G', 'H', 'e', 'f', 'g', 'h',
                     'I', 'J', 'L', 'M', 'i', 'j', 'l', 'm',
                     'N', 'O', 'P', 'R', 'n', 'o', 'p', 'r' };

	mutexHijos = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mtxHijos");
    if (mutexHijos == NULL) {    
		exit(1);
    }

    //CARGAMOS LA LIBRERIA
    lib = LoadLibrary("cambios2.dll");

    if (lib == NULL) {
		if (mutexHijos != NULL) {
			CloseHandle(mutexHijos);
		}
        exit(1);
    }

	//FUNCIONES SIN ARGUMENTOS
    FARPROC incrementarCuenta = GetProcAddress(lib, "incrementarCuenta");
    if (incrementarCuenta == NULL) {
		if (mutexHijos != NULL) {
			CloseHandle(mutexHijos);
		}
		if (lib != NULL) {
			FreeLibrary(lib);
		}
        exit(1);
    }

    FARPROC refrescar = GetProcAddress(lib, "refrescar");
    if (refrescar == NULL) {
        if (mutexHijos != NULL) {
            CloseHandle(mutexHijos);
        }
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        exit(1);
    }

	//FUNCIONES CON ARGUMENTOS
    aQueGrupo = (INT(*) (INT)) GetProcAddress(lib, "aQuEGrupo");
    if (aQueGrupo == NULL) {
        if (mutexHijos != NULL) {
            CloseHandle(mutexHijos);
        }
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        exit(1);
    }

    //INICIAMOS LA MEMORIA COMPARTIDA
    mem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "memoria");
    if (mem == NULL) {
        if (mutexHijos != NULL) {
            CloseHandle(mutexHijos);
        }
        if (lib != NULL) {
            FreeLibrary(lib);
        }
        exit(1);
    }

    refM = (LPCH)MapViewOfFile(mem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(char) * 80 + sizeof(int) * 3 + sizeof(DWORD));
    if (refM == NULL) {
        if (mutexHijos != NULL) {
            CloseHandle(mutexHijos);
        }
        if (lib != NULL) {
            FreeLibrary(lib);
        }
		if (mem != NULL) {
			CloseHandle(mem);
		}
        exit(1);
    }

    //ASIGNACION INICIAL
	WaitForSingleObject(mutexHijos, INFINITE);
    if (posi < 8) {
		identificador = grupo[posi];
        grupoA = 1;

        //ALAMCENAMOS EL ID DE ZACACRIAS
		refM = refM + sizeof(char) * 80 + sizeof(int) * 3;
		ZID = *refM;
		refM = refM - sizeof(char) * 80 - sizeof(int) * 3;

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
		ReleaseMutex(mutexHijos);
	}
	else if (8 <= posi && posi < 16) {
		identificador = grupo[posi];
        posi = posi + 2;
        grupoA = 2;

        //ALAMCENAMOS EL ID DE ZACACRIAS
        refM = refM + sizeof(char) * 80 + sizeof(int) * 3;
        ZID = *refM;
        refM = refM - sizeof(char) * 80 - sizeof(int) * 3;

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
        ReleaseMutex(mutexHijos);
	}
	else if (16 <= posi && posi < 24) {
		identificador = grupo[posi];
        posi = posi + 4;
        grupoA = 3;

        //ALAMCENAMOS EL ID DE ZACACRIAS
        refM = refM + sizeof(char) * 80 + sizeof(int) * 3;
        ZID = *refM;
        refM = refM - sizeof(char) * 80 - sizeof(int) * 3;

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

        refrescar();
        refM = refM - posi * 2 - 1;
        ReleaseMutex(mutexHijos);
	}
	else {
		identificador = grupo[posi];
        posi = posi + 6;
        grupoA = 4;
        
		//ALAMCENAMOS EL ID DE ZACACRIAS
        refM = refM + sizeof(char) * 80 + sizeof(int) * 3;
        ZID = *refM;
        refM = refM - sizeof(char) * 80 - sizeof(int) * 3;

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

        refrescar();
        refM = refM - posi * 2 - 1;
        ReleaseMutex(mutexHijos);
	} 

    while (TRUE) {
        if (GetMessage(&msg, NULL, 1000, 1000) > 0) {
			printf("Mensaje recibido, me muero\r\n");

            FreeLibrary(lib);

            UnmapViewOfFile(refM);
            CloseHandle(mem);

			CloseHandle(mutexHijos);

			return 0;
        }
    }
}

DWORD WINAPI Zacarias(LPVOID param) {
    MSG msg;
	PeekMessage(&msg, NULL, 1000, 1000, PM_NOREMOVE);

    while (TRUE) {
        if (GetMessage(&msg, NULL, 1000, 1000) > 0) {
            printf("Mensaje recibido, me muero\r\n");
            return 0;
        }
    }
}

void fin() {
    if (glob.pInfo.hProcess != NULL) {
        WaitForSingleObject(glob.pInfo.hProcess, INFINITE);
        CloseHandle(glob.pInfo.hThread);
        CloseHandle(glob.pInfo.hProcess);
    }
    if (glob.Z != NULL) {
        WaitForSingleObject(glob.Z, INFINITE);
        CloseHandle(glob.Z);
    }
    if (glob.mtxLib != NULL) {
		CloseHandle(glob.mtxLib);
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
	if (glob.hProcess != NULL) {
		CloseHandle(glob.hProcess);
	}
}