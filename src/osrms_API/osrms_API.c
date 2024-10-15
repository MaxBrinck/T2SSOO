#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "osrms_API.h"
#include <ctype.h>

FILE* memory_file = NULL;


typedef struct {
    File* file_ptr;
    char* file_nombre;
    char mode;
} osrmsFile;



void os_mount(char* memory_path) {
    memory_file = fopen(memory_path, "r+b");
    if (memory_file == NULL) {
        printf("Error al abrir el archivo de memoria.\n");
        exit(1);
    } else {
        printf("Archivo de memoria abierto correctamente.\n");
    }
}

void os_ls_process() {
    // Volver al inicio del archivo de memoria para leer los procesos
    fseek(memory_file, 0, SEEK_SET);

    printf("Procesos en ejecución:\n");

    // Iterar sobre las 32 entradas de la tabla de PCBs (cada entrada tiene 256 bytes)
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char process_id;
        char process_nombre[11];

        // Leer el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Si el estado es 0x01, significa que el proceso está en ejecución
        if (estado == 0x01) {
            // Leer el ID del proceso (1 byte)
            fread(&process_id, 1, 1, memory_file);

            // Leer el nombre del proceso (10 bytes) y asegurar la terminación de la cadena
            fread(process_nombre, 1, 10, memory_file);
            process_nombre[10] = '\0';  // Agregar el carácter nulo para que sea una cadena válida

            // Imprimir la información del proceso activo
            printf("Proceso ID: %d, Nombre: %s\n", process_id, process_nombre);
        } else {
            // Si el proceso no está activo, saltar el ID y nombre
            fseek(memory_file, 11, SEEK_CUR);  // Saltamos los 11 bytes del ID y nombre
        }

        // Saltar el resto de los 256 bytes que no hemos leído aún
        fseek(memory_file, 256 - 12, SEEK_CUR);  // Saltamos los bytes restantes
    }
}



