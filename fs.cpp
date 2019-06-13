//Author: Zihao Zhang
//Date: 5.29.2019

#include "fs.h"
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;

#define FS_MAX_NUM_FILE 64
#define MAX_NUM_FILE_DESCRIPTOR 32
#define NUM_DATA_BLOCK 4096
#define DATA_BLOCK_SIZE 4096
#define NO_BLOCK_AVAILABLE -3
#define FAT_BLOCK_AVAILABLE -2
#define FAT_Block_END -1
#define MAX_LEN_FILE_NAME 15

static Super_Block current_super_block;
static list<Directory_Node> current_directory_list;
static int current_FAT[NUM_DATA_BLOCK];
static list<File_Descriptor> file_descriptor_list;
static int numOfFireDescriptor;


void delete_file_by_name(char* name){
    for(auto i = current_directory_list.begin(); i != current_directory_list.end(); i++){
        if(strcmp(i->file_name, name) == 0){
            current_directory_list.erase(i);
            return;
        }
    }
}


Directory_Node* find_file_by_name(char* name){
    for(auto i = current_directory_list.begin(); i != current_directory_list.end(); i++){
        if(strcmp(name, i->file_name) == 0){
            return &(*i);
        }
    }
    return NULL;    // If not found
}


void add_block_to_FAT(int block_entry, int block){
    int current = block_entry;
    while(current_FAT[current] != -1){
        current = current_FAT[current];
    }
    current_FAT[current] = block;
    current_FAT[block] = -1;
}


int find_available_block_in_FAT(){
    for(int i = 0; i < NUM_DATA_BLOCK; i++){
        if(current_FAT[i] == FAT_BLOCK_AVAILABLE){
            return i;
        }
    }
    return NO_BLOCK_AVAILABLE;
}


int count_block_by_entry(int entry){
    int current = entry;
    int count = 0;
    while(current_FAT[current] != -1){
        count++;
        current = current_FAT[current];
    }
    return count + 1;
}


int find_block_by_order(int block_entry, int order){
    int current = block_entry;
    for(int i = 0; i < order; i++){
        current = current_FAT[current];
    }
    return current;
}


void clean_block_start_at_order(int block_entry, int order){
    int index = find_block_by_order(block_entry, order);
    int start = current_FAT[index];
    int temp;
     while(current_FAT[start] != -1){
         temp = start;
         start = current_FAT[start];
         current_FAT[temp] = FAT_BLOCK_AVAILABLE;
     }
     current_FAT[start] = FAT_BLOCK_AVAILABLE;
}


void free_FAT_by_name(char* name){
     int entry_block = find_file_by_name(name)->file_FAT_entry;
     int temp;
     while(current_FAT[entry_block] != -1){
         temp = entry_block;
         entry_block = current_FAT[entry_block];
         current_FAT[temp] = FAT_BLOCK_AVAILABLE;
     }
     current_FAT[entry_block] = FAT_BLOCK_AVAILABLE;
 }


void delete_fildes_by_id(int id){
     for(auto i = file_descriptor_list.begin(); i != file_descriptor_list.end(); i++){
        if(id == i->file_id){
            file_descriptor_list.erase(i);
            return;
        }
    }
}


File_Descriptor* find_fildes_by_id(int id){
    for(auto i = file_descriptor_list.begin(); i != file_descriptor_list.end(); i++){
        if(id == i->file_id){
            return &(*i);
        }
    }
    return NULL;    // If not found
}


int write_num_to_buffer(unsigned int num, char* buffer, int offset){
    string s = to_string(num);
    for(int i = 0; i < s.length(); i++){
        buffer[i + offset] = s[i];
    }
    buffer[s.length() + offset] = '\n';
    return s.length() + 1;
}


int write_directory_node_to_buffer(Directory_Node node, char* buffer, int offset){
    int node_length = 0;
    
    strcat(buffer, node.file_name);
    buffer[strlen(node.file_name) + offset] = '\n';

    node_length += strlen(node.file_name) + 1;

    node_length += write_num_to_buffer(node.file_size, buffer, offset + node_length);
    node_length += write_num_to_buffer(node.file_FAT_entry, buffer, offset + node_length);

    return node_length;
}


