#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>        //strcasecmp
#include <sys/stat.h>
#include <stddef.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>

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
 * @brief Counts how many digits are in the input number
 * @param num: The input number that the number of digits will be calculated for
 */
int countDigits(int num){
    int digits=1;
    num=abs(num);
    while((num/=10)>0){
        digits++;
    }
    return digits;
}

/**
    * @brief Takes in argc and argv, and a bunch of buffers. Sets those buffers to be flags, dirs, and count of each
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
        //loop over each character in a given argument
        for(int j=0;j<strnlen(inputArgs[i],1024);j++){
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

//Contains information about each file in a directory
typedef struct itemInDir {
    char* name;     //name of the file itself
    char* path;     //path to file - used for stat()
    bool isDir;     //Used for colorization

    char* fileType; //?
    char* permissions;  //permissions of the file
    int hardLinksCount;
    char* owner;
    char* group;
    size_t size;    //file size
    long mtime;    //modified time
} itemInDir;        //each item in a directory

typedef struct folderInfo {
    bool hasHeader;     //If printing the directory path above the contents
    char* header;       //If we are, what is it?
    //char* folderPath; //Absolute path to the folder
    itemInDir* items;   //array of item info structures for that directory
    int itemCount;      //number of items in directory
    bool doWePrint;     //do we print the contents of this folder?
    size_t totalBlocks;   //for -l, shows sum of size of all the items in the directory
} folderInfo;           //one folder read by ls

/**
 * @brief Given all the dirs, tells us which dirs we actually need to run ls on
 * @param inputDirs: The 2D array of directories passed in through argv
 * @param outputDirs: A 2D array of dirs that we actually need to run ls on. Modified by function.
 */
void whichDirs(char** const inputDirs, char** outputDirs, int arg){

}
/**
 * @brief Get information used in long list format printing
 * @param item: A pointer to the item information struct. Modified by function.
 * @param folder: A handle to the current folder, used for getting the total blocks taken up by the items in the folder
 */
