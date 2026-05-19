## Integrantes:
Richard Alejandro Reyes Gracia C212
Jorge Julio de Leon Masson C212

# 1. Introducción

El presente proyecto se enmarca en la asignatura Sistemas Operativos de la Facultad de 
Matemática y Computación de la Universidad de La Habana. Su objetivo principal es explorar 
y modificar componentes internos del sistema operativo MINIX 3, una plataforma educativa 
ampliamente utilizada para el estudio de sistemas operativos por su código abierto y su 
arquitectura de microkernel.

El trabajo se organizó en cuatro actividades fundamentales: personalización del mensaje de 
bienvenida del sistema, depuración de un bug en la implementación de 
`pthread_mutex_trylock`, implementación del comando `tree` para visualizar la estructura de 
directorios, y modificación del planificador de procesos para penalizar aquellos con uso 
intensivo de CPU (CPU-bound).

A través de estas tareas, se busca comprender en profundidad mecanismos clave como la 
concurrencia mediante mutex, el acceso al sistema de archivos mediante llamadas al sistema, 
y la planificación de procesos con prioridades dinámicas. El presente informe documenta el 
proceso de trabajo, las dificultades encontradas y los resultados obtenidos en cada 
componente.

# 2. Desarrollo y resultados por componentes
### 2.1. Instalacion y configuracion de minix
**RICHARD**

El emulador utilizado fue QEMU 10.2.2, elegido porque la computadora del estudiante es arquitectura ARM y MINIX 3.4.0 requiere x86. VirtualBox no se utilizó porque este solo virtualiza, no emula arquitecturas diferentes.

Los parámetros de configuración de la máquina virtual fueron los siguientes:

2GB de RAM: suficiente para compilar sin lentitud, sin desperdiciar recursos del host.
1 núcleo de CPU: MINIX no requiere más; emular múltiples núcleos en QEMU/ARM añade sobrecarga.
20GB de disco duro: espacio suficiente para código fuente y compilaciones; el formato dinámico ahorra espacio en el host.
IPv4 only con DHCP y redirección de puertos (SSH en puerto 7722): modo usuario simple, evita problemas de IPv6 en MINIX 3.
El primer emulador que se probó fue UTM, pero no reconocía la ISO por problemas de compatibilidad, por lo cual se cambió a QEMU. QEMU mostró un error "file driver requires regular file" debido a un punto en la ruta (richarda.), por lo cual se movieron los archivos a ~/minix_vm y se usaron rutas relativas. El error de SSL al hacer git push se resolvió deshabilitando la verificación SSL con git config --global http.sslVerify false.

Herramientas adicionales instaladas:

Git: para clonar el repositorio oficial (/usr/src) y subir cambios a GitHub.
Nano: para editar archivos como /usr/src/etc/motd.

**JORGE**

Para crear la máquina virtual, se decidió usar QEMU, ya que tiempo atrás el estudiante había tenido problemas con VirtualBox sobre su plataforma Linux y ha utilizado QEMU desde entonces para gestionar máquinas virtuales, inicialmente sobre la interfaz gráfica Gnome Boxes y posteriormente sobre terminal para aprovechar mejor las posibilidades que ofrece el software.

En primera instancia, se hizo un fork del repositorio del profesor Christopher y se clonó para intentar compilar una imagen ISO propia con los archivos de la carpeta /releasetools, lo cual no funcionó debido a una incompatibilidad del compilador de C que hizo fallar el proceso. Ante esto, se optó por seguir el proceso descrito por el profesor en el video, donde explicaba el procedimiento de instalación descargando la imagen ISO directamente desde la página oficial de MINIX.

En cuanto a la asignación de recursos, se configuró inicialmente la máquina virtual con 1 núcleo (valor por defecto cuando no se especifica en el comando), aunque posteriormente se incrementó a 4 núcleos, ya que el proyecto de MINIX es tan amplio que comandos relacionados con Git como git status se tornaban lentos. En memoria se asignaron 2 GB de RAM y un disco de 10 GB. El comando de arranque se guardó en un archivo llamado run_system.sh, al cual se le asignaron permisos de ejecución para evitar tener que buscar el comando cada vez que se necesitara ejecutar la máquina.