int os_exists(int process_id, char *file_nombre) {
    // Posicionamos el puntero al inicio de la memoria
    fseek(memory_file, 0, SEEK_SET);

    // Iteramos sobre los 32 procesos en la tabla de PCBs
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char pid;
        char process_nombre[12];

        // Leemos el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Leemos el ID del proceso (1 byte)
        fread(&pid, 1, 1, memory_file);

        // Leemos el nombre del proceso (11 bytes)
        fread(process_nombre, 1, 11, memory_file);
        process_nombre[11] = '\0';  // Aseguramos que la cadena esté terminada en '\0'

        if (pid == process_id) {
            // Hemos encontrado el proceso con el process_id dado
            // Ahora leemos su tabla de archivos
            // La tabla de archivos ocupa 115 bytes (5 entradas de 23 bytes cada una)
            // Ya hemos leído 1 + 1 + 11 = 13 bytes de la entrada del proceso

            // Iteramos sobre las 5 entradas de archivos
            for (int j = 0; j < 5; j++) {
                unsigned char file_validity;
                char file_name[15];

                // Leemos el byte de validez (1 byte)
                fread(&file_validity, 1, 1, memory_file);

                // Leemos el nombre del archivo (14 bytes)
                fread(file_name, 1, 14, memory_file);
                file_name[14] = '\0';  // Terminamos la cadena con '\0'

                // Si la entrada es válida, comparamos el nombre
                if (file_validity == 0x01) {
                    if (strncmp(file_name, file_nombre, 14) == 0) {
                        // El archivo existe para este proceso
                        return 1;
                    }
                }

                // Saltamos los 8 bytes restantes de la entrada del archivo (tamaño y dirección virtual)
                fseek(memory_file, 8, SEEK_CUR);
            }

            // Si llegamos aquí, el archivo no existe para este proceso
            return 0;
        } else {
            // No es el proceso que buscamos, saltamos al siguiente
            // Ya hemos leído 13 bytes de la entrada de proceso
            // Cada entrada de proceso ocupa 256 bytes
            // Por lo tanto, saltamos los bytes restantes
            fseek(memory_file, 256 - 13, SEEK_CUR);
        }
    }

    // Si llegamos aquí, el proceso con el process_id dado no existe
    printf("El proceso con ID %d no existe.\n", process_id);
    return 0;
}
void os_ls_files(int process_id) {
    // Volver al inicio del archivo de memoria para leer los procesos
    fseek(memory_file, 0, SEEK_SET);

    // Iterar sobre las 32 entradas de la tabla de PCBs (cada entrada tiene 256 bytes)
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char pid;
        char process_nombre[11];

        // Leer el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Leer el ID del proceso (1 byte)
        fread(&pid, 1, 1, memory_file);

        // Leer el nombre del proceso (10 bytes)
        fread(process_nombre, 1, 10, memory_file);
        process_nombre[10] = '\0';  // Asegurar la terminación de cadena

        // Si encontramos el proceso con el ID correcto
        if (pid == process_id) {
            printf("Archivos asociados al proceso %d:\n", process_id);

            // Iterar sobre los 5 archivos en la tabla de archivos
            for (int j = 0; j < 5; j++) {
                unsigned char file_validity;
                char file_nombre[15];
                unsigned int file_size = 0;
                unsigned char size_bytes[4];  // Para leer el tamaño byte a byte

                // Leer el byte de validez (1 byte)
                fread(&file_validity, 1, 1, memory_file);

                // Leer y descartar el primer byte (el caracter problemático)
                fseek(memory_file, 1, SEEK_CUR);

                // Leer el nombre del archivo (13 bytes después del byte que ignoramos)
                fread(file_nombre, 1, 13, memory_file);
                file_nombre[13] = '\0';  // Asegurar la terminación de cadena

                // Leer el tamaño del archivo (4 bytes)
                fread(size_bytes, 1, 4, memory_file);

                // Verificamos los bytes crudos leídos (hexadecimal)
                printf("Bytes crudos del tamaño del archivo (hex): %02x %02x %02x %02x\n", size_bytes[0], size_bytes[1], size_bytes[2], size_bytes[3]);

                // Si el primer byte es 00, lo movemos al final
                if (size_bytes[0] == 0x00) {
                    unsigned char temp = size_bytes[0];
                    size_bytes[0] = size_bytes[1];
                    size_bytes[1] = size_bytes[2];
                    size_bytes[2] = size_bytes[3];
                    size_bytes[3] = temp;
                }

                // Reconstruir el tamaño en little-endian
                file_size = (size_bytes[0]) | (size_bytes[1] << 8) | (size_bytes[2] << 16) | (size_bytes[3] << 24);

                // Imprimir el tamaño calculado
                printf("Tamaño calculado: %u bytes\n", file_size);

                // Saltar la dirección virtual (4 bytes, no es necesario para esta función)
                fseek(memory_file, 4, SEEK_CUR);

                // Imprimir la información del archivo, válido o no
                printf("\tArchivo: %s, Tamaño: %u bytes\n", file_nombre, file_size);
            }
            return;  // Una vez encontrado e impreso, salir de la función
        } else {
            // Si no es el proceso correcto, saltamos al siguiente proceso
            fseek(memory_file, 256 - 12, SEEK_CUR);
        }
    }

    // Si llegamos aquí, el proceso con el process_id dado no existe
    printf("El proceso con ID %d no existe.\n", process_id);
}




