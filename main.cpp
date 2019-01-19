//
//  main.cpp
//  
//
//  Created by Robert Dudziński on 18/01/2019.
//

#include <stdio.h>
#include <string.h>

const int VERSION = 2;

const int ORG_SIZE_FILENAME     = 256;
const int ORG_SIZE_BLOCK        = 1024 * 32;
const int ORG_LIMIT_FILES       = 256;
const int ORG_LIMIT_BLOCKS      = 1024;

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
    Header header;
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
};

Header GetHeader(FILE *disk)
{
    Header header;
    fseek(disk, 0, SEEK_SET);
    fread(&header, sizeof(Header), 1, disk);
    return header;
}

void SetHeader(FILE *disk, Header header)
{
    fseek(disk, 0, SEEK_SET);
    fwrite(&header, sizeof(Header), 1, disk);
}

DiskHandler OpenDisk(const char *diskName, const char *attr)
{
    DiskHandler disk;
    disk.status = 0;
    
    FILE *file = fopen(diskName, attr);
    if (!file)
    {
        printf("Cannot open the disk %s\n", diskName);
        disk.status = 1;
        return disk;
    }
    
    Header header = GetHeader(file);
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
    Header header;
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
    
    FILE *file = fopen(diskName, "wb");
    
    if (!file)
    {
        printf("Cannot create a file for the disk\n");
        return 1;
    }
    
    fwrite(&header, sizeof(Header), 1, file);
    
    Descriptor desc;
    desc.isUsed = 0;
    for (int i = 0; i < header.filesLimit; ++i)
        fwrite(&desc, sizeof(Descriptor), 1, file);
    
    Node node;
    node.isUsed = 0;
    for (int i = 0; i < header.blocksLimit; ++i)
        fwrite(&node, sizeof(Node), 1, file);
    
    char emptyData[header.blockSize];
    for (int i = 0; i < header.blocksLimit; ++i)
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
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return fileSize;
}

int GetDescriptorAddr(int index)
{
    return sizeof(Header) + sizeof(Descriptor) * index;
}

int GetNodeAddr(int index)
{
    return sizeof(Header) + sizeof(Descriptor) * LIMIT_FILES + sizeof(Node) * index;
}

int GetBlockAddr(int index)
{
    return sizeof(Header) + sizeof(Descriptor) * LIMIT_FILES + sizeof(Node) * LIMIT_BLOCKS + SIZE_BLOCK * index;
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
    for (; i < LIMIT_BLOCKS; ++i)
    {
        fread(&node, sizeof(Node), 1, disk);
        if (!node.isUsed) break;
    }
    return i;
}

