#ifndef TA_FILEINDEX_PRIV_H
#define TA_FILEINDEX_PRIV_H

/* Contains prototypes/definitions that must be used only privately by the
 * TA_FileIndex module.
 */
#ifndef TA_FILEINDEX_H
   #include "ta_fileindex.h"
#endif

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_TOKEN_H
   #include "ta_token.h"
#endif

typedef struct
{
   TA_String *value;
   TA_TokenId id;
} TA_TokenInfo;

typedef struct
{
   void *parent;
   TA_String *string;
   TA_List *child;
} TA_ValueTreeNode;

typedef struct
{
   TA_List *listCategory;     /* List of TA_FileIndexCategoryData */

   TA_List *listLocationToken;    /* TA_TokenInfo parsed from path. */

   TA_ValueTreeNode *root;    /* A tree containing the field value of some
                               * of the fields found in the listLocationToken.
                               */

   /* Copied at the creation of this struct. */
   TA_String *initialCategoryString; 
   TA_String *initialCategoryCountryString; 
   TA_String *initialCategoryExchangeString; 
   TA_String *initialCategoryTypeString; 

   /* Will point on constant strings used to access file systems with
    * wildcards. File system dependant.
    */
   TA_String *wildOneChar;
   TA_String *wildZeroOrMoreChar;
   TA_String *wildOneOrMoreChar;

   /* Large buffer used for temporary usage when processing
    * directories string.
    * Will be allocated with a size of TA_SOURCELOCATION_MAX_LENGTH.
    */
   char *scratchPad;

   /* Variables used to iterate within the 'listLocationToken'.
    * Only one iterator at the time.
    * These variables are mainly controled by:
    *   TA_FileIndexMoveToDepth
    *   TA_FileIndexMoveToNextToken
    *   TA_FileIndexMoveToPrevToken
    * This stuff is really not the best design in town...
    */
   unsigned int  curDirectionForward;
   unsigned int  curTokenDepth;
   TA_TokenInfo *curToken;
   TA_TokenInfo *nextToken;
   TA_TokenInfo *prevToken;

   /* Use to make our life easier while building the index.
    * Used also to iterate within the build index.
    */
   TA_String *currentSymbolString;

   /* The string for the TA_TOK_CAT token. */
   TA_String *currentCategoryString;

   /* The string for the TA_TOK_CATC,
    * TA_TOK_CATE, TA_TOK_CATT token.
    */
   TA_String *currentCategoryCountryString;
   TA_String *currentCategoryExchangeString;
   TA_String *currentCategoryTypeString;

   /* 'currentNode' identify where the next node can be added in the
    * value tree (Starting at root). Used only while building the index.
    */
   TA_ValueTreeNode *currentNode;
} TA_FileIndexPriv;

typedef struct
{
   TA_FileIndexPriv *parent;

   TA_String *string;    /* String for this category. Can be NULL. */
   TA_List *listSymbol;  /* List of TA_FileIndexSymbolData */
} TA_FileIndexCategoryData;

typedef struct
{
   TA_FileIndexCategoryData *parent;

   TA_String *string;

   /* Point to the last node in the "value tree" defining the fields for
    * correctly retreiving this symbol.
    * These fields are used in particular for:
    *  - re-building the path to access the file.
    *  - When applicable, do some sanity check for the symbol and
    *    category values.
    *
    * Note: the node belongs to the "value tree", consequently this pointer
    *       should not be used to free the node...
    */
   TA_ValueTreeNode *node;
} TA_FileIndexSymbolData;


/* Allocate/de-allocate TA_FileIndexPriv. */
TA_FileIndexPriv *TA_FileIndexPrivAlloc( TA_String *initialCategory,
                                         TA_String *initialCategoryCountry,
                                         TA_String *initialCategoryExchange,
                                         TA_String *initialCategoryType );

TA_RetCode TA_FileIndexPrivFree( TA_FileIndexPriv *toBeFreed );

/* The following allows to add elements to
 * the handle->opaqueData (this is the TA_PrivateHandle)
 *
 * Notice that there is no way to "remove" these elements
 * individually. If something wrong happen, 'freeHandle'
 * shall be called and all the related memory is freed!
 *
 * It is based on the principle that creating a new handle either
 * totaly succeed or totaly fail.
 */