void getLongListItems(itemInDir* item, folderInfo* folder){
    char permissions[]="----------";
    if(item->isDir==true){
        permissions[0]='d';
    }
    struct stat fileStat;
    if(lstat(item->path,&fileStat)<0){
        fprintf(stderr,"Error: lstat() failed. %s\n",strerror(errno));
    }
    strcpy(&permissions[1], (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    strcpy(&permissions[2], (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    strcpy(&permissions[3], (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    strcpy(&permissions[4], (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    strcpy(&permissions[5], (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    strcpy(&permissions[6], (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    strcpy(&permissions[7], (fileStat.st_mode & S_IROTH) ? "r" : "-");
    strcpy(&permissions[8], (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    strcpy(&permissions[9], (fileStat.st_mode & S_IXOTH) ? "x" : "-");

    item->permissions=malloc(sizeof(permissions));
    memcpy(item->permissions,permissions,sizeof(permissions));
    item->hardLinksCount=fileStat.st_nlink;

    struct passwd* pwd;
    struct group* grp;

    pwd=getpwuid(fileStat.st_uid);
    grp=getgrgid(fileStat.st_gid);

    item->owner=pwd->pw_name;
    item->group=grp->gr_name;
    item->size=fileStat.st_size;
    item->mtime=fileStat.st_mtime;

    folder->totalBlocks+=(fileStat.st_blocks/2);
    // printf("name: %s   blocks  %ld\n",item->name,fileStat.st_blocks);
}

/**
 * @brief In a given directory, which items do we need to run ls on
 * Also gives us the path to the items to make lstat() easier
 * Returns number of items to print. Accounts for -a and -A flags. Also checks if an item is a directory.
 * @param dir The current directory we are searching through 
 * @param flags Flags from argv. If 'a' or 'A' are in the flags, for example, that will affect the outputItems
 * @param outputItems The items in that folder that will be listed when the dir contents are printed
 */
int whichItems(char* const dir, char* const flags, itemInDir* outputItems, folderInfo* folder){
    struct dirent* dirp;
    DIR* dp;
    dp=opendir(dir);
    if(!dp){
        //if we cannot open the directory, print the error immediately.
        fprintf(stderr,"ls: cannot access '%s': %s",dir,strerror(errno));
        return 0;
    }
    int index=0;
    while((dirp=readdir(dp))!=NULL){
        //strchr(flags,'a')==NULL   -> 'a' is not a given flag
        //these cause valgrind errors. Will fix later
        char* hasa=strchr(flags,'a');
        char* hasA=strchr(flags,'A');
        if(!hasa && !hasA){
            //skip entries that start with .
            if(dirp->d_name[0]=='.'){
                continue;
            }
        }
        if(hasA && hasA>hasa){
            if(strcmp(dirp->d_name,".")==0 || strcmp(dirp->d_name,"..")==0){
                continue;
            }
        }
        char itemPath[PATH_MAX+1];      //+1 for newline
        strncpy(itemPath,dir,PATH_MAX);
        size_t dirnameLen=strnlen(dir,PATH_MAX-1);
        if(itemPath[dirnameLen-1]!='/'){
            itemPath[dirnameLen]='/';
            dirnameLen++;
        }
        strncat(itemPath,dirp->d_name,PATH_MAX);
        (outputItems[index]).name=strndup(dirp->d_name,256);
        (outputItems[index]).path=strndup(itemPath,PATH_MAX);
    
        struct stat fileStat;
        if(lstat(outputItems[index].path,&fileStat)==-1){
            fprintf(stderr,"Error: lstat(%s) failed: %s\n",outputItems[index].path,strerror(errno));
        }
        if(S_ISDIR(fileStat.st_mode)==1){
            outputItems[index].isDir=true;
        }
        else{
            outputItems[index].isDir=false;
        }

        char* hasl=strchr(flags,'l');
        if(hasl){
            getLongListItems(&outputItems[index],folder);
        }
        index++;
    }
    closedir(dp);
    return index;
}

/**
 * @brief The ls logic itself. Populates the structs above with information about folders and files in those 
 * folders so we can print them.
 * @param flags: The flags string. Currently, no functionality for flags is implemented yet
 * @param argDirCount: number of dirs passed in through argv. Not all can be used, since some may not exist or have bad permissions.
 * @param printDirCount: number of directories we actually print the contents of. Modified by the function.
 * @param dirs: A 2d array of all the directories we need to try to run ls on
 * @param folders: A (blank) array of structs that contains information we need for printing the contents of a folder. 
 *      Modified by function
*/
void ls(char* const flags, size_t argDirCount, size_t* printDirCount, char** const dirs, folderInfo* folders){
    struct dirent* dirp;
    DIR* dp;
    *printDirCount=argDirCount;
    //main loop. ls for one directory at a time
    for(int i=0;i<argDirCount;i++){       
        //get number of items in the directory. We need this number so that we know how much space to alloc
        //for the struct. Accounts for all items, even ones we don't print
        size_t totalItemsInDir=0;
        dp=opendir(dirs[i]);
        if(!dp){
            //we cannot open this directory, so move on to the next one.   
            fprintf(stderr,"ls: cannot access '%s': %s",dirs[i],strerror(errno));
            if(i<argDirCount){
                printf("\n");
            }
            (*printDirCount)--;
            //If we cannot access the directory, we cannot print it
            folders[i].doWePrint=false;
            continue;
        }
        else{
            //count number of items in the directory
            while((dirp=readdir(dp))!=NULL){
                totalItemsInDir++;
            }
        }
         //if we pass more than one directory, list the path above the contents of that directory
        if(dp!=NULL && argDirCount>1){      
            folders[i].hasHeader=true;
            //set the header equal to the dir path
            folders[i].header=strndup(dirs[i],PATH_MAX);
        }
        else{
            //valid folder, but just one, so don't list the path
            folders[i].hasHeader=false;
        }
        closedir(dp);

        //allocate space for info for each item in dir
        folders[i].items=(itemInDir*)malloc(totalItemsInDir*sizeof(itemInDir));
        folders[i].itemCount=whichItems(dirs[i],flags,folders[i].items,&folders[i]);

        folders[i].doWePrint=true;
    }

}

char* trimTime(char* timeString, char* outputString){
    outputString=calloc(13,sizeof(char));
    for(int i=4;i<16;i++){
        outputString[i-4]=timeString[i];
    }
    return outputString;
}

/**
 * @brief Using the structs we populated earlier, print the information to the screen, coloring directories as blue.
 * @param argDirCount: Number of directories passed in through argv
 * @param printDirCount: Number of directories we can actually print
 * @param folders: The folder structs we filled in with ls() 
 * @param flags: The flags string. 
*/
void printLS(size_t argDirCount, size_t printDirCount, folderInfo* folders, char* flags){
    //we only care about the folders we can actually print
    folderInfo* printableFolders=malloc(printDirCount*sizeof(folderInfo));
    for(int i=0,j=0;i<argDirCount;i++){
        //copy over folders we need to print, skipping ones we don't
        if(folders[i].doWePrint){
            memcpy(&printableFolders[j],&folders[i],sizeof(folderInfo));
            j++;
        }
    }

    //print the structs
    for(int i=0;i<printDirCount;i++){
        int numItems=printableFolders[i].itemCount;
        //if we print more than one dir, we want the path listed above the contents
        if(printableFolders[i].hasHeader){
            //since newlines between dirs are structured as \n,header\n,contents\n, we don't print a newline at the start
            //since that would create an extra newline at the top of the printed dirs
            if(i!=0){
                printf("\n");       
            }
            printf("%s:\n",printableFolders[i].header);
        }
        //go in reverse if -r flag is specified
        int startIndex=0;
        int step=1;
        if(strchr(flags,'r')){
            startIndex=numItems-1;
            step=-1;
        }
        char* hasl=strchr(flags,'l');
        //print each item in each printable folder, colorzing directories as blue
        if(hasl){
            printf("total %ld\n",printableFolders[i].totalBlocks);
            //get max widths
            int hardLinksMaxWidth=0;
            int hardLinksWidth[numItems];
            int sizeMaxWidth=0;
            int sizeWidth[numItems];
            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                hardLinksWidth[j]=countDigits(printableFolders[i].items[j].hardLinksCount);
                if(hardLinksWidth[j]>hardLinksMaxWidth){
                    hardLinksMaxWidth=hardLinksWidth[j];
                }
                sizeWidth[j]=countDigits(printableFolders[i].items[j].size);
                if(sizeWidth[j]>sizeMaxWidth){
                    sizeMaxWidth=sizeWidth[j];
                }
            }
            //print
            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                char* timeString;
                timeString=trimTime(ctime(&printableFolders[i].items[j].mtime),timeString);
                // printf("Time string 2: %s ",timeString);

                //permissions
                printf("%s ",printableFolders[i].items[j].permissions);
                //hard links count
                for(int k=0;k<hardLinksMaxWidth-hardLinksWidth[j];k++){
                    putchar(' ');
                }
                printf("%d ", printableFolders[i].items[j].hardLinksCount);
                //owner and group
                printf("%s %s ",printableFolders[i].items[j].owner,printableFolders[i].items[j].group);
                //size
                for(int k=0;k<sizeMaxWidth-sizeWidth[j];k++){
                    putchar(' ');
                }
                printf("%ld ", printableFolders[i].items[j].size);
                printf("%s %s%s%s",
                    timeString,
                    printableFolders[i].items[j].isDir==true ? 
                        BLUE : DEFAULT,
                    printableFolders[i].items[j].name,DEFAULT
                );
                
                if(step==-1 && j>0){
                    printf("\n");
                }
                else if(step==1 && j<numItems-1){
                    printf("\n");
                }

                //cleanup
                free(printableFolders[i].items[j].name);
                free(printableFolders[i].items[j].path);
                free(printableFolders[i].items[j].permissions);
            }
        }
        else {
            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                if(printableFolders[i].items[j].isDir==true){
                    printf("%s%s%s  ",BLUE, printableFolders[i].items[j].name, DEFAULT);
                }
                else if(printableFolders[i].items[j].isDir==false){
                    printf("%s  ",printableFolders[i].items[j].name);
                }
                free(printableFolders[i].items[j].name);
                free(printableFolders[i].items[j].path);
            }
        }
        //don't print a blank line for folders with no items
        if(numItems>0){
            printf("\n");
        }
    }

    //Cleanup
    for(int i=0;i<argDirCount;i++){
        if(folders[i].hasHeader){
            free(folders[i].header);
        }
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
    char flags[1024];
    char** directories=malloc(argc*sizeof(char*));

    size_t flagCount=0;
    size_t argDirCount=0;       //number of directories passed in through argv
    size_t printDirCount=0;     //number of directories that we can actually print
    getFlagsAndDirs(argc,args,flags,directories,&flagCount,&argDirCount);

    bool needToFree=true;
    //run ls on the current dirctory if we don't provide any directory arguments
    if(argDirCount<1){
        directories[0]=".";
        needToFree=false;
        argDirCount=1;
    }
    //allocate space in case we need to print all the directories.
    folderInfo* folders=malloc(argDirCount*sizeof(folderInfo));
    ls(flags,argDirCount,&printDirCount,directories,folders);
    // printf("gsd %d\n",printDirCount);
    printLS(argDirCount,printDirCount,folders,flags);
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