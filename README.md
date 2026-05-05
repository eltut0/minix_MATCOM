## Integrantes:
Richard Alejandro Reyes Gracia C212
Jorge Julio de Leon Masson C212

# 2. Desarrollo y resultados por componentes
### 2.1. Instalacion y configuracion de minix
**RICHARD**

El emulador que use fue Qemu 10.2.2 usado porque mi computadora es ARM y 
Minix 3.4.0 requiere x86, VirtuaBox no se us?? xq este solo virtualiza

2GB de RAM Suficiente para compilar sin lentitud, sin desperdiciar recursos del 
host
1 n??cleo de CPU MINIX no requiere m??s; emular m??ltiples n??cleos en QEMU/ARM 
a??ade sobrecarga
20GB de disco duro Eepacio suficiente para c??digo fuente y compilaciones; 
formato din??mico ahorra espacio en el host
IPv4 only con DHCP y redirecci??n de pueros (SSH en puerto 7722) modo usuario 
simple, evita problemas de IPv6 en MINIX 3

El 1er emulador que use fue UTM pero no reconooc??a la ISO por problemas de 
compatibiliad por lo cual cambii?? a Qemu
QEMU di?? error "file driver requires regular file" por punto en la ruta 
(richarda.) por lo cual mov?? los archivos a ~/minix_vm y usar rutas relativas
Error de SSL al hacer git push se arregl?? desabilitandoverificaci??n SSL con 
git config --global http.sslVerify false 

Herramientas adicionales instaladas
Git: Clonar repositorio oficial (/usr/src) y subir cambios a GitHub
Nano: Editar archivos como /usr/src/etc/motd

**JORGE**

Para crear la maquina virtual decidi usar Qemu ya que hace algun tiempo tuve
probelmas con virtual box sobre mi plataforma linux y he usado desde entonces
qemu para gestionar maquinas virtuales, inicialmente sobre la interfaz grafica
Gnome Boxes, y posteriormente sobre terminal para aprovechar mejor las posibilidades
que ofrece el software.

En primera instancia hice el fork del repositorio del profesor Christopher y lo clone
para intentar compilar mi propia imagen ISO con los archivos de la carpeta /releasetools, 
lo cual no salio nada bien ya que una incompatibilidad de mi compilador de C hizo q fallara
el proceso, asi que no quise romper nada y opte por seguir el proceso descrito por el profesor 
en el video donde explicaba el proceso de instalacion en el cual descargaba su imagen ISO 
directamente desde la pagina oficial de Minix.

Le asigne a la maquina virtual en principio 1 nucleo que es lo que viene por defecto cuando
no se especifica en el comando, aunque posteriormente termine incrementandolo a 4 ya que 
el mismo proyecto de minix al ser tan amplio, hace que comandos relacionados a git como un 
`bash git status` se sientan algo lentos, en memoria asigne 2G de ram, y un disco de 10G.
Tome el comando y lo puse en un archivo al que nombre run_system.sh, al cual asigne permisos
para poder evitar tener que estar buscando el comando cada vez que necesitara ejecutar la maquina.

Como herramientas adicionales una vez terminada de instalar la maquina virtual, agregue nano que 
es un editor con el que estoy algo familiarizado ya que es el que uso con regularidad para ediciones 
rapidas o revisar archivos como por ejemplo los de configuracion de mi sistema. Ademas instale git 
para gestionar el repositorio del proyecto. Ademas estableci una contrasena para el usuario root
y me cree otro usuario.


### 2.2. Personalizacion del mensaje de bienvenida

**JORGE**

En mi caso tuve algunos problemas, ya que a la hora de hacer un push con git, es necesario agregar un 
token de verificacion, y como no se permite, o al menos no conozco como compartir portapapeles entre 
ambas maquinas, me dispuse a activar el servicio ssh para facilitarme el trabajo. En un principio cuando
cree la maquina virtual no me di cuenta de lo siguiente: en el archivo de configuracion dice q el puerto 
por defecto es el 22 pero era necesario mapearlo a un puerto de mi pc, asi q anadi estas lineas que estaban 
en la seccion de ssh de la pagina de la documentacion dedicadas a la virtualizacion con qemu:
```bash
-net user,hostfwd=tcp::10022-:22 -net nic
```
Las agregue, arranque el ssh en minix, con una peculiaridad y es que aunque la documentacion brinda este 
comando:
```bash
/usr/pkg/etc/rc.d/sshd start
```
Me pidio que usara 'onestart' en lugar de 'start'.

Al intentar conectarme por ssh con normalidad tuve otro problema, y es que al intentar entrar por root
me fallaba la autenticación, me decía q la contraseña root era incorrecta. Despues de unos cuantos intentos
de romperme la cabeza se me ocurrió intentarlo por el usuario alternativo que me habia creado al crear la vm, 
y entonces me dejo entrar.

![Welcome message picture](assets/welcome.png)


### 2.3. Depuración de un bug en pthread


1. S??ntomas
Al ejecutar el programa de prueba proporcionado por el profesor, se observ?? el siguiente comportamiento an??malo:

La primera llamada a pthread_mutex_trylock retornaba 0 (OK), indicando que el mutex se hab??a bloqueado correctamente.
La segunda llamada a pthread_mutex_trylock (realizada por el mismo hilo) no retornaba nunca. El programa se quedaba congelado, sin mostrar los mensajes siguientes (unlock, destroy, PASS), y el 
prompt de MINIX no volv??a a aparecer hasta que se forzaba la interrupci??n con Ctrl + C.

2. Ana??lisis
En MINIX, las funciones de hilos se dividen en dos capas. Por un lado, la capa de compatibilidad (archivo libmthread/pthread_compat.c) provee las funciones est??ndar pthread_* que los 
programadores utilizan. Por otro lado, la implementaci??n nativa (archivo libmthread/mutex.c) provee las funciones reales mthread_* que manejan los mutex a bajo nivel. La capa de compatibilidad 
act??a como un traductor: cada funci??n pthread_* deber??a llamar a su equivalente mthread_*.

Al inspeccionar pthread_compat.c, se localiz?? la funci??n pthread_mutex_trylock y se encontr?? el siguiente c??digo: return pthread_mutex_trylock(mutex);

El problema es que la funci??n se llama a s?? misma recursivamente en lugar de llamar a mthread_mutex_trylock. Esto es un error tipogr??fico

3. Correc?i??n
La soluci??n es m??nima: cambiar la llamada recursiva por la llamada a la funci??n nativa.
Cambiar return pthread_mutex_trylock(mutex); por return mthread_mutex_trylock(mutex);

4. Verificaci??n
Despu??s de aplicar la correcci??n, se recompil?? la librer??a y se ejecut?? nuevamente el programa de prueba.
Soluci??n obtenid:
first trylock: 0 (OK)
second trylock: 11 (Resource deadlock avoided)
unlock: 0 (OK)
destroy: 0 (OK)
PASS
El programa ya no se congela. La segunda llamada retorna el c??digo de error esperadoy el programa contin??a ejecutando unlock, destroy y finalmente imprime PASS.

### 2.4. Implementacion del comando tree


Implemente en el metodo main la siguiente estructura para el manejo de argumentos en la invocacion del programa:

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

##### 1. Analisis previo
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
En este paso implementaremos dos programas de prueba y analizaremos su comportamiento sobre la implementacion
de scheduling original de minix.

Para ello se implementaron y probaron dos programas con una logica casi idéntica, un for que ejecuta
mil millones de iteraciones, con un if dentro que chequea cuando se supera el millón de iteraciones.
También en uno de ellos metimos dentro del if, un fragmento para imprimir un punto, y asi crear una 
llamada de I/O antes de completar el consumo del quantum (medí con otro programa y puedo asegurar que en mi 
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

Al ejecutar el programa tuve un problema, y es que el método con I/O me estaba dando un tiempo mayor,
lo cual me extrañó porque no debería pasar, asi que considere el hecho de que estaba trabajando desde la
terminal de mi propio sistema por ssh, por lo cual entre desde la vm de minix y los tiempos dieron lo que 
deberían.

Para el programa sin I/O, las mil millones de iteraciones se completaron en 26.1 segundos, mientras que en 
el otro caso, el tiempo fue de 25.86 segundos, inferior a pesar de que tiene operaciones extra, por lo cual 
se puede apreciar la influencia que tiene la liberación de cpu antes de consumir todo el quantum por parte del
que bloquea para hacer I/O.

##### 2. Modificación del scheduler

Comenzamos modificando el ```struct schedproc``` para agregar un nuevo campo ```unsigned used_quantums;```
para llevar la cuenta de quántums consumidos por el proceso, el mismo se inicializa en 0 en la funcion
```do_start_scheduling```.

Dentro de ```schedule.c```, definimos el macro ```MAX_PROC_Q``` para llevar la cuenta de la maxima cantidad de quántums
a consumir antes de rebajar la prioridad del proceso.

Despues comenzamos con la implementacion de la nueva logica de manejo de prioridad, para la cual en ampliamos 
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

Ahora checamos si se consumieron todos los quántums, si no pues se aumenta, de lo contrario disminuimos la 
prioridad y reiniciamos el contador.

Ya que existe logica de balanceo implementada, lo que hicimos fue agregar una linea en la cual se reinicia
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

Mas adelante formulamos una mejora en las anteriores para la restauración gradual de prioridad en caso de 
no consumir quántums completos en una ventana. Transformamos ```MAX_PROC_Q``` en ```CRISTIANO_RONALDO```
(no se me ocurría que  ponerle asi que le puse como el bicho siuuuuu), para q ahora el contador de quantums no
se reinicie al degradar la prioridad en caso de consumo excesivo, sino q la prioridad se degrade al alcanzar un valor 
en el contador divisible por ```CRISTIANO_RONALDO```, y de esta forma podemos checar en la funcion de balance
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

Aquí mantenemos la logica original de balance, y agregamos el caso específico que se propone en el documento
de orientacion del proyecto.