//Esto esta malo, no entiendo como acceder a los archivos, luego de encontrar el process id
osrmsFile* os_open(int process_id, char* file_nombre, char mode) {
    fseek(memory_file, 0, SEEK_SET);

    for(int i = 0; i < 32; i++){
        unsigned char estado;
        unsigned char pid;
        char process_nombre[11];

        fread(&estado, 1, 1, memory_file);

        fread(&pid, 1, 1, memory_file);

        fread(process_nombre, 1, 10, memory_file);
        process_nombre[10] = \0;

        if (pid == process_id){
            //Modo lectura
            if(mode == 'r'){
                // El archivo no existe
                if (!os_exists(process_id, file_nombre)) {
                    printf("El archivo %s no existe para el proceso %d.\n", file_nombre, process_id);
                    return NULL; 
                }
            }
            //Si en lugar se abre en modo escritura
            else if (mode == 'w'){
                if (os_exists(process_id, file_nombre)){
                    printf("El archivo %s ya existe para el proceso %d.\n", file_nombre, process_id);
                    return NULL;
                }
            }
            // Asignar memoria para el archivo
            osrmsFile* file = (osrmsFile*)malloc(sizeof(osrmsFile));
            if (file == NULL) {
                printf("Error al asignar memoria para el archivo.\n");
                return NULL;  // Error de asignación
            }


            file->file_nombre = malloc(strlen(file_nombre) + 1);
            if (file->file_nombre == NULL){
                printf("Error al asignar memoria para el nombre del archivo.\n");
                free(file);
                return NULL;
            }

            strcpy(file->file_nombre, file_nombre);
            file->mode = mode;

            return file;
        }

        // Saltar al siguiente proceso (cada proceso ocupa 256 bytes)
        fseek(memory_file, 256 - 12, SEEK_CUR);
    }

    // Si llegamos aquí, el proceso no existe
    printf("El proceso con ID %d no existe.\n", process_id);
    return NULL;
}




void listar_procesos() {
    // Posicionar el puntero al inicio del archivo de memoria
    fseek(memory_file, 0, SEEK_SET);

    printf("Lista de procesos en la memoria:\n");

    // Iterar sobre las 32 entradas de la tabla de procesos (PCBs)
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char process_id;
        char process_nombre[11];

        // Leer el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Leer el ID del proceso (1 byte)
        fread(&process_id, 1, 1, memory_file);

        // Leer el nombre del proceso (10 bytes)
        fread(process_nombre, 1, 10, memory_file);
        process_nombre[10] = '\0';  // Asegurar la terminación de cadena con \0

        // Imprimir el proceso encontrado
        printf("Proceso ID: %d, Estado: %02x, Nombre: %s\n", process_id, estado, process_nombre);

        // Saltar al siguiente proceso (cada proceso ocupa 256 bytes)
        fseek(memory_file, 256 - 12, SEEK_CUR);
    }
}

void listar_procesos_y_archivos() {
    // Posicionar el puntero al inicio del archivo de memoria
    fseek(memory_file, 0, SEEK_SET);

    printf("Lista de procesos en la memoria:\n");

    // Iterar sobre las 32 entradas de la tabla de procesos (PCBs)
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char process_id;
        char process_nombre[11];

        // Leer el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Leer el ID del proceso (1 byte)
        fread(&process_id, 1, 1, memory_file);

        // Leer el nombre del proceso (10 bytes)
        fread(process_nombre, 1, 10, memory_file);
        process_nombre[10] = '\0';  // Asegurar la terminación de cadena con \0

        // Imprimir el proceso encontrado
        printf("Proceso ID: %d, Estado: %02x, Nombre: %s\n", process_id, estado, process_nombre);

        // Ahora vamos a leer la tabla de archivos del proceso
        printf("Archivos asociados al proceso %d:\n", process_id);

        // Iterar sobre los 5 archivos en la tabla de archivos
        for (int j = 0; j < 5; j++) {
            unsigned char file_validity;
            char file_nombre[15];  // Almacenará el nombre del archivo

            // Leer el byte de validez (1 byte)
            fread(&file_validity, 1, 1, memory_file);

            // Leer el nombre del archivo (14 bytes)
            fread(file_nombre, 1, 14, memory_file);
            file_nombre[14] = '\0';  // Terminamos la cadena con \0

            // Si la entrada del archivo es válida, imprimimos el nombre
            if (file_validity == 0x01) {
                printf("\tArchivo válido: %s\n", file_nombre);
            } else {
                printf("\tEntrada de archivo no válida\n");
            }

            // Saltar los 8 bytes restantes de la entrada del archivo (tamaño del archivo y dirección virtual)
            fseek(memory_file, 8, SEEK_CUR);
        }

        // Saltar al siguiente proceso (cada proceso ocupa 256 bytes)
        fseek(memory_file, 256 - 12 - 5 * 23, SEEK_CUR);
    }
}

