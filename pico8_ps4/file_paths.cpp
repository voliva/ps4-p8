#include "file_paths.h"

#if defined _MSC_VER
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

void createFolder(const char *path) {
	DIR* dir = opendir(path);

	if (dir != NULL)
	{
		closedir(dir);
		return;
	}

#if defined _MSC_VER
	_mkdir(path);
#else
	mkdir(path, 755);
#endif
}

void createFolderRec(const char* path) {
	size_t i = 0;
	char* copiedPath = (char *)malloc(strlen(path) + 1);
	if (copiedPath == 0) {
		return;
	}

	strcpy(copiedPath, path);

	while (copiedPath[i] != 0) {
		if (copiedPath[i] == '/') {
			copiedPath[i] = 0;
			createFolder(copiedPath);
			copiedPath[i] = '/';
		}
		i++;
	}
	createFolder(path);

	free(copiedPath);
}

void prepareFilePaths() {
	createFolderRec(SAVE_FOLDER);
	createFolderRec(SAVE_STATES_FOLDER);
	createFolderRec(CARTRIDGE_FOLDER);
}
