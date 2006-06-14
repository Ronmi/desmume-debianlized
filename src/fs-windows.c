#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

const char FS_SEPARATOR = '\\';

void * FsReadFirst(const char * p, FsEntry * entry) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	HANDLE * ret;
	char path[1024];

	sprintf(path, "%s\\*", p);

	hFind = FindFirstFile(path, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return NULL;

	strcpy(entry->cFileName, FindFileData.cFileName);
	strcpy(entry->cAlternateFileName, FindFileData.cAlternateFileName);
	entry->flags = 0;
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		entry->flags = FS_IS_DIR;
	}

	ret = (void**)malloc(sizeof(HANDLE));
	*ret = hFind;
	return ret;
}

int FsReadNext(void * search, FsEntry * entry) {
	WIN32_FIND_DATA FindFileData;
	HANDLE * h = (HANDLE *) search;
	int ret;

	ret = FindNextFile(*h, &FindFileData);

	strcpy(entry->cFileName, FindFileData.cFileName);
	strcpy(entry->cAlternateFileName, FindFileData.cAlternateFileName);
	entry->flags = 0;
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		entry->flags = FS_IS_DIR;
	}

	return ret;
}

void FsClose(void * search) {
	FindClose(*((HANDLE *) search));
}

int FsError(void) {
	if (GetLastError() == ERROR_NO_MORE_FILES)
		return FS_ERR_NO_MORE_FILES;
}
