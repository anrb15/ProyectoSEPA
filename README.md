# ProyectoSEPA
Repositorio dedicado al trabajo de curso de la parte de microcontroladores de Sistemas Electrónicos para Automatización. 
## Conversiones
Consiste en las herramientas de conversión utilizadas para la implementación de las imágenes, señales infrarrojas y audios usados en la realización del proyecto:
- **Image Convert Tool V0.9.1**: Utilizada para convertir imágenes al formato compatible con la pantalla VM800B50A-BK.
- **aud_cvt_0.2**: Herramienta empleada para convertir archivos de audio al formato requerido por el narrador integrado en la pantalla.
- **csv**: Carpeta que contiene las conversiones de las señales infrarrojas medidas con el receptor IR y procesadas mediante Saleae Logic. Los archivos convertidos a formato Comma-Separated Values (CSV) se pueden analizar y convertir a declaraciones en C mediante el script **csv_to_ir_array_from_samples_V2.py**.

## Documentación
Contiene los recursos técnicos utilizados como referencia para el desarrollo del proyecto:
- **Carpeta Circuito Led IR**: Imágenes y esquemas del circuito implementado para el LED infrarrojo y su conexión al pin PWM de la TIVA.
- **Documentación FT800 (AN_252, AN_276, AN_303, AN_339, FT800 Programmers Guide)**: Conjunto de guías y notas de aplicación empleadas para comprender el funcionamiento del controlador gráfico FT800, así como los procesos de conversión de audio e imagen y el uso de formatos compatibles (incluyendo JPEG).

## MandoMicro y MandoReal
Incluye las señales infrarrojas grabadas del mando original (MandoMicro) y del mando conseguido en el proyecto (MandoReal), junto con las conversiones a formato CSV para su posterior reproducción mediante el microcontrolador. Cada archivo corresponde a una función específica del mando.

## ProyectoDefinitivo
Incluye todos los archivos **.c** y **.h** desarrollados para la implementación final del proyecto en Code Composer Studio.  
Estos ficheros conforman el firmware completo del mando infrarrojo con pantalla táctil y narrador.

### Contenido
Los archivos están organizados por módulos:

- **Controlador FT800 (pantalla táctil y gráficos)**
  - `FT800_TIVA.h`, `ft800_TIVA.c`
  - `FT800_TIVA.h`: Funciones y definiciones para la comunicación entre la TIVA y el chip FT800.
  - `ft800_TIVA.c`: Implementación de inicialización, gráficos, carga de recursos y gestión táctil.

- **Biblioteca del mando (gestión de señales y funciones de usuario)**
  - `MANDOLIB.h`, `mandolib.c`
  - Conjunto de todas las funciones significativas desarrolladas para el funcionamiento del mando. Abarca funciones de manipulación de datos en memoria, carga y reproducción de archivos multimedia, transmisión de señales infrarrojas y manejo de interrupciones y PWM.

- **Señales infrarrojas**
  - `signals.h`, `signals.c`
  - Almacenan y gestionan las señales IR en formato procesable para enviarlas mediante PWM.

- **Recursos multimedia**
  - `audios.h`: Datos de audio convertidos para el narrador.
  - `logos.h`: Imágenes convertidas para la pantalla.

- **Drivers y utilidades**
  - `driverlib2.h`: Extensiones y adaptaciones a la driverlib original para la TIVA.
  - `uartstdio.c`: Implementación de UART para depuración.

- **Archivo principal**
  - `ProyectoCompleto.c`: Punto de entrada del programa; inicialización, configuración y bucle principal del sistema.

### Cómo replicar el proyecto
Para compilar y cargar el firmware:

1. Crear un proyecto en **Code Composer Studio** siguiendo la configuración indicada en las diapositivas de clase.
2. Copiar **todos los archivos** de esta carpeta y pegarlos directamente en el proyecto recién creado, sobrescribiendo los que correspondan.
3. Compilar y cargar el proyecto en la TIVA.

Con esto, el mando táctil con envío de señales IR quedará completamente operativo.
