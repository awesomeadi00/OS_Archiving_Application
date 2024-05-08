# Instructions to Run Program
Before executing the program, please make sure all the necessary files are in place in your directory. Here is a template of how the directory should be structured: 

```
|___Project3/
    |___adzip.c
    |___makefile
    |___README.md
    |___Design_Report.md
```

Additionally, make sure that you execute these processes on a **Linux Machine/Server** since this program is built specifically for this OS and not any other.

Once that is ensured, you can begin compiling the program
1. First to compile the program, simply type the command: 
```
make
```

2. Now you should see an executables called `adzip` appear in the directory.

3. To execute this entire system, it's critical to type it in the following format for it to actually compute:
```
./adzip -{c | a | x | m | p} [Archive File Name] [Path of the File/Directory to Archive]
```

Where: 
- -c (Creation): This flag involves actually creating the archive, you will need to type the name of the archive file you want and include the path of the files/directories you wish to archive

- -a (Append): This flag involves appending files/directories into the archive file. Once again you will need to reference the archive file you created and also the path of the files/directories you want to append to the archive file.  

- -x (Extract): This flag involves extracting the files/directory hierarchy(-ies) in the archive file out into your current directory. 

- -m (Metadata): This flag involves printing the metadata for every file/directory within the archive on the terminal console. 

- -p (Display): This flag involves printing out the entire hierarchy(-ies) that are currently archived within an archived file on the terminal console. 


**Important**: The first two flags depend on the user inputting the archive file name and a file/directory path as arguments. However, the last three flags don't really depend on the file/directory path. However, it will still flag as an error if you don't put an argument for that so you can put a dummy input in that case. 



## Important Notes: 

- If you wish to clear the `adzip`, executable/files and the `.ad` files, simple type in your terminal: 
```
make clean
``` 
- Currently there is no feature to clear any extracted files hence deletion and clearing of those will have to be done manually. 


