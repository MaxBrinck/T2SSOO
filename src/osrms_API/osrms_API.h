#pragma once
#include "../osrms_File/Osrms_File.h"

#define MAX_PROCESSES 256

typedef struct osrmsFile {
    int process_id;  // ID del proceso al que pertenece el archivo
    char file_name[15];  // Nombre del archivo (hasta 14 caracteres + '\0')
    char mode;  // Modo de apertura ('r' para lectura, 'w' para escritura)
    unsigned int size;  // Tamaño del archivo
    unsigned int file_offset;  // Desplazamiento del archivo dentro de la memoria
} osrmsFile;

int os_exists(int process_id, char* file_nombre);