void store_sb_to_disk(){
    char buffer[4096];
    int offset = 0;

    offset += write_num_to_buffer(current_super_block.is_fs_initialized, buffer, offset);
    offset += write_num_to_buffer(current_super_block.file_count, buffer, offset);
    offset += write_num_to_buffer(current_super_block.super_block_address, buffer, offset);
    offset += write_num_to_buffer(current_super_block.directory_list_address, buffer, offset);
    offset += write_num_to_buffer(current_super_block.FAT_address, buffer, offset);
    offset += write_num_to_buffer(current_super_block.data_block_address, buffer, offset);
   
    block_write(current_super_block.super_block_address, buffer);

}


void store_dl_to_disk(){
    char buffer[4096];
    int offset = 0;
    Directory_Node temp;

    int file_count = current_super_block.file_count;
    if(file_count > FS_MAX_NUM_FILE){
        perror("store_dl_to_disk: file number exceed max fs capacity");
        exit(0);
    }

    for(auto i = current_directory_list.begin(); i != current_directory_list.end(); i++){
        offset += write_directory_node_to_buffer(*i, buffer, offset);
    }

    block_write(current_super_block.directory_list_address, buffer);

}


void store_FAT_to_disk(){
    char buffer[4096];
    int offset = 0;
    int temp;

    for(int i = 0; i < NUM_DATA_BLOCK; i++){
        temp = current_FAT[i];
        offset += write_num_to_buffer(temp, buffer, offset);
    }

    block_write(current_super_block.FAT_address, buffer);

}


void load_sb_to_memory(){
    char buffer[4096];
    char* pch;
    vector<int> int_buffer;

    block_read(0, buffer);
    pch = strtok (buffer,"'\n'");
    while (pch != NULL){
        int_buffer.push_back(atoi(pch));
        pch = strtok (NULL, "'\n'");
    }

    current_super_block.is_fs_initialized = int_buffer[0];
    current_super_block.file_count = int_buffer[1];
    current_super_block.super_block_address = int_buffer[2];
    current_super_block.directory_list_address = int_buffer[3];
    current_super_block.FAT_address = int_buffer[4];
    current_super_block.data_block_address = int_buffer[5];

}


void load_dl_to_memory(){
    char buffer[4096];
    char* pch;
    vector<Directory_Node> node_buffer;
    Directory_Node temp;
    int cnt = 0;

    block_read(current_super_block.directory_list_address, buffer);
    pch = strtok (buffer,"'\n''\0'");
    while (pch != NULL){
        if(cnt % 3 == 0){
            temp.file_name = pch;
            cnt++;
        }else if(cnt % 3 == 1){
            temp.file_size = atoi(pch);
            cnt++;
        }else if(cnt % 3 == 2){
            temp.file_FAT_entry = atoi(pch);
            node_buffer.push_back(temp);
            cnt++;
        }
        pch = strtok (NULL, "'\n''\0'");
    }

    for(int i = 0; i < node_buffer.size(); i++){
        current_directory_list.push_back(node_buffer[i]);
    }

}


void load_FAT_to_memory(){
    char buffer[4096];
    char* pch;
    vector<int> int_buffer;

    block_read(current_super_block.FAT_address, buffer);
    pch = strtok (buffer,"'\n''\0'");
    while (pch != NULL){
        int_buffer.push_back(atoi(pch));
        pch = strtok (NULL, "'\n''\0'");
    }

    for(int i = 0; i < int_buffer.size(); i++){
        current_FAT[i] = int_buffer[i];
    }

}


void store_memory_to_disk(){
    // Serialize mega info and Write to disk block
    store_sb_to_disk();
    store_dl_to_disk();
    store_FAT_to_disk();
}


void load_disk_to_memory(){
    // Load mega info from disk to staic data structures
    load_sb_to_memory();
    load_dl_to_memory();
    load_FAT_to_memory();
}


int make_fs(char *disk_name){

    if( make_disk(disk_name) < 0 ){
        perror("make_fs: cannot make disk");
        return -1;
    }

    if( open_disk(disk_name) < 0 ){
        perror("make_fs: cannot open disk");
        return -1;
    };

    // initialize mega informations
    current_super_block.is_fs_initialized = 1;
    current_super_block.file_count = 0;
    current_super_block.super_block_address = 0;
    current_super_block.directory_list_address = 1000;
    current_super_block.FAT_address = 2000;
    current_super_block.data_block_address = 4096;

    // directory list and FAT are empty
    for(int i = 0; i < NUM_DATA_BLOCK; i++){
        current_FAT[i] = FAT_BLOCK_AVAILABLE;
    }

    // serialized super block and save to disk
    store_memory_to_disk();

    if( close_disk() < 0 ){
        perror("make_fs: cannot close disk");
        return -1;
    };

    return 0;   // If Success
}