TA_RetCode TA_FileIndexAddCategoryData( TA_FileIndexPriv *data,
                                        TA_String *stringCategory,
                                        TA_FileIndexCategoryData **added );

TA_RetCode TA_FileIndexAddTokenInfo(  TA_FileIndexPriv *data,
                                      TA_TokenId id,
                                      TA_String *value,
                                      TA_TokenInfo *optBefore );

TA_RetCode TA_FileIndexAddSymbolData( TA_FileIndexCategoryData *categoryData,
                                      TA_String *stringSymbol,
                                      TA_ValueTreeNode *treeNodeValue,
                                      TA_FileIndexSymbolData **added );

/* The following served to build the "value tree". The best way to destroy
 * this tree is by deleting the whole TA_FileIndexPriv...
 * On the other side, a utility function TA_FileIndexFreeValueTree is provided
 * for detroying part of the tree starting to specific node (used specifically
 * while processing the directories and a directory is found empty... all
 * correponding fields value needs to be deleted in the "value tree").
 */
TA_RetCode TA_FileIndexAddTreeValue( TA_FileIndexPriv *data,
                                     TA_String *string,
                                     TA_ValueTreeNode **added );

TA_RetCode TA_FileIndexFreeValueTree( TA_ValueTreeNode *fromNode );

/* All the time that the "Value tree" exist in the TA_FileIndexPriv, there
 * is one node designated as the currentNode.
 * Value added by TA_FileIndexAddTreeValue are added as child of the current node.
 * The newly added value become the new current node.
 *
 * The function TA_FileIndexSetCurrentTreeValueNode allows to change the current
 * node. By default, the current node start as the root of the "value tree".
 */
TA_RetCode TA_FileIndexSetCurrentTreeValueNode( TA_FileIndexPriv *data,
                                                TA_ValueTreeNode *node );

TA_ValueTreeNode *TA_FileIndexGetCurrentTreeValueNode( TA_FileIndexPriv *data );

/* Two very limited function for walking up/dowm the tree.
 * TA_FileIndexGoDownTreeValue use the first child for going down.
 * The "current node" is adjusted as we go up/down.
 */
TA_ValueTreeNode *TA_FileIndexGoDownTreeValue( TA_FileIndexPriv *data );
TA_ValueTreeNode *TA_FileIndexGoUpTreeValue( TA_FileIndexPriv *data );

/* Change the value of a particular TA_ValueTreeNode */
TA_RetCode TA_FileIndexChangeValueTreeNodeValue( TA_ValueTreeNode *nodeToChange,
                                                 TA_String *newValue );

/* Note: When TA_FileIndexAddSymbolData or TA_FileIndexAddCategoryData are
 *       called with an already existing symbol or category, the existing entity
 *       is returned.
 */

/* Utility functions. */
TA_RetCode TA_FileIndexMoveToDepth( TA_FileIndexPriv *data, unsigned int depth );
TA_RetCode TA_FileIndexMoveToNextToken( TA_FileIndexPriv *data );
TA_RetCode TA_FileIndexMoveToPrevToken( TA_FileIndexPriv *data );
unsigned int TA_FileIndexIdentifyFileDepth( TA_FileIndexPriv *data );

/* This function will transform into tokens the pattern path.
 * The tokens are put in the TA_FileIndexPriv->listLocationToken
 */
TA_RetCode TA_FileIndexParsePath( TA_FileIndexPriv *fileIndexPriv,
                                  TA_String *path );

/* Build (or re-build) the complete index of exchange and symbol by using the
 * TA_FileIndexPriv->listLocationToken.
 */
TA_RetCode TA_FileIndexBuildIndex( TA_FileIndexPriv *fileIndexPriv );

/* Re-build the path to access the file correponding to the last
 * value of a branch (leaf) in the value tree.
 */
TA_RetCode TA_FileIndexMakePathPattern( TA_FileIndexPriv *fileIndexPriv,
                                        TA_ValueTreeNode *node,
                                        char *bufferToUse,
                                        int maxBufferSize );


#endif

