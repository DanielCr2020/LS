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

/*
    Flags implemented:
    -A
    -a
    -l
    -r
    -f: Output is not sorted.
    -n:  The same as −l, except that the owner and group IDs are displayed numerically rather than 
    converting to a owner or group name.

    Flags to do:
    -c: Use time when file status was last changed, instead of time of last modification of the file for 
sorting ( −t ) or printing ( −l ).
    -d: Directories are listed as plain files (not searched recursively) and symbolic links in the argument
list are not indirected through.
    -F: Display a slash ( ‘/’ ) immediately after each pathname that is a directory, an asterisk ( ‘∗’ ) after
each that is executable, an at sign ( ‘@’ ) after each symbolic link, a percent sign ( ‘%’ ) after each
whiteout, an equal sign ( ‘=’ ) after each socket, and a vertical bar ( ‘|’ ) after each that is a FIFO

    -h: Modifies the −s and −l options, causing the sizes to be reported in bytes displayed in a human
readable format. Overrides −k.
    -i: For each file, print the file’s file serial number (inode number).
    -k: Modifies the −s option, causing the sizes to be reported in kilobytes. The rightmost of the −k and
−h flags overrides the previous flag. See also −h
    -q: Force printing of non-printable characters in file names as the character ‘?’; this is the default when
output is to a terminal.
    -R: Recursively list subdirectories encountered
    -S: Sort by size, largest file first.
    -s: Display the number of file system blocks actually used by each file, in units of 512 bytes or
BLOCKSIZE (see ENVIRONMENT) where partial units are rounded up to the next integer value.
If the output is to a terminal, a total sum for all the file sizes is output on a line before the listing.
    -t: Sort by time modified (most recently modified first) before sorting the operands by lexicographical
order.
    -u: Use time of last access, instead of last modification of the file for sorting ( −t ) or printing ( −l ) 
    -w: Force raw printing of non-printable characters. This is the default when output is not to a terminal.
*/

int argSortComp(const void* argA, const void* argB){
    if(((char*)argA)[0] == '-'){
        return -1;
    }
    else return strncasecmp(*(char* const*)argA, *(char* const*)argB, 1024);
}

/**
 * @brief Counts how many digits are in the input number
 * @param num: The input number that the number of digits will be calculated for
 */
