#ifndef TA_FILEINDEX_H
#define TA_FILEINDEX_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

/* This utility allows to build an index of files using a path containing
 * some wildcard patterns.
 *
 * The path patterns can contains '?' and '*', but also some special fields
 * like [S],[YYYY] etc... the value of these fields are going to be
 * extracted for each applicable path/file.
 *
 * Simplified Example:
 *  c:\test\*\[s].txt
 *  This will build an index of all .txt files in a subdirectory inside test.
 *  Further more, the section before the .txt section will be extracted to
 *  become the "Symbol" fields.
 *
 *  Possible good value for this path would be:
 *  c:\test\nasdaq\ABC.txt   -> where [s]="ABC"
 *  c:\test\nasdaq\a.txt     -> where [s]="a"
 *  c:\test\amex\a.txt       -> where [s]="a"
 *
 * Once the FileIndex is allocated, it is possible to iterate between all
 * the symbols and categories. At each iteration, a TA_FileInfo is returned.
 * From TA_FileInfo can be obtained the extracted "fields" value and the
 * complete path of the file itself.
 */


/* Functions to build the index of files. */
typedef unsigned int TA_FileIndex; /* Implementation hidden. */

TA_RetCode TA_FileIndexAlloc( TA_String *path,
                              TA_String *initialCategory,
                              TA_String *initialCategoryCountry,
                              TA_String *initialCategoryExchange,
                              TA_String *initialCategoryType,
                              TA_FileIndex **newIndex );

TA_RetCode TA_FileIndexFree( TA_FileIndex *toBeFreed );

/* Return the number of category in this index. */
unsigned int TA_FileIndexNbCategory( TA_FileIndex *fileIndex );

/* Function to iterate sequentially within the index.
 * Only one iterator per TA_FileIndex.
 * When iterating symbol, it is iterating only in the current
 * active category.
 *
 * The active directory is determined by the TA_FileIndexFirstCategory,
 * TA_FileIndexNextCategory and TA_FileIndexSelectCategory.
 */
typedef unsigned int TA_FileInfo; /* Implementation hidden. */

TA_String *TA_FileIndexFirstCategory( TA_FileIndex *fileIndex );
TA_String *TA_FileIndexNextCategory ( TA_FileIndex *fileIndex );

TA_RetCode TA_FileIndexSelectCategory( TA_FileIndex *fileIndex,
                                       TA_String *category );

TA_FileInfo *TA_FileIndexFirstSymbol( TA_FileIndex *fileIndex );
TA_FileInfo *TA_FileIndexNextSymbol ( TA_FileIndex *fileIndex );

/* Return the number of symbol for the current category. */
unsigned int TA_FileIndexNbSymbol( TA_FileIndex *fileIndex );

/* Function to extract information from a TA_FileInfo. */
TA_String   *TA_FileInfoSymbol  ( TA_FileInfo *fileInfo );
TA_String   *TA_FileInfoCategory( TA_FileInfo *fileInfo );

const char *TA_FileInfoPath( TA_FileInfo *fileInfo );

/* Note 1: The pointer returned by TA_FileInfoPath will stay valid until
 *         the next call to any functions defined in this header file.
 *         If you need the pointer beyhond this timeframe, the caller will
 *         need to keep a copy of the pointed data.
 *
 * Note 2: TA_FileInfoSymbol and TA_FileInfoCategory will ALWAYS returned
 *         a string since each symbol will always fall into one category.
 *
 * Note 3: If many files are found for the same extracted symbol string, the
 *         index keep track of only the first file.
 */

#endif

