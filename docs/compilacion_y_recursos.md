# compilacion y recursos

este documento explica la configuracion de los scripts de construccion, las banderas del compilador, la composicion de los recursos embebidos y el manifiesto de la aplicacion.

## script de construccion (build.bat)

el archivo build.bat permite compilar el proyecto sin requerir entornos de desarrollo pesados ni configuraciones complejas de cmake. implementa autodeteccion de la cadena de herramientas (toolchain) disponible en el sistema.

### flujo de deteccion

1. comprueba si g++ (gcc / mingw-w64) esta registrado en la variable de entorno path mediante where g++.
2. si g++ esta presente, procede a compilar el archivo de recursos con windres y enlazar con g++.
3. si g++ no existe, busca la presencia del compilador de visual studio (cl.exe) mediante where cl.
4. si cl.exe esta activo en el entorno, compila los recursos con rc.exe y el codigo fuente con cl.exe.
5. si ninguna herramienta esta disponible, el script interrumpe la ejecucion devolviendo codigo de error 1.

### banderas para gcc / mingw-w64

la invocacion realizada por el script es:

```bat
windres -i resource.rc -o resource.o
g++ -std=c++17 -O2 -mwindows -municode main.cpp resource.o -o unkown_viewer.exe -static -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32
```

explicacion tecnica de cada parametro:

- -std=c++17: habilita las caracteristicas del estandar c++17 requeridas por el codigo fuente (constexpr avanzadas, tipos estructurados, deduccion de tipos).
- -O2: optimizacion de segundo nivel del compilador, aplicando eliminacion de subexpresiones comunes, vectorizacion de bucles y reordenamiento de instrucciones en ensamblador.
- -mwindows: configura el subsistema del binario pe (portable executable) como aplicacion gui de windows (subsystem windows). esto evita que el sistema operativo cree una consola negra auxiliar al iniciar el programa.
- -municode: instruye al enlazador de mingw para que utilice la funcion wwinaimcrtstartup como punto de entrada de la biblioteca en tiempo de ejecucion de c, reconociendo correctamente la funcion wwinaim con parametros unicode (lpwstr).
- -static -static-libgcc -static-libstdc++: fuerza el enlace estatico de todos los componentes del lenguaje c y c++. sin estas banderas, el ejecutable requeriria bibliotecas dinamicas externas (como libgcc_s_seh-1.dll o libstdc++-6.dll) para ejecutarse en otros equipos. con estas banderas, el binario resultante es un archivo completamente autonomo.
- -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32: vincula las bibliotecas de importacion del sistema operativo que resuelven las llamadas a la api win32 empleadas por la aplicacion.

### banderas para msvc (cl.exe)

si el usuario ejecuta el script dentro del entorno de herramientas de desarrollador de visual studio (developer command prompt):

```bat
rc /nologo /fo resource.res resource.rc
cl /nologo /std:c++17 /O2 /W3 /EHsc main.cpp resource.res /Fe:unkown_viewer.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib kernel32.lib
```

- /std:c++17: modo c++17 en el frontend de msvc.
- /O2: optimizacion para maxima velocidad de ejecucion.
- /W3: nivel 3 de advertencias para verificacion estricta de tipos.
- /EHsc: modelo estandar de excepciones sincrono de c++.
- /SUBSYSTEM:WINDOWS: define el subsistema grafico en el linker de microsoft (link.exe).

## archivo de recursos (resource.rc)

el archivo resource.rc define la inclusion de componentes binarios no ejecutables en la seccion .rsrc del ejecutable pe:

```rc
#include <windows.h>
#include "resource.h"

IDI_APP_ICON ICON "app.ico"
1 RT_MANIFEST "app.manifest"

VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_APP
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName",      "NTASH"
            VALUE "FileDescription",  "Unkown Viewer"
            VALUE "FileVersion",      "1.0.0.0"
            VALUE "InternalName",     "unkown_viewer"
            VALUE "OriginalFilename", "unkown_viewer.exe"
            VALUE "ProductName",      "Unkown Viewer"
            VALUE "ProductVersion",   "1.0.0.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
```

componentes incluidos:

1. idi_app_icon (id 101 definido en resource.h): referencia al archivo fisico app.ico. este icono es cargado tanto para la representacion del proceso en el explorador de windows como para los iconos de barra de titulo (hicon) y barra de tareas (hiconsm).
2. 1 rt_manifest: incrusta el archivo de manifiesto xml dentro del binario sin requerir archivos .manifest externos sueltos en el disco.
3. vs_version_info: estructura estandar de metadatos de windows que permite al explorador y a las herramientas de diagnostico consultar la version del producto, nombre interno y derechos de autor.

## manifiesto de la aplicacion (app.manifest)

el archivo app.manifest es un documento xml estructurado que declara capacidades de plataforma requeridas por la aplicacion antes de que el kernel comience la ejecucion:

### activacion de common controls v6

declara una dependencia sobre la version 6.0.0.0 de microsoft.windows.common-controls:

```xml
<dependency>
  <dependentAssembly>
    <assemblyIdentity
        type="win32"
        name="Microsoft.Windows.Common-Controls"
        version="6.0.0.0"
        processorArchitecture="*"
        publicKeyToken="6595b64144ccf1df"
        language="*"/>
  </dependentAssembly>
</dependency>
```

esta directiva activa los temas visuales modernos de windows sobre los controles comunes. sin ella, las barras de desplazamiento (scrollbars) del control edit se dibujarian con el aspecto visual clasico de windows 95.

### soporte per-monitor v2 para alta resolucion (dpi)

declara la compatibilidad con pantallas de alta densidad de pixeles (4k, pantallas retina en laptops):

```xml
<application xmlns="urn:schemas-microsoft-com:asm.v3">
  <windowsSettings>
    <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
    <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor</dpiAwareness>
  </windowsSettings>
</application>
```

con permonitorv2, windows no escala la ventana mediante estiramiento artificial de mapas de bits (lo cual causaria tipografia borrosa), sino que permite que el procedimiento de ventana reciba mensajes de cambio de escala y calcule las alturas de fuente y coordenadas en pixeles fisicos reales de cada monitor.

## script de ejecucion (run.bat)

el archivo run.bat automatiza el ciclo de prueba y ejecucion diaria:

```bat
@echo off
setlocal

cd /d "%~dp0"

if not exist unkown_viewer.exe (
    echo [Unkown Viewer] Ejecutable no encontrado. Compilando primero...
    call build.bat
    if errorlevel 1 exit /b 1
)

echo [Unkown Viewer] Lanzando unkown_viewer.exe...
start "" unkown_viewer.exe %*
```

su comportamiento es el siguiente:

1. fija el directorio de trabajo a la carpeta donde reside el script (%~dp0).
2. evalua si el ejecutable unkown_viewer.exe existe en disco.
3. si no existe, invoca inmediatamente a build.bat para generar el binario. si la compilacion falla, detiene el flujo con codigo de salida 1.
4. si el ejecutable ya esta presente (o fue generado exitosamente), lo ejecuta en segundo plano con el comando start, reenviando cualquier argumento recibido en la linea de comandos (%*).
