//
//  main.cpp
//  
//
//  Created by Robert Dudziński on 18/01/2019.
//

#include <stdio.h>
#include <string.h>

#define SIZE_HEADER         sizeof(Header)
#define SIZE_FILE_DESC      sizeof(Descriptor)
#define SIZE_FILENAME       256
#define SIZE_NODE           sizeof(Node)
#define SIZE_BLOCK          1024 * 32

#define FILES_LIMIT         256
#define BLOCKS_LIMIT        1024

#define VERSION             1

struct Header
{
    int filesCounter;
    int blocksCounter;
};

struct Node
{
    int isUsed;
    int nextNode;
};

struct Descriptor
{
    char name[SIZE_FILENAME];
    int firstNode;
    int isUsed;
    int fileSize;
};

void CreateDisk(const char *diskName)
{
    FILE *file = fopen(diskName, "wb");
    
    int versionCode = VERSION;
    fwrite(&versionCode, sizeof(int), 1, file);
    
    Header header;
    header.filesCounter = 0;
    header.blocksCounter = 0;
    fwrite(&header, sizeof(Header), 1, file);
    
    Descriptor desc;
    desc.isUsed = 0;
    for (int i = 0; i < FILES_LIMIT; ++i)
        fwrite(&desc, sizeof(Descriptor), 1, file);
    
    Node node;
    node.isUsed = 0;
    for (int i = 0; i < BLOCKS_LIMIT; ++i)
        fwrite(&node, sizeof(Node), 1, file);
    
    char emptyData[SIZE_BLOCK];
    for (int i = 0; i < BLOCKS_LIMIT; ++i)
        fwrite(emptyData, sizeof(char), SIZE_BLOCK, file);
    
    fclose(file);
}

void RemoveDisk(const char *diskName)
{
    remove(diskName);
}

int GetFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return fileSize;
}

int GetDescriptorAddr(int index)
{
    return sizeof(int) + sizeof(Header) + sizeof(Descriptor) * index;
}

int GetNodeAddr(int index)
{
    return sizeof(int) + sizeof(Header) + sizeof(Descriptor) * FILES_LIMIT + sizeof(Node) * index;
}

int GetBlockAddr(int index)
{
    return sizeof(int) + sizeof(Header) + sizeof(Descriptor) * FILES_LIMIT + sizeof(Node) * BLOCKS_LIMIT + SIZE_BLOCK * index;
}

Descriptor GetDescriptor(FILE *disk, int index)
{
    Descriptor result;
    fseek(disk, GetDescriptorAddr(index), SEEK_SET);
    fread(&result, sizeof(Descriptor), 1, disk);
    return result;
}

Node GetNode(FILE *disk, int index)
{
    Node result;
    fseek(disk, GetNodeAddr(index), SEEK_SET);
    fread(&result, sizeof(Node), 1, disk);
    return result;
}

void SetDescriptor(FILE *disk, int index, Descriptor desc)
{
    fseek(disk, GetDescriptorAddr(index), SEEK_SET);
    fwrite(&desc, sizeof(Descriptor), 1, disk);
}

void SetNode(FILE *disk, int index, Node node)
{
    fseek(disk, GetNodeAddr(index), SEEK_SET);
    fwrite(&node, sizeof(Node), 1, disk);
}

int NextFreeBlock(FILE *disk, int cur)
{
    Node node;
    int i = cur+1;
    fseek(disk, GetNodeAddr(i), SEEK_SET);
    for (; i < BLOCKS_LIMIT; ++i)
    {
        fread(&node, sizeof(Node), 1, disk);
        if (!node.isUsed) break;
    }
    return i;
}

