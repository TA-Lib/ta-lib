#ifndef TA_STRING_H
#define TA_STRING_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

/* This is a very simple reference counting for small CONSTANT string.
 * This allows to manage efficiently "copies" of a large number of strings.
 * The string will be freed when the last reference to it is deleted.
 *
 * Here is an example of use:
 *  TA_String *s1, *s2;
 *  TA_StringCache *cache;
 *
 *  TA_StringCacheAlloc( &cache );
 *
 *  s1 = TA_StringAlloc( cache, "ABC" ); <- Copy of "ABC" is alloc here.
 *  s2 = TA_StringDup( cache, s1 );    <- Ref counter is incremented here.
 *
 *  printf( "This will print ABC=%s\n", TA_StringToChar( s2 ) );
 *
 *  TA_StringFree( cache, s1 ); <- The reference count is decremented by 1.
 *  TA_StringFree( cache, s2 ); <- Last reference freed.
 *
 *  TA_StringCacheFree( cache );
 *
 * The cache mechanism allows to optimize memory allocation.
 * 
 * The TA_String guarantee safe multithread copy/manipulation.
 */

/* Important: The TA_String must stay INVISIBLE to the user of the TA-LIB.
 *            This "String Data Type" is tailored specifically for TA-LIB
 *            and not for external use.
 */

typedef const char *TA_String;
typedef unsigned int TA_StringCache; /* hidden implementation. */

TA_RetCode TA_StringCacheAlloc( TA_StringCache **newStringCache );
TA_RetCode TA_StringCacheFree( TA_StringCache *stringCacheToFree );

/* Create a TA_String from a normal C 'string'. */
TA_String *TA_StringAlloc( TA_StringCache *stringCache,
                           const char *string );

/* Same as TA_StringAlloc, except TA_String will be all uppercase. */
TA_String *TA_StringAlloc_UC( TA_StringCache *stringCache,
                              const char *string );


/* Like TA_String but truncate after 'n' character. */
TA_String *TA_StringAllocN( TA_StringCache *stringCache,
                            const char *string,
                            unsigned int maxNbChar );

/* Same as TA_StringAllocN, except TA_String will be all uppercase. */
TA_String *TA_StringAllocN_UC( TA_StringCache *stringCache,
                               const char *string,
                               unsigned int maxNbChar );

/* Create a TA_String from a normal C 'string'. Remove whitespaces in the copy. */
TA_String *TA_StringAllocTrim( TA_StringCache *stringCache,
                               const char *string );

/* Create a TA_String by concatenating a normal C 'string' with an integer value. */
TA_String *TA_StringValueAlloc( TA_StringCache *stringCache,
                                const char *string,
                                unsigned int value );

/* Create a TA_String from an unsigned long integer. */
TA_String *TA_StringAlloc_ULong( TA_StringCache *stringCache,
                                 unsigned long value );

/* Allocate a "path string": and make the appropriate adaptation depending
 * of the platform.
 */
TA_String *TA_StringAlloc_Path( TA_StringCache *stringCache, const char *string );

/* Free this copy of the 'string'. */
void TA_StringFree( TA_StringCache *stringCache, TA_String *string );

/* Make a duplicate of a string.
 * (Do not assume that a reference counting occured, sometimes a re-allocation
 *  and copy could be needed if the counter reaches the maximum).
 */
TA_String *TA_StringDup( TA_StringCache *stringCache, TA_String *string );

/* Allows to use a TA_String like a normal 'const char *'.
 * Never override the "const" modifier.
 *
 * Also, this is not a "Copy" so you must stop to refer to that
 * pointer if TA_StringFree is called and there is no other reference
 * left on that string....
 */
#define TA_StringToChar(x) (((const char * const)(x))+1)

/* A very particular macro that should not be used if you do
 * not understand PERFECTLY the TA_String implementation.
 * This is provided for some speed optimization.
 *
 * Basicaly, that macro allows to re-identify the TA_String
 * correponding to a 'const char *'. Of course that 'const char *'
 * must come from a valid TA_String...
 */
#define TA_StringFromChar(x) ((TA_String *)((const char * const)(x)-1))

#endif

