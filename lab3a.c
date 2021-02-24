#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int imageFD = -1;

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Error when opening file.\n");
        exit(1);
    }

    imageFD = open(argv[1], O_RDONLY);
    if(imageFD < 0){
        fprintf(stderr, "Error when reading from file descriptor.\n");
        exit(1);
    }

}