int countDigits(int num){
    if(num == 0){
        return 1;
    }
    int digits = 1;
    int num1 = abs(num);
    while((num1 /= 10)>0){
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
    for(int i = 0;i<argc;i++){    //loop over each arg in inputArgs
        bool isFlag = false;
        if(inputArgs[i][0] == '-'){
            isFlag = true;
        }
        //loop over each character in a given argument
        for(int j = 0;j<strnlen(inputArgs[i],1024);j++){
            if(isFlag && inputArgs[i][j] != '-'){
                (outputFlags[*flagCount]) = inputArgs[i][j];
                // printf("flag: %c\n",outputFlags[*flagCount]);
                (*flagCount)++;
            }
        }
        if(isFlag == false && i>0){
            (outputDirs[*argDirCount]) = strndup(inputArgs[i],1024);
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
    item->isLink = false;
    if(S_ISLNK(fileStat.st_mode)){
        char* pointsTo = calloc(256,1);
        int nbytes = readlink(item->path,pointsTo,256);
        char* pointsToPath = realpath(item->path,NULL);
        // printf("Realpath: %s\n",pointsToPath);
        if(nbytes != -1){
            struct stat linkStat;
            if(stat(pointsToPath,&linkStat)<0){
                // fprintf(stderr,"stat(%s -> %s) on link endpoint failed: %s\n",item->path,pointsToPath,strerror(errno));
                item->pointsToDir = false;
                // free(pointsTo);
                // free(pointsToPath);
                // return;
            }
            if(secondCall == false)
                item->link = calloc(nbytes+1,1);
            strncpy(item->link,pointsTo,nbytes);
            strcat(item->link,"\0");
            if(S_ISDIR(linkStat.st_mode)){
                // printf("%s points to dir %s\n",item->name,pointsTo);
                item->pointsToDir = true;
            }
            // item->permissions[0] = 'l';
        }
        free(pointsTo);
        free(pointsToPath);
        item->isLink = true;
        item->isDir = false;
    }
}
/**
 * @brief Get information (for one file) used in long list format printing
 * @param item: A pointer to the item information struct. Modified by function.
 * @param folder: A handle to the current folder, used for getting the total blocks taken up by the items in the folder
 * @param flags: Used for -n, which specifies group and owner as numbers, not strings
 */
void getLongListInfo(itemInDir* item, folderInfo* folder, char* flags){
    char permissions[] = "----------";
    if(item->isDir == true){
        permissions[0] = 'd';
    }
    item->permissions = malloc(sizeof(permissions));
    struct stat fileStat;
    if(lstat(item->path,&fileStat)<0){
        // fprintf(stderr,"Error: lstat(%s) failed 1. %s\n",item->path,strerror(errno));
        item->lstatSuccessful = false;
        item->hardLinksCount = SENTINEL;
        strncpy(item->permissions,"-?????????",11);
        item->owner = "?";
        item->group = "?";
        item->size = SENTINEL; //sentinel value
        item->mtime = SENTINEL;
        return;
    }
    else {
        item->lstatSuccessful = true;
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
    item->hardLinksCount = fileStat.st_nlink;
    keepMax(folder->widths.hardLinksWidth,countDigits(item->hardLinksCount));

    struct passwd* pwd;
    struct group* grp;

    pwd = getpwuid(fileStat.st_uid);
    grp = getgrgid(fileStat.st_gid);

    if(item->lstatSuccessful == true){
        char* owner = pwd->pw_name;
        char* group = grp->gr_name;
        if(strchr(flags,'n')){
            sprintf(owner,"%d",pwd->pw_uid);
            sprintf(group,"%d",grp->gr_gid);
        }
        item->owner = owner;
        item->group = group;
        item->size = fileStat.st_size;
        item->mtime = fileStat.st_mtime;

        keepMax(folder->widths.ownerWidth,strnlen(item->owner,256));
        keepMax(folder->widths.groupWidth,strnlen(item->group,256));
        keepMax(folder->widths.sizeWidth,countDigits(item->size));
    }

    folder->totalBlocks += (fileStat.st_blocks/2);
    getLinkInfo(item,fileStat,true);
    if(item->isLink == true){
        item->permissions[0] = 'l';
        item->isDir = false;
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
    dp = opendir(dir);
    if(!dp){
        //if we cannot open the directory, print the error immediately.
        fprintf(stderr,"ls: cannot access '%s': %s",dir,strerror(errno));
        return 0;
    }
    int dirIndex = 0;
    char* has_l = "\0";
    has_l = strchr(flags,'l');
    char* has_n = strchr(flags,'n');
    //these cause valgrind errors. Will fix later
    char* has_a = "\0";
    has_a = strchr(flags,'a');
    char* has_A = "\0";
    has_A = strchr(flags,'A');
    while((dirp = readdir(dp)) != NULL){
        //sets these to false so they are not set true by garbage values
        outputItems[dirIndex].isDir = false;
        outputItems[dirIndex].isLink = false;
        //strchr(flags,'a') == NULL   -> 'a' is not a given flag

        if(!has_a && !has_A){
            //skip entries that start with .
            if(dirp->d_name[0] == '.'){
                continue;
            }
        }
        if(has_A && has_A>has_a){
            if(strcmp(dirp->d_name,".") == 0 || strcmp(dirp->d_name,"..") == 0){
                continue;
            }
        }
        char itemPath[PATH_MAX+1];      //+1 for newline
        strncpy(itemPath,dir,PATH_MAX);
        size_t dirnameLen = strnlen(dir,PATH_MAX-1);
        if(itemPath[dirnameLen-1] != '/'){
            itemPath[dirnameLen] = '/';
            dirnameLen++;
        }
        strncat(itemPath,dirp->d_name,PATH_MAX);
        (outputItems[dirIndex]).name = strndup(dirp->d_name,256);
        (outputItems[dirIndex]).path = strndup(itemPath,PATH_MAX);
    
        struct stat fileStat;
        if(lstat(outputItems[dirIndex].path,&fileStat) == -1){
            fprintf(stderr,"Error: lstat(%s) failed: %s\n",outputItems[dirIndex].path,strerror(errno));
            outputItems[dirIndex].lstatSuccessful = false;
            // dirIndex++;
            // continue;
        }
        else {
            outputItems[dirIndex].lstatSuccessful = true;
        }
        if(S_ISDIR(fileStat.st_mode) == 1){
            outputItems[dirIndex].isDir = true;
        }
        //getLinkInfo is called twice because sometimes S_ISLNK thinks something like .gitignore is a link
        getLinkInfo(&outputItems[dirIndex],fileStat,false);

        // printf("is link?: %d\n",S_ISLNK(fileStat.st_mode));
        if(has_l || has_n){
            getLongListInfo(&outputItems[dirIndex],folder,flags);
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
    *printDirCount = argDirCount;
    //main loop. ls for one directory at a time
    for(int i = 0;i<argDirCount;i++){       
        //get number of items in the directory. We need this number so that we know how much space to alloc
        //for the struct. Accounts for all items, even ones we don't print
        size_t totalItemsInDir = 0;
        dp = opendir(dirs[i]);
        if(!dp){
            //we cannot open this directory, so move on to the next one.   
            fprintf(stderr,"ls: cannot access '%s': %s",dirs[i],strerror(errno));
            if(i<argDirCount){
                printf("\n");
            }
            (*printDirCount)--;
            //If we cannot access the directory, we cannot print it
            folders[i].doWePrint = false;
            continue;
        }
        else{
            //count number of items in the directory
            while((dirp = readdir(dp)) != NULL){
                totalItemsInDir++;
            }
        }
         //if we pass more than one directory, list the path above the contents of that directory
        if(dp != NULL && argDirCount>1){      
            folders[i].hasHeader = true;
            //set the header equal to the dir path
            folders[i].header = strndup(dirs[i],PATH_MAX);
        }
        else{
            //valid folder, but just one, so don't list the path
            folders[i].hasHeader = false;
        }
        closedir(dp);

        //allocate space for info for each item in dir
        folders[i].items = (itemInDir*)malloc(totalItemsInDir*sizeof(itemInDir));
        // folders[i].widths = calloc(sizeof(widthInfo),1);
        folders[i].itemCount = whichItems(dirs[i],flags,folders[i].items,&folders[i]);

        folders[i].doWePrint = true;
    }

}

void trimTime(char* timeString, char* outputString){
    for(int i = 4;i<16;i++){
        outputString[i-4] = timeString[i];
    }
}

int sortByName(const void* name1, const void* name2){
    return strcmp( ((itemInDir*) name1)->name,((itemInDir*) name2)->name);
}

/**
 * @brief Using the folder structs and the flags, sort the folder structs according to the flags
 * @param folders: folderInfo structs to be sorted
 * @param flags: processed argv input flags
 */
void sortOutput(folderInfo* folders, char* const flags){
    // printf("Flags: %s\n",flags);
    qsort((folders)->items,(folders)->itemCount,sizeof(itemInDir),sortByName);
}

//used for sorting names for table printing
int sortNames(const void* name1, const void* name2){
    return ((nameAndLen*) name1)->len < ((nameAndLen*) name2)->len;
}

int sortLengths(const void* item1, const void* item2){
    return ((itemInDir*) item1)->nameLength < ((itemInDir*) item2)->nameLength;
}

//table printing
void createPrintConfig(itemInDir* items, int numItems){
    //handling terminal width
    struct winsize w;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&w);
    int rows = w.ws_row;
    int cols = w.ws_col;        //usable width
    // printf("Rows: %d, cols: %d\n",rows,cols);

    int minColumnWidth = 3; //1 char + 2 spaces
    // int maxItemsPerRow = rows / minColumnWidth;



    for(int i = 0;i<numItems;i++){
        items[i].nameLength = strnlen(items[i].name,256);
    }
    itemInDir* items2 = malloc(sizeof(itemInDir)*numItems);
    memcpy(items2,items,sizeof(itemInDir)*numItems);
    qsort(items2,numItems,sizeof(itemInDir),sortLengths);

    //find starting table configuration
    int usedLength = 0;
    int minimumRows = 0;
    for(int i=0;i<numItems-1;i++){
        // printf("Lengths: %s: %d\n",items2[i].name,items2[i].nameLength); 
        usedLength += items2[i].nameLength + 2;
        if(usedLength + items2[i+1].nameLength > cols){
            break;
        }
        minimumRows++;
    }
    // printf("%d %d\n",usedLength,minimumRows);
    

}

/**
 * @brief called when -l flag is specified. Print using long list format
 * @param printableFolders: folderInfo structs that we actually print
 * @param startIndex: for for loop, used for if -r flag is specified
 * @param step: 1 or -1 depending on if printing order is reversed (-r)
 * @param i: index into which of the printable folders we are printing
 */
void longFormatPrint(folderInfo* printableFolders, int startIndex, int step, int numItems, int i){
    for(int j = startIndex;step == -1 ? j >= 0 : j<numItems;j += step){
        char* timeString = calloc(13,sizeof(char));
        if(printableFolders[i].items[j].lstatSuccessful == false){
            strncpy(timeString,"           ?",13);
        }
        else{
            trimTime(ctime(&printableFolders[i].items[j].mtime),timeString);
        }

        //permissions
        printf("%s ",printableFolders[i].items[j].permissions);
        //hard links count
        if(printableFolders[i].items[j].hardLinksCount<0)
            printf("? ");
        else    
            printf("%*d ", printableFolders[i].widths.hardLinksWidth,printableFolders[i].items[j].hardLinksCount);
        //owner
        printf("%*s ",printableFolders[i].widths.ownerWidth,printableFolders[i].items[j].owner);
        //group
        printf("%*s ",printableFolders[i].widths.groupWidth, printableFolders[i].items[j].group);
        //size
        if(printableFolders[i].items[j].size == SENTINEL)
            printf("%*s ", printableFolders[i].widths.sizeWidth,"?");
        else
            printf("%*ld ", printableFolders[i].widths.sizeWidth,printableFolders[i].items[j].size);
        //time
        printf("%s ", timeString);
        //name
        //if dir
        if(printableFolders[i].items[j].isDir == true){
            printf("%s%s%s",BLUE,printableFolders[i].items[j].name,DEFAULT);
        }
        //if link
        else if(printableFolders[i].items[j].isLink == true){
            printf("%s%s%s -> ",CYAN,printableFolders[i].items[j].name,DEFAULT);
            if(printableFolders[i].items[j].pointsToDir == true){
                printf("%s%s%s",BLUE,printableFolders[i].items[j].link,DEFAULT);
            }
            else{
                printf("%s",printableFolders[i].items[j].link);
            }
        }
        //if neither dir nor link, print default
        else {
            printf("%s",printableFolders[i].items[j].name);
        }
        
        if(step == -1 && j>0){
            printf("\n");
        }
        else if(step == 1 && j<numItems-1){
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

/**
 * @brief Using the structs we populated earlier, print the information to the screen, coloring directories as blue.
 * @param argDirCount: Number of directories passed in through argv
 * @param printDirCount: Number of directories we can actually print
 * @param folders: The folder structs we filled in with ls() 
 * @param flags: The flags string. 
*/
void printLS(size_t argDirCount, size_t printDirCount, folderInfo* folders, char* flags){
    //we only care about the folders we can actually print
    folderInfo* printableFolders = malloc(printDirCount*sizeof(folderInfo));
    for(int i = 0, j = 0;i<argDirCount;i++){
        //copy over folders we need to print, skipping ones we don't
        if(folders[i].doWePrint){
            memcpy(&printableFolders[j],&folders[i],sizeof(folderInfo));
            j++;
        }
    }

    char* has_l = strchr(flags,'l');
    char* has_n = strchr(flags,'n');
    int startIndex = 0;
    int step = 1;

    //if no f flag, sort output
    //if f flag is present, do not sort output
    if(!strchr(flags,'f')){
        sortOutput(folders, flags);
    }

    //print the structs
    for(int i = 0;i<printDirCount;i++){
        int numItems = printableFolders[i].itemCount;
        // printf("\nItems in folder: %d\n",numItems);

        //set up for formatted printing
        // nameAndLen names[numItems];
        // for(int j = 0;j<numItems;j++){
            // int len = strlen(printableFolders[i].items[j].name);
            // names[j].name = printableFolders[i].items[j].name;
            // names[j].len = len;
        // }
        // qsort(names,numItems,sizeof(nameAndLen),sortNames);

        // for(int j=0;j<numItems;j++){
        //     printf("%s, %d\n",names[j].name,names[j].len);
        // }

        //if we print more than one dir, we want the path listed above the contents
        if(printableFolders[i].hasHeader){
            //since newlines between dirs are structured as \n,header\n,contents\n, we don't print a newline at the start
            //since that would create an extra newline at the top of the printed dirs
            if(i != 0){
                printf("\n");
            }
            printf("%s:\n",printableFolders[i].header);
        }
        //go in reverse if -r flag is specified
        if(strchr(flags,'r')){
            startIndex = numItems-1;
            step = -1;
        }

        //print each item in each printable folder, colorzing directories as blue
        if(has_l || has_n){
            printf("total %ld\n",printableFolders[i].totalBlocks);
            longFormatPrint(printableFolders,startIndex,step,numItems,i);
        }
        //if not using long listing format
        else {
            createPrintConfig(printableFolders[i].items,numItems);
            for(int j = startIndex;step == -1 ? j >= 0 : j<numItems;j += step){
                // printf("")
                if(printableFolders[i].items[j].isDir == true){
                    printf("%s%s%s  ",BLUE, printableFolders[i].items[j].name, DEFAULT);
                }
                //check for link first because the "default" print case should be last
                else if(printableFolders[i].items[j].isLink == true){
                    printf("%s%s%s  ",CYAN, printableFolders[i].items[j].name, DEFAULT);
                }
                else if(printableFolders[i].items[j].isDir == false){
                    printf("%s  ",printableFolders[i].items[j].name);
                }
                free(printableFolders[i].items[j].name);
                free(printableFolders[i].items[j].path);
                if(printableFolders[i].items[j].isLink){
                    free(printableFolders[i].items[j].link);
                }
            }
        }
        //don't print a blank line for folders with no items
        if(numItems>0){
            printf("\n");
        }
    }

    //Cleanup
    for(int i = 0;i<argDirCount;i++){
        if(folders[i].hasHeader){
            free(folders[i].header);
        }
        free(folders[i].items);
    }
    free(printableFolders);
}

int main(int argc, char* argv[]){
    // char** sortedArgs = malloc(argc*sizeof(char*));
    // memcpy(sortedArgs,argv,argc*sizeof(char*));
    // qsort(&sortedArgs[1],argc-1,sizeof(char*),&argSortComp);

    char** args = malloc(argc*sizeof(char*));
    memcpy(args,argv,argc*sizeof(char*));
    // for(int i = 1;i<argc;i++){
        // printf("%s\n",args[i]);
    char flags[1024] = {'0'};
    char** directories = malloc(argc*sizeof(char*));

    size_t flagCount = 0;
    size_t argDirCount = 0;       //number of directories passed in through argv
    size_t printDirCount = 0;     //number of directories that we can actually print
    getFlagsAndDirs(argc,args,flags,directories,&flagCount,&argDirCount);

    bool needToFree = true;
    //run ls on the current dirctory if we don't provide any directory arguments
    if(argDirCount<1){
        directories[0] = ".";
        needToFree = false;
        argDirCount = 1;
    }
    //allocate space in case we need to print all the directories.
    folderInfo* folders = malloc(argDirCount*sizeof(folderInfo));
    ls(flags,argDirCount,&printDirCount,directories,folders);
    // printf("gsd %d\n",printDirCount);
    printLS(argDirCount,printDirCount,folders,flags);
    free(folders);
    // printf("\nFlag count: %ld\nDir count: %ld\n",flagCount,argDirCount);
    // puts("Flags:");
    for(int i = 0;i<flagCount;i++){
        // printf("%c\n",flags[i]);
    }
    // puts("\nDirs:");
    for(int i = 0;i<argDirCount;i++){
        // printf("%s\n",directories[i]);
    }
    // free(sortedArgs);

    //cleanup (kinda)
    free(args);
    if(needToFree){
        for(int i = 0;i<argDirCount;i++){
            free(directories[i]);
        }
    }
    free(directories);
}