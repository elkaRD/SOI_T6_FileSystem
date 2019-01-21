#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int CreateDisk(const char *diskName, int diskSize);
void RemoveDisk(const char *diskName);
int InsertFile(const char *diskName, const char *path, const char *newName);
int DisplayMap(const char *diskName);
int DisplayFiles(const char *diskName);
int ExportFile(const char *diskName, const char *fileToExport, const char *newName);
int DeleteFile(const char *diskName, const char *fileName);
int DisplayInfo(const char *diskName);

#endif
