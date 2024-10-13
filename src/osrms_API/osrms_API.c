#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "osrms_API.h"

FILE* memory_file = NULL;

void os_mount(char* memory_path){
    memory_file = fopen(memory_path, "r+b");
    if (memory_file == NULL){
        printf("Error al abrir el archivo de memoria.\n");
        exit(1);
    }
}

void os_ls_process(){
    fseek(memory_file, 0, SEEK_SET);
    for (int i = 0; i < 32; i++){
        //Se usa para leer cada entrada de la tabla de PCB
        unsigned char estado;
        fread(&estado, 1, 1, memory_file);

        if (estado == 0X01){ 
            //Cuando es 0X01 indica que el proceso está en ejecucion
            unsigned char process_id;
            char process_nombre[11];
            
            //Leemos el id y el nombre del proceso
            fread(&process_id, 1, 1, memory_file);
            fread(&process_nombre, 1, 10, memory_file);
            //Esta linea es para asegurarse de que el nombre es una cadena terminada en \0
            process_nombre[10] = '\0';

            printf("Proceso ID: %d, Nombre: %s\n", process_id, process_nombre);
        }
        //Para ir a la siguiente entrada de la PCB se hace lo siguiente, ya que leemos los primeros 13 bytes y saltamos lo siguiente.
        fseek(memory_file, 256 - 13, SEEK_CUR);
    }
}

int os_exists(char *file_nombre){
    //Comenzar en el inicio de la tabla de archivos
    fseek(memory_file, 32 * 256, SEEK_SET);
    //Guardamos espacio para almacenar el nombre de archivo leido
    char file_name_in_table[16];

    for(int i = 0; i < 128; i++){
        //Leer el archivo y asegurarse que sea una cadena terminada en \0
        fread(file_name_in_table, 1, 15, memory_file);
        file_name_in_table[15] = '\0';

        //Hacemos la comparacion del nombre leido con el que buscamos
        if (strcmp(file_name_in_table, file_nombre) == 0){
            return 1; //Indicamos que el archivo existe retornando un 1

        }
        fseek(memory_file, 32 - 16, SEEK_CUR);
    }
    return 0; //Caso de que no existe
}


osrmsFile* os_open(int process_id, char* file_nombre, char mode){
    if (process_id < 0 || process_id >= MAX_PROCESSES){
        return NULL; 
        //Se retorna cuando el proceso es invalido
    }

    // Si modo es r, buscaremos para lectura

    if(mode=='r'){
        //Verificar existencia
        if(!os_exists(process_id, file_nombre)){
            return NULL;
        }

        osrmsFile* file = (osrmsFile)malloc(sizeof(osrmsFile));
        file->process_id = process_id;
        strcpy(file->file_nombre, file_nombre);
        file->mode = 'r';
        return file;
    }

    //para modo w

    else if (mode == 'w'){
        if (os_exists(process_id, file_nombre)){
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
