#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// This function is for parsing the arguments into the corresponding variables and to account for invalid checks
void ParseArguments(int argc, char **argv, char **flag, char **archiveFlag, char **file_directory) 
{
    // If the number of arguments is not the expected amount
    if (argc != 4) {
        printf("Invalid number of Input Arguments!\n");
        printf("Proper Usage: adzip {-c | -a | -x | -m | -p} <archive-file> <file/directory list>\n");
        exit(EXIT_FAILURE);
    }

    // If the incorrect flag is used, if the correct flag is used, it will return (0):
    if (strcmp(argv[1], "-c") != 0 && strcmp(argv[1], "-a") != 0 && strcmp(argv[1], "-x") != 0 && strcmp(argv[1], "-m") != 0 && strcmp(argv[1], "-p") != 0)
    {
        printf("Invalid flag inputted!\n");
        printf("Proper Usage: adzip {-c | -a | -x | -m | -p} <archive-file> <file/directory list>\n");
        exit(EXIT_FAILURE);
    }

    *flag = argv[1];
    *archiveFlag = argv[2];
    *file_directory = argv[3];
}

// Helper function for createArchive() to recursively go through each file in the directory and archive it into the output file passed
void archiveDirectory(FILE *outfile, const char *path)
{
    // Opens the directory specified by "path" which is the filesOrDirectory
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Failed to open directory");
        return;
    }

    // Iterates over each entry of the directory table
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip the '.' and '..' entries (first two which are cd and parent)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; 

        // Constructs the fullpath of file/directory in directory (path + entryname)
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // Once again, receiving information about the file/directory 
        struct stat entry_stat;
        stat(fullpath, &entry_stat);

        // If it is a directory, it will recursively call this function once more and repeat. 
        if (S_ISDIR(entry_stat.st_mode))
        {
            archiveDirectory(outfile, fullpath);
        }

        // Otherwise it's a file and the same steps for reading the data from the file and writing to the archive file are in place as the main createArchive() function. 
        else
        {
            FILE *infile = fopen(fullpath, "rb");
            if (infile == NULL)
            {
                printf("Could not open %s for reading\n", fullpath);
                continue;
            }
            fprintf(outfile, "%s\n", fullpath); // Write the file path and a newline
            char buffer[1024];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
            {
                fwrite(buffer, 1, bytes, outfile);
            }
            fclose(infile);
        }
    }
    closedir(dir);
}

// This function is invoked when the "-c" flag is executed on the command line to create an archive and store all the files/directories in it.
void createArchive(const char *archiveFile, const char *fileOrDirectory)
{
    // Create the actual archive file
    FILE *outfile = fopen(archiveFile, "wb"); //write-only and binary mode
    if (outfile == NULL)
    {
        perror("Failed to create archive file");
        return;
    }

    // This structure is used to get information about the file/directory
    struct stat path_stat;
    stat(fileOrDirectory, &path_stat);

    // Check to see if path is a directory then we can execute the following recusrive storage system for archiving directories: 
    if (S_ISDIR(path_stat.st_mode))
    {
        archiveDirectory(outfile, fileOrDirectory);
    }

    // Otherwise, we can simply open a file from this path (read binary mode)
    else
    {
        FILE *infile = fopen(fileOrDirectory, "rb");
        if (infile == NULL)
        {
            printf("Could not open %s for reading\n", fileOrDirectory);
            fclose(outfile);
            return;
        }

        char buffer[1024];
        size_t bytes;

        // Reading in all data from the file in chunks of buffer (1024 bytes), then writing it to the archive file
        while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
        {
            fwrite(buffer, 1, bytes, outfile);
        }
        fclose(infile);
    }

    fclose(outfile);
}

// Helper function for appendToArchive() for directories to recursively append each file in it to the archive file
void archiveDirectoryAppend(FILE *outfile, const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // Skip the '.' and '..' entries

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat entry_stat;
        stat(fullpath, &entry_stat);
        if (S_ISDIR(entry_stat.st_mode))
        {
            archiveDirectoryAppend(outfile, fullpath);
        }
        else
        {
            FILE *infile = fopen(fullpath, "rb");
            if (infile == NULL)
            {
                printf("Could not open %s for reading\n", fullpath);
                continue;
            }
            fprintf(outfile, "%s\n", fullpath); // Write the file path and a newline
            char buffer[1024];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
            {
                fwrite(buffer, 1, bytes, outfile);
            }
            fclose(infile);
        }
    }
    closedir(dir);
}

// This function is invoked when the "-a" flag is executed on the command line to append files and directories to the archive file
void appendToArchive(const char *archiveFile, const char *fileOrDirectory)
{
    FILE *outfile = fopen(archiveFile, "ab"); // Open file in append mode
    if (outfile == NULL)
    {
        perror("Failed to open archive file for appending");
        return;
    }

    struct stat path_stat;
    stat(fileOrDirectory, &path_stat);
    if (S_ISDIR(path_stat.st_mode))
    {
        archiveDirectoryAppend(outfile, fileOrDirectory);
    }
    else
    {
        FILE *infile = fopen(fileOrDirectory, "rb");
        if (infile == NULL)
        {
            printf("Could not open %s for reading\n", fileOrDirectory);
            fclose(outfile);
            return;
        }
        fprintf(outfile, "%s\n", fileOrDirectory);
        char buffer[1024];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
        {
            fwrite(buffer, 1, bytes, outfile);
        }
        fclose(infile);
    }

    fclose(outfile);
}

// Main Function
int main(int argc, char *argv[]) 
{
    char *flag = NULL, *archiveFile = NULL, *file_directory = NULL;
    ParseArguments(argc, argv, &flag, &archiveFile, &file_directory);

    printf("Flag: %s\n", flag);
    printf("Archive File: %s\n", archiveFile);
    printf("File/Directory: %s\n", file_directory);

    // If the flag is "-c" for create
    if(strcmp(flag, "-c") == 0) {
        createArchive(archiveFile, file_directory);
        printf("Successfully created Archive File with this File/Directory!\n");
    }

    // Else if the flag is "-a" for append
    else if (strcmp(flag, "-a") == 0)
    {
        createArchive(archiveFile, file_directory);
        printf("Successfully created Archive File with this File/Directory!\n");
    }

    // Else if the flag is "-x" for extract
    else if (strcmp(flag, "-x") == 0)
    {

    }

    // Else if the flag is "-m" for metadata
    else if (strcmp(flag, "-m") == 0)
    {

    }

    // Else if the flag is "-p" for display
    else if (strcmp(flag, "-p") == 0)
    {

    }

    return (EXIT_SUCCESS);
}