int mount_fs(char *disk_name){

    if( open_disk(disk_name) < 0 ){
        perror("mount_fs: cannot open disk");
        return -1;
    };

    // Load mega info
    load_disk_to_memory();

    // If current disk is not initialized with a fs, return -1
    if(current_super_block.is_fs_initialized != 1){
        perror("mount_fs: fs uninitialized");
        return -1;
    }

    // Initialize file descriptor system
    numOfFireDescriptor = 0;
    for(auto i = current_directory_list.begin(); i != current_directory_list.end(); i++){
        i->file_reference_count = 0;
    }
    while(file_descriptor_list.empty() != true){
        file_descriptor_list.pop_front();
    }

    return 0;   // If success

}


int umount_fs(char *disk_name){

    store_memory_to_disk();

    // Clean up file descriptor system
    while(file_descriptor_list.empty() != true){
        find_file_by_name(file_descriptor_list.front().file_name)->file_reference_count--;
        file_descriptor_list.pop_front();
        numOfFireDescriptor--;
    }

    if( close_disk() < 0 ){
        perror("umount_fs: cannot close disk");
        return -1;
    };

    return 0;   // If Success

}


int fs_create(char *name){

    if(strlen(name) > MAX_LEN_FILE_NAME){
        perror("fs_create: file name exceed maximum characters");
        return -1;
    }

    if(find_file_by_name(name) != NULL){
        perror("fs_create: file with this name already existed");
        return -1;
    }

    if(current_super_block.file_count == FS_MAX_NUM_FILE){
        perror("fs_create: total file number exceeding maximum");
        return -1;
    }

    Directory_Node new_dn;
    new_dn.file_name = name;
    new_dn.file_size = 0;
    new_dn.file_FAT_entry = find_available_block_in_FAT();
    new_dn.file_reference_count = 0;

    current_FAT[new_dn.file_FAT_entry] = FAT_Block_END;
    current_directory_list.push_back(new_dn);
    current_super_block.file_count++;

    return 0;   // If success
}


int fs_delete(char *name){

    if(find_file_by_name(name) == NULL){
        perror("fs_delete: file with this name can not be found");
        return -1;
    }

    if(find_file_by_name(name)->file_reference_count > 0){
        perror("fs_delete: file descriptor(s) open, can not delete file");
        return -1;
    }

    free_FAT_by_name(name);
    delete_file_by_name(name);
    current_super_block.file_count--;

    return 0;   // If success
} 


int fs_open(char *name){

    if(find_file_by_name(name) == NULL){
        perror("fs_open: file with this name can not be found");
        return -1;
    }

    if(file_descriptor_list.size() == MAX_NUM_FILE_DESCRIPTOR){
        perror("fs_open: file descriptor number exceeding maximum");
        return -1;
    }

    File_Descriptor new_fd;
    new_fd.file_id = numOfFireDescriptor;
    new_fd.file_name = name;
    new_fd.file_offset = 0;

    find_file_by_name(name)->file_reference_count++;

    file_descriptor_list.push_back(new_fd);
    numOfFireDescriptor++;

    return new_fd.file_id;

}


int fs_close(int fildes){

    if(find_fildes_by_id(fildes) == NULL){
        perror("fs_close: file descriptor with this id can not be found");
        return -1;
    }

    find_file_by_name(find_fildes_by_id(fildes)->file_name)->file_reference_count--;
    delete_fildes_by_id(fildes);

    return 0;   // If success

}


int fs_read(int fildes, void *buf, size_t nbyte){

    File_Descriptor* curr_fd = find_fildes_by_id(fildes);

    if(curr_fd == NULL){
        perror("fs_read: file descriptor with this id can not be found");
        return -1;
    }

    int bytes_read = 0;
    char* name = curr_fd->file_name;
    int offset = curr_fd->file_offset;
    Directory_Node* curr_dn = find_file_by_name(name);
    int block_entry = curr_dn->file_FAT_entry;
    int file_size = curr_dn->file_size;

    for(int cnt = 0, idx = offset; cnt < nbyte; ++cnt, ++idx){
        unsigned int bn, boff, bid;
        char buffer[4096];

        if(idx >= file_size){
            break;
        }

        bn = idx / DATA_BLOCK_SIZE;
        boff = idx % DATA_BLOCK_SIZE;

        bid = find_block_by_order(block_entry, bn);
        block_read(bid + 4096, buffer);

        char* dst = ((char *) buf) + cnt;
        *dst = buffer[boff];

        curr_fd->file_offset++;
        bytes_read++;
    }
    return bytes_read;
}