void os_frame_bitmap() {
    // Posicionamos el puntero al inicio del Frame Bitmap (después de la tabla de procesos y otros segmentos)
    fseek(memory_file, (32 * 256) + 128 + (128 * 1024), SEEK_SET);

    unsigned char byte;
    int frames_ocupados = 0;
    int frames_libres = 0;

    printf("Estado del Frame Bitmap (en formato binario):\n");

    // Como hay 65,536 frames, recorremos 8 KB del Frame Bitmap (8 * 1024 bytes)
    for (int i = 0; i < (65536 / 8); i++) {  // 65536 frames, 8 frames por byte
        // Leer un byte del Frame Bitmap
        fread(&byte, 1, 1, memory_file);

        // Revisar cada bit del byte y mostrarlo como binario
        for (int j = 7; j >= 0; j--) {  // Mostrar el byte en orden inverso (bit más significativo primero)
            int bit = (byte >> j) & 1;  // Extraemos el bit j-ésimo
            printf("%d", bit);  // Imprimir el bit (0 o 1)

            if (bit == 1) {
                frames_ocupados++;
            } else {
                frames_libres++;
            }
        }

        // Imprimir un salto de línea cada 8 bytes (para agrupar visualmente)
        if ((i + 1) % 8 == 0) {
            printf("\n");
        } else {
            printf(" ");  // Espacio entre grupos de bytes
        }
    }

    // Imprimir el conteo de frames ocupados y libres
    printf("Total de frames ocupados: %d\n", frames_ocupados);
    printf("Total de frames libres: %d\n", frames_libres);
}

void os_tp_bitmap() {
    // Posicionamos el puntero al inicio del Bitmap de Tablas de Páginas
    fseek(memory_file, 32 * 256, SEEK_SET);  // Nos ubicamos en el bitmap de Tablas de Páginas (128 bytes)

    unsigned char byte;
    int tp_ocupadas = 0;
    int tp_libres = 0;

    printf("Estado del Bitmap de Tablas de Páginas:\n");

    // Como el Bitmap de Tablas de Páginas tiene 128 bytes, recorremos 128 bytes
    for (int i = 0; i < 128; i++) {
        // Leer un byte del Bitmap de Tablas de Páginas
        fread(&byte, 1, 1, memory_file);

        // Imprimir el byte en formato binario
        for (int j = 7; j >= 0; j--) {
            int bit = (byte >> j) & 1;  // Extraemos el bit j-ésimo
            printf("%d", bit);

            // Contar si la tabla está ocupada o libre
            if (bit == 1) {
                tp_ocupadas++;
            } else {
                tp_libres++;
            }
        }

        // Salto de línea después de cada byte (8 tablas de páginas por línea)
        printf("\n");
    }

    // Imprimir el conteo de Tablas de Páginas ocupadas y libres
    printf("Total de Tablas de Páginas ocupadas: %d\n", tp_ocupadas);
    printf("Total de Tablas de Páginas libres: %d\n", tp_libres);
}

