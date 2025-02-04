#include <iostream>
#include <fstream>
//#include "stdafx.h"

#include "file_open_close.h"


int file_open_write(FILE*& file, const char* path)
{
	// Открываем файл для складывания данных в режиме передачи
	file = fopen(path, "wb");
	if (file == NULL)
	{
		fprintf(stderr, "Failed to open file: %s\n", path);
	}
	else
	{
		// Change file buffer to have bigger one to store or read data on/to HDD
		int result = setvbuf(file, NULL, _IOFBF, FD_BUFFER_SIZE);
		if (result != 0)
		{
			fprintf(stderr, "setvbuf() for file %s failed: %d\n", path, result);
			return 1;
		}
	}
	return 0;
}
int file_open_read(FILE*& file, const char* path)
{
	// Открываем файл для складывания данных в режиме передачи
	file = fopen(path, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Failed to open file: %s\n", path);
		exit(-9);
	}
	else
	{
		// Change file buffer to have bigger one to store or read data on/to HDD
		int result = setvbuf(file, NULL, _IOFBF, FD_BUFFER_SIZE);
		if (result != 0)
		{
			fprintf(stderr, "setvbuf() for file %s failed: %d\n", path, result);
			return 1;
		}
	}
	return 0;
}

void file_close(FILE*& file)
{
	if (file != NULL)
	{
		fflush(file);
		fclose(file);
		file = NULL;
		fprintf(stderr, "fclose() done\n");
	}
}

void file_read_hex(const char *file_name, int **words, size_t *readed_size)
{
    FILE *file;
    size_t capacity = 2048;
    *readed_size = 0;

    file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "Error, can't open file(%s)", file_name);
    }
    *words = (int*)malloc(capacity * sizeof(int));
    unsigned address, word;

    while (fscanf(file, "@%4X %8X\n", &address, &word) == 2) {
        (*readed_size)++;
        // printf("readed_size(%u), read @%04X %08X\n", *readed_size, address, word);
        if ((*readed_size) > capacity) {
            int *tmp = (int*)malloc((capacity * sizeof(int)) << 2);
            memcpy(tmp, *words, capacity * sizeof(int));
            free(words);
            *words = tmp;
            capacity = capacity << 2;
        }
        (*words)[(*readed_size) - 1] = word;
    }
    fclose(file);
}

template <typename T> 
void file_binary_write(const char* filename, const T* buff, size_t *buff_size) { 
	std::ofstream file(filename, std::ios::binary); 

	if (!file.is_open()) { 
		fprintf(stderr, "[%s] Error open file\n", __FUNCTION__);  
		return; 
	}

	buff = (T*) malloc(*buff_size * sizeof(T));

	file.write(reinterpret_cast<const char*>(buff), *buff_size * sizeof(T));

	file.close(); 
}


/**
 * TODO: docs
 */
template <typename T> 
void file_binary_read(const char* filename, T* buff, size_t *buff_size) { 
	std::ifstream file(filename, std::ios::binary); 

	if (!file.is_open()) { 
		fprintf(stderr, "[%s] Error open file\n", __FUNCTION__); 
		return; 
	}

	buff = (T*) malloc(*buff_size * sizeof(T));

	file.read(reinterpret_cast<char *>(buff), *buff_size * sizeof(T)); 

	file.close(); 
}