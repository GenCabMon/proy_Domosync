# Proyecto del curso de Electrónica Digital III

## Nombre del proyecto
DomoSync

## Descripción del proyecto
El desarrollo de este apartamiento inteligente es un proyecto de domótica enfocado en la integración de varios subsistemas bajo un control centralizado a través de un microcontrolador, la Raspberry Pi Pico 2040. 
El sistema incluye un sensor de reconocimiento de ruido, que detecta aplausos para encender o apagar la luz de un cuarto, y un teclado matricial que permite el control de acceso a la puerta principal de la casa al ingresar un código correcto. 
Además, el sistema utiliza un sensor de temperatura, que en conjunto con un controlador PI, ajusta la velocidad de un ventilador para mantener la temperatura deseada en el ambiente. 
Para la detección de movimiento, se implementa un sensor PIR, el cual activa la luz de la puerta principal cuando detecta alguien cerca a la puerta principal. 
Todos estos subsistemas están conectados a una pantalla LCD, que muestra el estado de los sensores y el acceso, proporcionando información en tiempo real.
Así, cada subsistema se comunica con la RP2040, la cual procesa las entradas de los sensores y toma decisiones como encender luces, abrir la puerta o ajustar la ventilación. 
El diagrama de bloques mostrado a continuación ilustra la interacción entre los componentes principales del proyecto.

![Diagrama de bloques de conexiones del apartamento inteligente](./images/apto_block_diagram.png)

## Requisitos funcionales

### Sensor de reconocimiento por aplauso - Luz del cuarto

El sistema activará o desactivará la luz del cuarto mediante el reconocimiento de un aplauso. 
El sensor de sonido puede comunicarse con la Raspberry Pi Pico mediante comunicación digital directa (GPIO) o comunicación analógica. 
En el caso de la comunicación digital, el sensor envía una señal binaria cuando detecta un aplauso. 
Por otro lado, si se utiliza un sensor analógico, este enviará una señal proporcional a la intensidad del sonido, que se conecta a un pin ADC para procesar los datos de manera más precisa, permitiendo ajustar la sensibilidad al aplauso. 
Cuando la luz esté apagada y se detecte un aplauso, esta se encenderá; si ya está encendida, el aplauso siguiente la apagará. 
El sistema deberá procesar la entrada de sonido en un tiempo máximo de 1 segundo para asegurar una respuesta fluida y evitar retardos percibidos por el usuario. 
En caso de detectar múltiples aplausos en menos de 3 segundos, estos serán ignorados para prevenir parpadeos indeseados de la luz.

### Sistema de acceso mediante teclado - Puerta principal

La puerta principal se desbloqueará mediante la introducción de un código de 4 dígitos en el teclado. 
Si el usuario no ingresa el código completo en un plazo máximo de 10 segundos, el sistema reiniciará el proceso, obligando a introducir nuevamente la clave. 
Al ingresar un código incorrecto, se activará una alarma visual y se mostrará "Acceso denegado" en el LCD. La comunicación entre el teclado y la Raspberry Pi Pico se realizará a través de pines GPIO, garantizando una respuesta rápida y precisa. 
Adicionalmente, el sistema asegurará que los datos introducidos sean procesados de manera eficiente, minimizando errores de lectura.

### Control de temperatura - Control PI del ventilador

El sistema ajustará la velocidad del ventilador mediante un control PI, basado en las lecturas de temperatura obtenidas cada 2 segundos por un sensor analógico. La velocidad del ventilador se ajustará proporcionalmente en función de valores de referencia predefinidos para mantener una temperatura agradable en el ambiente. Si la temperatura alcanza un límite máximo (por ejemplo, 30°C), el ventilador operará a su máxima velocidad, y si desciende por debajo de un límite mínimo (por ejemplo, 18°C), el ventilador se apagará para optimizar el consumo energético. La comunicación con el sensor de temperatura podría realizarse utilizando protocolo analógico para capturar las variaciones continuas del ambiente, mientras que el control del ventilador podría gestionarse mediante GPIO o PWM
- LCD para muestreo de estado de sensores y acceso:

El LCD mostrará información relevante sobre el estado del sistema, incluyendo notificaciones de acceso autorizado o denegado y la condición operativa de dispositivos conectados, como el ventilador. 
El procesamiento de datos garantizará que la pantalla se actualice con un tiempo de respuesta máximo de 2 segundo, brindando al usuario una retroalimentación clara. 
La comunicación I2C permitirá la transmisión eficiente de datos entre el microcontrolador y la pantalla, siendo este el protocolo de comunicación del LCD, minimizando interferencias. 
Para evitar solapamientos, se implementará una separación clara en los tiempos de comunicación entre el LCD y el sensor de temperatura, asegurando que no se envíen datos al LCD mientras se leen los valores del sensor. 
Además, se priorizarán las tareas; si se recibe una entrada del teclado, se dará prioridad a su procesamiento, permitiendo que la lectura del sensor de temperatura se posponga brevemente si es necesario, sin comprometer la frecuencia de actualización del sensor. 
En términos de seguridad, se implementará un control de errores básico para evitar lecturas erróneas o datos incompletos durante la comunicación.

