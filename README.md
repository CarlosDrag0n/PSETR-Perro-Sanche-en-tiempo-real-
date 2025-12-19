# PSETR-Perro-Sanche-en-tiempo-real

Práctica Realizada por Luis Laria Urbina y Carlos García Escolano

## 1. Organización del código

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
* **Constantes:** Tras varias pruebas empíricas, ajustamos las constantes a:
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

Antes de nada, como comentamos anteriormente, hicimos una prueba para mandar mensajes simples a través de las placas, por MQTT y la conexión wifi.

Una vez entendido todo, lo que hice primero fue crear una función que crease el mensaje JSON dependiendo del caracter y el valor introducido, comprobé su funcionamiento con un código de prueba del arduino mandando caracteres y valores.

Ya con los mensajes bien estructurados, añadí en el código de movimiento del arduino todo lo necesario para poder mandar mensajes, y añadí en cada state el mensaje correspondiente.

También al final de la práctica cambié el ping, este en un primer momento calculaba el tiempo a través de un thread en arduino y cada 4s se mandaba a la esp32 la orden de enviar por wifi el ping, pero, hablando con compañeros en el laboratorio, decidí hacer el "cálculo" del tiempo esp32, ya que tenerlo en la arduino producía más errores tiempo por el envío del mensaje entre placas y todas las tareas.
Para hacer esto simplemente copié el thread en el esp32, con un booleano y un contador que pongo a true cuando llega la orden de enviar el mensaje de START_LAP e inicio el contador y a false en END_LAP y a través del thread decido si mandarlo el ping o no.

Además esta implementación hace que el contador del ping y de la vuelta sean distintos, acercando aún más los tiempos del ping a 4s.

Algunas de las dificultades que he tenido han sido:
 
1. depurando errores como envio de mensajes vacíos, que solucioné comprobando la longitud del mensaje enviado por el arduino y añadiendo al switch de la funcion de crear los mensajes un estado deault que ponga el mensaje vacío.

2. La conexión wifi, conectarse a eduroam ha sido imposible y a la hora de conectar la móvil, he tenido problemas porque el móvil manda señal 5G y la placa solo se conecta a 2.4GHz.

3. Por último remarcar que me costó un poco entender las conexiones entre placas y MQTT

## 4. Video

Dejo un video demostración para ver el funcionamiento del robot en:

[![Watch the video]](https://youtube.com/shorts/-B3TAr8ko5M)


