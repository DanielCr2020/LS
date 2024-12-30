#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>        //strcasecmp
#include <stddef.h>
#include <pwd.h>
#include <grp.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include "ls.h"

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
    if(num==0){
        return 1;
    }
    int digits=1;
    int num1=abs(num);
    while((num1/=10)>0){
        digits++;
    }
    //add a digit for a negatve number (sentinel value)
    if(num<0){
        // digits++;
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

/**
 * @brief Given all the dirs, tells us which dirs we actually need to run ls on
 * @param inputDirs: The 2D array of directories passed in through argv
 * @param outputDirs: A 2D array of dirs that we actually need to run ls on. Modified by function.
 */
void whichDirs(char** const inputDirs, char** outputDirs, int arg){

}
/**
 * @brief Checks if item is a link using S_ISLNK. Only sets item as a link if S_ISLNK is true,
 * and the file it points to makes sense
 * @param item: item we check link status and update link information for.
 * @param fileStat: stat of the file. used to check for link and to get handle to link endpoint
 * @param secondCall: If this is the second time calling this function per one file. If so,
 * don't reassign item->link, as that created a memory leak
 * @returns if item should be considered a link
 */
void getLinkInfo(itemInDir* item, struct stat fileStat, bool secondCall){
    item->isLink=false;
    if(S_ISLNK(fileStat.st_mode)){
        char* pointsTo=calloc(256,1);
        int nbytes=readlink(item->path,pointsTo,256);
        char* pointsToPath=realpath(item->path,NULL);
        // printf("Realpath: %s\n",pointsToPath);
        if(nbytes!=-1){
            struct stat linkStat;
            if(stat(pointsToPath,&linkStat)<0){
                fprintf(stderr,"stat(%s) on link endpoint failed: %s\n",pointsToPath,strerror(errno));
                free(pointsTo);
                free(pointsToPath);
                return;
            }
            if(secondCall==false)
                item->link=calloc(nbytes+1,1);  //memory leak
            strncpy(item->link,pointsTo,nbytes);
            strcat(item->link,"\0");
            if(S_ISDIR(linkStat.st_mode)){
                item->pointsToDir=true;
            }
            // item->permissions[0]='l';
        }
        free(pointsTo);
        free(pointsToPath);
        item->isLink=true;
        item->isDir=false;
    }
}
/**
 * @brief Get information used in long list format printing
 * @param item: A pointer to the item information struct. Modified by function.
 * @param folder: A handle to the current folder, used for getting the total blocks taken up by the items in the folder
 */
void getLongListInfo(itemInDir* item, folderInfo* folder){
    char permissions[]="----------";
    if(item->isDir==true){
        permissions[0]='d';
    }
    item->permissions=malloc(sizeof(permissions));
    struct stat fileStat;
    if(lstat(item->path,&fileStat)<0){
        // fprintf(stderr,"Error: lstat(%s) failed 1. %s\n",item->path,strerror(errno));
        item->lstatSuccessful=false;
        item->hardLinksCount=SENTINEL;
        strncpy(item->permissions,"-?????????",11);
        item->owner="?";
        item->group="?";
        item->size=SENTINEL; //sentinel value
        item->mtime=SENTINEL;
        return;
    }
    else {
        item->lstatSuccessful=true;
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

    memcpy(item->permissions,permissions,sizeof(permissions));
    item->hardLinksCount=fileStat.st_nlink;

    struct passwd* pwd;
    struct group* grp;

    pwd=getpwuid(fileStat.st_uid);
    grp=getgrgid(fileStat.st_gid);

    if(item->lstatSuccessful==true){
        item->owner=pwd->pw_name;
        item->group=grp->gr_name;
        item->size=fileStat.st_size;
        item->mtime=fileStat.st_mtime;
    }

    folder->totalBlocks+=(fileStat.st_blocks/2);
    getLinkInfo(item,fileStat,true);
    if(item->isLink==true){
        item->permissions[0]='l';
    }
    // printf("name: %s   blocks  %ld\n",item->name,fileStat.st_blocks);
}

/**
 * @brief In a given directory, which items do we need to run ls on
 * Also gives us the path to the items to make lstat() easier
 * @returns number of items to print. Accounts for -a and -A flags. Also checks if an item is a directory.
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
    int dirIndex=0;
    char* hasl="\0";
    hasl=strchr(flags,'l');
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
        (outputItems[dirIndex]).name=strndup(dirp->d_name,256);
        (outputItems[dirIndex]).path=strndup(itemPath,PATH_MAX);
    
        struct stat fileStat;
        if(lstat(outputItems[dirIndex].path,&fileStat)==-1){
            fprintf(stderr,"Error: lstat(%s) failed: %s\n",outputItems[dirIndex].path,strerror(errno));
            outputItems[dirIndex].lstatSuccessful=false;
            // dirIndex++;
            // continue;
        }
        else {
            outputItems[dirIndex].lstatSuccessful=true;
        }
        if(S_ISDIR(fileStat.st_mode)==1){
            outputItems[dirIndex].isDir=true;
        }
        //getLinkInfo is called twice because sometimes S_ISLNK thinks something like .gitignore is a link
        getLinkInfo(&outputItems[dirIndex],fileStat,false);

        // printf("is link?: %d\n",S_ISLNK(fileStat.st_mode));
        if(hasl){
            getLongListInfo(&outputItems[dirIndex],folder);
        }
        dirIndex++;
    }
    closedir(dp);
    return dirIndex;
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

void trimTime(char* timeString, char* outputString){
    for(int i=4;i<16;i++){
        outputString[i-4]=timeString[i];
    }
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
            int hardLinksMaxWidth=0, hardLinksWidth[numItems];
            int sizeMaxWidth=1, sizeWidth[numItems];
            int ownerMaxWidth=0, ownerWidth[numItems];
            int groupMaxWidth=0, groupWidth[numItems];

            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                hardLinksWidth[j]=countDigits(printableFolders[i].items[j].hardLinksCount);
                if(hardLinksWidth[j]>hardLinksMaxWidth){
                    hardLinksMaxWidth=hardLinksWidth[j];
                }
                sizeWidth[j]=countDigits(printableFolders[i].items[j].size);
                if(sizeWidth[j]>sizeMaxWidth){
                    sizeMaxWidth=sizeWidth[j];
                }
                ownerWidth[j]=strnlen(printableFolders[i].items[j].owner,256);
                if(ownerWidth[j]>ownerMaxWidth){
                    ownerMaxWidth=ownerWidth[j];
                }
                groupWidth[j]=strnlen(printableFolders[i].items[j].group,256);
                if(groupWidth[j]>groupMaxWidth){
                    groupMaxWidth=groupWidth[j];
                }
            }
            //print
            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                char* timeString=calloc(13,sizeof(char));
                if(printableFolders[i].items[j].lstatSuccessful==false){
                    strncpy(timeString,"            ",13);
                }
                else{
                    trimTime(ctime(&printableFolders[i].items[j].mtime),timeString);
                }

                //permissions
                printf("%s ",printableFolders[i].items[j].permissions);
                //hard links count
                for(int k=0;k<hardLinksMaxWidth-hardLinksWidth[j];k++){
                    putchar(' ');
                }
                if(printableFolders[i].items[j].hardLinksCount<0)
                    printf("? ");
                else    
                    printf("%d ", printableFolders[i].items[j].hardLinksCount);

                //owner
                printf("%s ",printableFolders[i].items[j].owner);
                for(int k=0;k<ownerMaxWidth-ownerWidth[j];k++){
                    putchar(' ');
                }

                //group
                printf("%s ",printableFolders[i].items[j].group);
                for(int k=0;k<groupMaxWidth-groupWidth[j];k++){
                    putchar(' ');
                }

                //size
                for(int k=0;k<sizeMaxWidth-sizeWidth[j];k++){
                    putchar(' ');
                }
                if(printableFolders[i].items[j].size==SENTINEL)
                    printf("? ");
                else
                    printf("%ld ", printableFolders[i].items[j].size);
                
                //time
                printf("%s ", timeString);

                //name
                //if dir
                if(printableFolders[i].items[j].isDir==true){
                    printf("%s%s%s",BLUE,printableFolders[i].items[j].name,DEFAULT);
                }
                //if link
                else if(printableFolders[i].items[j].isLink==true){
                    printf("%s%s%s -> ",CYAN,printableFolders[i].items[j].name,DEFAULT);
                    if(printableFolders[i].items[j].pointsToDir==true){
                        printf("%s%s%s",BLUE,printableFolders[i].items[j].link,DEFAULT);
                    }
                    else{
                        printf("%s",printableFolders[i].items[j].link);
                    }
                        //,printableFolders[i].items[j].link);
                }
                //if neither
                else {
                    printf("%s",printableFolders[i].items[j].name);
                }
                
                if(step==-1 && j>0){
                    printf("\n");
                }
                else if(step==1 && j<numItems-1){
                    printf("\n");
                }

                //cleanup
                if(printableFolders[i].items[j].isLink){
                    free(printableFolders[i].items[j].link);
                }
                free(printableFolders[i].items[j].name);
                free(printableFolders[i].items[j].path);
                free(printableFolders[i].items[j].permissions);
                free(timeString);
            }
        }
        else {
            for(int j=startIndex;step==-1 ? j>=0 : j<numItems;j+=step){
                if(printableFolders[i].items[j].isDir==true){
                    printf("%s%s%s  ",BLUE, printableFolders[i].items[j].name, DEFAULT);
                }
                //check for link first because the "default" print case should be last
                else if(printableFolders[i].items[j].isLink==true){
                    printf("%s%s%s  ",CYAN, printableFolders[i].items[j].name, DEFAULT);
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