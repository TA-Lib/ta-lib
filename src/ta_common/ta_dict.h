#ifndef TA_DICT_H
#define TA_DICT_H

/* This module makes abstraction of a "dictionary" container.
 *
 * The dictionary simply store key-value pair in alphabetical order.
 *
 * The 'key' is a NULL terminated string.
 * The 'value' is an opaque 'void *' provided by the
 * user of the dictionary.
 *
 * This module offer 6 basic operations:
 *        - Create a dictionary.
 *          A 'freeValueFunc' function could be provided when the dictionary
 *          is created. This free function will be called for each 'value'
 *          being deleted. If no de-allocator function is needed, just
 *          initialize 'freeValueFunc' to NULL.
 *
 *        - Add a key-value pair to a dictionary.
 *
 *        - Delete a key-value pair. The 'value' is deleted with
 *          the 'freeValueFunc' function provided at initialization.
 *
 *        - Lookup a 'key' for getting the corresponding 'value'.
 *
 *        - Access the dictionary sequentially in alphabetical order of key.
 *
 *        - Destroy a dictionary and all its 'key-value' pair. The 'value' is
 *          deleted with the 'freeValueFunc' provided at initialization.
 *
 * Distinctive type of key are supported. Only one type can be used with
 * a given dictionary. The type of key is specified when the dictionary
 * is created with the 'flag' parameter:
 *    TA_DICT_KEY_ONE_STRING
 *          A single string is the key.
 *          Use function with the "_S" suffix only.
 *
 *    TA_DICT_KEY_TWO_STRING
 *          Two string for together the key.
 *          Use function with the "_S2" suffix only.
 *
 *    TA_DICT_KEY_INTEGER
 *          A signle integer is the key.
 *          Use function with the "_I" suffix only.
 * 
 * Note: When adding a key-value, if the key is already existing, the
 *       older key-value pair is deleted. Only the last added key-value pair
 *       is kept. Of course, the 'freeValueFunc' will be called for the deleted
 *       key-value pair.
 *
 * Note: This module is not multithread safe, and the user must provide
 *       adequate protection in a multithread environment.
 *
 */
#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

typedef struct
{
   /* Implementation is hidden. */
   void *hiddenData;
} TA_Dict;

/* Flags when creating the dictionary. */
#define TA_DICT_KEY_ONE_STRING (0x00000001)
#define TA_DICT_KEY_TWO_STRING (0x00000002)
#define TA_DICT_KEY_INTEGER    (0x00000004)

/* Create an empty dictionnary.
 *
 * Whenever a key-value pair is deleted, the 'freeValueFunc'
 * will be called for the 'value'.
 *
 * Set 'freeValueFunc' to NULL if that functionality is not needed.
 *
 * Return: An handle used to manipulate the dictionary or NULL if
 *         there is an allocation error.
 */
TA_Dict *TA_DictAlloc( unsigned int flags, void (*freeValueFunc)(void *) );

/* Free all ressource related to a dictionary.
 *
 * Each element in the dictionary will be deleted
 * with the 'freeValueFunc' (if it was set at initialization).
 *
 * Return: TA_SUCCESS or TA_BAD_PARAM.
 */
TA_RetCode TA_DictFree( TA_Dict *dict );

/* Return the number of key-value pair in the
 * dictionary.
 * Return: If an error occured, return 0 by default.
 *         But returing zero does not mean an error,
 *         since an empty dict would return 0 also.
 */
unsigned int TA_DictSize( TA_Dict *dict );

/* Add a key-value pair to a dictionary.
 *
 * All 'value' are valid except NULL.
 *
 * Return: TA_SUCCESS, TA_BAD_PARAM or TA_ALLOC_ERR.
 */
/* For TA_DICT_KEY_ONE_STRING */
TA_RetCode TA_DictAddPair_S ( TA_Dict   *dict,
                              TA_String *key,
                              void      *value );

/* For TA_DICT_KEY_TWO_STRING */
TA_RetCode TA_DictAddPair_S2( TA_Dict   *dict,
                              TA_String *key1,
                              TA_String *key2,
                              void      *value );