Como herramientas adicionales, una vez terminada la instalación de la máquina virtual, se agregó nano, un editor con el cual el estudiante está familiarizado y que usa regularmente para ediciones rápidas o revisión de archivos como los de configuración del sistema. Además, se instaló Git para gestionar el repositorio del proyecto. También se estableció una contraseña para el usuario root y se creó otro usuario adicional.


### 2.2. Personalizacion del mensaje de bienvenida

Durante esta etapa surgieron algunos problemas relacionados con la autenticación en Git y la conexión remota. Al hacer un push con Git, fue necesario agregar un token de verificación. Dado que no se conocía una forma de compartir el portapapeles entre ambas máquinas, se optó por activar el servicio SSH para facilitar el trabajo. Inicialmente, al crear la máquina virtual, no se había considerado que el puerto por defecto (22) debía ser mapeado a un puerto del equipo anfitrión. Para resolverlo, se añadieron las siguientes líneas, tomadas de la documentación oficial de QEMU:
```bash
-net user,hostfwd=tcp::10022-:22 -net nic
```
Una vez agregadas, se intentó arrancar SSH en MINIX. Aunque la documentación sugiere el comando:
```bash
/usr/pkg/etc/rc.d/sshd start
```
fue necesario utilizar onestart en lugar de start para que el servicio se iniciara correctamente.

Al intentar la conexión SSH, se presentó otro problema: la autenticación fallaba al intentar acceder como usuario root, indicando que la contraseña era incorrecta. Tras varios intentos, se probó con el usuario alternativo creado durante la instalación de la máquina virtual, permitiendo el acceso exitosamente.

![Welcome message picture](assets/welcome.png)


### 2.3. Depuración de un bug en pthread


1. Síntomas
Al ejecutar el programa de prueba proporcionado por el profesor, se observa el siguiente comportamiento anómalo:

La primera llamada a pthread_mutex_trylock retornaba 0 (OK), indicando que el mutex se había bloqueado correctamente.
La segunda llamada a pthread_mutex_trylock (realizada por el mismo hilo) no retornaba nunca. El programa se quedaba congelado, sin mostrar los mensajes siguientes (unlock, destroy, PASS), y el 
prompt de MINIX no volvía a aparecer hasta que se forzaba la interrupción con Ctrl + C.

2. Anaálisis
En MINIX, las funciones de hilos se dividen en dos capas. Por un lado, la capa de compatibilidad (archivo libmthread/pthread_compat.c) provee las funciones estándar pthread_* que los 
programadores utilizan. Por otro lado, la implementación nativa (archivo libmthread/mutex.c) provee las funciones reales mthread_* que manejan los mutex a bajo nivel. La capa de compatibilidad 
actúa como un traductor: cada función pthread_* debería llamar a su equivalente mthread_*.

Al inspeccionar pthread_compat.c, se localizó la función pthread_mutex_trylock y se encontré el siguiente código: return pthread_mutex_trylock(mutex);

El problema es que la función se llama a sí misma recursivamente en lugar de llamar a mthread_mutex_trylock. Esto es un error tipográfico

3. Corrección
La solución es mínima: cambiar la llamada recursiva por la llamada a la función nativa.
Cambiar return pthread_mutex_trylock(mutex); por return mthread_mutex_trylock(mutex);

4. Verificación
Después de aplicar la corrección, se recompilé la librería y se ejecuta nuevamente el programa de prueba.
Solución obtenid:
first trylock: 0 (OK)
second trylock: 11 (Resource deadlock avoided)
unlock: 0 (OK)
destroy: 0 (OK)
PASS
El programa ya no se congela. La segunda llamada retorna el código de error esperadoy el programa continúa ejecutando unlock, destroy y finalmente imprime PASS.

### 2.4. Implementacion del comando tree


Se implementó en el método main la siguiente estructura:

```c

int main(const int argc, char *argv[]) {
    char *firstArg = argc > 1 ? argv[1] : NULL;
    char *secondArg = argc > 2 ? argv[2] : NULL;
    char *thirdArg = argc > 3 ? argv[3] : NULL;
    char *path;
    long depth = -1;

    switch (argParse(firstArg, secondArg)) {
        case 1:
            printHelp();
            return 0;
        case 2:
            maxDepth = strtol(secondArg, NULL, 10);
            path = thirdArg == NULL ? "." : thirdArg;
            break;
        case -2:
            printf("%s\n", "Enter a valid number for depth");
            return 0;
        default:
            path = firstArg == NULL ? "." : firstArg;

    }

    printf("%s\n", path);
    tree(path, 1);
}
```

