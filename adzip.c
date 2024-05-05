#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

// Structure design for the archive file
typedef struct
{
    long offset;            // Offset where the file's data begins in the archive
    int size;               // Size of the file data
    char type;              // 'F' for file, 'D' for directory
    char name[256];         // File or directory name
} ArchiveMeta;

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
            free(*archiveFile);
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
// Helper function that archives a file into the archive file
void writeFileToArchive(FILE *outfile, const char *filePath, long *metaDataOffset)
{
    struct stat statbuf;
    stat(filePath, &statbuf);

    // Record all of the file's metadata into the structure
    ArchiveMeta meta = {.offset = ftell(outfile), .size = statbuf.st_size, .type = 'F'};
    strcpy(meta.name, filePath); 

    // Open the file for reading
    FILE *infile = fopen(filePath, "rb");
    char buffer[1024];
    size_t bytes;

    // Read the files data and write it onto the archive file
    while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
    {
        fwrite(buffer, 1, bytes, outfile);
    }
    fclose(infile);

    // Record metadata at the end of the file
    fseek(outfile, 0, SEEK_END);
    fwrite(&meta, sizeof(meta), 1, outfile);

    // Update metadata offset value (keeps track of where the offset is)
    *metaDataOffset = ftell(outfile); 
}

// Helper function for createArchive() to recursively go through each file in the directory and archive it into the output file passed
void writeDirectorytoArchive(FILE *outfile, const char *path, long *metaDataOffset)
{
    // Open the directory
    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("Failed to open directory");
        return;
    }

    // Write metadata for the directory itself 
    struct stat statbuf;
    if (stat(path, &statbuf) != -1)
    {
        ArchiveMeta meta = {.offset = -1, .size = 0, .type = 'D'};
        strncpy(meta.name, path, sizeof(meta.name) - 1);
        meta.name[sizeof(meta.name) - 1] = '\0';

        fseek(outfile, 0, SEEK_END);
        fwrite(&meta, sizeof(meta), 1, outfile);
        *metaDataOffset = ftell(outfile);
    }

    // While there are still entries in the directory, keep reading
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // The the entries are the first two in the directory table '.' and '..', skip them. 
        if (entry->d_name[0] == '.')
            continue; 

        // Append the directory to each entity inside the directory which will be the full path
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        if (stat(fullPath, &statbuf) == -1)
            continue;

        // If the entity is a directory once more, we recurse
        if (S_ISDIR(statbuf.st_mode))
        {
            writeDirectorytoArchive(outfile, fullPath, metaDataOffset);
        }

        // Otherwise it is a file: 
        else
        {
            writeFiletoArchive(outfile, fullPath, metaDataOffset);
        }
    }
    closedir(dir);
}

// This function is invoked when the "-c" flag is executed on the command line to create an archive and store all the files/directories in it.
void createArchive(const char *archiveFile, const char *fileOrDirectory)
{
    // Creating an archive file in write mode (binary) for writing data onto it
    FILE *outfile = fopen(archiveFile, "wb");
    if (!outfile)
    {
        perror("Failed to create archive file");
        exit(EXIT_FAILURE);
    }

    // Initializing an initial offset between data and the metadata in the archive file (will be overwritten later when files are actually written)
    long metaDataOffset = sizeof(long);                          
    fwrite(&metaDataOffset, sizeof(metaDataOffset), 1, outfile); 

    // Getting the entities information and reading if it's a directory or a regular file
    struct stat path_stat;
    stat(fileOrDirectory, &path_stat);

    // If it's a directory, then we handle recursively:
    if (S_ISDIR(path_stat.st_mode))
    {
        writeDirectoryToArchive(outfile, fileOrDirectory, &metaDataOffset);
    }

    // Otherwise it's a file and we can handle it directly
    else
    {
        writeFileToArchive(outfile, fileOrDirectory, &metaDataOffset);
    }

    // Go back to the beginning inital bytes created at the beginning of the file and write the metadata offset there. 
    fseek(outfile, 0, SEEK_SET);
    fwrite(&metaDataOffset, sizeof(metaDataOffset), 1, outfile);

    fclose(outfile);
}

//================================================================ APPENDING ================================================================================


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
