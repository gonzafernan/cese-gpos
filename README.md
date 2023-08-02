
# Sistemas Operativos de Propósito General

## Trabajo práctico 1

### Objetivo
Comunicar dos procesos por medio de un named FIFO. El proceso *writer* podrá recibir texto por
la consola y signals. El proceso *reader* deberá loguear en un archivo el texto que recibe del
proceso *writer* y en otro archivo las signals que recibe el proceso *writer*.

### Arquitectura del sistema

![](imgs/tp1_arch.png)

**Proceso writer:**

Este proceso iniciará y se quedará esperando que el usuario ingrese texto hasta que presione ENTER.
En ese momento escribirá en una named FIFO los datos con el siguiente formato: *DATA:XXXXXXXXXXXX*

En cualquier momento el proceso podrá recibir las signals SIGUSR1 y SIGUSR2. En dicho caso deberá
escribir en el named FIFO el siguiente mensaje: *SIGN:1* o *SIGN:2*

**Proceso reader:**

Este proceso leerá los datos del named fifo y según el encabezado "DATA" o "SIGN" escribirá en el archivo *log.txt* o *signals.txt*

### Ejecución

Para iniciar proceso writer (compilación y ejecución):
```sh
make writer
```

Para iniciar proceso reader (compilación y ejecución):
```sh
make reader
```

Para enviar señales SIGUSR al proceso writer:
```sh
kill -SIGUSR1 {writer PID}
kill -SIGUSR2 {writer PID}
```

El PID del writer se ofrece en el mensaje de inicialización del proceso.
