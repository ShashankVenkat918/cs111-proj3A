#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
//#include <sys/stats.h>
#include <time.h>

#include "ext2_fs.h"

int imageFD = -1;

struct ext2_super_block supBlock;
const int supBlockOffset = 1024;

struct ext2_group_desc groupDesc;

struct ext2_inode inode_struct;

__u32 inodeSize;
__u32 blockSize;

int block_offset(int block)
{
    return block * blockSize;
}

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

    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blockCount, inodeCount, blockSize, supBlock.s_inode_size, blocksPerGroup, inodesPerGroup, firstNRInode);
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

void print_free_block_entries()
{
    char *bitmap = (char *)malloc(blockSize);
    int ret = pread(imageFD, bitmap, blockSize, block_offset(groupDesc.bg_block_bitmap));
    if (ret < 0)
    {
        fprintf(stderr, "Error in reading into free block bitmap.\n");
        exit(1);
    }
    int offset = 0;
    int index;
    __u32 block_number = 1;
    for (block_number = 1; block_number < blockSize; block_number++)
    {
        index = (block_number - 1) / 8;
        offset = (block_number - 1) % 8;
        if ((bitmap[index] & (1 << offset)) == 0)
            fprintf(stdout, "BFREE,%d\n", block_number);
    }
    //some for loop
}

char checkFileType(struct ext2_inode inode)
{
    if ((inode.i_mode & 0xF000) == 0xA000)
        return 'l';
    else if ((inode.i_mode & 0xF000) == 0x4000)
        return 'd';
    else if ((inode.i_mode & 0xF000) == 0x8000)
        return 'f';
    else
        return '?';

} //checked the values according to the documentation given in spec

char *timeFormat(__u32 inodeTime)
{
    char *formattedTime = malloc(sizeof(char) * 32);

    time_t rawTime = inodeTime;
    struct tm *info = gmtime(&rawTime);

    //fprintf(stderr, "here\n");

    strftime(formattedTime, 32, "%m/%d/%y %H:%M:%S", info);
    return formattedTime;
}

void print_directory_entires(int inode_value, int block)
{

    struct ext2_dir_entry directory_entry;

    //readin
    // __u32 directory_starting_location = block_offset();
    __u32 logical_byte_offset = 0;

    while (1024 > logical_byte_offset)
    {
        pread(imageFD, &directory_entry, sizeof(directory_entry), (inode_value * blockSize + logical_byte_offset));
        if (directory_entry.inode != 0)
        {
            fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", block, logical_byte_offset, directory_entry.inode, directory_entry.rec_len, directory_entry.name_len, directory_entry.name);
        }
        logical_byte_offset += directory_entry.rec_len;
        // fprintf(stdout, "logical_byte_offset: %d\n", logical_byte_offset);
    }
}

void print_indirect_entries(int inodeBlock, int inodeNum, int level, int begOffset)
{
    if (level == 0 || inodeBlock == 0)
        return;

    for (__u32 i = 0; i < blockSize / 4; i++)
    {
        //fprintf(stderr, "%d\n", i);
        __u32 currBlock;
        pread(imageFD, &currBlock, 4, block_offset(inodeBlock) + i * 4);
        if (currBlock != 0)
        {
            //fprintf(stderr, "Offset: %d I:%u\t", begOffset, i);
            begOffset += i;
            //fprintf(stderr, "%d\n", begOffset);
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", inodeNum, level, begOffset, inodeBlock, currBlock);
            print_indirect_entries(currBlock, inodeNum, level - 1, begOffset);
        }
    }
    //fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d,%d\n", block, 1, logical_block_offset, block_number, )
}

void printInodeTable()
{
    /* Read in the inode table, then iterate through and print the inode summaries*/

    __u32 inodeLoc = groupDesc.bg_inode_table;
    __u32 inodeTableSize = sizeof(struct ext2_inode) * supBlock.s_inodes_per_group;

    //create the inode table by making a dynamic array of ext2_inode pointers
    struct ext2_inode *inodeTable = malloc(inodeTableSize);

    int ret = pread(imageFD, inodeTable, inodeTableSize, block_offset(inodeLoc));
    if (ret < 0)
    {
        fprintf(stderr, "Error in reading into inode table.\n");
        exit(1);
    }
    char fileType;
    int inodeNum;
    __u32 i = 0;
    for (; i < supBlock.s_inodes_per_group; i++)
    {
        struct ext2_inode inode = inodeTable[i];
        if (inode.i_mode == 0 || inode.i_links_count == 0)
            continue;
        else
        {
            inodeNum = i + 1;
            fileType = checkFileType(inode);

            char *cTime = timeFormat(inode.i_ctime);
            char *mTime = timeFormat(inode.i_mtime);
            char *aTime = timeFormat(inode.i_atime);
            __u32 mode = inode.i_mode & 0xFFF;

            fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", inodeNum, fileType, mode, inode.i_uid, inode.i_gid, inode.i_links_count,
                    cTime, mTime, aTime, inode.i_size, inode.i_blocks);
        }
        int k;
        for (k = 0; k < 15; k++)
        {
            fprintf(stdout, ",%d", inode.i_block[k]);
        }
        fprintf(stdout, "\n");
        int j;
        if (fileType == 'd')
        {
            for (j = 0; j < 12; j++)
            {
                if (inode.i_block[j] != 0)
                {
                    //fprintf(stdout, "hello");
                    print_directory_entires(inode.i_block[j], inodeNum);
                }
            }
        }

        //from the nongnu ext2 documentation, the first indirect block is from 13-258, the second indirect block is from 268-(65536+268), and the third indirect block is from 65536+268 onwards
        if (inode.i_block[12] != 0)
            print_indirect_entries(inode.i_block[12], inodeNum, 1, 12);
        //indirect call or action

        if (inode.i_block[13] != 0)
            print_indirect_entries(inode.i_block[13], inodeNum, 2, 268); //indirect call or action

        if (inode.i_block[14] != 0)
            print_indirect_entries(inode.i_block[14], inodeNum, 3, 65804); //indirect call or action
    }
}

void printFreeInodeEntries()
{
    char *bitmap = (char *)malloc(blockSize);
    pread(imageFD, bitmap, blockSize, block_offset(groupDesc.bg_inode_bitmap));
    int ret = pread(imageFD, bitmap, blockSize, block_offset(groupDesc.bg_inode_bitmap));
    if (ret < 0)
    {
        fprintf(stderr, "Error in reading into free inode bitmap.\n");
        exit(1);
    }
    int offset = 0;
    int index;
    __u32 block_number = 1;
    for (block_number = 1; block_number < blockSize; block_number++)
    {
        index = (block_number - 1) / 8;
        offset = (block_number - 1) % 8;
        if ((bitmap[index] & (1 << offset)) == 0)
            fprintf(stdout, "IFREE,%d\n", block_number);
    }
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

    print_free_block_entries();

    printFreeInodeEntries();

    printInodeTable();

    return 0;
}