/* For TA_DICT_KEY_INTEGER */
TA_RetCode TA_DictAddPair_I ( TA_Dict *dict,
                              int      key,
                              void    *value );

/* Delete a key-value pair from a dictionary.
 *
 * Whenever an entry is deleted, the 'freeValueFunc'
 * will be called for the 'value'.
 *
 * Return: TA_SUCCESS, TA_BAD_PARAM or TA_KEY_NOT_FOUND.
 */
/* For TA_DICT_KEY_ONE_STRING */
TA_RetCode TA_DictDeletePair_S ( TA_Dict *dict, const char *key );

/* For TA_DICT_KEY_TWO_STRING */
TA_RetCode TA_DictDeletePair_S2( TA_Dict *dict, const char *key1, const char *key2 );

/* For TA_DICT_KEY_INTEGER */
TA_RetCode TA_DictDeletePair_I ( TA_Dict *dict, int key );

/* Return the 'value' corresponding to a 'key'.
 * Will return NULL if not found.
 */
/* For TA_DICT_KEY_ONE_STRING */
void *TA_DictGetValue_S ( TA_Dict *dict, const char *key );

/* For TA_DICT_KEY_TWO_STRING */
void *TA_DictGetValue_S2( TA_Dict *dict, const char *key1, const char *key2 );

/* For TA_DICT_KEY_INTEGER */
void *TA_DictGetValue_I ( TA_Dict *dict, int key );

/* Access in alphabetical order all the entry in a dictionary.
 *
 * It is assume that the dictionary is not modified while you are
 * sequentially accessing it.
 * If the dictionary is modified WHILE an access, you will
 * have to start over with the sequential access.
 *
 * Here is a code snippet printing all the content of 'dict':
 * (Assuming the 'value' in 'dict' are pointer to string).
 *
 * void printContent( TA_Dict *dict )
 * {
 *    int nbElementToAccess;
 *    nbElementToAccess = TA_DictAccessFirst( dict );
 *
 *    if( nbElementToAccess == 0 )
 *       printf( "Dictionary is empty!\n" );
 *    else
 *    {
 *       do
 *       {
 *          printf( "key=%s value=%s\n", TA_DictAccessKey( dict ),
 *                           (const char *)TA_DictAccessValue( dict ) );
 *
 *       } while( TA_DictAccessNext( dict ) );
 *    }
 * }
 *
 */

/* Initiate the sequential access.
 *
 * Return:  0 if the dictionary is empty else return the number
 *          of element in the dictionary.
 */

int TA_DictAccessFirst( TA_Dict *dict );

/* Move to the next key-value pair.
 * That next key-value pair becomes the 'actual' pair that can be
 * accessed with TA_DictGetKey() and TA_DictGetValue().
 *
 * Return:  0 if no other key-value pair are available else
 *          return the number of element available.
 */
int TA_DictAccessNext( TA_Dict *dict );

/* Allows to access the 'actual' key-value pair. */

/* Return the 'key'.  
 * You must use the TA_String while the key-value pair
 * is still in the dictionary. If you wish to use that
 * key beyond that point, make sure to make a copy
 * with TA_StringDup.
 */
TA_String *TA_DictAccessKey( TA_Dict *dict );

/* Return the 'value'. You must use that value prior to any
 * other dictionary operation. You must keep in mind that this
 * 'value' can get change if another pair is added with the
 * same 'key'.
 *
 * If you expect to use that 'value' on a longer term, you
 * should make a complete copy of it.
 */
void *TA_DictAccessValue( TA_Dict *dict );

/* Move to the next key-value pair.
 * That next key-value pair becomes the 'actual' pair that can be
 * accessed with TA_DictAccessKey_XX() and TA_DictAccessValue().
 *
 * Return:  1 if a new key-value pair is now accessible.
 *          0 if no other key-value pair are available.
 */
int TA_DictAccessNext( TA_Dict *dict );

#endif

