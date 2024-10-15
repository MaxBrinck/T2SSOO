#include "../osrms_API/osrms_API.h"
#include <stdio.h>


int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ruta_al_archivo_de_memoria>\n", argv[0]);
        return 1;
    }

    // Montar la memoria
    os_mount((char *)argv[1]);

    // Listar los procesos en ejecución
    printf("Listando procesos en ejecución...\n");
    os_ls_process();
 

    // Variables fijas para testear os_exists
    int process_id = 117;  // Cambia este valor si quieres probar otro proceso
    char *file_nombre = "dino.jpg";  // Cambia este valor para probar otro archivo

    // Verificar si el archivo existe para el proceso dado
    printf("\nVerificando si el archivo '%s' existe para el proceso %d...\n", file_nombre, process_id);
    if (os_exists(process_id, file_nombre)) {
        printf("El archivo '%s' existe para el proceso %d.\n", file_nombre, process_id);
    } else {
        printf("El archivo '%s' no existe para el proceso %d.\n", file_nombre, process_id);
    }

    os_ls_files(117);
    //os_frame_bitmap();
    os_tp_bitmap();
    
    return 0;
}