Se manejan como posibles argumentos :
--depth, -d para la profundidad de la recursion, la cual se maneja en una variable 
estática dentro del archivo y sirve como un break extra

```c
static long maxDepth = -1; //-1 significa sin profundidad establecida

y se agrega el siguiente caso base a la funcion

void tree(...){
    ...
    if (depth > maxDepth && maxDepth != -1) {
        return;
    }   
    ...

```
![Depth usage](assets/tree_usage_1.png)


--help, -h que simplemente imprime en pantalla un mensaje de ayuda
![Help usage](assets/tree_usage_2.png)


el manejo de los argumentos se realiza en una funcion aparte que retorna un entero que se interpreta en el switch mediante una codificación implementada para interpretar
las configuraciones, mientras que -1*$code equivaldría a un error en el uso de esa opción:

```c
int argParse(const char *firstArg, const char *secondArg) {

    if (firstArg == NULL) {
        return 0;
    }

    if (strcmp(firstArg, "--help") == 0 || strcmp(firstArg, "-h") == 0) {
        return 1;
    }

    if (secondArg == NULL) {
        return 0;
    }
    if (strcmp(firstArg, "--depth") == 0 || strcmp(firstArg, "-d") == 0) {
        char *endptr;

        long _ = strtol(secondArg, &endptr, 10);

        if (strcmp(endptr, "") != 0) {
            return -2;
        }

        return 2;
    }

    return 0;
}
```


En el metodo se realizan comparaciones pertinentes para retornar un codigo entendible por el switch en main()


Ejemplo de uso:
![Usage](assets/tree_usage_3.png)


### 2.5. Penalizacion por uso intensivo de CPU

##### 1. Analisis teórico
El planificador original de MINIX 3 utiliza colas multinivel con prioridades fijas y 
envejecimiento (aging). Cada proceso tiene una prioridad numérica: los valores más bajos 
indican mayor prioridad. Los procesos interactivos suelen tener prioridad alta, mientras que 
los procesos CPU-bound son penalizados progresivamente.

Cuando un proceso agota su quantum, la función do_noquantum reduce su prioridad en una 
unidad, permitiendo que otros procesos ejecuten. Cada 5 segundos, balance_queues restaura la 
prioridad de los procesos penalizados hasta su valor máximo original (max_priority). En 
este esquema, la penalización y recuperación dependen únicamente del consumo del quantum 
y del balanceo periódico, sin considerar otras métricas como la cantidad de quantums 
consumidos antes de una operación de E/S.
 
El archivo donde se realiza la gestion del agotamiento de quantum es en el main.c, dentro de la funcion main, 
la cual corre un bucle "while" infinitamente q se encarga de la gestion de los procesos, al recibir el proceso,
dentro de este se extrae información relevante y salta a un switch, donde uno de los casos es 
```case SCHEDULING_NO_QUANTUM:``` el cual llama a la funcion do_notquantum q se encarga de rebajar la prioridad
al proceso.

Los datos de planificación se encuentran en el archivo schedproc.h, un header que define el struct schedproc, 
el cual agrupa la información relevante que necesita almacenar la entrada de un proceso para ser trabajada por
el scheduler.

Las funciones ```do_noquantum, balance_queues, do_start_scheduling y do_stop_scheduling...``` se definen en 
schedule.c, todas reciben un puntero que apunta al proceso gestionado, e influyen directamente sobre el 
struct y sus características y, por tanto, influyendo en la forma en que es tratado en el main, además
de algunas como ```do_start_scheduling``` que registra un proceso en la tabla y ```do_stop_scheduling```
libera su slot.

La prioridad se maneja en el struct, mediante la variable ```priority```, la prioridad maxima igualmente en la 
variable ```max_priority``` y el quantum asignado esta en ```time_slice```

##### $ Comportamiento previo a los cambios en el scheduler
En este paso se implementaron dos programas de prueba y analizaremos su comportamiento sobre la implementacion
de scheduling original de minix.

Para ello se implementaron y probaron dos programas con una logica casi idéntica, un for que ejecuta
mil millones de iteraciones, con un if dentro que chequea cuando se supera el millón de iteraciones.
También en uno de ellos se incluyó dentro del if un fragmento para imprimir un punto, y asi crear una 
llamada de I/O antes de completar el consumo del quantum (se midió con otro programa y se pudo constatar que en mi 
máquina virtual un millón de iteraciones se ejecutan en menos de 200ms, 50M de iteraciones consumen poco
más de 1 segundo)

