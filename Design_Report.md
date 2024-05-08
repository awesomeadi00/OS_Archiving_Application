### Aditya Pandhare - ap7146
# Operating Systems: Project 3 - Archiving File System Manager
This report will consist of the design choices made in this system and why everything was designed the way it was.


## 1. Parsing Arguments
As per the project description, we need to compile a program `adzip.c`, to which it has specific parameters we need to send through from the command line, hence it is vital to parse these flagged values through their respective Parsing Functions. 

This is how it will be compiled: 
```
./adzip -{c | a | x | m | p} [Archive File Name] [Path of the File/Directory to Archive]
```

Here are the main checks I made sure to implement: 

1. **Argument Number**: The input needs to have 4 parameters, therefore in the `main()`, `argc == 4`

2. **Flags Check**: I introduced a check to ensure that each of the flags: `-c, -a, -x, -m, -p` were part of the input. Any other inputs in place of the flags are incorrect.

These were the two main general checks in placed needed to parse all the appropriate information into variables for the entire system to work. 

Obviously there are more checks such as checking if there's an existing archive file already or checking if the file/diretory path exists etc. However, those are more specific to each of the flags which we will get into soon. 

3. **Unqiue Archive File Name**: This was a external check specifically for the creation flag. Obviously we cannot have two archive files with the same name. Hence, I ensured that if there was a file in the current directory (cd) with the same name inputted from user on the command line, it would simply append a number to the file name. For example, if `archive.ad` exists on the cd, then the new archive will be called `archive1.ad`. In this manner the number will increment to ensure unique names. 

4. **Checks if Archive File or File/Directory Paths exist**: For instance, in the appending flag, for this to work, the archive file needs to exist and so does the file/directory path. Hence, this check is placed as well for the appending 

## 2. File System Organization Structure

For this assignment, I have decided to follow the exact same file system structure provided in the assignment description document. For reference, here is an iamge: 

![File_System_Structure](../AdityaPandhare-Proj3/File%20System%20Structure.png)

In this structure we have 3 main components: 
1.  **Header**: This area is the header and contains information about how many entries are currently in this file system structure as well as keeping track of where the metadata area is located through the offset of the entire structure. Here is the structure in code: 
```
typedef struct
{
    long metadataOffset; // Tracks where the metadata is in the structure
    int numEntries;      // Tracks how many entries are in this structure
} ArchiveHeader;
``` 


2.  **Data Area**: This is where all of the data for each of the files are written plainly put. 

3.  **Metadata Area**: This area contains metadata for each and every file system entity whether it's a file or directory. It contains information of the following:
```
typedef struct
{
    char type;          // File type can be either 'F' for file or 'D' for directory
    long size;          // Size of the file
    long offset;        // The offset of its data within the dataarea
    char name[256];     // Name of the file
    uid_t owner;        // Owner of the file
    gid_t group;        // Group owner of the file
    mode_t rights;      // Access rights of the file
} ArchiveEntry;
```
This way, the header keeps track of where the metadata is and the metadata contains information about each file's or directory's data information.  

## 3. Creation of Archive File
For the creation of the archive file system, we need 3 things: 

1. The user needs to type the flag: `-c`
2. The user needs to type the name of the archive file they wish through the terminal execution
3. The user needs to type the path of the file/directory they wish to archive. 

Once these things are provided, the program does several things sequentially: 

1. **Unqiue Archive File Name**: Obviously we cannot have two archive files with the same name. Hence, I ensured that if there was a file in the current directory (cd) with the same name inputted from user on the command line, it would simply append a number to the file name. For example, if `archive.ad` exists on the cd, then the new archive will be called `archive1.ad`. In this manner the number will increment to ensure unique names. 

2. **Check if File/Directory Path Exists**: Once more, we cannot archive something that doesn't exist. Hence there is a check to ensure that whatever we are trying to archive exists first. If it does we can carry on. 

3. **Header Initialization**: Here the header is initialized with 0 entries and it reserves some space for the metadata area so that it has an inital track on where it will be located in the structure. 

4. **Checks if it's a directory or file**: It checks if the path provided is a file or directory. If it's a file it simply writes the data into the archive and records its metadata appropriately. Otherwise if it's a directory, it recursively goes through and writes all the data for everything inside and records their metadata as well. 

