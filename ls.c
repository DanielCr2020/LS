#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>        //strcasecmp
#include <sys/stat.h>
#include <stddef.h>
#include <pwd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <errno.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
#define GREEN "\x1b[32;1m"

int argSortComp(const void* argA, const void* argB){
    if(((char*)argA)[0]=='-'){
        return -1;
    }
    else return strncasecmp(*(char* const*)argA, *(char* const*)argB, 1024);
}

/**
    * Takes in argc and argv, and a bunch of buffers. Sets those buffers to be flags, dirs, and count of each
    * @param argc: number of arguments passed via command line
    * @param inputArgs: argv, copied by value
    * @param outputFlags: A string where each index is a flag passed in through argv
    * @param outputDirs: A 2D array for every command line argument that is not a flag. Everything we want to actually list.
    * @param flagCount: The number of flags. Set by the function.
    * @param argDirCount: The number of directories passed in through argv
*/
void getFlagsAndDirs(int argc, char** const inputArgs, char* outputFlags, char** outputDirs, size_t* flagCount, size_t* argDirCount){
    for(int i=0;i<argc;i++){    //loop over each arg in inputArgs
        bool isFlag=false;
        if(inputArgs[i][0]=='-'){
            isFlag=true;
        }
        for(int j=0;j<strnlen(inputArgs[i],1024);j++){        //loop over each character in a given argument
            if(isFlag && inputArgs[i][j]!='-'){
                (outputFlags[*flagCount])=inputArgs[i][j];
                // printf("flag: %c\n",outputFlags[*flagCount]);
                (*flagCount)++;
            }
        }
        if(isFlag==false && i>0){
            (outputDirs[*argDirCount])=strndup(inputArgs[i],1024);
            // printf("dir: %s\n",outputDirs[*argDirCount]);
            (*argDirCount)++;
        }
    }
    
}

//these will be populated with information from ls(), and then sorted and printed

typedef struct itemInDir {
    char* name;
    bool isDir;
} itemInDir;        //each item in a directory

typedef struct folderInfo {
    bool hasHeader;    //If printing the directory path above the contents
    char* header;      //If we are, what is it?
    itemInDir* items;
    int itemCount;      //number of items in directory
    bool isPrintable;   //used for proper line breaks
} folderInfo;        //one folder read by ls

/**
 * The ls logic itself. Populates the structs above with information about folders and files in those folders so we can print them.
 * @param flags: The flags string. Currently, no functionality for flags is implemented yet
 * @param argDirCount: number of dirs passed in through argv. Not all can be used, since some may not exist or have bad permissions.
 * @param printDirCount: number of directories we actually print the contents of. Modified by the function.
 * @param dirs: A 2d array of all the directories we need to try to run ls on
 * @param folders: A (blank) array of structs that contains information we need for printing the contents of a folder. 
 *      Modified by function
*/
void ls(char* const flags, int argDirCount, size_t* printDirCount, char** const dirs, folderInfo* folders){
    struct dirent* dirp;
    DIR* dp;
    *printDirCount=argDirCount;
    for(int i=0;i<argDirCount;i++){        //main loop. ls for one directory
        //get number of entries in the directory. We need this number so that we know how much space to allocate
        //for the struct
        size_t itemsInDir=0;
        dp=opendir(dirs[i]);
        if(!dp){       //we cannot open this directory, so move on to the next one.
            fprintf(stderr,"ls: cannot access '%s': %s",dirs[i],strerror(errno));
            if(i<argDirCount){
                printf("\n");
            }
            (*printDirCount)--;     //used for knowing how many dirs we actually need to print
            folders[i].isPrintable=false;
            continue;
        }
        else{
            while((dirp=readdir(dp))!=NULL){
                itemsInDir++;
            }
        }
        if(dp && argDirCount>1){       //if we pass more than one directory, list the path above the contents of that directory
            folders[i].header=strndup(dirs[i],1024);    //set the header equal to the dir path
        }
        else{                       //valid folder, but just one, so don't list the path
            folders[i].header=strdup("\0");     //A file name can't be null (I don't think)
        }
        closedir(dp);

        folders[i].itemCount=itemsInDir;
        
        int fileIndex=0;    //index for file
        folders[i].items=malloc(itemsInDir*sizeof(itemInDir));
        dp=opendir(dirs[i]);
        char path[PATH_MAX];
        strncpy(path,dirs[i],PATH_MAX);     //need to copy over, since dirp->d_name kinda doesn't work
        size_t dirnameLen=strnlen(dirs[i],PATH_MAX-1);
        //append a / so that stat works correctly for checking if the file is a directory
        if(path[dirnameLen-1]!='/'){    
            path[dirnameLen]='/';
            dirnameLen++;
        }
        //populate structs with info
        while((dirp=readdir(dp))!=NULL){
            struct stat fileStat;
            strncpy(&path[dirnameLen],dirp->d_name,1024);       //strcat won't work. 
            if(stat(path, &fileStat)==-1) {          //stat needs an absolute path
                fprintf(stderr,"Error: stat(%s) failed: %s\n",path,strerror(errno));
            }
            if(S_ISDIR(fileStat.st_mode)==1){
                folders[i].items[fileIndex].isDir=true;
            }
            else{
                folders[i].items[fileIndex].isDir=false;
            }
            folders[i].items[fileIndex].name=strndup(dirp->d_name,256);
            fileIndex++;
        }
        folders[i].isPrintable=true;
        closedir(dp);
    }
}

