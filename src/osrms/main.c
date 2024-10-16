#include "../osrms_API/osrms_API.h"
#include <stdio.h>


int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ruta_al_archivo_de_memoria>\n", argv[0]);
        return 1;
    }


    os_mount((char *)argv[1]);

    

    
    return 0;
}

