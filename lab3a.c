#include <stdio.h>
#include <stdlib.h>
#include "ext2_fs.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int imageFD = -1;

struct ext2_super_block supBlock;
const int supBlockOffset = 1024;

struct ext2_group_desc groupDesc;

__u32 inodeSize;
__u32 blockSize;

void readAndPrintSB()
{
    int ret = pread(imageFD, &supBlock, sizeof(supBlock), supBlockOffset);
    if (ret < 0)
    {
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

void printGroupSummary()
{
    int ret = pread(imageFD, &groupDesc, sizeof(groupDesc), supBlockOffset + sizeof(supBlock));
    if (ret < 0)
    {
        fprintf(stderr, "Error in reading from image fd to group descriptor.\n");
        exit(1);
    }

    __u32 blockCount = supBlock.s_blocks_count;
    __u32 inodeCount = supBlock.s_inodes_count;
    __u32 freeBlocks = groupDesc.bg_free_blocks_count;
    __u32 freeInodes = groupDesc.bg_free_inodes_count;
    __u32 freeBlockBitmap = groupDesc.bg_block_bitmap;
    __u32 freeInodeBitmap = groupDesc.bg_inode_bitmap;
    __u32 firstInodeBlock = groupDesc.bg_inode_table;

    fprintf(stdout, "GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", blockCount, inodeCount, freeBlocks, freeInodes, freeBlockBitmap, freeInodeBitmap, firstInodeBlock);
}

void print_free_block_entries(__u32 freeBlockBitmap)
{
    char *bitmap = (char *)malloc(blockSize);
    pread(imageFD, bitmap, blockSize, 1024);
    int offset = 0;
    int index;
    int block_number = 1;
    for (block_number = 1; block_number < blockSize; block_number++)
    {
        index = (block_number - 1) / 8;
        offset = (block_number - 1) % 8;
        if (bitmap[index] & (1 << offset) == 1)
            fprintf(stdout, "BFREE,%d\n", block_number);
    }
    //some for loop
}

void print_directory_entires(struct ext_inode current_inode, ) {
    
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Error in given arguments.\n");
        exit(1);
    }

    imageFD = open(argv[1], O_RDONLY);
    if (imageFD < 0)
    {
        fprintf(stderr, "Error when opening file.\n");
        exit(1);
    }

    inodeSize = supBlock.s_inode_size;
    blockSize = EXT2_MIN_BLOCK_SIZE << supBlock.s_log_block_size;

    readAndPrintSB();

    printGroupSummary();

    print_free_block_entries(groupDesc.bg_block_bitmap);
}