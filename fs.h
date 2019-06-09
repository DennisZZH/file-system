//Author: Zihao Zhang
//Date: 5.29.2019

#include <cstdlib>
#include <iostream>
#include "disk.h"

using namespace std;

int make_fs(char *disk_name);

int mount_fs(char *disk_name);

int umount_fs(char *disk_name);

int fs_open(char *name);

int fs_close(int fildes);

int fs_create(char *name);

int fs_delete(char *name);

int fs_read(int fildes, void *buf, size_t nbyte);

int fs_write(int fildes, void *buf, size_t nbyte);

int fs_get_filesize(int fildes);

int fs_lseek(int fildes, off_t offset);

int fs_truncate(int fildes, off_t length);


typedef struct{
    char* file_name;
    unsigned int file_size;
    int file_FAT_entry;
    int file_reference_count;
}Directory_Node;


typedef struct{
    unsigned int is_fs_initialized;
    unsigned int file_count;
    unsigned int super_block_address;
    unsigned int directory_list_address;
    unsigned int FAT_address;
    unsigned int data_block_address;

}Super_Block;


typedef struct{
    int file_id;
    int file_offset;
    char* file_name;
}File_Descriptor;
