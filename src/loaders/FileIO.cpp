#include "FileIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <direct.h>

#define GetCurrentDir _getcwd

static char g_ApplicationDirectory[FILENAME_MAX];
static bool g_WasInitialized = false;

void InitializeFileSystem()
{
	if (g_WasInitialized)
	{
		return;
	}
	g_WasInitialized = true;

	const bool ret = GetCurrentDir(g_ApplicationDirectory, sizeof(g_ApplicationDirectory));
	assert(ret);
	if (ret)
	{
		printf("ApplicationDirectory: %s\n", g_ApplicationDirectory);
	}
	else
	{
		printf("ERROR: Unable to get current working directory!\n");
	}
}

void RelativePathToFullPath(const char* relativePathName, char* fullPath)
{
	InitializeFileSystem();

	sprintf(fullPath, "%s/%s", g_ApplicationDirectory, relativePathName);
}

/**
* Open the file and read
*/
bool GetFileData(const char* fileNameLocal, unsigned char** data, unsigned int& size)
{
	InitializeFileSystem();

	char fileName[2048];
	sprintf(fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal);

	// open file for reading
	FILE* file = fopen(fileName, "rb");

	// handle any errors
	if (file == NULL)
	{
		return false;
	}

	// get file size
	fseek(file, 0, SEEK_END);
	fflush(file);
	size = ftell(file);
	fflush(file);
	rewind(file);
	fflush(file);

	// create the data buffer
	*data = (unsigned char*)malloc((size + 1) * sizeof(unsigned char));

	// handle any error
	if (*data == NULL)
	{
		printf("ERROR: Failed to allocate memory!     %s\n", fileName);
		fclose(file);
		return false;
	}

	// zero out the memory
	memset(*data, 0, (size + 1) * sizeof(unsigned char));

	// read
	unsigned int bytesRead = (unsigned int)fread(*data, sizeof(unsigned char), size, file);
	fflush(file);

	assert(bytesRead == size);

	// handle any errors
	if (bytesRead != size)
	{
		printf("ERROR: Reading file gose wrong!    %s\n", fileName);
		fclose(file);
		if (*data != NULL)
		{
			free(*data);
		}
		return false;
	}

	fclose(file);
	printf("Successfully read file: %s !\n", fileName);
	return true;
}

bool SaveFileData(const char* fileNameLocal, const void* data, unsigned int size)
{
	InitializeFileSystem();

	char fileName[2048];
	sprintf(fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal);

	// open file for writing
	FILE* file = fopen(fileName, "wb");

	// handle any error
	if (file == NULL)
	{
		printf("ERROR: Failed to open file for writing!	%s\n", fileName);
		fclose(file);
		return false;
	}

	unsigned int bytesWritten = static_cast<unsigned int>(fwrite(data, 1, size, file));
	assert(bytesWritten == size);

	if (bytesWritten != size)
	{
		printf("ERROR:writing file gose wrong!	%s\n", fileName);
		fclose(file);
		return false;
	}

	fclose(file);
	printf("Successfully write to file: %s !\n", fileName);
	return true;
}