int InsertFile(const char *diskName, const char *path, const char *newName)
{
    printf("Started inserting\n");
    
    DiskHandler dh = OpenDisk(diskName, "r+b");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    int remainingMemory = LIMIT_BLOCKS - header.usedBlocks;
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
    for (int i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(Descriptor), 1, file);
        
        if (desc.isUsed && strcmp(desc.name, newName) == 0)
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
    newDescriptor.isUsed = 1;
    newDescriptor.fileSize = fileSize;
    newDescriptor.firstNode = curBlock;
    strcpy(newDescriptor.name, newName);
    
    SetDescriptor(file, freeIndex, newDescriptor);
    
    while (copiedBytes < fileSize)
    {
        //printf("DEBUG: %d\n", freeIndex);
        
        Node node = GetNode(file, curBlock);
        node.isUsed = 1;
        node.nextNode = -1;
        
        int toRead = fileSize - copiedBytes;
        if (toRead > SIZE_BLOCK) toRead = SIZE_BLOCK;
        int got = fread(data, sizeof(char), toRead, src);
        
        fseek(file, GetBlockAddr(curBlock), SEEK_SET);
        fwrite(data, sizeof(char), got, file);
        copiedBytes += got;
        
        int prev = curBlock;
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
    DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    printf("     Used memory in the disk %s\n\n", diskName);
    
    printf("%9d - %9lu     %9luB: FS header\n", 0, sizeof(Header)-1, sizeof(Header));
    printf("%9lu - %9d     %9luB: %d File descriptors\n", sizeof(Header), GetNodeAddr(0)-1, GetNodeAddr(0) - sizeof(Header), LIMIT_FILES);
    printf("%9d - %9d     %9dB: %d Nodes\n", GetNodeAddr(0), GetBlockAddr(0)-1, GetBlockAddr(0)-GetNodeAddr(0), LIMIT_BLOCKS);
    printf("%9d - %9d     %9dB: %d Blocks\n", GetBlockAddr(0), SIZE_BLOCK * (LIMIT_BLOCKS-1)-1, SIZE_BLOCK * (LIMIT_BLOCKS-1) - GetBlockAddr(0), LIMIT_BLOCKS);
    printf("\n\nBLOCKS MEMORY MAP:\n\n");
    //printf("%9d - %9d: Files descriptors\n", pointer, pointer+= sizeof(Descriptor) * BLOCKS_LIMIT);
    
    fseek(file, GetNodeAddr(0), SEEK_SET);
    Node node;
    fread(&node, sizeof(Node), 1, file);
    int isUsed = node.isUsed;
    int begPointer = GetBlockAddr(0);
    int begIndex = 0;
    int pointer = GetBlockAddr(0);
    
    for (int i = 1; i < LIMIT_BLOCKS; ++i)
    {
        fread(&node, sizeof(Node), 1, file);
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
    DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    Descriptor desc;
    int counter = 0;
    
    fseek(file, GetDescriptorAddr(0), SEEK_SET);
    
    for (int i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(Descriptor), 1, file);
        if (desc.isUsed == 1)
        {
            printf(" %3d %9dB - %s\n", ++counter, desc.fileSize, desc.name);
        }
    }
    
    printf("%d files in total\n", header.usedFiles);
    
    fclose(file);
    return 0;
}

int ExportFile(const char *diskName, const char *fileToExport, const char *newName)
{
    DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    Descriptor desc;
    int fileIndex = -1;
    
    for (int i = 0; i < LIMIT_FILES; ++i)
    {
        fread(&desc, sizeof(Descriptor), 1, file);
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
    
    FILE *dst = fopen(newName, "wb");
    if (!dst)
    {
        printf("Cannot create destination file %s\n", newName);
        fclose(file);
        return 4;
    }
    
    int curBlock = desc.firstNode;
    Node node = GetNode(file, curBlock);
    int copiedBytes = 0;
    
    char data[SIZE_BLOCK];
    
    printf("DEBUG file size: %d\n", desc.fileSize);
    
    while (copiedBytes < desc.fileSize)
    {
        int toRead = desc.fileSize - copiedBytes;
        if (toRead > SIZE_BLOCK) toRead = SIZE_BLOCK;
            
        fseek(file, GetBlockAddr(curBlock), SEEK_SET);
        int got = fread(data, sizeof(char), toRead, file);
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
    DiskHandler dh = OpenDisk(diskName, "r+b");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    Descriptor desc;
    int nodeIndex = -1;
    
    for (int i = 0; i < LIMIT_FILES; ++i)
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
    
    Node curNode = GetNode(file, nodeIndex);
    
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
    DiskHandler dh = OpenDisk(diskName, "rb");
    if (dh.status) return dh.status;
    
    FILE *file = dh.file;
    Header header = dh.header;
    
    int totalMemory = header.blocksLimit * header.blockSize;
    int notAvailable = header.usedBlocks * header.blockSize;
    double frag = 100.0 - (double)header.usedMemory / (double)notAvailable * 100.0;
    
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
        if (CreateDisk(diskName, 15000000))
            printf("Error creating disk\n");
        else
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
            printf("Reomved the disk %s\n", diskName);
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
            if (InsertFile(diskName, fileToInsert, newName))
                printf("Error inserting file\n");
            else
                printf("Inserted %s to the disk %s\n", newName, fileToInsert);
        }
        else
        {
            if (InsertFile(diskName, fileToInsert, fileToInsert))
                printf("Error inserting file\n");
            else
                printf("Inserted %s to the disk %s\n", fileToInsert, fileToInsert);
        }
    }
    else if (strcmp(mode, "help") == 0)
    {
        printf("\n\n\n SOI T6 File system by Robert Dudzinski\n\n");
        printf("   List of all commands:\n\n");
        printf("new (DISK_NAME) \n\t- creates a new disk with the name DISK_NAME\n\n");
        printf("remove (DISK_NAME) \n\t- deletes a new disk with the name DISK_NAME\n\n");
        printf("insert (DISK_NAME) (EXT_FILE) [INTERNAL_NAME] \n\t- copies a file EXT_FILE to the disk DISK_NAME and changes its name to INTERNAL_NAME (or name of EXT_NAME if internal name it's not provided\n\n");
        printf("memory (DISK_NAME) \n\t- displays map of memory in the disk DISK_NAME\n\n");
        printf("list (DISK_NAME) \n\t- displays list of all files\n\n");
        printf("export (DISK_NAME) (FILE_NAME) [EXPORT_NAME] \n\t- copies file FILE_NAME from disk DISK_NAME to the folder where disk exists\n\n");
        printf("delete (DISK_NAME) (FILE_NAME) \n\t- deletes file FILE_NAME from the disk DISK_NAME\n\n");
        printf("info (DISK_NAME) \n\t- displays information about given disk DISK_NAME\n\n");
        printf("\n\n\n");
    }
    else if (strcmp(mode, "memory") == 0)
    {
        printf("memory\n");
        if (argc <= 2) return 0;
        
        char *diskName = argv[2];
        if (DisplayMap(diskName))
            printf("Error display memory map\n");
    }
    else if (strcmp(mode, "list") == 0)
    {
        printf("list\n");
        if (argc <= 2) return 0;
        
        char *diskName = argv[2];
        if (DisplayFiles(diskName))
            printf("Error display list of files\n");
    }
    else if (strcmp(mode, "export") == 0)
    {
        printf("export\n");
        if (argc <= 4) return 0;
        
        char *diskName = argv[2];
        char *fileToExport = argv[3];
        if (ExportFile(diskName, fileToExport, argv[4]))
            printf("Error exporting file %s from disk %s\n", fileToExport, diskName);
        else
            printf("Exported file %s from the disk %s\n", fileToExport, diskName);
    }
    else if (strcmp(mode, "delete") == 0)
    {
        printf("delete file\n");
        if (argc <= 3) return 0;
        
        char *diskName = argv[2];
        char *fileToDelete = argv[3];
        if (DeleteFile(diskName, fileToDelete))
            printf("Error deleting file %s from the disk %s\n", fileToDelete, diskName);
        else
            printf("Deleted file %s from the disk %s\n", fileToDelete, diskName);
    }
    else if (strcmp(mode, "info") == 0)
    {
        printf("list\n");
        if (argc <= 2) return 0;
        
        char *diskName = argv[2];
        if (DisplayInfo(diskName))
            printf("Error display information about disk %s\n", diskName);
    }
    else
    {
        printf("Could not find command '%s' to execute; use 'help' to display all commands\n", mode);
    }
    
    return 0;
}
