#include <string.h>
#include <stdlib.h>
#include "ls.h"

int sortByName(const void* name1, const void* name2);

int sortFoldersByName(const void* name1, const void* name2);

int sortBySize(const void* item1, const void* item2);

//sort by modified time
int sortByMtime(const void* item1, const void* item2);

//sort by access time
int sortByAtime(const void* item1, const void* item2);

//sort by status change time
int sortByCtime(const void* item1, const void* item2);

void sortOutput(lsRequestedItem* folders, char* const flags);

//used for sorting names for table printing
int sortNames(const void* name1, const void* name2);

int sortLengths(const void* item1, const void* item2);