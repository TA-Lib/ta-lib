#ifndef TA_TOKEN_H
#define TA_TOKEN_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

typedef enum
{
   TA_TOK_FIX,  /* A fixed string. */

   TA_TOK_YYYY, /* 4 digits Year string. */
   TA_TOK_YY,   /* 2 digits Year string. */
   TA_TOK_Y,    /* Year string (delimitated integer) */

   TA_TOK_M,    /* Month string (delimitated integer) */
   TA_TOK_MM,   /* 2 digits Month string  */
   TA_TOK_MMM,  /* 3 letters Month string */

   TA_TOK_D,    /* Day string (delimitated integer) */
   TA_TOK_DD,   /* 2 digits Day string. */

   TA_TOK_CAT,    /* Category String.  */
   TA_TOK_CATC,   /* Category Country  */
   TA_TOK_CATX,   /* Category Exchange */
   TA_TOK_CATT,   /* Category Type     */

   TA_TOK_SYM,    /* Symbol string (wild). */
   TA_TOK_SYMF,   /* Symbol string (fixed). */

   TA_TOK_SEP,  /* Seperator string (used in directory path). */

   TA_TOK_WILD,      /* Any value string. This is the '*' symbol. */
   TA_TOK_WILD_CHAR, /* Any value character. This is the '?' symbol. */

   TA_TOK_OPEN,
   TA_TOK_HIGH,
   TA_TOK_LOW,
   TA_TOK_CLOSE,
   TA_TOK_VOLUME,
   TA_TOK_OPENINTEREST,

   TA_TOK_HOUR,
   TA_TOK_MIN,
   TA_TOK_SEC,

   TA_TOK_HH, /* 2 digit hour string. */
   TA_TOK_MN, /* 2 digit minute string. */
   TA_TOK_SS, /* 2 digit second string. */

   TA_TOK_SKIP_N_REAL,
   TA_TOK_SKIP_N_INTEGER,
   TA_TOK_SKIP_N_CHAR,
   TA_TOK_SKIP_N_HEADER_LINE,

   TA_TOK_END,  /* Indicates no more token available. */

   TA_NB_TOKEN_ID,

   TA_INVALID_TOKEN_ID = -1
} TA_TokenId;

/* The following will return the number of character a token
 * can take maximum. If this is an unlimited size, Zero is returned.
 */
unsigned int TA_TokenMaxSize( TA_Libc *libHandle, TA_TokenId id );

/* Return the string corresponding to the specified TA_TokenId.
 * That string is the one used within the '[' and ']'.
 */
const char *TA_TokenString( TA_Libc *libHandle, TA_TokenId id );

/* Return a string corresponding to a TA_TokenId.
 * That string can be used to display debug information.
 */
const char *TA_TokenDebugString( TA_Libc *libHandle, TA_TokenId id );

#endif
