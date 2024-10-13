#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Osrms_File.h"

void os_ls_files(int process_id) {
    fseek(memory_file, 32*256, SEEK_SET);
    for (int i = 0; i < 128; i++){
        unsigned char file_process_id;
        char file_nombre[16];

        //Leer el id del proceso asociado al archivo
        fread(&file_process_id, 1, 1, memory_file);

        //Se revisa si el archivo pertenece al proceso actual
        if (file_process_id == process_id){

            //Leer el nombre del archivo y asegurarse que sea una cadena terminada en \0
            fread(file_name, 1, 15, memory_file);
            file_name[15] ='\0';

            printf("Archivo: %s\n", file_name);
        }
        //Pasar al siguiente archivo, nos saltamos el resto de los 32 bytes
        fseek(memory_file, 32 - 16, SEEK_CUR);
    }
}