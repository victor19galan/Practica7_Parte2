# Parte B - Reproducción de audio desde tarjeta SD

## Descripción
En esta parte de la práctica se reproduce un archivo de audio almacenado en una tarjeta SD externa.  
El ESP32 lee el archivo WAV desde la tarjeta SD y envía los datos de audio al módulo MAX98357 mediante el protocolo I2S.

## Material utilizado
- ESP32 NodeMCU
- Módulo lector de tarjeta MicroSD
- Tarjeta MicroSD con archivo de audio WAV
- Módulo amplificador I2S MAX98357
- Altavoz
- Cables de conexión
- Arduino IDE

## Librería utilizada
Se utiliza la librería ESP32-audioI2S:

https://github.com/schreibfaul1/ESP32-audioI2S

## Pines principales
### Tarjeta SD
- CS: GPIO 5
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK: GPIO 18

### I2S
- DOUT: GPIO 25
- BCLK: GPIO 27
- LRC: GPIO 26

## Funcionamiento
El programa inicializa la comunicación SPI para poder leer la tarjeta SD.  
Después configura los pines I2S y el volumen de reproducción.  
Mediante la función `audio.connecttoFS()` se indica el archivo WAV que se quiere reproducir desde la tarjeta SD.

Ejemplo:

audio.connecttoFS(SD, "Ensoniq-ZR-76-01-Dope-77.wav");

Si el archivo tiene otro nombre, se debe cambiar esa línea por el nombre correcto del archivo.

## Salida por el puerto serie
El puerto serie muestra información sobre el estado de la reproducción.  
Puede mostrar datos como:

- info
- bitrate
- streaminfo
- eof_mp3
- id3data

Estos mensajes sirven para comprobar si el archivo se ha abierto correctamente, si se está reproduciendo y si la reproducción ha terminado.

## Conclusión
Esta parte permite reproducir archivos de audio externos desde una tarjeta SD, lo que resulta más práctico que almacenar el sonido directamente en la memoria interna del ESP32.
