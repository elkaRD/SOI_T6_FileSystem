//
//  SOI T6
//  File system
//
//  File: FS.c
//  Copyright (C) Robert Dudzinski 2019
//

#include "FS.h"

const int VERSION = 2;

#define ORG_SIZE_FILENAME      256
#define ORG_SIZE_BLOCK         1024 * 4
#define ORG_LIMIT_FILES        512
#define ORG_LIMIT_BLOCKS       1024 * 8

int SIZE_FILENAME           = ORG_SIZE_FILENAME;
int SIZE_BLOCK              = ORG_SIZE_BLOCK;
int LIMIT_FILES             = ORG_LIMIT_FILES;
int LIMIT_BLOCKS            = ORG_LIMIT_BLOCKS;

struct Header
{
    int version;
    int usedFiles;
    int usedBlocks;
    
    int usedMemory;
    
    int blockSize;
    int blocksLimit;
    int filesLimit;
    int nameSize;
};

struct DiskHandler
{
    FILE *file;
    struct Header header;
    int status;
};

struct Node
{
    int isUsed;
    int nextNode;
};

struct Descriptor
{
    char name[ORG_SIZE_FILENAME];
    int firstNode;
    int isUsed;
    int fileSize;
    time_t timeAdded;
};

struct Header GetHeader(FILE *disk)
{
    struct Header header;
    fseek(disk, 0, SEEK_SET);
    fread(&header, sizeof(struct Header), 1, disk);
    return header;
}

void SetHeader(FILE *disk, struct Header header)
{
    fseek(disk, 0, SEEK_SET);
    fwrite(&header, sizeof(struct Header), 1, disk);
}

struct DiskHandler OpenDisk(const char *diskName, const char *attr)
{
    struct DiskHandler disk;
    struct Header header;

    FILE *file = fopen(diskName, attr);
    if (!file)
    {
        printf("Cannot open the disk %s\n", diskName);
        disk.status = 1;
        return disk;
    }
    
    disk.status = 0;
    header = GetHeader(file);
    if (header.version != VERSION)
    {
        printf("Disk was configurated for a different version of file system (%d vs %d)\n", header.version, VERSION);
        fclose(file);
        disk.status = 2;
        return disk;
    }
    
    SIZE_FILENAME = header.nameSize;
    SIZE_BLOCK = header.blockSize;
    LIMIT_FILES = header.filesLimit;
    LIMIT_BLOCKS = header.blocksLimit;
    
    disk.file = file;
    disk.header = header;
    
    return disk;
}

int CreateDisk(const char *diskName, int diskSize)
{
    FILE *file;
    struct Header header;
    struct Descriptor desc;
    struct Node node;
    
    char emptyData[ORG_SIZE_BLOCK];
    int i;

    header.version = VERSION;
    header.usedFiles = 0;
    header.usedBlocks = 0;
    header.usedMemory = 0;
    header.blockSize = ORG_SIZE_BLOCK;
    header.blocksLimit = ORG_LIMIT_BLOCKS;
    header.filesLimit = ORG_LIMIT_FILES;
    header.nameSize = ORG_SIZE_FILENAME;
    
    header.blocksLimit = diskSize / header.blockSize;
    if (diskSize % header.blockSize != 0) header.blocksLimit++;
    
    file = fopen(diskName, "wb");
    
    if (!file)
    {
        printf("Cannot create a file for the disk\n");
        return 1;
    }
    
    fwrite(&header, sizeof(struct Header), 1, file);
    
    desc.isUsed = 0;

    for (i = 0; i < header.filesLimit; ++i)
        fwrite(&desc, sizeof(struct Descriptor), 1, file);
    
    node.isUsed = 0;
    for (i = 0; i < header.blocksLimit; ++i)
        fwrite(&node, sizeof(struct Node), 1, file);
    
    for (i = 0; i < header.blocksLimit; ++i)
        fwrite(emptyData, sizeof(char), header.blockSize, file);
    
    fclose(file);
    return 0;
}

void RemoveDisk(const char *diskName)
{
    remove(diskName);
}