/**
 * Using the structs we populated earlier, print the information to the screen, coloring directories as blue.
 * @param argDirCount: Number of directories passed in through argv
 * @param printDirCount: Number of directories we can actually print
 * @param folders: The folder structs we filled in with ls() 
*/
void printLS(size_t argDirCount, size_t printDirCount, folderInfo* folders){
    //we only care about the folders we can actually print
    folderInfo* printableFolders=malloc(printDirCount*sizeof(folderInfo));
    for(int i=0,j=0;i<argDirCount;i++){     //copy over folders we need to print, skipping ones we don't
        if(folders[i].isPrintable){
            memcpy(&printableFolders[j],&folders[i],sizeof(folderInfo));
            j++;
        }
    }
    //print the structs
    for(int i=0;i<printDirCount;i++){
        if(printableFolders[i].header[0]!='\0'){        //if we print more than one dir, we want the path listed above the contents
            if(i!=0){       //since newlines between dirs are structed as \n,header\n,contents\n, we don't print a newline at the start,
                printf("\n");       //since that would create an extra newline at the top of the printed dirs
            }
            printf("%s:\n",printableFolders[i].header);
        }
        for(int j=0;j<printableFolders[i].itemCount;j++){       //print each item in each printable folder, colorzing directories as blue
            if(printableFolders[i].items[j].isDir==true){
                printf("%s%s%s   ",BLUE, printableFolders[i].items[j].name, DEFAULT);
            }
            else if(printableFolders[i].items[j].isDir==false){
                printf("%s   ",printableFolders[i].items[j].name);
            }
            free(printableFolders[i].items[j].name);
        }
        printf("\n");
    }
    for(int i=0;i<argDirCount;i++){
        free(folders[i].header);
        free(folders[i].items);
    }
    free(printableFolders);
}

int main(int argc, char* argv[]){
    // char** sortedArgs=malloc(argc*sizeof(char*));
    // memcpy(sortedArgs,argv,argc*sizeof(char*));
    // qsort(&sortedArgs[1],argc-1,sizeof(char*),&argSortComp);

    char** args=malloc(argc*sizeof(char*));
    memcpy(args,argv,argc*sizeof(char*));
    // for(int i=1;i<argc;i++){
        // printf("%s\n",args[i]);
    // }

    char flags[1024];
    char** directories=malloc(argc*sizeof(char*));

    size_t flagCount=0;
    size_t argDirCount=0;
    size_t printDirCount=0;     //number of directories that we can actually display
    getFlagsAndDirs(argc,args,flags,directories,&flagCount,&argDirCount);

    bool needToFree=true;
    if(argc<=1){        //run ls on the current dirctory if we don't provide any directory arguments
        directories[0]=".";
        needToFree=false;
        argDirCount++;
    }

    folderInfo* folders=malloc(argDirCount*sizeof(folderInfo));
    ls(flags,argDirCount,&printDirCount,directories,folders);

    printLS(argDirCount,printDirCount,folders);
    free(folders);
    // printf("\nFlag count: %ld\nDir count: %ld\n",flagCount,argDirCount);
    // puts("Flags:");
    for(int i=0;i<flagCount;i++){
        // printf("%c\n",flags[i]);
    }
    // puts("\nDirs:");
    for(int i=0;i<argDirCount;i++){
        // printf("%s\n",directories[i]);
    }
    // free(sortedArgs);

    //cleanup (kinda)
    free(args);
    if(needToFree){
        for(int i=0;i<argDirCount;i++){
            free(directories[i]);
        }
    }
    free(directories);
}