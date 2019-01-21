//
//  SOI T6
//  File system
//
//  File: main.c
//  Copyright (C) Robert Dudzinski 2019
//

#include "FS.h"

void RemoveUpperCase(char **str)
{
    int i = 0;
    for (; i < strlen(*str); ++i)
        if ((*str)[i] >= 'A' && (*str)[i] <= 'Z')
            (*str)[i] += 32;
}

int main(int argc, char **argv)
{
	char *mode = argv[1];
    char *diskName = argv[2];

    if (argc <= 2)
    {
        printf("Too few arguments\n");
        return 0;
    }
    
    RemoveUpperCase(&mode);
    
    if (strcmp(mode, "new") == 0)
    {
        int desiredSize = 15000000;
        
        if (argc > 3) desiredSize = atoi(argv[3]);
        
        if (CreateDisk(diskName, desiredSize))
            printf("Error creating disk\n");
        else
            printf("Created disk %s\n", diskName);
    }
    else if (strcmp(mode, "remove") == 0)
    {
		char respond;
        printf("Do you really want to delete disk %s? [Y/n]\n", diskName);
        
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
        if (argc > 3)
		{
            char *fileToInsert = argv[3];
            
            if (argc > 4)
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
		else return 0;
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
        if (DisplayMap(diskName))
            printf("Error display memory map\n");
    }
    else if (strcmp(mode, "list") == 0)
    {
        if (DisplayFiles(diskName))
            printf("Error display list of files\n");
    }
    else if (strcmp(mode, "export") == 0)
    {
        if (argc > 4)
		{
            char *diskName = argv[2];
            char *fileToExport = argv[3];
            if (ExportFile(diskName, fileToExport, argv[4]))
                printf("Error exporting file %s from disk %s\n", fileToExport, diskName);
            else
                printf("Exported file %s from the disk %s\n", fileToExport, diskName);
		}
		else return 0;
    }
    else if (strcmp(mode, "delete") == 0)
    {
        if (argc > 3)
		{
            char *diskName = argv[2];
            char *fileToDelete = argv[3];
            if (DeleteFile(diskName, fileToDelete))
                printf("Error deleting file %s from the disk %s\n", fileToDelete, diskName);
            else
                printf("Deleted file %s from the disk %s\n", fileToDelete, diskName);
        }
		else return 0;
    }
    else if (strcmp(mode, "info") == 0)
    {
        if (DisplayInfo(diskName))
            printf("Error display information about disk %s\n", diskName);
    }
    else
    {
        printf("Could not find command '%s' to execute; use 'help' to display all commands\n", mode);
    }
    
    return 0;
}