int GetFileSize(FILE *file)
{
    int fileSize;
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return fileSize;
}

int GetDescriptorAddr(int index)
{
    return sizeof(struct Header) + sizeof(struct Descriptor) * index;
}

int GetNodeAddr(int index)
{
    return sizeof(struct Header) + sizeof(struct Descriptor) * LIMIT_FILES + sizeof(struct Node) * index;
}

int GetBlockAddr(int index)
{
    return sizeof(struct Header) + sizeof(struct Descriptor) * LIMIT_FILES + sizeof(struct Node) * LIMIT_BLOCKS + SIZE_BLOCK * index;
}

struct Descriptor GetDescriptor(FILE *disk, int index)
{
    struct Descriptor result;
    fseek(disk, GetDescriptorAddr(index), SEEK_SET);
    fread(&result, sizeof(struct Descriptor), 1, disk);
    return result;
}

struct Node GetNode(FILE *disk, int index)
{
    struct Node result;
    fseek(disk, GetNodeAddr(index), SEEK_SET);
    fread(&result, sizeof(struct Node), 1, disk);
    return result;
}

void SetDescriptor(FILE *disk, int index, struct Descriptor desc)
{
    fseek(disk, GetDescriptorAddr(index), SEEK_SET);
    fwrite(&desc, sizeof(struct Descriptor), 1, disk);
}

void SetNode(FILE *disk, int index, struct Node node)
{
    fseek(disk, GetNodeAddr(index), SEEK_SET);
    fwrite(&node, sizeof(struct Node), 1, disk);
}

int NextFreeBlock(FILE *disk, int cur)
{
    struct Node node;
    int i = cur+1;
    
    fseek(disk, GetNodeAddr(i), SEEK_SET);
    for (; i < LIMIT_BLOCKS; ++i)
    {
        fread(&node, sizeof(struct Node), 1, disk);
        if (!node.isUsed) break;
    }
    return i;
}

int InsertFile(const char *diskName, const char *path, const char *newName)
{   
    FILE *file, *src;
    struct DiskHandler dh;
    struct Header header;
    struct Descriptor desc;
    struct Descriptor newDescriptor;
    
    int remainingMemory;
    int fileSize;
    int freeIndex;
    int curBlock;
    int i;
    int copiedBytes = 0;
    
    char data[ORG_SIZE_BLOCK];
    
    dh = OpenDisk(diskName, "r+b");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    remainingMemory = LIMIT_BLOCKS - header.usedBlocks;
    remainingMemory *= SIZE_BLOCK;
    
    src = fopen(path, "rb");
    
    if (!src)
    {
        printf("Could not open the file %s\n", path);
        fclose(file);
        return 2;
    }
    
    fileSize = GetFileSize(src);
    
    if (fileSize > remainingMemory)
    {
        printf("No enough space for the file %s\n", path);
        fclose(file);
        fclose(src);
        return 3;
    }
    
    freeIndex = 0;

    fseek(file, GetDescriptorAddr(0), SEEK_SET);
    
    for (i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(struct Descriptor), 1, file);
        
        if (desc.isUsed && strcmp(desc.name, newName) == 0)
        {
            printf("File %s already exists in the disc %s\n", newName, diskName);
            fclose(file);
            fclose(src);
            return 4;
        }
        
        if (!desc.isUsed) freeIndex = i;
    }
    
    curBlock = NextFreeBlock(file, -1);
    
    newDescriptor.isUsed = 1;
    newDescriptor.fileSize = fileSize;
    newDescriptor.firstNode = curBlock;
    time(&newDescriptor.timeAdded);
    strcpy(newDescriptor.name, newName);
    
    SetDescriptor(file, freeIndex, newDescriptor);
    
    while (copiedBytes < fileSize)
    {
        int toRead;
        int got;
        int prev;
        
        struct Node node = GetNode(file, curBlock);
        node.isUsed = 1;
        node.nextNode = -1;
        
        toRead = fileSize - copiedBytes;
        if (toRead > SIZE_BLOCK) toRead = SIZE_BLOCK;
        got = fread(data, sizeof(char), toRead, src);
        
        fseek(file, GetBlockAddr(curBlock), SEEK_SET);
        fwrite(data, sizeof(char), got, file);
        copiedBytes += got;
        
        prev = curBlock;
        curBlock = NextFreeBlock(file, curBlock);
        if (copiedBytes < fileSize) node.nextNode = curBlock;
        
        SetNode(file, prev, node);
        header.usedBlocks++;
    }
    
    header.usedMemory += fileSize;
    header.usedFiles++;
    SetHeader(file, header);
    
    fclose(file);
    fclose(src);
    return 0;
}

