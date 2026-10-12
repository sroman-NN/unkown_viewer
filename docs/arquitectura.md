# arquitectura general

este documento describe la organizacion estructural, el modelo de ejecucion y los principios de diseno implementados en el visor de texto.

## modelo de ejecucion

el programa opera sobre el subsistema nativo win32 mediante el punto de entrada wwinaim. no utiliza capas intermedias de abstraccion, envoltorios de terceros ni entornos de ejecucion dependientes.

la inicializacion sigue el flujo estandar del sistema operativo:

1. registro de la clase de ventana mediante registerclassexw definiendo un procedimiento de ventana principal (mainwindowproc), iconos de aplicacion y el color base del lienzo.
2. inicializacion del subsistema de controles comunes mediante initcommoncontrolsex para compatibilidad con la version 6 de common-controls.
3. creacion de la ventana contenedora con createwindowexw usando las banderas ws_overlappedwindow y ws_clipchildren. esta ultima es critica para evitar que el fondo de la ventana principal pinte sobre el area de los controles hijos, eliminando parpadeos.
4. entrada al bucle de mensajes sincrono mediante getmessagew, translatemessage y dispatchmessagew.

el ciclo de vida concluye cuando mainwindowproc recibe un mensaje wm_destroy, invocando postquitmessage para desbloquear el bucle y devolver el codigo de salida al sistema operativo.

## estado global de la aplicacion

todos los punteros de recursos, identificadores de ventana (handles) y variables de estado del visor residen en una estructura estatica de memoria contigua denominada appstate.

los campos principales de appstate son:

- hwndmain: manejador de la ventana contenedora principal.
- hwndedit: manejador del control multiline nativo de edicion.
- origeditproc: puntero al procedimiento de ventana original del control edit, preservado para encadenar las llamadas de subclaseo mediante callwindowprocw.
- hfonteditor: manejador de la fuente monoespaciada aplicada al texto en edicion.
- hfontstatus: manejador de la fuente empleada exclusivamente en la barra inferior.
- hbrushwhite: brocha solida para el fondo del control de edicion.
- hbrushbar: brocha solida gris para el fondo de la barra de estado.
- hpenborder: pluma solida de un pixel para trazar la linea divisoria superior de la barra.
- currentfilepath: cadena unicode que contiene la ruta absoluta del archivo cargado. si esta vacia, el documento se considera sin guardar.
- statusmessage: mensaje temporal de estado que sobreescribe la informacion secundaria de la barra.
- statustimerid: identificador del temporizador wm_timer para expirar mensajes temporales.
- charwidth y charheight: dimensiones en pixeles de un glifo de la fuente del editor, calculadas una sola vez durante wm_create para definir el caret.
- currentline y currentcol: posicion actual del cursor dentro del flujo de texto, calculada en base 1.
- ismodified: bandera booleana que indica si el contenido del buffer en memoria difiere del estado almacenado en disco.

esta organizacion concentra el estado en un unico bloque de datos accesible por las funciones de soporte sin fragmentar asignaciones dinamicas en el monticulo (heap).

## division del area de cliente

el area cliente de la ventana principal se divide estrictamente en dos zonas funcionales:

```
+--------------------------------------------------------+
|                                                        |
|  area de edicion principal (control edit hijo)        |
|  - lienzo blanco con texto negro                       |
|  - fuente consolas con caret solido tipo terminal      |
|  - desplazamiento vertical y horizontal nativo         |
|  - altura calculada: altura_total - altura_barra       |
|                                                        |
+--------------------------------------------------------+
|  linea divisoria superior (1 pixel)                    |
|  barra de estado inferior (28 pixeles de altura fijos) |
|  - comandos a la izquierda, posicion a la derecha      |
+--------------------------------------------------------+
```

la separacion fisica entre ambas zonas se controla en el manejador del mensaje wm_size. el control hijo nunca se solapa con la barra inferior: su altura siempre es recortada en 28 pixeles exactos respecto a la altura total del area cliente. si la altura total es inferior a 28 pixeles, el control se reduce a tamano cero para evitar valores negativos en las coordenadas de la api.

## gestion de recursos gdi

los objetos graficos (fuentes, brochas y plumas) se crean durante el evento wm_create y permanecen vigentes durante toda la sesion de la aplicacion.

durante el procesamiento de wm_destroy, cada manejador es liberado de forma explicita mediante deleteobject. este protocolo previene fugas de recursos en la tabla de objetos del subsistema gdi del kernel de windows.
