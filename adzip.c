#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h> 
#include <limits.h>
#include <ctype.h>

// Dictionary Structure for a file entity's metadata
#pragma pack(push, 1)
typedef struct
{
    char type;
    long size;
    long offset;
    char name[256];
} ArchiveEntry;

// Header of the archive which keeps track of the metadataOffset and the number of entries
typedef struct
{
    long metadataOffset;
    int numEntries;
} ArchiveHeader;
#pragma pack(pop)

//================================================================ PARSING ================================================================================
// Helper function, especially for creating an archive. If a file of the same archive already exists, then it will generate a new name by appending a number to it.
void GenerateUniqueFilename(char **archiveFile)
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
    if (argc != 4)
    {
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
    *archiveFile = strdup(argv[2]);
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
    if (strcmp(*flag, "-c") == 0)
    {
        GenerateUniqueFilename(archiveFile);
    }
}

//================================================================ UTILITY FUNCTIONS ================================================================================
// Function to get the size of a file
long getFileSize(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

// Function to write a single file's data to the archive
void writeFileToArchive(FILE *archive, const char *filePath, const char *filePathfromRoot, ArchiveEntry *entry)
{
    // We open the file in read mode
    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        perror("Failed to open file for reading");
        return;
    }

    // We get according information of the file and the archive (where the file will be writen
    long size = getFileSize(filePath);
    long offset = ftell(archive);

    // Write file data to archive
    char buffer[PATH_MAX];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        fwrite(buffer, 1, bytesRead, archive);
    }
    fclose(file);

    // Set up the entry for metadata
    entry->type = 'F';
    entry->size = size;
    entry->offset = offset;
    strcpy(entry->name, filePathfromRoot);

    // // Debug print
    // printf("Pre-Write Metadata: Name=%s, Type=%c, Offset=%ld, Size=%ld\n\n",
    //        entry->name, entry->type, entry->offset, entry->size);
}

// Recursive function to process directories
void processDirectory(FILE *archive, const char *inputPath, const char *directoryPathFromRoot, ArchiveEntry *entries, int *entryCount)
{
    ArchiveEntry *dirEntry = &entries[*entryCount];
    strcpy(dirEntry->name, directoryPathFromRoot);
    dirEntry->type = 'D';
    dirEntry->size = 0;                // Directory size can be 0 as it holds no "data" itself
    dirEntry->offset = ftell(archive); // Offset where directory data would be, not applicable here
    (*entryCount)++;                   // Increment entry count

    // // Debug print
    // printf("Pre-Write Metadata: Name=%s, Type=%c, Offset=%ld, Size=%ld\n\n",
    //        dirEntry->name, dirEntry->type, dirEntry->offset, dirEntry->size);

    // Open up the directory
    DIR *dir = opendir(inputPath);
    if (!dir)
    {
        perror("Failed to open directory");
        return;
    }

    // We read all contents from the directory table:
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // We skip the first two entries which are '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; 

        // Build the full path of the current file or directory by appending it with this directory
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", inputPath, entry->d_name);

        // Build the full path including the root
        char fullPathfromRoot[PATH_MAX];
        snprintf(fullPathfromRoot, sizeof(fullPath), "%s/%s", directoryPathFromRoot, entry->d_name);

        struct stat path_stat;  
        stat(fullPath, &path_stat);

        // Check once more if it's a directory we will recurse
        if (S_ISDIR(path_stat.st_mode))
        {
            processDirectory(archive, fullPath, fullPathfromRoot, entries, entryCount);
        }

        // Otherwise if it's a file we can write its content to the archive and record its metadata
        else
        {
            writeFileToArchive(archive, fullPath, fullPathfromRoot, &entries[*entryCount]);
            (*entryCount)++;
        }
    }
    closedir(dir);
}

// Creates a directory if it doesn't exist - invoked from the extraction function
void createDirectoryIfNotExists(const char *path)
{
    printf("Creating directory: %s\n", path);
    struct stat st = {0};

    // If this directory doesn't exist, then we will make it
    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0755) == -1)
        {
            perror("Failed to create directory");
            exit(EXIT_FAILURE);
        }
    }
}

// Just to trim the trailing space of the path names so everything is clean
void trimTrailingSpaces(char *str)
{
    if (!str)
        return;

    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        len--;
    }
    str[len] = '\0'; 
}

