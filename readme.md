# documentacion tecnica de unkown viewer

este directorio contiene las especificaciones tecnicas completas de la aplicacion unkown viewer, desarrollada en c++17 nativo sobre la api  de win32.

## indice de documentos

la documentacion esta dividida en cuatro modulos tecnicos:

1. [arquitectura.md](file:///C:/Users/Usuario/dev/unkown%20viewer/main/docs/arquitectura.md):
   describe el modelo de ejecucion nativo, el punto de entrada wwinaim, la estructura de estado global appstate, la subdivision del area cliente entre el lienzo y la barra de comandos, y la politica de gestion de recursos gdi.

2. [interfaz_y_renderizado.md](file:///C:/Users/Usuario/dev/unkown%20viewer/main/docs/interfaz_y_renderizado.md):
   detalla la creacion y configuracion del control de texto multiline, el calculo de metricas de fuente tipografica para el caret solido tipo terminal, el protocolo de coloreado mediante wm_ctlcoloredit y el renderizado gdi directo de la barra inferior en wm_paint.

3. [eventos_y_subclaseo.md](file:///C:/Users/Usuario/dev/unkown%20viewer/main/docs/eventos_y_subclaseo.md):
   explica la tecnica de subclaseo sobre el procedimiento de ventana del control edit, la captura de atajos de teclado (ctrl+o, ctrl+s, ctrl+x, ctrl+z, ctrl+y, ctrl+a), el calculo matematico de linea y columna en tiempo real, el manejo del temporizador de notificaciones y la recepcion de archivos mediante wm_dropfiles.

4. [almacenamiento_y_formatos.md](file:///C:/Users/Usuario/dev/unkown%20viewer/main/docs/almacenamiento_y_formatos.md):
   documenta la lectura y escritura fisica de archivos mediante funciones del subsistema kernel32, el limite de seguridad de memoria de 128 mb, la deteccion y eliminacion de prefijos bom en utf-8, la conversion bidireccional entre secuencias utf-8 de disco y cadenas utf-16 de memoria, y los parametros de los dialogos de comdlg32.

5. [compilacion_y_recursos.md](file:///C:/Users/Usuario/dev/unkown%20viewer/main/docs/compilacion_y_recursos.md):
   describe la logica de autodeteccion de compiladores en build.bat (gcc y msvc), las banderas de optimizacion y enlace estatico total, la definicion de recursos en resource.rc y la integracion del manifiesto app.manifest para common controls v6 y soporte dpi per-monitor v2.

## resumen de archivos del repositorio

la raiz del proyecto contiene los siguientes archivos:

- main.cpp: codigo fuente monolitico de la aplicacion. contiene el procedimiento principal, el procedimiento de subclase, las rutinas de conversion de texto y las llamadas directas a win32.
- resource.h: encabezado en lenguaje c que define los identificadores numericos de los recursos compilados (idi_app_icon).
- resource.rc: script de recursos de windows que incrusta el icono de aplicacion, el manifiesto y la informacion de version en el ejecutable.
- app.ico: archivo binario de icono con resoluciones de 16x16 y 32x32 pixeles a 32 bits con canal alfa.
- app.manifest: manifiesto xml que declara dependencias de plataforma de windows (common-controls 6.0) y escala per-monitor v2.
- build.bat: script por lotes que detecta g++ o cl.exe, compila los recursos con windres o rc, y genera el binario unkown_viewer.exe con enlace estatico completo.
- run.bat: script por lotes que verifica la existencia del ejecutable, invoca a build.bat si es necesario y lanza el proceso.
