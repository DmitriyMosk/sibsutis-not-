#pragma once
#include <stdio.h>
#define FD_BUFFER_SIZE (8*1024)


int file_open_write(FILE*& file, const char* path);

int file_open_read(FILE*& file, const char* path);

void file_close(FILE*& file);

void file_read_hex(const char *file_name, int **words, size_t *readed_size);

template <typename T> 
void file_binary_write(const char* filename, const T* buff, size_t *buff_size);

template <typename T> 
void file_binary_read(const char* filename, T* buff, size_t *buff_size);