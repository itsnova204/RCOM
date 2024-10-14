#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "application_layer.h"
#include "link_layer.h"

//added verifying args in main, declaring link

int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Should be 3 arguments: <file> <serialPort> <role>\n");
        printf("role: 'tx' for transmitter and 'rx' for receiver\n");
        return -1;
    }

    if(strcmp(argv[3],"tx") == 0)
        transmitFile(argv[1], argv[2]);
    else if(strcmp(argv[3],"rx") == 0)
        receiveFile(argv[1],argv[2]);
    else{
        printf("Invalid role: Choose 'tx' or 'rx'");
        return -1;
    }

    return 0;
}