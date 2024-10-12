#include <stdio.h>
#include <string.h>       // For handling strings (like file names)
#include <stdlib.h>       // For dynamic memory allocation

#include "application_layer.h"

int main(int argc, char *argv[]){
    //TODO verify arguments: file_name, serialPort, role

    //LinkLayer link;

    if(strcmp(argv[2],"tx") == 0)
        transmitFile(argv[0]);
    else if(strcmp(argv[2],"rx") == 0)
        receiveFile(argv[0]);
    else{
        printf("Invalid role: Choose 'tx' or 'rx'");
        return -1;
    }

    return 0;
}