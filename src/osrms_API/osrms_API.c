#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "osrms_API.h"
#include <ctype.h>

FILE* memory_file = NULL;

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



osrmsFile* os_open(int process_id, char* file_nombre, char mode) {
    if (process_id < 0 || process_id >= MAX_PROCESSES) {
        return NULL;
    }

    if (mode == 'r') {
        if (!os_exists(process_id, file_nombre)) {
            return NULL;
        }

        osrmsFile* file = (osrmsFile*)malloc(sizeof(osrmsFile));
        file->process_id = process_id;
        strcpy(file->file_nombre, file_nombre);
        file->mode = 'r';
        return file;
    }

    else if (mode == 'w') {
        if (os_exists(process_id, file_nombre)) {
            return NULL;
        }

        osrmsFile* file = (osrmsFile*)malloc(sizeof(osrmsFile));
        file->process_id = process_id;
        strcpy(file->file_nombre, file_nombre);
        file->mode = 'w';
        return file;
    }

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