int InsertFile(const char *diskName, const char *path, const char *newName)
{
    printf("Started inserting\n");
    
    FILE *file = fopen(diskName, "r+b");
    
    int versionCode;
    fread(&versionCode, sizeof(int), 1, file);
    if (versionCode != VERSION)
    {
        printf("Disk was configurated for a different version of file system\n");
        fclose(file);
        return 1;
    }
    
    Header header;
    fread(&header, sizeof(Header), 1, file);
    
    int remainingMemory = BLOCKS_LIMIT - header.blocksCounter;
    remainingMemory *= SIZE_BLOCK;
    
    FILE *src = fopen(path, "rb");
    
    if (!src)
    {
        printf("Could not open the file %s\n", path);
        fclose(file);
        return 2;
    }
    
    int fileSize = GetFileSize(src);
    
    if (fileSize > remainingMemory)
    {
        printf("No enough space for the file %s\n", path);
        fclose(file);
        fclose(src);
        return 3;
    }
    
    int freeIndex = 0;
    
    Descriptor desc;
    fseek(file, GetDescriptorAddr(0), SEEK_SET);
    for (int i = 0; i < FILES_LIMIT; ++i)
    {
        fread(&desc, sizeof(Descriptor), 1, file);
        if (desc.isUsed && strcmp(desc.name, newName) != 0)
        {
            printf("File %s already exists in the disc %s\n", newName, diskName);
            fclose(file);
            fclose(src);
            return 4;
        }
        
        if (!desc.isUsed) freeIndex = i;
    }
    
    int copiedBytes = 0;
    char data[SIZE_BLOCK];
    
    int curBlock = NextFreeBlock(file, -1);
    
    Descriptor newDescriptor;
    newDescriptor.fileSize = fileSize;
    newDescriptor.firstNode = curBlock;
    
//    for (int i = 0; i < strlen(newName); ++i)
//    {
//        //TODO: repair
//        //newDescriptor.name[i] = newName[i];
//    }
    
    newDescriptor.name[0] = 'a';
    newDescriptor.name[1] = 'b';
    newDescriptor.name[2] = NULL;
    
    SetDescriptor(file, freeIndex, newDescriptor);
    
    while (copiedBytes < fileSize)
    {
        printf("CURRENT BLOCK %d  %d\n", curBlock, freeIndex);
        
        Node node = GetNode(file, curBlock);
        node.isUsed = 1;
        node.nextNode = -1;
        
        int toRead = fileSize - copiedBytes;
        if (toRead > SIZE_BLOCK) toRead = SIZE_BLOCK;
        int got = fread(data, sizeof(char), toRead, src);
        
        fseek(file, GetBlockAddr(curBlock), SEEK_SET);
        int test = fwrite(data, sizeof(char), got, file);
        printf("DEBUG: %d  %d\n", got, test);
        copiedBytes += got;
        
        int prev = curBlock;
        curBlock = NextFreeBlock(file, curBlock);
        node.nextNode = curBlock;
        
        SetNode(file, prev, node);
    }
    
    fclose(file);
    fclose(src);
    return 0;
}

void RunDiskLoop()
{
    
}

void RemoveUpperCase(char **str)
{
    for (int i = 0; i < strlen(*str); ++i)
    {
        if ((*str)[i] >= 'A' && (*str)[i] <= 'Z')
        {
            (*str)[i] += 32;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc <= 1) return 0;
    
    char *mode = argv[1];
    
    RemoveUpperCase(&mode);
    
    if (strcmp(mode, "new") == 0)
    {
        printf("new\n");
        if (argc <= 2) return 0;
        
        char *diskName = argv[2];
        CreateDisk(diskName);
        printf("Created disk %s\n", diskName);
    }
    else if (strcmp(mode, "remove") == 0)
    {
        printf("remove\n");
        if (argc <= 2) return 0;
        
        char *diskName = argv[2];
        printf("Do you really want to delete disk %s? [Y/n]\n", diskName);
        char respond;
        scanf("%c", &respond);
        if (respond == 'Y')
        {
            RemoveDisk(diskName);
            printf("Removed disk %s\n", diskName);
        }
        else
        {
            printf("Aborted\n");
        }
    }
    else if (strcmp(mode, "insert") == 0)
    {
        printf("insert %d\n", argc);
        if (argc <= 3) return 0;
        
        char *diskName = argv[2];
        char *fileToInsert = argv[3];
        
        if (argc <= 4)
        {
            char *newName = argv[4];
            InsertFile(diskName, fileToInsert, newName);
        }
        else
        {
            InsertFile(diskName, fileToInsert, fileToInsert);
        }
    }
    
    /*while (1)
    {
        printf("     SOI T6 - File System by Robert Dudzinski\n\n");
        printf(" [1] Create new disk\n");
        printf(" [2] Load disk\n");
        printf("\n [0] Exit\n");
        
        int menu;
        scanf("%d", &menu);
        
        if (menu == 0)
        {
            break;
        }
        else if (menu == 1)
        {
            printf("Enter name of the new disk: ");
            char diskName[256];
            scanf("%s", diskName);
            CreateDisk(diskName);
            printf("Created disk %s\n", diskName);
        }
        else if (menu == 2)
        {
            printf("Disk name: ");
            char diskName[256];
            scanf("%s", &diskName);
            RunDiskLoop();
        }
    }*/
    
    return 0;
}
