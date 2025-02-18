#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
#define GREEN "\x1b[32;1m"
#define CYAN "\x1b[36;1m"

#define SENTINEL -1
#define max(a,b) a < b ? b : a
#define keepMax(a,b) a < b ? a=b : a

//maximum widths for each attribute
typedef struct widthInfo {
    //int permissionsWidth;
    int hardLinksWidth;
    int ownerWidth;
    int groupWidth;
    int sizeWidth;
    //int timeWidth;
    // int nameWidth;
} widthInfo;

//these will be populated with information from ls(), and then sorted and printed

//Contains information about each file in a directory
typedef struct itemInDir {
    char* name;     //name of the file itself
    char* path;     //path to file - used for stat()
    int nameLength; //length of file name
    bool isDir;     //Used for colorization

    struct stat fileStat;   //stat information for each file in a directory

    char* fileType; //?
    char* permissions;  //permissions of the file
    int hardLinksCount;
    char* owner;
    char* group;
    size_t size;    //file size
    long mtime;    //modified time
    bool lstatSuccessful;
    char* link;     //where the link points to if the file is a link
    bool pointsToDir;   //if the file is a link, then does the file point to a directory?
    bool isLink;

} itemInDir;        //each item in a directory

//information about one ls target we are reading
typedef struct lsTargetInfo {
    bool showPath;     //If printing the directory path above the contents
    char* path;         //Absolute path to the folder
    itemInDir* items;   //array of item info structures for that directory
    int itemCount;      //number of items in directory
    bool doWePrint;     //do we print the contents of this folder?
    size_t totalBlocks;   //for -l, shows sum of size of all the items in the directory
    widthInfo widths;     //width information for each directory
    struct stat fileStat;   //stat information for this directory
} lsTargetInfo;           //one folder read by ls

// typedef struct printConfigTable {

// } printConfigTable;

//struct with string and length of that string
typedef struct nameAndLen {
    char* name;
    int len;
} nameAndLen;

int argSortComp(const void* argA, const void* argB);

int countDigits(int num);

void getFlagsAndDirs(int argc, char** const inputArgs, char* outputFlags, char** outputTargets, int* flagCount, int* argDirCount);

void getLinkInfo(itemInDir* item, struct stat fileStat, bool secondCall);

void getLongListInfo(itemInDir* item, lsTargetInfo* folder, char* flags);

int whichItems(char* const dir, char* const flags, itemInDir* outputItems, lsTargetInfo* folder);

void ls(char* const flags, int argDirCount, int* printDirCount, char** const dirs, lsTargetInfo* folders);

void trimTime(char* timeString, char* outputString);

void printLS(int argDirCount, int printDirCount, lsTargetInfo* folders, char* flags);

void longFormatPrint(lsTargetInfo* printableFolders, int startIndex, int step, int numItems, int i);