int DisplayMap(const char *diskName)
{
    FILE *file;
    struct Header header;
    struct Node node;
    
    int isUsed;
    int begPointer;
    int begIndex;
    int pointer;
    int i;

    struct DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    printf("     Used memory in the disk %s\n\n", diskName);
    
    printf("%9d - %9lu     %9luB: FS header\n", 0, sizeof(struct Header)-1, sizeof(struct Header));
    printf("%9lu - %9d     %9luB: %d File descriptors\n", sizeof(struct Header), GetNodeAddr(0)-1, GetNodeAddr(0) - sizeof(struct Header), LIMIT_FILES);
    printf("%9d - %9d     %9dB: %d Nodes\n", GetNodeAddr(0), GetBlockAddr(0)-1, GetBlockAddr(0)-GetNodeAddr(0), LIMIT_BLOCKS);
    printf("%9d - %9d     %9dB: %d Blocks\n", GetBlockAddr(0), SIZE_BLOCK * (LIMIT_BLOCKS-1)-1, SIZE_BLOCK * (LIMIT_BLOCKS-1) - GetBlockAddr(0), LIMIT_BLOCKS);
    printf("\n\nBLOCKS MEMORY MAP:\n\n");
    
    fseek(file, GetNodeAddr(0), SEEK_SET);
    
    fread(&node, sizeof(struct Node), 1, file);
    isUsed = node.isUsed;
    begPointer = GetBlockAddr(0);
    begIndex = 0;
    pointer = GetBlockAddr(0);
    
    for (i = 1; i < LIMIT_BLOCKS; ++i)
    {
        fread(&node, sizeof(struct Node), 1, file);
        pointer += SIZE_BLOCK;
        
        if (isUsed != node.isUsed)
        {
            if (isUsed) printf("%7d - %7d     %9d - %9d     %9dB: USED\n", begIndex, i-1, begPointer, pointer-1, pointer-begPointer);
            else        printf("%7d - %7d     %9d - %9d     %9dB: FREE\n", begIndex, i-1, begPointer, pointer-1, pointer-begPointer);
            
            begPointer = pointer;
            begIndex = i;
            isUsed = node.isUsed;
        }
    }
    
    if (node.isUsed) printf("%7d - %7d     %9d - %9d     %9dB: USED\n", begIndex, LIMIT_BLOCKS-1, begPointer, pointer-1, pointer-begPointer);
    else             printf("%7d - %7d     %9d - %9d     %9dB: FREE\n", begIndex, LIMIT_BLOCKS-1, begPointer, pointer-1, pointer-begPointer);
    
    fclose(file);
    return 0;
}

int DisplayFiles(const char *diskName)
{
    FILE *file;
    struct Header header;
    struct Descriptor desc;
    
    int counter = 0;
    int i;

    struct DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    fseek(file, GetDescriptorAddr(0), SEEK_SET);
    
    for (i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(struct Descriptor), 1, file);
        if (desc.isUsed == 1)
        {
            char strDate[30];
            struct tm * timeinfo = localtime (&desc.timeAdded);
            strcpy(strDate, asctime(timeinfo));
            strDate[strlen(strDate)-1] = '\0';
            
            printf(" %3d %9dB  %30s - %s\n", ++counter, desc.fileSize, strDate, desc.name);
        }
    }
    
    printf("%d files in total\n", header.usedFiles);
    
    fclose(file);
    return 0;
}

