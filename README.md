# Tarea 2

Para ocupar los comandos hay que colocar en main estos, no se realiza una interfaz de uso de usuario ya que no se especifica donde guardar la información de los archivos.
El archivo se corre usando:
make
valgrind ./osrms (archivo de memoria).bin
Si no se ocupa valgrind tira segmentation fault pero si se ocupa no lo arroja
En la función os_open por algun motivo transformaba la r y la w a entero, asique se trabajan como enteros pero estas reciben chars.
