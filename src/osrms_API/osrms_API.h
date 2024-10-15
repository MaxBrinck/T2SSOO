#pragma once
#include "../osrms_File/Osrms_File.h"

#define MAX_PROCESSES 256

typedef struct {
    int process_id;
    char file_nombre[16];
    char mode;
} osrmsFile;

int os_exists(int process_id, char* file_nombre);