Tendremos en cuenta los tiempos máximos de quantum (200ms) y balanceo (5s) definidos en schedule.c:

```c++
#include <stdio.h>
#include <time.h>

#define TOTAL_ITERS  1000000000LL
#define PRINT_EVERY  1000000LL

int main(void)
{
    long long iter;
    clock_t start, end;

    start = clock();

    for (iter = 1; iter <= TOTAL_ITERS; iter++) {
        if (iter % PRINT_EVERY == 0) {
            printf(".");    // lineas que varian entre 
            fflush(stdout); // uno y otro programa
        }
    }

    end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n1e9 iteraciones completadas.\n");
    printf("Tiempo total: %.4f segundos\n", elapsed);

    return 0;
}

```

Al ejecutar el programa hubo un problema, y es que el método con I/O estaba dando un tiempo mayor,
lo cual fue extrañó porque no debería pasar, asi que se consideró el hecho de que estaba trabajando desde la
terminal de mi propio sistema por ssh, por lo cual entre desde la vm de minix y los tiempos dieron lo que 
deberían.

Para el programa sin I/O, las mil millones de iteraciones se completaron en 26.1 segundos, mientras que en 
el otro caso, el tiempo fue de 25.86 segundos, inferior a pesar de que tiene operaciones extra, por lo cual 
se puede apreciar la influencia que tiene la liberación de cpu antes de consumir todo el quantum por parte del
que bloquea para hacer I/O.

##### 2. Modificación del scheduler

Se comenzó modificando el ```struct schedproc``` para agregar un nuevo campo ```unsigned used_quantums;```
para llevar la cuenta de quántums consumidos por el proceso, el mismo se inicializa en 0 en la funcion
```do_start_scheduling```.

Dentro de ```schedule.c```, se definió el macro ```MAX_PROC_Q``` para llevar la cuenta de la maxima cantidad de quántums
a consumir antes de rebajar la prioridad del proceso.

Despues se comenzó con la implementacion de la nueva logica de manejo de prioridad, para la cual en ampliamos 
la condicional dentro de ```do_noquantum``` que chequea que un proceso no entre por debajo de su prioridad
minima, cambiándolo por esto:

```c++
if (rmp->priority < MIN_USER_Q) { // checamos no descender por debajo de la prioridad minima
	if (rmp->used_quantums < MAX_PROC_Q) { // aumentamos el contador de quantums si estamos por debajo 
		rmp->used_quantums++;
	}
	else { // de lo contrario bajamos prioridad y reiniciamos contador
		rmp->priority += 1;/
		rmp->used_quantums = 0;
	}
}
```

A continuación se verifica si se consumieron todos los quantums, si no pues se aumenta, de lo contrario disminuimos la 
prioridad y reiniciamos el contador.

Ya que existe logica de balanceo implementada, se agregó una línea en la cual se reinicia
el contador de quantums al efectuar un balanceo:

```c++
if (rmp->flags & IN_USE) {
	if (rmp->priority > rmp->max_priority) { // logica original de recuoeracion de prioridad
		rmp->priority -= 1;
		schedule_process_local(rmp);
	}
	
	rmp->used_quantums = 0; // reiniciamos contador
}
```

Más adelante se formuló una mejora en las anteriores para la restauración gradual de prioridad en caso de 
no consumir quántums completos en una ventana. Transformamos ```MAX_PROC_Q``` en ```CRISTIANO_RONALDO```
(no se nos ocurría que  ponerle asi que le puse como el bicho siuuuuu), para q ahora el contador de quantums no
se reinicie al degradar la prioridad en caso de consumo excesivo, sino q la prioridad se degrade al alcanzar un valor 
en el contador divisible por ```CRISTIANO_RONALDO```, y de esta forma se puede verificar en la funcion de balance
si el contador de quantums llego limpio, y en ese caso se le da una mejora extra a su prioridad, siempre checando
no sobrepasar la maxima. Tras ello, entonces se reinicia el contador de quantums.

```c++
if (rmp->priority < MIN_USER_Q) {
	rmp->used_quantums++;
	if (rmp->used_quantums % CRISTIANO_RONALDO == 0) {
		rmp->priority += 1;
	}
}
```

