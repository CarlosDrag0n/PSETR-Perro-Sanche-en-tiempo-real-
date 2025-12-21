# PSETR-Perro-Sanche-en-tiempo-real

Práctica Realizada por Luis Laria Urbina y Carlos García Escolano

## 1. Organización del código

La primera decisión que tomamos fue hacerlo con Arduino threads y si luego nos daba tiempo pasarlo a rt, ya que al no haber trabajado nunca de esta forma y con no mucho tiempo por delante decidimos no arriesgarnos.

Para organizar el proyecto, primero hemos hecho pruebas simples de los sensores, utilizando como guias las códigos proporcionados. 
Una vez que comprobamos que todo funciona bien, empezamos a realizar el movimiento del siguelineas.

## 2. Movimiento 

Para la gestión del movimiento del robot hemos implementado una arquitectura basada en **Hilos (Threads)** utilizando la librería `ThreadController`. Esto nos permite ejecutar las tareas de lectura de sensores y control de motores de forma pseudo-paralela con intervalos de tiempo definidos:

* **Lectura de Infrarrojos:** Cada 5ms.
* **Lectura de Ultrasonidos:** Cada 45ms.
* **Control de Motores y Lógica de Estados:** Cada 10ms.

El núcleo del movimiento se basa en una **Máquina de Estados Finitos** que decide el comportamiento del robot en función de las lecturas de los sensores.

### 2.1. Máquina de Estados

Hemos definido un `enum` llamado `movimiento` que gestiona cuatro estados principales. La transición entre ellos se controla mediante la función auxiliar `go_state()`, la cual también se encarga de gestionar los flags de comunicación y cambiar el color del LED de estado.

1.  **SEGUIR_LINEA (LED Verde):** Es el estado por defecto. El robot utiliza un control PID para mantenerse sobre la línea negra.
2.  **BUSCANDO_LINEA (LED Rojo):** Se activa cuando los tres sensores infrarrojos leen "blanco" (`lineaPerdida`). El robot entra en modo de recuperación.
3.  **PARANDO_OBSTACULO (LED Azul):** Se activa cuando el sensor de ultrasonidos detecta un objeto a menos de `distanciaInicioFrenado` (28 cm).
4.  **FINALIZADO:** Estado de parada total. Se activa cuando el robot ha llegado a la distancia objetivo del obstáculo (7 cm), indicando el fin de la vuelta.

### 2.2. Algoritmo de Control (PID)

Mientras el robot se encuentra en el estado `SEGUIR_LINEA`, el movimiento se rige por un algoritmo **PD (Proporcional-Derivativo)**.

* **Cálculo del error:** Se obtiene restando la lectura del sensor derecho menos el izquierdo (`valRight - valLeft`).
* **Constantes:** Tras varias pruebas, ajustamos las constantes a:
    * $K_p = 0.4$: Para la reacción proporcional al error actual.
    * $K_d = 2.5$: Para suavizar la oscilación prediciendo el error futuro.
* **Velocidad Base:** Fijada en 110 PWM.

El ajuste resultante se suma a un motor y se resta al otro, permitiendo correcciones suaves en rectas y curvas abiertas.

### 2.3. Lógica de Recuperación (Buscando Línea)

Si el robot pierde la línea (todos los sensores por debajo del `umbralNegro`), entra en el estado `BUSCANDO_LINEA`. Para saber hacia dónde girar, utilizamos una variable global `ultimoLadoVisto`:

* Si lo último que vio fue la **Izquierda**, aplica un giro hacia la izquierda.
* Si fue la **Derecha**, gira hacia la derecha.

Este giro no es un movimiento suave, sino un giro sobre su propio eje o muy cerrado, configurado con una `velocidadGiroRapida` (180) en una rueda y una `velocidadGiroLenta` (-20) en la otra. El valor negativo invierte la polaridad del motor, haciendo que la reacción para volver a encontrar la línea sea inmediata.

### 2.4. Detección de Obstáculos y Frenado

El hilo de ultrasonidos calcula la distancia constantemente. Si baja de 28cm, cambiamos al estado `PARANDO_OBSTACULO`.

