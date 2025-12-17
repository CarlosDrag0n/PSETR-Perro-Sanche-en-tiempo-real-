# PSETR-Perro-Sanche-en-tiempo-real

Práctica Realizada por Luis Laria Urbina y Carlos García Escolano

## 1. Organización del código

Para organizar el proyecto, primero hemos hecho pruebas simples de los sensores, utilizando como guias las códigos proporcionados. 
Una vez que comprobamos que todo funciona bien, empezamos a realizar el movimiento del siguelineas.

## 2. Movimiento 

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


