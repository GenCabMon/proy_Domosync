# Proyecto del curso de Electrónica Digital III

## Nombre del proyecto
[Coloca aquí el nombre del proyecto]

## Descripción del proyecto
El desarrollo de este apartamiento inteligente es un proyecto de domótica enfocado en la integración de varios subsistemas bajo un control centralizado a través de un microcontrolador, la Raspberry Pi Pico 2040. 
El sistema incluye un sensor de reconocimiento de ruido, que detecta aplausos para encender o apagar la luz de un cuarto, y un teclado matricial que permite el control de acceso a la puerta principal de la casa al ingresar un código correcto. 
Además, el sistema utiliza un sensor de temperatura, que en conjunto con un controlador PI, ajusta la velocidad de un ventilador para mantener la temperatura deseada en el ambiente. 
Para la detección de movimiento, se implementa un sensor PIR, el cual activa luces o alarmas cuando se detecta movimiento en áreas designadas del apartamento. 
Todos estos subsistemas están conectados a una pantalla LCD, que muestra el estado de los sensores y el acceso, proporcionando información en tiempo real.
Así, cada subsistema se comunica con la RP2040, la cual procesa las entradas de los sensores y toma decisiones como encender luces, abrir la puerta o ajustar la ventilación. 
El diagrama de bloques mostrado a continuación ilustra la interacción entre los componentes principales del proyecto.


## Requisitos funcionales
- Sensor de reconocimiento por aplauso - Luz del cuarto:
El sistema activará o desactivará la luz del cuarto mediante el reconocimiento de un aplauso. El sensor de sonido puede comunicarse con la Raspberry Pi Pico mediante comunicación digital directa (GPIO) o comunicación analógica. En el caso de la comunicación digital, el sensor envía una señal binaria cuando detecta un aplauso. Por otro lado, si se utiliza un sensor analógico, este enviará una señal proporcional a la intensidad del sonido, que se conecta a un pin ADC para procesar los datos de manera más precisa, permitiendo ajustar la sensibilidad al aplauso. Cuando la luz esté apagada y se detecte un aplauso, esta se encenderá; si ya está encendida, el aplauso siguiente la apagará. El sistema deberá procesar la entrada de sonido en un tiempo máximo de 1 segundo para asegurar una respuesta fluida y evitar retardos percibidos por el usuario. En caso de detectar múltiples aplausos en menos de 3 segundos, estos serán ignorados para prevenir parpadeos indeseados de la luz.
- Sistema de acceso mediante teclado - Puerta principal:
La puerta principal se desbloqueará mediante la introducción de un código de 4 dígitos en el teclado. Si el usuario no ingresa el código completo en un plazo máximo de 10 segundos, el sistema reiniciará el proceso, obligando a introducir nuevamente la clave. Al ingresar un código incorrecto, se activará una alarma visual y se mostrará "Acceso denegado" en el LCD. La comunicación entre el teclado y la Raspberry Pi Pico se realizará a través de pines GPIO, garantizando una respuesta rápida y precisa. Adicionalmente, el sistema asegurará que los datos introducidos sean procesados de manera eficiente, minimizando errores de lectura
- Control de temperatura - Control PI del ventilador
El sistema ajustará la velocidad del ventilador mediante un control PI, basado en las lecturas de temperatura obtenidas cada 2 segundos por un sensor analógico. La velocidad del ventilador se ajustará proporcionalmente en función de valores de referencia predefinidos para mantener una temperatura agradable en el ambiente. Si la temperatura alcanza un límite máximo (por ejemplo, 30°C), el ventilador operará a su máxima velocidad, y si desciende por debajo de un límite mínimo (por ejemplo, 18°C), el ventilador se apagará para optimizar el consumo energético. La comunicación con el sensor de temperatura podría realizarse utilizando protocolo analógico para capturar las variaciones continuas del ambiente, mientras que el control del ventilador podría gestionarse mediante GPIO o PWM
- LCD para muestreo de estado de sensores y acceso:
El LCD mostrará información relevante sobre el estado del sistema, incluyendo notificaciones de acceso autorizado o denegado y la condición operativa de dispositivos conectados, como el ventilador. El procesamiento de datos garantizará que la pantalla se actualice con un tiempo de respuesta máximo de 2 segundo, brindando al usuario una retroalimentación clara. La comunicación I2C permitirá la transmisión eficiente de datos entre el microcontrolador y la pantalla, siendo este el protocolo de comunicación del LCD, minimizando interferencias. Para evitar solapamientos, se implementará una separación clara en los tiempos de comunicación entre el LCD y el sensor de temperatura, asegurando que no se envíen datos al LCD mientras se leen los valores del sensor. Además, se priorizarán las tareas; si se recibe una entrada del teclado, se dará prioridad a su procesamiento, permitiendo que la lectura del sensor de temperatura se posponga brevemente si es necesario, sin comprometer la frecuencia de actualización del sensor. En términos de seguridad, se implementará un control de errores básico para evitar lecturas erróneas o datos incompletos durante la comunicación.
- Sensor PIR - Movimiento para detección de presencia:
El sensor PIR se utilizará para detectar movimiento para encender luz de la calle (Puerta principal) al reconocer la presencia de una persona. Este sistema garantizará un procesamiento de datos eficiente, con un tiempo de respuesta máximo de 6 segundos, asegurando que la luz se encienda rápidamente al detectar movimiento. El sensor PIR proporciona una salida digital, generando un pulso alto al detectar movimiento y volviendo a bajo cuando no hay presencia. Este pulso se conectará directamente a un pin GPIO de la Raspberry Pi Pico, facilitando una respuesta rápida. Haciendo uso de un relé se proporciona aislamiento eléctrico entre la Raspberry Pi Pico y el circuito, protegiendo el microcontrolador de picos de voltaje o corrientes que podrían dañarlo. Además, se planea implementar controles para ajustar la sensibilidad del sensor PIR, evitando así falsas alarmas provocadas por objetos pequeños o animales. Si no se detecta movimiento en un lapso de 1 minuto, el sistema apagará automáticamente la luz.


## Requisitos NO funcionales
- [Requisito NO funcional 1]
- [Requisito NO funcional 2]
- [Requisito NO funcional 3]

## Escenario de pruebas
1. [Paso 1 del escenario de pruebas]
2. [Paso 2 del escenario de pruebas]
3. [Paso 3 del escenario de pruebas]

## Presupuesto
- [Detalle de costos 1]
- [Detalle de costos 2]
- [Detalle de costos 3]

## Repositorio
- [Enlace al repositorio del proyecto]