En este estado, el control PID se desactiva y entra en juego un **frenado proporcional**:
* La velocidad de los motores se calcula en función de la distancia restante hasta el objetivo (7 cm) multiplicada por una constante de frenado (`kp_freno = 10.0`).
* Esto hace que el robot decelere suavemente a medida que se acerca a la pared, deteniéndose por completo y cambiando al estado `FINALIZADO` al llegar a la distancia objetivo.

## 3. Comunicación

Antes de nada, como comentamos anteriormente, hicimos una prueba para mandar mensajes simples a través de las placas a través del puerto serie con el código de prueba que se nos proporcionó. Una vez que comprobamos que funcionaba la conexión comprobamos la conexión wifi con otro código de prueba que se nos proporcionó.

### 3.1 MQTT

Una vez entendido todo, lo que hice primero fue crear una función que devolviese el mensaje en un String con el formato JSON dependiendo del carácter y el valor de entrada, comprobé su funcionamiento con un código de prueba del arduino mandando carácteres y valores.

Para filtrar los mensajes que envía el arduino al ESP32, utilicé dos funciones, primero como el mensaje se acaba cuando se lee el carácter **"}"**, lo que hice fue cambiar este carácter por un carácter vacío **""** y luego eliminar el carácter vacío u otros elementos como **\r\n**, los cuales se meten en los mensajes enviados al hacer un *println*,al principio los utilizamos, pero luego lo modificamos a simplemente *print*

Ya con los mensajes bien estructurados, añadí en el código de movimiento del arduino todo lo necesario para poder mandar mensajes, es decir, meter en el setup el while que comprueba que haya conexión con la ESP32, y añadí en cada *state* el envío del mensaje correspondiente.

También al final de la práctica cambié el ping, este en un primer momento calculaba el tiempo a través de un *thread* en arduino y cada 4s se mandaba a la ESP32 la orden de enviar por WiFi el ping, pero, hablando con compañeros en el laboratorio, decidí hacer el "cálculo" del tiempo ESP32, ya que tenerlo en la arduino producía más errores de tiempo por el envío del mensaje entre placas y todas las tareas.
Para hacer esto simplemente copié el *thread* en el ESP32, con un booleano que pongo a true y un contador que inicio cuando llega la orden de enviar el mensaje de START_LAP y pongo a false en END_LAP y a través del *thread* decido si mandar el ping o no.

Además, esta implementación hace que el contador del ping y de la vuelta sean distintos, acercando aún más los tiempos del ping a 4s.

### 3.2 WiFi

En cuanto a la comunicación WiFi, lo primero, cambiamos las credenciales de eduroam a las que se nos dijeron en clase y creamos un ssid y contraseña para el WiFi del móvil.

Luego en el setup iniciamos la configuración con el ssid y la contraseña y se intenta conectar, hasta que no se detecta que se ha conectado correctamente la conexión WiFi, el programa no avanzará.

### 3.3 Dificultades encontradas

Algunas de las dificultades que he tenido han sido:
 
1. Depurando errores como envío de mensajes vacíos, que solucioné comprobando la longitud del mensaje enviado por el arduino y añadiendo al switch de la función de crear los mensajes un estado *default* que ponga el mensaje vacío si no se le ha introducido uno de los caracteres deseados.

2. La conexión wifi, conectarse a eduroam me ha sido imposible y a la hora de conectar el móvil, he tenido problemas porque el móvil manda señal 5G y la placa solo se conecta a 2.4GHz.

3. Por último remarcar que me costó un poco entender las conexiones entre placas y MQTT, ya que no entendía muy bien como se mandaban los mensajes o porque en el while del setup se tenían que enviar mensajes.

## 4. Video

Dejo un video demostración para ver el funcionamiento del robot en:
 
1. Circuito de examen:

[![Watch the video]](https://youtu.be/BNQ70QTSwKs)

2. Circuito de laboratorio:

[![Watch the video]](https://youtube.com/shorts/-B3TAr8ko5M)


