#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int imageFD = -1;

struct ext2_super_block supBlock;
const int supBlockOffset = 1024;

__u16 inodeSize;
__u32 blockSize;

void readAndPrintSB(){
    int ret = pread(imageFD, &supBlock, sizeof(supBlock), supBlockOffset);
    if(ret < 0){
        fprintf(stderr, "Error when reading from .img\n");
        exit(1);
    }
    __u32 blockCount = supBlock.s_blocks_count;
    __u32 inodeCount = supBlock.s_inodes_count;
    __u32 blocksPerGroup = supBlock.s_blocks_per_group;
    __u32 inodesPerGroup = supBlock.s_inodes_per_group;
    __u32 firstNRInode = supBlock.s_first_ino;

    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blockCount, inodeCount, blockSize, inodeSize, blocksPerGroup, inodesPerGroup, firstNRInode);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Error in given arguments.\n");
        exit(1);
    }

    imageFD = open(argv[1], O_RDONLY);
    if(imageFD < 0){
        fprintf(stderr, "Error when opening file.\n");
        exit(1);
    }

    inodeSize = supBlock.s_inode_size;
    blockSize = EXT2_MIN_BLOCK_SIZE << supBlock.s_log_block_size;

    readAndPrintSB();
    

}