//================================================================ CREATION ================================================================================
// Function to initialize an archive
void CreateArchive(const char *archiveFile, char *inputPath)
{
    // Open the archive file as writing binary mode 
    FILE *archive = fopen(archiveFile, "wb+");
    if (!archive)
    {
        perror("Failed to create archive file");
        exit(EXIT_FAILURE);
    }

    // Initialize the header for having initially 0 offset and 0 entries
    ArchiveHeader header = {0, 0};
    fwrite(&header, sizeof(ArchiveHeader), 1, archive); // Reserve space for the header on the archive

    // For now we will just have a cap of 1000 entries, plan to make this dynamic later
    ArchiveEntry entries[1000]; 
    int entryCount = 0;

    // Get information about the inputhPath provided
    struct stat path_stat;
    stat(inputPath, &path_stat);

    // Extract only the last part of the path to get the root of this archive
    char *rootOfArchive = strrchr(inputPath, '/');
    if (rootOfArchive) {
        rootOfArchive++; // Skip the slash to get only the name or directory
    }
    else {
        rootOfArchive = inputPath; // The path does not contain any slashes
    }
    printf("Root of Archive: %s\n", rootOfArchive);

    // If it's a directory then we will process the directory into the archive
    if (S_ISDIR(path_stat.st_mode))
    {
        processDirectory(archive, inputPath, rootOfArchive, entries, &entryCount);
    }

    // Otherwise we will simply call the function to write the file directly into the archive
    else
    {
        writeFileToArchive(archive, inputPath, rootOfArchive, &entries[entryCount]);
        entryCount++;
    }

    // Update all header metadata at the end of the archive
    header.metadataOffset = ftell(archive);
    header.numEntries = entryCount; 

    fseek(archive, 0, SEEK_END);
    // For each entry we will update their metadata into the metadata section of the archive
    for (int i = 0; i < entryCount; i++)
    {
        if (fwrite(&entries[i], sizeof(ArchiveEntry), 1, archive) != 1)
        {
            perror("Failed to write metadata entry");
        }
    }

    // Go back to the beginning and write the header
    rewind(archive);
    if (fwrite(&header, sizeof(ArchiveHeader), 1, archive) != 1)
    {
        perror("Failed to write archive header");
    }

    // // Test read header, metadata
    // // Rewind and read the header to print its contents
    // rewind(archive);
    // if (fread(&header, sizeof(ArchiveHeader), 1, archive) == 1)
    // {
    //     printf("Header Read: MetadataOffset=%ld, NumberOfEntries=%d\n\n",
    //            header.metadataOffset, header.numEntries);
    // }
    // else
    // {
    //     perror("Failed to read archive header for verification");
    // }

    // fseek(archive, header.metadataOffset, SEEK_SET);
    // for (int i = 0; i < header.numEntries; i++)
    // {
    //     ArchiveEntry testEntry;
    //     if (fread(&testEntry, sizeof(ArchiveEntry), 1, archive) == 1)
    //     {
    //         printf("Test Read Entry: Name=%s, Type=%c, Offset=%ld, Size=%ld\n",
    //                testEntry.name, testEntry.type, testEntry.offset, testEntry.size);
    //     }
    //     else
    //     {
    //         perror("Failed to read metadata for testing");
    //     }
    // }

    fclose(archive);
    printf("Archive created successfully\n");
}

//================================================================ EXTRACTION ================================================================================
void ExtractArchive(const char *archiveFile)
{
    // Get the current working directory path
    char basePath[PATH_MAX];
    if (!getcwd(basePath, sizeof(basePath)))
    {
        perror("Failed to get current working directory");
        return;
    }

    trimTrailingSpaces(basePath);

    // Opens up the archive file in read binary mode
    FILE *archive = fopen(archiveFile, "rb");
    if (!archive)
    {
        perror("Failed to open archive file for reading");
        exit(EXIT_FAILURE);
    }

    // Read the header information from the archive file
    ArchiveHeader header;
    if (fread(&header, sizeof(ArchiveHeader), 1, archive) != 1)
    {
        perror("Failed to read archive header");
        fclose(archive);
        return;
    }

    // Retrieve the metadata offset entry, set it as the start of it
    long nextEntryOffset = header.metadataOffset; 

    // For each entry (each file entity within the archive), we will attempt to extract
    for (int i = 0; i < header.numEntries; i++)
    {   
        // Move to the next metadata entry
        fseek(archive, nextEntryOffset, SEEK_SET); 

        ArchiveEntry entry;
        if (fread(&entry, sizeof(ArchiveEntry), 1, archive) != 1)
        {
            perror("Failed to read metadata entry");
            exit(EXIT_FAILURE);
        }

        // Update the position to the next metadata entry
        nextEntryOffset = ftell(archive); 

        // Append the entry to the current working directory path
        char fullCDPath[PATH_MAX];
        snprintf(fullCDPath, sizeof(fullCDPath), "%s/%s", basePath, entry.name);

        printf("\nProcessing Entry: %s\nType: %c\nOffset: %ld\nSize: %ld\n",
               fullCDPath, entry.type, entry.offset, entry.size);


        // Create a directory in this fullpath if it doesn't exist
        if(entry.type == 'D') {
            createDirectoryIfNotExists(fullCDPath); 
        }

        // Extracting files
        if (entry.type == 'F')
        {
            FILE *outFile = fopen(fullCDPath, "wb");
            if (!outFile)
            {
                perror("Failed to open output file for writing");
                exit(EXIT_FAILURE);
            }

            fseek(archive, entry.offset, SEEK_SET);
            char buffer[1024];
            long toRead = entry.size;
            while (toRead > 0)
            {
                size_t bytesRead = fread(buffer, 1, (sizeof(buffer) < toRead ? sizeof(buffer) : toRead), archive);
                fwrite(buffer, 1, bytesRead, outFile);
                toRead -= bytesRead;
            }

            fclose(outFile);
            printf("Extracted: %s\n", fullCDPath);
        }
    }

    fclose(archive);
    printf("Extraction completed successfully\n");
}

//================================================================ MAIN ================================================================================
// Main Function
int main(int argc, char *argv[])
{
    char *flag = NULL, *archiveFile = NULL, *file_directory = NULL;
    ParseArguments(argc, argv, &flag, &archiveFile, &file_directory);

    // If the flag is "-c" for create
    if (strcmp(flag, "-c") == 0)
    {
        CreateArchive(archiveFile, file_directory);
    }

    // Else if the flag is "-a" for append
    else if (strcmp(flag, "-a") == 0)
    {
    }

    // Else if the flag is "-x" for extract
    else if (strcmp(flag, "-x") == 0)
    {
        ExtractArchive(archiveFile);
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
