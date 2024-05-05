#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

// Helper function, especially for creating an archive. If a file of the same archive already exists, then it will generate a new name by appending a number to it.
void generateUniqueFilename(char **archiveFile)
{
    struct stat buffer;
    int exists = stat(*archiveFile, &buffer);

    // If the file exists, then it will make the new name otherwise it won't
    if (exists == 0)
    {
        int count = 1;
        size_t len = strlen(*archiveFile);

        // Making some space for the number and potential extension change
        char *newName = malloc(len + 10);
        if (newName == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        // While there is a file duplicate that exists, we will increase count till it doesn't exist)
        do
        {
            // Removing last 3 characters of old name (removing ".ad" since it would already be there)
            // Appending the 'count' to the end of the name then adding the extension
            sprintf(newName, "%.*s%d.ad", (int)(len - 3), *archiveFile, count);

            // Checking if this new name exists as well
            exists = stat(newName, &buffer);
            count++;
        } while (exists == 0);

        free(*archiveFile);
        *archiveFile = newName;
    }
}

// This function is for parsing the arguments into the corresponding variables and to account for invalid checks
void ParseArguments(int argc, char **argv, char **flag, char **archiveFile, char **file_directory) 
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
    *archiveFile = argv[2];
    *file_directory = argv[3];

    // Ensure the archive file has the .ad extension
    const char *extension = ".ad";
    int len = strlen(*archiveFile);
    int ext_len = strlen(extension);

    // If the archive file name doesn't end with .ad, append .ad to the filename
    if (len > ext_len && strcmp(*archiveFile + len - ext_len, extension) != 0)
    {
        char *newName = malloc(len + ext_len + 1); 
        if (newName == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        strcpy(newName, *archiveFile);
        strcat(newName, extension);
        *archiveFile = newName; // Update archiveFile to point to the new name with .ad
    }

    // Passes it through to generate a unique file name in case of duplicates ONLY if it's creation of archives
    if(strcmp(flag, "-c") == 0) {
        generateUniqueFilename(archiveFile);
    }
}

//================================================================ CREATION ================================================================================
// Helper function for createArchive() to recursively go through each file in the directory and archive it into the output file passed
void archiveDirectory(FILE *outfile, const char *path)
{
    // Opens the directory specified by "path" which is the filesOrDirectory
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Failed to open directory");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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

//================================================================ APPENDING ================================================================================
// Helper function for appendToArchive() for directories to recursively append each file in it to the archive file
void archiveDirectoryAppend(FILE *outfile, const char *path)
{
    // Opens up the directory 
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Failed to open directory");
        exit(EXIT_FAILURE);
    }

    // While there are still entries in the directory we keep recursively reading
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip the first two entries of the directory table '.' and '..' 
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; 


        // Append the fullpath to each entry name
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        
        // If the entry is a directory then we recurse
        struct stat entry_stat;
        stat(fullpath, &entry_stat);

        if (S_ISDIR(entry_stat.st_mode))
        {
            archiveDirectoryAppend(outfile, fullpath);
        }

        // Otherwise we open the file and write the file path and read the data onto the outfile
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
    // First, we check if the archive file exists
    struct stat buffer;
    if (stat(archiveFile, &buffer) != 0)
    {
        if (errno == ENOENT)
        {
            // The archive file does not exist
            printf("Error: Archive file does not exist. Cannot append to a non-existing archive.\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Other errors such as permissions issues
            perror("Failed to access archive file");
            exit(EXIT_FAILURE);
        }
    }

    // We open the archive file in 'append' mode
    FILE *outfile = fopen(archiveFile, "ab"); 
    if (outfile == NULL)
    {
        perror("Failed to open archive file for appending");
        exit(EXIT_FAILURE);
    }

    // We check if it's a directory, then we pass it to the archive directory function
    struct stat path_stat;
    stat(fileOrDirectory, &path_stat);
    if (S_ISDIR(path_stat.st_mode))
    {
        archiveDirectoryAppend(outfile, fileOrDirectory);
    }

    // Otherwise we simply open the file and write it's path and data onto the archive file. 
    else
    {
        FILE *infile = fopen(fileOrDirectory, "rb");
        if (infile == NULL)
        {
            printf("Could not open %s for reading\n", fileOrDirectory);
            fclose(outfile);
            exit(EXIT_FAILURE);
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
        appendToArchive(archiveFile, file_directory);
        printf("Successfully appended to the Archive File!\n");
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