### Sensor PIR - Movimiento para detección de presencia

El sensor PIR se utilizará para detectar movimiento para encender luz de la calle (Puerta principal) al reconocer la presencia de una persona. 
Este sistema garantizará un procesamiento de datos eficiente, con un tiempo de respuesta máximo de 6 segundos, asegurando que la luz se encienda rápidamente al detectar movimiento. 
El sensor PIR proporciona una salida digital, generando un pulso alto al detectar movimiento y volviendo a bajo cuando no hay presencia. 
Este pulso se conectará directamente a un pin GPIO de la Raspberry Pi Pico, facilitando una respuesta rápida. 
Haciendo uso de un relé se proporciona aislamiento eléctrico entre la Raspberry Pi Pico y el circuito, protegiendo el microcontrolador de picos de voltaje o corrientes que podrían dañarlo. 
Además, se planea implementar controles para ajustar la sensibilidad del sensor PIR, evitando así falsas alarmas provocadas por objetos pequeños o animales. 
Si no se detecta movimiento en un lapso de 1 minuto, el sistema apagará automáticamente la luz.


## Requisitos NO funcionales
- [Requisito NO funcional 1]
- [Requisito NO funcional 2]
- [Requisito NO funcional 3]

## Escenario de pruebas
1. Escenario 1: Prueba del Sensor de Ruido (Aplauso)

Condición inicial: La luz del cuarto está apagada.
Acción: Aplaudir cerca del sensor.
Resultado esperado: La luz del cuarto debe encenderse en respuesta al aplauso.
Prueba adicional: Aplaudir nuevamente debe apagar la luz.

2. Escenario 2: Prueba del Sistema de Acceso (Teclado matricial)

Condición inicial: La puerta está cerrada.
Acción: Ingresar un código de 6 dígitos en el teclado matricial.
Resultado esperado: Si el código es correcto, el actuador (servomotor) debe desbloquear la puerta y en la LCD se debe mostrar un mensaje de “Acceso concedido”. Si el código es incorrecto, la puerta debe permanecer cerrada y el LCD debe mostrar “Acceso denegado”.
Pruebas adicionales: Ingresar un código erróneo varias veces para asegurar que el sistema maneja bien los errores de autenticación y el bloqueo de usuarios. 
Verificar el funcionamiento de las opciones de cambio de contraseña de un residente y agregación o eliminación de usuario de un residente.

3. Escenario 3: Prueba del Control de Temperatura (Ventilador con control PI)

Condición inicial: La temperatura ambiente es baja, y el ventilador está apagado o funcionando a velocidad mínima.
Acción: Simular un aumento en la temperatura ambiente utilizando una fuente de calor controlada cerca del sensor.
Resultado esperado: A medida que la temperatura aumenta, el control PI debe ajustar la velocidad del ventilador de manera gradual para mantener la temperatura estable dentro de un rango definido.
Prueba adicional: Al retirar la fuente de calor, la velocidad del ventilador debe disminuir o apagarse conforme la temperatura vuelva a su nivel normal.

4. Escenario 4: Prueba del Sensor PIR (Movimiento)

Condición inicial: No hay movimiento en la habitación o área cubierta por el sensor PIR, y la luz de la puerta principal está apagada.
Acción: Caminar dentro del área de detección del sensor PIR.
Resultado esperado: La luz de la puerta principal debe activarse un lapso de tiempo breve tras la detección de movimiento.
Prueba adicional: Permanecer quieto o salir del área de detección debe apagar dicha luz después de un minuto de no detectar movimiento.

5. Escenario 5: Prueba de Monitoreo en LCD

Condición inicial: El sistema está encendido y los sensores están activos.
Acción: Monitorear las actividades de cada uno de los sensores mencionados anteriormente, asegurando que la LCD muestre el estado de acceso, el funcionamiento de los sensores de luz, PIR, y temperatura en tiempo real.
Resultado esperado: La LCD debe actualizarse dinámicamente, mostrando el estado del sistema en todo momento sin retrasos de tiempo significativos. 
El muestreo del estado del sistema debería ser con avisos del estado de cada sensor que se visualicen en la LCD, mostrados en intervalos de tiempo distintos. 
Mientras que los avisos relacionados con el sistema de acceso interrumpen la visualización de los avisos anteriores sólo cuando se ha realizado un intento de autenticación de usuario, para cualquiera de las opciones disponibles para la persona.

## Presupuesto
- [Detalle de costos 1]
- [Detalle de costos 2]
- [Detalle de costos 3]

## Repositorio
- [Enlace al repositorio del proyecto]
