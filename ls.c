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

//takes in argv. Gives back a string of flags, an array of dirs, and the number of each
void getFlagsAndDirs(int argc, char** inputArgs, char* outputFlags, char** outputDirs, size_t* flagCount, size_t* dirCount){
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
            (outputDirs[*dirCount])=strndup(inputArgs[i],1024);
            // printf("dir: %s\n",outputDirs[*dirCount]);
            (*dirCount)++;
        }
    }
    
}

//these will be populated with information from ls(), and then sorted and printed

typedef struct {
    char* name;
    bool isDir;
} itemInDir;        //each item in a directory

typedef struct {
    char* header;      //If printing the directory path above the contents
    itemInDir* items;
    int itemCount;      //number of items in directory
} folderInfo;        //one folder read by ls


int ls(char* flags, int dirCount, char** dirs){     //run ls with all the selected flags
    struct dirent* dirp;
    DIR* dp;

    folderInfo folders[dirCount];

    for(int i=0;i<dirCount;i++){        //main loop. ls for one directory

        if(dirCount>1){
            folders[i].header=strndup(dirs[i],1024);
            strcat(folders[i].header,":\n");
        }
        else{
            folders[i].header=strdup("");
        }     
        //get number of entries in the directory. We need this number so that we know how much space to allocate
        //for the struct
        size_t itemsInDir=0;
        dp=opendir(dirs[i]);
        // puts("here1");
        if(!dp){       //we cannot open this directory, so move on to the next one.
            fprintf(stderr,"Error: Cannot open directory '%s'. %s\n",dirs[i],strerror(errno));
            continue;
        }
        else{
            while((dirp=readdir(dp))!=NULL){
                // puts("here2");
                itemsInDir++;
            }
        }
            
        closedir(dp);

        folders[i].itemCount=itemsInDir;
        
        int fileIndex=0;    //index for file
        folders[i].items=malloc(itemsInDir*sizeof(itemInDir));
        dp=opendir(dirs[i]);
        // printf("%s",folders[i].header);
        char path[PATH_MAX];
        strncpy(path,dirs[i],PATH_MAX);     //need to copy over, since dirp->d_name kinda doesn't work
        size_t dirNameLen=strnlen(dirs[i],PATH_MAX-1);
        //append a / so that stat works correctly for checking if the file is a directory
        if(path[dirNameLen-1]!='/'){    
            path[dirNameLen]='/';
            dirNameLen++;
        }
        //ls logic
        while((dirp=readdir(dp))!=NULL){
            struct stat fileStat;
            strncpy(&path[dirNameLen],dirp->d_name,1024);
            stat(path, &fileStat);
            if(S_ISDIR(fileStat.st_mode)==1){
                // printf("%s%s%s   ",GREEN,dirp->d_name,DEFAULT);
                folders[i].items[fileIndex].isDir=true;
            }
            else{
                // printf("%s   ",dirp->d_name);
                folders[i].items[fileIndex].isDir=false;
            }
            folders[i].items[fileIndex].name=strndup(dirp->d_name,256);
            // printf("%s     ",folders[i].items[fileIndex].name);
            fileIndex++;
        }
        // puts("");
        closedir(dp);
    }

    //print the structs
    for(int i=0;i<dirCount;i++){
        // printf("itemcount: %d\n",folders[i].itemCount);

        printf("%s",folders[i].header);
        free(folders[i].header);
        for(int j=0;j<folders[i].itemCount;j++){
            if(folders[i].items[j].isDir==true){
                printf("%s%s%s   ",GREEN, folders[i].items[j].name, DEFAULT);
            }
            else if(folders[i].items[j].isDir==false){
                printf("%s   ",folders[i].items[j].name);
            }
            free(folders[i].items);
            free(folders[i].items[j].name);
        }
        printf("\n");
        if(i!=dirCount-1){
            printf("\n");
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    // char** sortedArgs=malloc(argc*sizeof(char*));
    // memcpy(sortedArgs,argv,argc*sizeof(char*));
    // qsort(&sortedArgs[1],argc-1,sizeof(char*),&argSortComp);

    char** args=malloc(argc*sizeof(char*));
    memcpy(args,argv,argc*sizeof(char*));
    for(int i=1;i<argc;i++){
        // printf("%s\n",args[i]);
    }

    char flags[1024];
    char** directories=malloc(argc*sizeof(char*));

    size_t flagCount=0;
    size_t dirCount=0;
    getFlagsAndDirs(argc,args,flags,directories,&flagCount,&dirCount);

    bool needToFree=true;
    if(argc<=1){
        directories[0]=".";
        needToFree=false;
        dirCount++;
    }

    ls(flags,dirCount,directories);


    // printf("\nFlag count: %ld\nDir count: %ld\n",flagCount,dirCount);
    // puts("Flags:");
    for(int i=0;i<flagCount;i++){
        // printf("%c\n",flags[i]);
    }
    // puts("\nDirs:");
    for(int i=0;i<dirCount;i++){
        // printf("%s\n",directories[i]);
    }
    // free(sortedArgs);

    //cleanup (kinda)

    free(args);
    if(needToFree){
        for(int i=0;i<dirCount;i++){
            free(directories[i]);
        }
    }
    free(directories);
}