int ExportFile(const char *diskName, const char *fileToExport, const char *newName)
{
    FILE *file, *dst;
    struct Header header;
    struct Descriptor desc;
    struct Node node;
    
    int fileIndex = -1;
    int i;
    int curBlock;
    int copiedBytes = 0;
    
    char data[ORG_SIZE_BLOCK];
    
    struct DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    for (i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(struct Descriptor), 1, file);
        if (desc.isUsed && strcmp(desc.name, fileToExport) == 0)
        {
            fileIndex = i;
            break;
        }
    }
    
    if (fileIndex < 0)
    {
        printf("Could not find file %s\n", fileToExport);
        fclose(file);
        return 3;
    }
    
    dst = fopen(newName, "wb");
    if (!dst)
    {
        printf("Cannot create destination file %s\n", newName);
        fclose(file);
        return 4;
    }
    
    curBlock = desc.firstNode;
    node = GetNode(file, curBlock);
    copiedBytes = 0;
    
    printf("DEBUG file size: %d\n", desc.fileSize);
    
    while (copiedBytes < desc.fileSize)
    {
        int got;
        int toRead = desc.fileSize - copiedBytes;
        if (toRead > SIZE_BLOCK) toRead = SIZE_BLOCK;
        
        fseek(file, GetBlockAddr(curBlock), SEEK_SET);
        got = fread(data, sizeof(char), toRead, file);
        fwrite(data, sizeof(char), got, dst);
        copiedBytes += got;
        
        curBlock = node.nextNode;
        node = GetNode(file, curBlock);
    }
    
    fclose(dst);
    fclose(file);
    return 0;
}

int DeleteFile(const char *diskName, const char *fileName)
{
    FILE *file;
    struct Header header;
    struct Descriptor desc;
    struct Node curNode;
    
    int nodeIndex = -1;
    int i;

    struct DiskHandler dh = OpenDisk(diskName, "r+b");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    for (i = 0; i < LIMIT_FILES; ++i)
    {
        desc = GetDescriptor(file, i);
        if (desc.isUsed && strcmp(desc.name, fileName) == 0)
        {
            desc.isUsed = 0;
            SetDescriptor(file, i, desc);
            nodeIndex = desc.firstNode;
            break;
        }
    }
    
    if (nodeIndex < 0)
    {
        printf("File %s does not exist in the disk %s\n", fileName, diskName);
        fclose(file);
        return 3;
    }
    
    curNode = GetNode(file, nodeIndex);
    
    do
    {
        curNode.isUsed = 0;
        SetNode(file, nodeIndex, curNode);
        
        nodeIndex = curNode.nextNode;
        curNode = GetNode(file, nodeIndex);
        
        header.usedBlocks--;
    } while (nodeIndex >= 0);
    
    header.usedFiles--;
    header.usedMemory -= desc.fileSize;
    SetHeader(file, header);
    
    fclose(file);
    return 0;
}

int DisplayInfo(const char *diskName)
{
    FILE *file;
    struct Header header;
    g
    int totalMemory;
    int notAvailable;
    double frag;
    
    struct DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    file = dh.file;
    header = dh.header;
    
    totalMemory = header.blocksLimit * header.blockSize;
    notAvailable = header.usedBlocks * header.blockSize;
    frag = 100.0 - (double)header.usedMemory / (double)notAvailable * 100.0;
    
    printf("\n\n      Information about disk %s\n\n", diskName);
    printf(" Total memory:          %9dB\n", totalMemory);
    printf(" Available memory:      %9dB\n", totalMemory - notAvailable);
    printf(" Used memory:           %9dB\n", header.usedMemory);
    printf(" Not available:         %9dB\n", notAvailable);
    printf(" Int fragmentation:     %.3f%%\n", frag);
    
    printf("\n");
    printf(" Files:                 %d\n", header.usedFiles);
    printf(" Max number of files:   %d\n", header.filesLimit);
    
    printf("\n");
    printf(" Version:               %d\n", header.version);
    printf(" Block size:            %dB\n", header.blockSize);
    printf(" Blocks:                %d\n", header.blocksLimit);
    printf(" Used blocks:           %d\n", header.usedBlocks);
    
    printf("\n");
    
    fclose(file);
    return 0;
}