5. **Updating Header**: Once everything is done, we update the header by updating where the metadata offset area is and also updating the number of entries added to the structure. 


## 4. Appending to Archive File: 
For the appending of a file/directory to the archive file system, we need 3 things: 

1. The user needs to type the flag: `-a`
2. The user needs to type the reference of the archive file they wish append to
3. The user needs to type the path of the file/directory they wish to append to the archive. 

Once these things are provided, the program does several things sequentially: 

1. **Check if File/Directory Path Exists**: We cannot archive something that doesn't exist. Hence there is a check to ensure that whatever we are trying to archive exists first. If it does we can carry on. 

2. **Check if Archive File Exists**: Since we're appending to the archive, the archive file needs to already exist, hence a check for this is in place. 

1. **Unqiue Appended File Names**: In the same light, when we append a file/directory to the archive, there cannot be duplicates with the same name in the archive. Hence another helper function is used to compare the incoming appended file/directory name with every entity already in the archive. If it is unique, it remains the same, else we do the same thing where we append a number to its name such as going from `file.txt` to `file 1.txt`. 

3. **Header Retrieval**: We retrieve the information from the header as we need to append the new entity's information in the metadata and increment the entrycount. 

4. **Checks if it's a directory or file**: In the same way as the creation, we check if it's a directory or a file. If it's a file we directly write it and record its metadata. If it's a directory, we do the same recursive strategy and write everything and then record their metdata. 

5. **Updating Header**: Once everything is done, we update the header once more by updating where the metadata offset area is and also updating the number of entries added to the structure. 


## 5. Extracting from Archive File
For the extraction of the archive file system, we need 2 things: 

1. The user needs to type the flag: `-x`
2. The user needs to type the reference of the archive file they wish extract to

The file/directory path is not needed, they can type anything they want there in that case. They need to actually because there the parsing ensures we need 4 arguments. 

Once these things are provided, the program does several things sequentially: 

1. **Check if Archive File Exists**: Since we're extracting from the archive, the archive file needs to already exist, hence a check for this is in place. 

2. **Retrieve the Current Directory**: We get the current working directory and for each file/directory entry. We append their names to the current directory working path. This way their full path will be defined when we actually write the data in these paths. 

3. **Dierctory Cretaion**: If the entry type is 'D', it indicates it's a directory therefore we create the directory based on the new path we created. 

4. **Write files**: We then create a file within this new path and start writing the same data contents on it from the archive. This is done recursively for each entry in the file system structure 


## 5. Printing Metadata: 
For the printing of the metadata of each entry in the archive file system, we need 2 things: 

1. The user needs to type the flag: `-m`
2. The user needs to type the reference of the archive file they wish extract to

The file/directory path is not needed, they can type anything they want there in that case. They need to actually because there the parsing ensures we need 4 arguments. 

Once these things are provided, the program does several things sequentially: 

1. **Check if Archive File Exists**: Since we're extracting from the archive, the archive file needs to already exist, hence a check for this is in place. 

2. **Print Entries**: Since we have all the metadata stored in the archive file, we can simple read them and print all the information in the structure proposed for each entity: 
```
typedef struct
{
    char type;          // File type can be either 'F' for file or 'D' for directory
    long size;          // Size of the file
    long offset;        // The offset of its data within the dataarea
    char name[256];     // Name of the file
    uid_t owner;        // Owner of the file
    gid_t group;        // Group owner of the file
    mode_t rights;      // Access rights of the file
} ArchiveEntry;
```


## 6. Displaying Archive File Hierarchies
For the displaying the hierarchy of the archive file system we need 2 things: 

1. The user needs to type the flag: `-p`
2. The user needs to type the reference of the archive file they wish extract to

The file/directory path is not needed, they can type anything they want there in that case. They need to actually because there the parsing ensures we need 4 arguments. 

Once these things are provided, the program can print the hierarchy. In my program, the program will print out the hierarchy in this manner: 

```
___directory
      ___file1
      ___file2
      ___file3
      ___inner_directory
            ___file4

___file5

___directory2
```

This way it's understandable that the indented entities are under a directory and the ones on the same indention line are the entities within the same group. 