int fs_write(int fildes, void *buf, size_t nbyte){
    cout<<111111<<endl;
    File_Descriptor* curr_fd = find_fildes_by_id(fildes);
    cout<<222222<<endl;
    if(curr_fd == NULL){
        perror("fs_write: file descriptor with this id can not be found");
        return -1;
    }

    int curr = 0;
    char* name = curr_fd->file_name;
    int offset = curr_fd->file_offset;
    cout<<"name = "<<name<<endl;
    Directory_Node* curr_dn = find_file_by_name(name);
    int block_entry = curr_dn->file_FAT_entry;
    int file_size = curr_dn->file_size;
    int block_count = count_block_by_entry(block_entry);
    char tmp[BLOCK_SIZE];
    int bytes = nbyte;
    cout<<333333<<endl;
    cout<<"file size"<<file_size<<endl;
    cout<<"block entry"<<block_entry<<endl;
    cout<<"offset = "<<offset<<endl;
    cout<<"block count = "<<block_count<<endl;
    // start reading from offset data block
    for(int i = offset / DATA_BLOCK_SIZE; i < block_count; i++){
        cout<<444444<<endl;
        memset(tmp, 0, DATA_BLOCK_SIZE);
        block_read(i + 4096, tmp);
        int read = (BLOCK_SIZE - offset < nbyte) ? BLOCK_SIZE - offset : nbyte; // bytes read
        memcpy(tmp + offset, (char*) buf + curr, read);
		curr += read;
		block_write(i + 4096, tmp);
		offset = 0;
    }
    cout<<777777<<endl;
    // append extra data blocks
    int block;
    for(block = find_available_block_in_FAT(); (curr < nbyte) && (block != NO_BLOCK_AVAILABLE); block = find_available_block_in_FAT()){
        memcpy(tmp, (char*) buf + curr, (bytes < BLOCK_SIZE) ? bytes : BLOCK_SIZE);
		block_write(block + 4096, tmp);
		curr += BLOCK_SIZE;
		bytes -= BLOCK_SIZE;
		add_block_to_FAT(block_entry, block);
		memset(tmp, 0, BLOCK_SIZE);
    }
    cout<<101010<<endl;
    curr_fd->file_offset += nbyte;
    curr_dn->file_size = (curr_fd->file_offset < file_size) ? file_size : curr_fd->file_offset;
    cout<<121212<<endl;
    return (block == NO_BLOCK_AVAILABLE) && (curr < nbyte) ? curr : nbyte;

}


int fs_get_filesize(int fildes){

    File_Descriptor* curr_fd = find_fildes_by_id(fildes);

    if(curr_fd == NULL){
        perror("fs_get_filesize: file descriptor with this id can not be found");
        return -1;
    }

    return find_file_by_name(curr_fd->file_name)->file_size;
}


int fs_lseek(int fildes, off_t offset){

    File_Descriptor* curr_fd = find_fildes_by_id(fildes);

    if(curr_fd == NULL){
        perror("fs_lseek: file descriptor with this id can not be found");
        return -1;
    }

    Directory_Node* curr_dn = find_file_by_name(curr_fd->file_name);

    if(offset < 0 || offset > curr_dn->file_size){
        perror("fs_lseek: invalid offset");
        return -1;
    }

    curr_fd->file_offset = offset;

    return 0;   // If success

}


int fs_truncate(int fildes, off_t length){
    
    File_Descriptor* curr_fd = find_fildes_by_id(fildes);

    if(curr_fd == NULL){
        perror("fs_truncate: file descriptor with this id can not be found");
        return -1;
    }

    Directory_Node* curr_dn = find_file_by_name(curr_fd->file_name);

    if(length > curr_dn->file_size){
        perror("fs_truncate: length exceed file size");
        return -1;
    }

    int block_entry = curr_dn->file_FAT_entry;
    int bn = length / DATA_BLOCK_SIZE;
    int total_block = count_block_by_entry(block_entry);

    curr_dn->file_size = length;
    if(bn + 1 > total_block){
        clean_block_start_at_order(block_entry, bn + 1);
    }
    
    if(curr_fd->file_offset > length){
        curr_fd->file_offset = length;
    }

    return 0;   // If success
}