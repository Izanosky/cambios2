// El siguiente bloque ifdef muestra la forma estándar de crear macros que facilitan 
// la exportación de archivos DLL. Todos los archivos de este archivo DLL se compilan con el símbolo CAMBIOS2_EXPORTS
// definido en la línea de comandos. Este símbolo no se debe definir en ningún proyecto
// que utilice este archivo DLL. De este modo, otros proyectos cuyos archivos de código fuente incluyan el archivo
// interpreta que las funciones CAMBIOS2_API se importan de un archivo DLL, mientras que este archivo DLL interpreta los símbolos
// definidos en esta macro como si fueran exportados.
#ifdef CAMBIOS2_EXPORTS
#define CAMBIOS2_API __declspec(dllexport)
#else
#define CAMBIOS2_API __declspec(dllimport)
#endif

#define PERROR(a) \
    {             \
        LPVOID lpMsgBuf;                                         \
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |           \
                   FORMAT_MESSAGE_FROM_SYSTEM |                  \
                   FORMAT_MESSAGE_IGNORE_INSERTS, NULL,          \
                   GetLastError(),                               \
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),    \
                   (LPTSTR) &lpMsgBuf,0,NULL );                  \
        fprintf(stderr,"%s:(%d)%s\n",a,GetLastError(),lpMsgBuf); \
        LocalFree( lpMsgBuf );                                   \
    }    


#define LETRAS "ABCDEFGHIJLMNOPRabcdefghijlmnopr"

#ifdef CAMBIOS2_EXPORTS
extern "C" CAMBIOS2_API int fncambios2(void);
extern "C" CAMBIOS2_API  int aQuEGrupo(int grupoActual);
extern "C" CAMBIOS2_API void pon_error(char *mensaje);
extern "C" CAMBIOS2_API void incrementarCuenta(void);
extern "C" CAMBIOS2_API int refrescar(void);
extern "C" CAMBIOS2_API int inicioCambios(int ret, HANDLE semAforos, char *z);
extern "C" CAMBIOS2_API int inicioCambiosHijo(int ret, HANDLE semAforos, char *z);
extern "C" CAMBIOS2_API int finCambios(void);
#endif