void os_start_process(int process_id, char* process_name){
     fseek(memory_file, 0, SEEK_SET);

    // Iteramos sobre los 32 procesos en la tabla de PCBs
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char pid;
        char process_nombre[12];

        int encontrado = 0;

        fread(&estado, 1, 1, memory_file);

        // Leemos el ID del proceso (1 byte)
        fread(&pid, 1, 1, memory_file);

        // Leemos el nombre del proceso (11 bytes)
        fread(process_nombre, 1, 11, memory_file);
        process_nombre[11] = '\0';  // Aseguramos que la cadena esté terminada en '\0'
        

        //Lei en una issue que se puede asumir que siempre que el proceso exista y se pida un start su estado será cero

        //Entramos en un if en caso de encontrar el id del proceso en la tabla de PCBs
        if (pid == process_id) {
            encontrado = 1;
            printf("Archivos asociados al proceso %d:\n", process_id);

            //Movemos para poder escribir en el lugar donde se encuentra el estado
            fseek(memory_file,-(12 + 1), SEEK_CUR);

            printf("iniciando el proceso de id: %d\n", process_id);
            estado = 0x01;
            fwrite(&estado, 1, 1, memory_file);

            break;
        }

        fseek(memory_file, 256 - 12, SEEK_CUR);  // Saltamos los bytes restantes
    }
    //Ahora se debe analizar el caso en que no existe el proceso, por lo que no entra en
    if (encontrado == 0){
        //Regresamos el cursor al incio
        fseek(memory_file, 0, SEEK_SET)

        for (int i = 0; i < 32; i++){
            unsigned char estado;
            unsigned char pid;
            // Leer el estado del proceso (1 byte)
            fread(&estado, 1, 1, memory_file);
            // Leer el ID del proceso (1 byte)
            fread(&pid, 1, 1, memory_file);

            //Asumiendo que el process id es 0 si el espacio está libre
            if (pid == 0){

                //Devolvemos el cursor
                fseek(memory_file,-(2), SEEK_CUR);
                
                estado = 0x01;
                fwrite(&estado, 1, 1, memory_file);

                fwrite(&process_id, 1, 1, memory_file);

                // Inicializamos el array con ceros
                char nombre_proceso[11] = {0};

                // Copiamos como máximo 11 caracteres
                strncpy(nombre_proceso, process_name, 11);  
                fwrite(nombre_proceso, 1, 11, memory_file);
                printf("Creando nuevo proceso %d con nombre '%s'\n", process_id, process_name);
                return; 

            }
            
            fseek(memory_file, 256 - 2, SEEK_CUR);
        if (i == 32){
            printf("No hay espacio para crear un nuevo proceso.\n");
        }
        }
    }
}

void os_finish_process(int process_id) {
    // Vamos al inicio del archivo
    fseek(memory_file, 0, SEEK_SET);

    // Iterar sobre las 32 entradas de la tabla de PCBs
    for (int i = 0; i < 32; i++) {
        unsigned char estado;
        unsigned char pid;
        char process_nombre[11];

        // Leer el estado del proceso (1 byte)
        fread(&estado, 1, 1, memory_file);

        // Leer el ID del proceso (1 byte)
        fread(&pid, 1, 1, memory_file);

        // Leer el nombre del proceso (10 bytes)
        fread(process_nombre, 1, 10, memory_file);
        process_nombre[10] = '\0';  // Asegurar la terminación de cadena

        // Si encontramos el proceso con el ID correcto y su estado es en ejecución
        if (pid == process_id && estado == 0x01) {
            // (cambiamos el estado a 0x00)
            unsigned char nuevo_estado = 0x00;
            fseek(memory_file, -12, SEEK_CUR);  // Volver un bytes atras
            fwrite(&nuevo_estado, 1, 1, memory_file);  // Escribir el nuevo estado





            //Agregue esto para mover hacia adelante y que el proximo a leer sea el file validity
            fread(&pid, 1, 1, memory_file);
            // Leer el nombre del proceso (10 bytes)
            fread(process_nombre, 1, 10, memory_file);
            process_nombre[10] = '\0';  // Asegurar la terminación de cadena






            // Liberar los archivos asociados al proceso
            for (int j = 0; j < 5; j++) {
                unsigned char file_validity;
                char file_nombre[15];

                // Leer el byte de validez (1 byte)
                fread(&file_validity, 1, 1, memory_file);

                // Si el archivo es válido, liberamos su espacio
                if (file_validity == 0x01) {
                    // Si se requiere, actualizar el estado de validez del archivo a no válido
                    fseek(memory_file, -1, SEEK_CUR);  // Volver un byte atrás para escribir
                    unsigned char invalido = 0x00;  // Estado inválido
                    fwrite(&invalido, 1, 1, memory_file);  // Marcar como no válido
                }

                // Saltar el resto de la entrada de archivo
                //creo que este salto está mal hecho, no cache muy bien cuanto hay que saltar
                fseek(memory_file, 8, SEEK_CUR);  // Saltar a la siguiente entrada de archivo
            }

            printf("Proceso con ID %d terminado correctamente.\n", process_id);
            return;  // Salir de la función

        } else {
            // Si no es el proceso correcto, saltamos al siguiente
            fseek(memory_file, 256 - 12, SEEK_CUR);
        }
    }

    // Si llegamos aquí, el proceso no estaba activo o no se encontró
    printf("No se encontró un proceso activo con ID %d.\n", process_id);
}