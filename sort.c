#include "sort.h"

int sortByName(const void* name1, const void* name2){
    return strcmp( ((itemInDir*) name1)->name,((itemInDir*) name2)->name);
}

int sortFoldersByName(const void* name1, const void* name2){
    return strcmp( ((itemInDir*) name1)->name,((itemInDir*) name2)->name);
}

int sortBySize(const void* item1, const void* item2){
    return ((itemInDir*) item1)->itemStat.st_size < ((itemInDir*) item2)->itemStat.st_size;
}

//sort by modified time
int sortByMtime(const void* item1, const void* item2){
    int cond = ((itemInDir*) item1)->itemStat.st_mtime < ((itemInDir*) item2)->itemStat.st_mtime;
    if(cond == 1){
        return 1;
    }
    else if(((itemInDir*) item1)->itemStat.st_mtime > ((itemInDir*) item2)->itemStat.st_mtime){
        return 0;
    }
    else {
        return ((itemInDir*) item1) - ((itemInDir*) item2);
    }
    // return ((itemInDir*) item1)->itemStat.st_mtime < ((itemInDir*) item2)->itemStat.st_mtime;
}

//sort by access time
int sortByAtime(const void* item1, const void* item2){
                                                // <= makes this comparison actually correct
    return ((itemInDir*) item1)->itemStat.st_atime <= ((itemInDir*) item2)->itemStat.st_atime;
}

//sort by status change time
int sortByCtime(const void* item1, const void* item2){
                                                // <= makes this comparison actually correct
    return ((itemInDir*) item1)->itemStat.st_ctime < ((itemInDir*) item2)->itemStat.st_ctime;
}

/**
 * @brief Using the folder structs and the flags, sort the folder structs according to the flags
 * @param folders: lsRequestedItem structs to be sorted
 * @param flags: processed argv input flags
 */
void sortOutput(lsRequestedItem* folders, char* const flags){
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