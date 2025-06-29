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
#include <string>

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

void moveOldStuff(
	const char *oldPath,
	const char *path
) {
	DIR *dir = opendir(oldPath);
	if (dir == NULL)
		return;

	std::string oldPathStr = oldPath;
	std::string pathStr = path;

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		std::string filename = ent->d_name;
		if (filename.find("moved_to_") != std::string::npos)
			continue;
		
		rename(
			(oldPathStr + "/" + filename).c_str(),
			(pathStr + "/" + filename).c_str()
		);
	}
	size_t k;
	while ((k = pathStr.find("/")) != std::string::npos) {
		pathStr.replace(k, 1, "_");
	}
	std::string moved_notice = oldPathStr + "/moved_to_" + pathStr;
	FILE *f = fopen(moved_notice.c_str(), "wb");
	if (f != NULL) {
		fclose(f);
	}

	closedir(dir);
}

void prepareFilePaths() {
	createFolderRec(SAVE_FOLDER);
	createFolderRec(SAVE_STATES_FOLDER);
	createFolderRec(CARTRIDGE_FOLDER);

#ifdef __SWITCH__
	moveOldStuff("/p8-saves", SAVE_FOLDER);
	moveOldStuff("/p8-savestates", SAVE_STATES_FOLDER);
	moveOldStuff("/data/p8-cartridges", CARTRIDGE_FOLDER);
#endif
}