De esta forma siempre se reduce la prioridad al consumir la cantidad especifica de quantums, 
sin reiniciar el contador

```c++
if (rmp->used_quantums == 0 && rmp->priority > rmp->max_priority) {
	rmp->priority -= 1;
}

rmp->used_quantums = 0;
```

Aquí se mantiene la lógica original de balance, y agregamos el caso específico que se propone en el documento
de orientacion del proyecto.

# 3. Resultados globales y discusión

A continuación se resume el estado final de cada componente del proyecto:

| Componente | Estado | Observaciones |
|------------|--------|---------------|
| Instalación y configuración de MINIX | â Funciona completamente | Se logró emular 
MINIX 3.4.0 sobre QEMU en arquitectura ARM |
| Personalización del mensaje de bienvenida | â Funciona completamente | El mensaje se 
muestra correctamente al iniciar sesión |
| Depuración del bug en `pthread_mutex_trylock` | â Funciona completamente | El 
programa de prueba retorna `EDEADLK` (11) y finaliza con `PASS` |
| Implementación del comando `tree` | â Funciona completamente | Muestra la jerarquÃ­a 
de directorios con opciones `-d`, `--depth` y `--help` |
| Penalización por uso intensivo de CPU | â Funciona según lo esperado | Los procesos 
CPU-bound ven reducida su prioridad tras consumir mÃºltiples quantums |

Las principales dificultades encontradas estuvieron relacionadas con la autenticación en 
GitHub mediante tokens, la configuración del servicio SSH en MINIX para trabajar 
remotamente, y la comprensión de la capa de compatibilidad entre `pthread_*` y `mthread_*`. 
En todos los casos, se lograron soluciones funcionales.

La modificación del scheduler requiriá un análisis cuidadoso del código fuente de MINIX 
para no introducir efectos no deseados en el balanceo de prioridades. La solución 
implementada penaliza gradualmente a los procesos CPU-bound sin afectar drásticamente el 
rendimiento general del sistema.

# 4. Conclusiones

El proyecto permitió cumplir con los objetivos planteados en la introducción. Se logró:

- Personalizar el mensaje de bienvenida de MINIX modificando el archivo `/usr/src/etc/motd`, 
lo que implica comprender el proceso de compilación del sistema.
- Depurar un bug crÃ­tico en `pthread_mutex_trylock`, identificando la causa raíz como una 
llamada recursiva accidental en la capa de compatibilidad.
- Implementar un comando `tree` funcional, utilizando llamadas al sistema como `opendir`, 
`readdir`, `closedir` y `lstat` para evitar ciclos con enlaces simbólicos.
- Modificar el planificador de MINIX para penalizar procesos CPU-bound mediante un contador 
de quantums consumidos, logrando que los procesos interactivos mantengan prioridades más 
altas.

Como lecciones aprendidas, se comprendiá la importancia de la capa de compatibilidad en 
sistemas operativos, el funcionamiento de los mutex y las condiciones de carrera, y la 
relevancia de una planificación justa en sistemas multitarea. La experiencia resultá 
fundamental para consolidar conceptos teóricos a través de su implementación práctica.

Como trabajo futuro, se propone extender el planificador para considerar también el uso de 
memoria o la frecuencia de operaciones de entrada/salida como mÃ©tricas adicionales para el 
ajuste de prioridades.

# 5. Referencias consultadas

1. Tanenbaum, A. S., & Bos, H. (2015). *Modern Operating Systems* (4th ed.). Pearson.

2. MINIX 3 Documentation. (s.f.). *The MINIX 3 Operating System*. Recuperado de 
https://wiki.minix3.org/

3. QEMU Documentation. (s.f.). *QEMU User Documentation*. Recuperado de 
https://www.qemu.org/docs/master/

4. MINIX 3 Source Code. (s.f.). *MINIX 3 Repository*. Recuperado de https://git.minix3.org/

5. GNU C Library Documentation. (s.f.). *System Interface and Headers*. Recuperado de 
https://www.gnu.org/software/libc/manual/

6. Open Group. (2018). *POSIX.1-2017 Standard*. Recuperado de 
https://pubs.opengroup.org/onlinepubs/9699919799/

7. GitHub Guides. (s.f.). *Forking Projects*. Recuperado de 
https://guides.github.com/activities/forking/
