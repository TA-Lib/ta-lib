/*************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 1998 Bjorn Reese and Daniel Stenberg.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 *************************************************************************
 *
 * A note to trio contributors:
 *
 * Avoid heap allocation at all costs to ensure that the trio functions
 * are async-safe. The exceptions are the printf/fprintf functions, which
 * uses fputc, and the asprintf functions and the <alloc> modifier, which
 * by design are required to allocate form the heap.
 *
 ************************************************************************/

/*
 * TODO:
 *  - Scan is probably too permissive about its modifiers.
 *  - C escapes in %#[] ?
 *  - C99 support has not been properly tested.
 *  - Multibyte characters (done for format parsing, except scan groups)
 *  - Complex numbers? (C99 _Complex)
 *  - Boolean values? (C99 _Bool)
 *  - C99 NaN(n-char-sequence) missing
 *  - Should we support the GNU %a alloc modifier? GNU has an ugly hack
 *    for %a, because C99 used %a for other purposes. If specified as
 *    %as or %a[ it is interpreted as the alloc modifier, otherwise as
 *    the C99 hex-float. This means that you cannot scan %as as a hex-float
 *    immediately followed by an 's'.
 *  - Scanning of collating symbols.
 */

static const char rcsid[] = "@(#)$Id$";

/*************************************************************************
 * Trio include files
 */
#include "triodef.h"
#include "trio.h"
#include "triop.h"
#include "trionan.h"
#include "strio.h"

/*
 * Encode the error code and the position. This is decoded
 * with TRIO_ERROR_CODE and TRIO_ERROR_POSITION.
 */
#if TRIO_ERRORS
# define TRIO_ERROR_RETURN(x,y) (- ((x) + ((y) << 8)))
#else
# define TRIO_ERROR_RETURN(x,y) (-1)
#endif

#if defined(__STDC_ISO_10646__) || defined(MB_LEN_MAX) || defined(USE_MULTIBYTE) || TRIO_WIDECHAR
# define TRIO_COMPILER_SUPPORTS_MULTIBYTE
# if !defined(MB_LEN_MAX)
#  define MB_LEN_MAX 6
# endif
#endif


/*************************************************************************
 * Generic definitions
 */

#if !(defined(DEBUG) || defined(NDEBUG))
# define NDEBUG
#endif
#include <assert.h>
#include <ctype.h>
#if !defined(TRIO_COMPILER_SUPPORTS_C99)
# define isblank(x) (((x)==32) || ((x)==9))
#endif
#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>

#ifndef NULL
# define NULL 0
#endif
#define NIL ((char)0)
#ifndef FALSE
# define FALSE (1 == 0)
# define TRUE (! FALSE)
#endif
#define BOOLEAN_T int

/* mincore() can be used for debugging purposes */
#define VALID(x) (NULL != (x))

/* xlC crashes on log10(0) */
#define guarded_log10(x) (((x) == 0.0) ? trio_ninf() : log10(x))
#define guarded_log16(x) (guarded_log10(x) / log10(16.0))


/*************************************************************************
 * Platform specific definitions
 */
#if defined(TRIO_PLATFORM_UNIX)
# include <unistd.h>
# include <signal.h>
# include <locale.h>
# define USE_LOCALE
#endif /* TRIO_PLATFORM_UNIX */
#if defined(TRIO_PLATFORM_WIN32)
# include <io.h>
# define read _read
# define write _write
#endif /* TRIO_PLATFORM_WIN32 */

#define TRIO_MSVC_VERSION_5 1100

#if TRIO_WIDECHAR
# if defined(TRIO_COMPILER_SUPPORTS_ISO94)
#  include <wchar.h>
#  include <wctype.h>
# else
typedef char wchar_t;
typedef int wint_t;
#  define WCONST(x) L ## x
#  define WEOF EOF
#  define iswalnum(x) isalnum(x)
#  define iswalpha(x) isalpha(x)
#  define iswblank(x) isblank(x)
#  define iswcntrl(x) iscntrl(x)
#  define iswdigit(x) isdigit(x)
#  define iswgraph(x) isgraph(x)
#  define iswlower(x) islower(x)
#  define iswprint(x) isprint(x)
#  define iswpunct(x) ispunct(x)
#  define iswspace(x) isspace(x)
#  define iswupper(x) isupper(x)
#  define iswxdigit(x) isxdigit(x)
# endif
#endif


/*************************************************************************
 * Compiler dependent definitions
 */

/* Support for long long */
#ifndef __cplusplus
# if !defined(USE_LONGLONG)
#  if defined(TRIO_COMPILER_GCC) && !defined(__STRICT_ANSI__)
#   define USE_LONGLONG
#  elif defined(TRIO_COMPILER_SUNPRO)
#   define USE_LONGLONG
#  elif defined(_LONG_LONG) || defined(_LONGLONG)
#   define USE_LONGLONG
#  endif
# endif
#endif

/* The extra long numbers */
#if defined(USE_LONGLONG)
typedef signed long long int trio_longlong_t;
typedef unsigned long long int trio_ulonglong_t;
#elif defined(TRIO_COMPILER_MSVC)
# if (_MSC_VER >= TRIO_MSVC_VERSION_5)
typedef signed __int64 trio_longlong_t;
typedef unsigned __int64 trio_ulonglong_t;
# else
typedef signed long int trio_longlong_t;
typedef unsigned long int trio_ulonglong_t;
# endif
#else
typedef signed long int trio_longlong_t;
typedef unsigned long int trio_ulonglong_t;
#endif

/* Maximal and fixed integer types */
#if defined(TRIO_COMPILER_SUPPORTS_C99)
# include <stdint.h>
typedef intmax_t trio_intmax_t;
typedef uintmax_t trio_uintmax_t;
typedef int8_t trio_int8_t;
typedef int16_t trio_int16_t;
typedef int32_t trio_int32_t;
typedef int64_t trio_int64_t;
#elif defined(TRIO_COMPILER_SUPPORTS_UNIX98)
# include <inttypes.h>
typedef intmax_t trio_intmax_t;
typedef uintmax_t trio_uintmax_t;
typedef int8_t trio_int8_t;
typedef int16_t trio_int16_t;
typedef int32_t trio_int32_t;
typedef int64_t trio_int64_t;
#elif defined(TRIO_COMPILER_MSVC) && (_MSC_VER >= TRIO_MSVC_VERSION_5)
typedef trio_longlong_t trio_intmax_t;
typedef trio_ulonglong_t trio_uintmax_t;
typedef __int8 trio_int8_t;
typedef __int16 trio_int16_t;
typedef __int32 trio_int32_t;
typedef __int64 trio_int64_t;
#else
typedef trio_longlong_t trio_intmax_t;
typedef trio_ulonglong_t trio_uintmax_t;
# if defined(TRIO_INT8_T)
typedef TRIO_INT8_T trio_int8_t;
# else
typedef signed char trio_int8_t;
# endif
# if defined(TRIO_INT16_T)
typedef TRIO_INT16_T trio_int16_t;
# else
typedef signed short trio_int16_t;
# endif
# if defined(TRIO_INT32_T)
typedef TRIO_INT32_T trio_int32_t;
# else
typedef signed int trio_int32_t;
# endif
# if defined(TRIO_INT64_T)
typedef TRIO_INT64_T trio_int64_t;
# else
typedef trio_longlong_t trio_int64_t;
# endif
#endif


/*************************************************************************
 * Internal definitions
 */

/* Long double sizes */
#ifdef LDBL_DIG
# define MAX_MANTISSA_DIGITS LDBL_DIG
# define MAX_EXPONENT_DIGITS 4
#else
# define MAX_MANTISSA_DIGITS DBL_DIG
# define MAX_EXPONENT_DIGITS 3
#endif

/* The maximal number of digits is for base 2 */
#define MAX_CHARS_IN(x) (sizeof(x) * CHAR_BIT)
/* The width of a pointer. The number of bits in a hex digit is 4 */
#define POINTER_WIDTH ((sizeof("0x") - 1) + sizeof(void *) * CHAR_BIT / 4)

/* Infinite and Not-A-Number for floating-point */
#define INFINITE_LOWER "inf"
#define INFINITE_UPPER "INF"
#define LONG_INFINITE_LOWER "infinite"
#define LONG_INFINITE_UPPER "INFINITE"
#define NAN_LOWER "nan"
#define NAN_UPPER "NAN"

/* Various constants */
enum {
  TYPE_PRINT = 1,
  TYPE_SCAN  = 2,

  /* Flags. Use maximum 32 */
  FLAGS_NEW                 = 0,
  FLAGS_STICKY              = 1,
  FLAGS_SPACE               = 2 * FLAGS_STICKY,
  FLAGS_SHOWSIGN            = 2 * FLAGS_SPACE,
  FLAGS_LEFTADJUST          = 2 * FLAGS_SHOWSIGN,
  FLAGS_ALTERNATIVE         = 2 * FLAGS_LEFTADJUST,
  FLAGS_SHORT               = 2 * FLAGS_ALTERNATIVE,
  FLAGS_SHORTSHORT          = 2 * FLAGS_SHORT,
  FLAGS_LONG                = 2 * FLAGS_SHORTSHORT,
  FLAGS_QUAD                = 2 * FLAGS_LONG,
  FLAGS_LONGDOUBLE          = 2 * FLAGS_QUAD,
  FLAGS_SIZE_T              = 2 * FLAGS_LONGDOUBLE,
  FLAGS_PTRDIFF_T           = 2 * FLAGS_SIZE_T,
  FLAGS_INTMAX_T            = 2 * FLAGS_PTRDIFF_T,
  FLAGS_NILPADDING          = 2 * FLAGS_INTMAX_T,
  FLAGS_UNSIGNED            = 2 * FLAGS_NILPADDING,
  FLAGS_UPPER               = 2 * FLAGS_UNSIGNED,
  FLAGS_WIDTH               = 2 * FLAGS_UPPER,
  FLAGS_WIDTH_PARAMETER     = 2 * FLAGS_WIDTH,
  FLAGS_PRECISION           = 2 * FLAGS_WIDTH_PARAMETER,
  FLAGS_PRECISION_PARAMETER = 2 * FLAGS_PRECISION,
  FLAGS_BASE                = 2 * FLAGS_PRECISION_PARAMETER,
  FLAGS_BASE_PARAMETER      = 2 * FLAGS_BASE,
  FLAGS_FLOAT_E             = 2 * FLAGS_BASE_PARAMETER,
  FLAGS_FLOAT_G             = 2 * FLAGS_FLOAT_E,
  FLAGS_QUOTE               = 2 * FLAGS_FLOAT_G,
  FLAGS_WIDECHAR            = 2 * FLAGS_QUOTE,
  FLAGS_ALLOC               = 2 * FLAGS_WIDECHAR,
  FLAGS_IGNORE              = 2 * FLAGS_ALLOC,
  FLAGS_IGNORE_PARAMETER    = 2 * FLAGS_IGNORE,
  FLAGS_VARSIZE_PARAMETER   = 2 * FLAGS_IGNORE_PARAMETER,
  FLAGS_FIXED_SIZE          = 2 * FLAGS_VARSIZE_PARAMETER,
  /* Reused flags */
  FLAGS_EXCLUDE             = FLAGS_SHORT,
  FLAGS_USER_DEFINED        = FLAGS_IGNORE,
  /* Compounded flags */
  FLAGS_ALL_VARSIZES        = FLAGS_LONG | FLAGS_QUAD | FLAGS_INTMAX_T | FLAGS_PTRDIFF_T | FLAGS_SIZE_T,
  FLAGS_ALL_SIZES           = FLAGS_ALL_VARSIZES | FLAGS_SHORTSHORT | FLAGS_SHORT,

  NO_POSITION  = -1,
  NO_WIDTH     =  0,
  NO_PRECISION = -1,
  NO_SIZE      = -1,

  NO_BASE      = -1,
  MIN_BASE     =  2,
  MAX_BASE     = 36,
  BASE_BINARY  =  2,
  BASE_OCTAL   =  8,
  BASE_DECIMAL = 10,
  BASE_HEX     = 16,

  /* Maximal number of allowed parameters */
  MAX_PARAMETERS = 64,
  /* Maximal number of characters in class */
  MAX_CHARACTER_CLASS = UCHAR_MAX + 1,

  /* Maximal string lengths for user-defined specifiers */
  MAX_USER_NAME = 64,
  MAX_USER_DATA = 256,
  
  /* Maximal length of locale separator strings */
  MAX_LOCALE_SEPARATOR_LENGTH = MB_LEN_MAX,
  /* Maximal number of integers in grouping */
  MAX_LOCALE_GROUPS = 64
};

#define NO_GROUPING ((int)CHAR_MAX)

/* Fundamental formatting parameter types */
#define FORMAT_UNKNOWN   0
#define FORMAT_INT       1
#define FORMAT_DOUBLE    2
#define FORMAT_CHAR      3
#define FORMAT_STRING    4
#define FORMAT_POINTER   5
#define FORMAT_COUNT     6
#define FORMAT_PARAMETER 7
#define FORMAT_GROUP     8
#if TRIO_GNU
# define FORMAT_ERRNO    9
#endif
#if TRIO_EXTENSION
# define FORMAT_USER_DEFINED 10
#endif

/* Character constants */
#define CHAR_IDENTIFIER '%'
#define CHAR_BACKSLASH '\\'
#define CHAR_QUOTE '\"'
#define CHAR_ADJUST ' '

/* Character class expressions */
#define CLASS_ALNUM ":alnum:"
#define CLASS_ALPHA ":alpha:"
#define CLASS_BLANK ":blank:"
#define CLASS_CNTRL ":cntrl:"
#define CLASS_DIGIT ":digit:"
#define CLASS_GRAPH ":graph:"
#define CLASS_LOWER ":lower:"
#define CLASS_PRINT ":print:"
#define CLASS_PUNCT ":punct:"
#define CLASS_SPACE ":space:"
#define CLASS_UPPER ":upper:"
#define CLASS_XDIGIT ":xdigit:"

/*
 * SPECIFIERS:
 *
 *
 * a  Hex-float
 * A  Hex-float
 * c  Character
 * C  Widechar character (wint_t)
 * d  Decimal
 * e  Float
 * E  Float
 * F  Float
 * F  Float
 * g  Float
 * G  Float
 * i  Integer
 * m  Error message
 * n  Count
 * o  Octal
 * p  Pointer
 * s  String
 * S  Widechar string (wchar_t *)
 * u  Unsigned
 * x  Hex
 * X  Hex
 * [] Group
 * <> User-defined
 *
 * Reserved:
 *
 * D  Binary Coded Decimal %D(length,precision) (OS/390)
 */
#define SPECIFIER_CHAR 'c'
#define SPECIFIER_STRING 's'
#define SPECIFIER_DECIMAL 'd'
#define SPECIFIER_INTEGER 'i'
#define SPECIFIER_UNSIGNED 'u'
#define SPECIFIER_OCTAL 'o'
#define SPECIFIER_HEX 'x'
#define SPECIFIER_HEX_UPPER 'X'
#define SPECIFIER_FLOAT_E 'e'
#define SPECIFIER_FLOAT_E_UPPER 'E'
#define SPECIFIER_FLOAT_F 'f'
#define SPECIFIER_FLOAT_F_UPPER 'F'
#define SPECIFIER_FLOAT_G 'g'
#define SPECIFIER_FLOAT_G_UPPER 'G'
#define SPECIFIER_POINTER 'p'
#define SPECIFIER_GROUP '['
#define SPECIFIER_UNGROUP ']'
#define SPECIFIER_COUNT 'n'
#if TRIO_UNIX98
# define SPECIFIER_CHAR_UPPER 'C'
# define SPECIFIER_STRING_UPPER 'S'
#endif
#if TRIO_C99
# define SPECIFIER_HEXFLOAT 'a'
# define SPECIFIER_HEXFLOAT_UPPER 'A'
#endif
#if TRIO_GNU
# define SPECIFIER_ERRNO 'm'
#endif
#if TRIO_EXTENSION
# define SPECIFIER_BINARY 'b'
# define SPECIFIER_BINARY_UPPER 'B'
# define SPECIFIER_USER_DEFINED_BEGIN '<'
# define SPECIFIER_USER_DEFINED_END '>'
# define SPECIFIER_USER_DEFINED_SEPARATOR ':'
#endif

/*
 * QUALIFIERS:
 *
 *
 * Numbers = d,i,o,u,x,X
 * Float = a,A,e,E,f,F,g,G
 * String = s
 * Char = c
 *
 *
 * 9$ Position
 *      Use the 9th parameter. 9 can be any number between 1 and
 *      the maximal argument
 *
 * 9 Width
 *      Set width to 9. 9 can be any number, but must not be postfixed
 *      by '$'
 *
 * h  Short
 *    Numbers:
 *      (unsigned) short int
 *
 * hh Short short
 *    Numbers:
 *      (unsigned) char
 *
 * l  Long
 *    Numbers:
 *      (unsigned) long int
 *    String:
 *      as the S specifier
 *    Char:
 *      as the C specifier
 *
 * ll Long Long
 *    Numbers:
 *      (unsigned) long long int
 *
 * L  Long Double
 *    Float
 *      long double
 *
 * #  Alternative
 *    Float:
 *      Decimal-point is always present
 *    String:
 *      non-printable characters are handled as \number
 *
 *    Spacing
 *
 * +  Sign
 *
 * -  Alignment
 *
 * .  Precision
 *
 * *  Parameter
 *    print: use parameter
 *    scan: no parameter (ignore)
 *
 * q  Quad
 *
 * Z  size_t
 *
 * w  Widechar
 *
 * '  Thousands/quote
 *    Numbers:
 *      Integer part grouped in thousands
 *    Binary numbers:
 *      Number grouped in nibbles (4 bits)
 *    String:
 *      Quoted string
 *
 * j  intmax_t
 * t  prtdiff_t
 * z  size_t
 *
 * !  Sticky
 * @  Parameter (for both print and scan)
 *
 * I  n-bit Integer
 *    Numbers:
 *      The following options exists
 *        I8  = 8-bit integer
 *        I16 = 16-bit integer
 *        I32 = 32-bit integer
 *        I64 = 64-bit integer
 */
#define QUALIFIER_POSITION '$'
#define QUALIFIER_SHORT 'h'
#define QUALIFIER_LONG 'l'
#define QUALIFIER_LONG_UPPER 'L'
#define QUALIFIER_ALTERNATIVE '#'
#define QUALIFIER_SPACE ' '
#define QUALIFIER_PLUS '+'
#define QUALIFIER_MINUS '-'
#define QUALIFIER_DOT '.'
#define QUALIFIER_STAR '*'
#define QUALIFIER_CIRCUMFLEX '^'
#if TRIO_C99
# define QUALIFIER_SIZE_T 'z'
# define QUALIFIER_PTRDIFF_T 't'
# define QUALIFIER_INTMAX_T 'j'
#endif
#if TRIO_BSD || TRIO_GNU
# define QUALIFIER_QUAD 'q'
#endif
#if TRIO_GNU
# define QUALIFIER_SIZE_T_UPPER 'Z'
#endif
#if TRIO_MISC
# define QUALIFIER_WIDECHAR 'w'
#endif
#if TRIO_MICROSOFT
# define QUALIFIER_FIXED_SIZE 'I'
#endif
#if TRIO_EXTENSION
# define QUALIFIER_QUOTE '\''
# define QUALIFIER_STICKY '!'
# define QUALIFIER_VARSIZE '&' /* This should remain undocumented */
# define QUALIFIER_PARAM '@' /* Experimental */
# define QUALIFIER_COLON ':' /* For scanlists */
# define QUALIFIER_EQUAL '=' /* For scanlists */
#endif


/*************************************************************************
 * Internal structures
 */

/* Parameters */
typedef struct {
  /* An indication of which entry in the data union is used */
  int type;
  /* The flags */
  unsigned long flags;
  /* The width qualifier */
  int width;
  /* The precision qualifier */
  int precision;
  /* The base qualifier */
  int base;
  /* The size for the variable size qualifier */
  int varsize;
  /* The marker of the end of the specifier */
  int indexAfterSpecifier;
  /* The data from the argument list */
  union {
    char *string;
#if TRIO_WIDECHAR
    wchar_t *wstring;
#endif
    void *pointer;
    union {
      trio_uintmax_t as_signed;
      trio_intmax_t as_unsigned;
    } number;
    double doubleNumber;
    double *doublePointer;
    long double longdoubleNumber;
    long double *longdoublePointer;
    int errorNumber;
  } data;
  /* For the user-defined specifier */
  char user_name[MAX_USER_NAME];
  char user_data[MAX_USER_DATA];
} trio_parameter_t;

/* General trio "class" */
typedef struct _trio_class_t {
  /*
   * The function to write characters to a stream.
   */
  void (*OutStream)(struct _trio_class_t *, int);
  /*
   * The function to read characters from a stream.
   */
  void (*InStream)(struct _trio_class_t *, int *);
  /*
   * The current location in the stream.
   */
  void *location;
  /*
   * The character currently being processed.
   */
  int current;
  /*
   * The number of characters that would have been written/read
   * if there had been sufficient space.
   */
  int processed;
  /*
   * The number of characters that are actually written/read.
   * Processed and committed will only differ for the *nprintf
   * and *nscanf functions.
   */
  int committed;
  /*
   * The upper limit of characters that may be written/read.
   */
  int max;
} trio_class_t;

/* References (for user-defined callbacks) */
typedef struct _trio_reference_t {
  trio_class_t *data;
  trio_parameter_t *parameter;
} trio_reference_t;

/* Registered entries (for user-defined callbacks) */
typedef struct _trio_userdef_t {
  struct _trio_userdef_t *next;
  trio_callback_t callback;
  char *name;
} trio_userdef_t;


/*************************************************************************
 * Internal variables
 */

static const char internalNullString[] = "(nil)";

#if defined(USE_LOCALE)
static struct lconv *internalLocaleValues = NULL;
#endif

/*
 * UNIX98 says "in a locale where the radix character is not defined,
 * the radix character defaults to a period (.)"
 */
static char internalDecimalPoint[MAX_LOCALE_SEPARATOR_LENGTH + 1] = ".";
static char internalThousandSeparator[MAX_LOCALE_SEPARATOR_LENGTH + 1] = ",";
static char internalGrouping[MAX_LOCALE_GROUPS] = { (char)NO_GROUPING };

static const char internalDigitsLower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char internalDigitsUpper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static BOOLEAN_T internalDigitsUnconverted = TRUE;
static int internalDigitArray[128];
#if TRIO_EXTENSION
static BOOLEAN_T internalCollationUnconverted = TRUE;
static char internalCollationArray[MAX_CHARACTER_CLASS][MAX_CHARACTER_CLASS];
#endif

static volatile trio_callback_t internalEnterCriticalRegion = NULL;
static volatile trio_callback_t internalLeaveCriticalRegion = NULL;
static trio_userdef_t *internalUserDef = NULL;


/*************************************************************************
 * trio_strerror [public]
 */
const char *trio_strerror(int errorcode)
{
  /* Textual versions of the error codes */
  switch (TRIO_ERROR_CODE(errorcode))
    {
    case TRIO_EOF:
      return "End of file";
    case TRIO_EINVAL:
      return "Invalid argument";
    case TRIO_ETOOMANY:
      return "Too many arguments";
    case TRIO_EDBLREF:
      return "Double reference";
    case TRIO_EGAP:
      return "Reference gap";
    case TRIO_ENOMEM:
      return "Out of memory";
    case TRIO_ERANGE:
      return "Invalid range";
    default:
      return "Unknown";
    }
}

/*************************************************************************
 * TrioIsQualifier
 *
 * Description:
 *  Remember to add all new qualifiers to this function.
 *  QUALIFIER_POSITION must not be added.
 */
TRIO_PRIVATE BOOLEAN_T
TrioIsQualifier(const char ch)
{
  /* QUALIFIER_POSITION is not included */
  switch (ch)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case QUALIFIER_PLUS:
    case QUALIFIER_MINUS:
    case QUALIFIER_SPACE:
    case QUALIFIER_DOT:
    case QUALIFIER_STAR:
    case QUALIFIER_ALTERNATIVE:
    case QUALIFIER_SHORT:
    case QUALIFIER_LONG:
    case QUALIFIER_LONG_UPPER:
    case QUALIFIER_CIRCUMFLEX:
#if defined(QUALIFIER_SIZE_T)
    case QUALIFIER_SIZE_T:
#endif
#if defined(QUALIFIER_PTRDIFF_T)
    case QUALIFIER_PTRDIFF_T:
#endif
#if defined(QUALIFIER_INTMAX_T)
    case QUALIFIER_INTMAX_T:
#endif
#if defined(QUALIFIER_QUAD)
    case QUALIFIER_QUAD:
#endif
#if defined(QUALIFIER_SIZE_T_UPPER)
    case QUALIFIER_SIZE_T_UPPER:
#endif
#if defined(QUALIFIER_WIDECHAR)
    case QUALIFIER_WIDECHAR:
#endif
#if defined(QUALIFIER_QUOTE)
    case QUALIFIER_QUOTE:
#endif
#if defined(QUALIFIER_STICKY)
    case QUALIFIER_STICKY:
#endif
#if defined(QUALIFIER_VARSIZE)
    case QUALIFIER_VARSIZE:
#endif
#if defined(QUALIFIER_PARAM)
    case QUALIFIER_PARAM:
#endif
#if defined(QUALIFIER_FIXED_SIZE)
    case QUALIFIER_FIXED_SIZE:
#endif
      return TRUE;
    default:
      return FALSE;
    }
}

/*************************************************************************
 * TrioSetLocale
 */
#if defined(USE_LOCALE)
TRIO_PUBLIC void
TrioSetLocale(void)
{
  internalLocaleValues = (struct lconv *)localeconv();
  if (internalLocaleValues)
    {
      if ((internalLocaleValues->decimal_point) &&
	  (internalLocaleValues->decimal_point[0] != NIL))
	{
	  StrCopyMax(internalDecimalPoint,
		     sizeof(internalDecimalPoint),
		     internalLocaleValues->decimal_point);
	}
      if ((internalLocaleValues->thousands_sep) &&
	  (internalLocaleValues->thousands_sep[0] != NIL))
	{
	  StrCopyMax(internalThousandSeparator,
		     sizeof(internalThousandSeparator),
		     internalLocaleValues->thousands_sep);
	}
      if ((internalLocaleValues->grouping) &&
	  (internalLocaleValues->grouping[0] != NIL))
	{
	  StrCopyMax(internalGrouping,
		     sizeof(internalGrouping),
		     internalLocaleValues->grouping);
	}
    }
}
#endif /* defined(USE_LOCALE) */

/*************************************************************************
 * TrioGetPosition
 *
 * Get the %n$ position.
 */
TRIO_PRIVATE int
TrioGetPosition(const char *format,
		int *indexPointer)
{
  char *tmpformat;
  int number = 0;
  int index = *indexPointer;

  number = (int)StrToLong(&format[index], &tmpformat, BASE_DECIMAL);
  index = (int)(tmpformat - format);
  if ((number != 0) && (QUALIFIER_POSITION == format[index++]))
    {
      *indexPointer = index;
      /*
       * number is decreased by 1, because n$ starts from 1, whereas
       * the array it is indexing starts from 0.
       */
      return number - 1;
    }
  return NO_POSITION;
}

/*************************************************************************
 * TrioFindNamespace
 *
 * Find registered user-defined specifier.
 * The prev argument is used for optimisation only.
 */
TRIO_PRIVATE trio_userdef_t *
TrioFindNamespace(const char *name, trio_userdef_t **prev)
{
  trio_userdef_t *def;
  
  if (internalEnterCriticalRegion)
    (void)internalEnterCriticalRegion(NULL);
  
  for (def = internalUserDef; def; def = def->next)
    {
      /* Case-sensitive string comparison */
      if (StrEqualCase(def->name, name))
	break;
      
      if (prev)
	*prev = def;
    }
  
  if (internalLeaveCriticalRegion)
    (void)internalLeaveCriticalRegion(NULL);
  
  return def;
}

/*************************************************************************
 * TrioParse
 *
 * Description:
 *  Parse the format string
 */
TRIO_PRIVATE int
TrioParse(int type,
	  const char *format,
	  trio_parameter_t *parameters,
	  va_list arglist,
	  void **argarray)
{
#if TRIO_ERRORS
  /* Count the number of times a parameter is referenced */
  unsigned short usedEntries[MAX_PARAMETERS];
#endif
  /* Parameter counters */
  int parameterPosition;
  int currentParam;
  int maxParam = -1;
  /* Utility variables */
  unsigned long flags;
  int width;
  int precision;
  int varsize;
  int base;
  int index;  /* Index into formatting string */
  int dots;  /* Count number of dots in modifier part */
  BOOLEAN_T positional;  /* Does the specifier have a positional? */
  BOOLEAN_T got_sticky = FALSE;  /* Are there any sticky modifiers at all? */
  /*
   * indices specifies the order in which the parameters must be
   * read from the va_args (this is necessary to handle positionals)
   */
  int indices[MAX_PARAMETERS];
  int pos = 0;
  /* Various variables */
  char ch;
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  int charlen;
#endif
  int i = -1;
  int num;
  char *tmpformat;


#if TRIO_ERRORS
  /*
   * The 'parameters' array is not initialized, but we need to
   * know which entries we have used.
   */
  memset(usedEntries, 0, sizeof(usedEntries));
#endif

  index = 0;
  parameterPosition = 0;
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  mblen(NULL, 0);
#endif
  
  while (format[index])
    {
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
      if (! isascii(format[index]))
	{
	  /*
	   * Multibyte characters cannot be legal specifiers or
	   * modifiers, so we skip over them.
	   */
	  charlen = mblen(&format[index], MB_LEN_MAX);
	  index += (charlen > 0) ? charlen : 1;
	  continue; /* while */
	}
#endif /* TRIO_COMPILER_SUPPORTS_MULTIBYTE */
      if (CHAR_IDENTIFIER == format[index++])
	{
	  if (CHAR_IDENTIFIER == format[index])
	    {
	      index++;
	      continue; /* while */
	    }

	  flags = FLAGS_NEW;
	  dots = 0;
	  currentParam = TrioGetPosition(format, &index);
	  positional = (NO_POSITION != currentParam);
	  if (!positional)
	    {
	      /* We have no positional, get the next counter */
	      currentParam = parameterPosition;
	    }
          if(currentParam >= MAX_PARAMETERS)
	    {
	      /* Bail out completely to make the error more obvious */
	      return TRIO_ERROR_RETURN(TRIO_ETOOMANY, index);
	    }

	  if (currentParam > maxParam)
	    maxParam = currentParam;

	  /* Default values */
	  width = NO_WIDTH;
	  precision = NO_PRECISION;
	  base = NO_BASE;
	  varsize = NO_SIZE;

	  while (TrioIsQualifier(format[index]))
	    {
	      ch = format[index++];

	      switch (ch)
		{
		case QUALIFIER_SPACE:
		  flags |= FLAGS_SPACE;
		  break;

		case QUALIFIER_PLUS:
		  flags |= FLAGS_SHOWSIGN;
		  break;

		case QUALIFIER_MINUS:
		  flags |= FLAGS_LEFTADJUST;
		  flags &= ~FLAGS_NILPADDING;
		  break;

		case QUALIFIER_ALTERNATIVE:
		  flags |= FLAGS_ALTERNATIVE;
		  break;

		case QUALIFIER_DOT:
		  if (dots == 0) /* Precision */
		    {
		      dots++;

		      /* Skip if no precision */
		      if (QUALIFIER_DOT == format[index])
			break;
		      
		      /* After the first dot we have the precision */
		      flags |= FLAGS_PRECISION;
		      if ((QUALIFIER_STAR == format[index]) ||
			  (QUALIFIER_PARAM == format[index]))
			{
			  index++;
			  flags |= FLAGS_PRECISION_PARAMETER;

			  precision = TrioGetPosition(format, &index);
			  if (precision == NO_POSITION)
			    {
			      parameterPosition++;
			      if (positional)
				precision = parameterPosition;
			      else
				{
				  precision = currentParam;
				  currentParam = precision + 1;
				}
			    }
			  else
			    {
			      if (! positional)
				currentParam = precision + 1;
			      if (width > maxParam)
				maxParam = precision;
			    }
			  if (currentParam > maxParam)
			    maxParam = currentParam;
			}
		      else
			{
			  precision = StrToLong(&format[index], &tmpformat, BASE_DECIMAL);
			  index = (int)(tmpformat - format);
			}
		    }
		  else if (dots == 1) /* Base */
		    {
		      dots++;
		      
		      /* After the second dot we have the base */
		      flags |= FLAGS_BASE;
		      if ((QUALIFIER_STAR == format[index]) ||
			  (QUALIFIER_PARAM == format[index]))
			{
			  index++;
			  flags |= FLAGS_BASE_PARAMETER;
			  base = TrioGetPosition(format, &index);
			  if (base == NO_POSITION)
			    {
			      parameterPosition++;
			      if (positional)
				base = parameterPosition;
			      else
				{
				  base = currentParam;
				  currentParam = base + 1;
				}
			    }
			  else
			    {
			      if (! positional)
				currentParam = base + 1;
			      if (base > maxParam)
				maxParam = base;
			    }
			  if (currentParam > maxParam)
			    maxParam = currentParam;
			}
		      else
			{
			  base = StrToLong(&format[index], &tmpformat, BASE_DECIMAL);
			  if (base > MAX_BASE)
			    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
			  index = (int)(tmpformat - format);
			}
		    }
		  else
		    {
		      return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		    }
		  break; /* QUALIFIER_DOT */

		case QUALIFIER_PARAM:
		  type = TYPE_PRINT;
		  /* FALLTHROUGH */
		case QUALIFIER_STAR:
		  /* This has different meanings for print and scan */
		  if (TYPE_PRINT == type)
		    {
		      /* Read with from parameter */
		      flags |= (FLAGS_WIDTH | FLAGS_WIDTH_PARAMETER);
		      width = TrioGetPosition(format, &index);
		      if (width == NO_POSITION)
			{
			  parameterPosition++;
			  if (positional)
			    width = parameterPosition;
			  else
			    {
			      width = currentParam;
			      currentParam = width + 1;
			    }
			}
		      else
			{
			  if (! positional)
			    currentParam = width + 1;
			  if (width > maxParam)
			    maxParam = width;
			}
		      if (currentParam > maxParam)
			maxParam = currentParam;
		    }
		  else
		    {
		      /* Scan, but do not store result */
		      flags |= FLAGS_IGNORE;
		    }

		  break; /* QUALIFIER_STAR */

		case '0':
		  if (! (flags & FLAGS_LEFTADJUST))
		    flags |= FLAGS_NILPADDING;
		  /* FALLTHROUGH */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		  flags |= FLAGS_WIDTH;
		  /* &format[index - 1] is used to "rewind" the read
		   * character from format
		   */
		  width = StrToLong(&format[index - 1], &tmpformat, BASE_DECIMAL);
		  index = (int)(tmpformat - format);
		  break;

		case QUALIFIER_SHORT:
		  if (flags & FLAGS_SHORTSHORT)
		    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		  else if (flags & FLAGS_SHORT)
		    flags |= FLAGS_SHORTSHORT;
		  else
		    flags |= FLAGS_SHORT;
		  break;

		case QUALIFIER_LONG:
		  if (flags & FLAGS_QUAD)
		    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		  else if (flags & FLAGS_LONG)
		    flags |= FLAGS_QUAD;
		  else
		    flags |= FLAGS_LONG;
		  break;

		case QUALIFIER_LONG_UPPER:
		  flags |= FLAGS_LONGDOUBLE;
		  break;

#if defined(QUALIFIER_SIZE_T)
		case QUALIFIER_SIZE_T:
		  flags |= FLAGS_SIZE_T;
		  /* Modify flags for later truncation of number */
		  if (sizeof(size_t) == sizeof(trio_ulonglong_t))
		    flags |= FLAGS_QUAD;
		  else if (sizeof(size_t) == sizeof(long))
		    flags |= FLAGS_LONG;
		  break;
#endif

#if defined(QUALIFIER_PTRDIFF_T)
		case QUALIFIER_PTRDIFF_T:
		  flags |= FLAGS_PTRDIFF_T;
		  if (sizeof(ptrdiff_t) == sizeof(trio_ulonglong_t))
		    flags |= FLAGS_QUAD;
		  else if (sizeof(ptrdiff_t) == sizeof(long))
		    flags |= FLAGS_LONG;
		  break;
#endif

#if defined(QUALIFIER_INTMAX_T)
		case QUALIFIER_INTMAX_T:
		  flags |= FLAGS_INTMAX_T;
		  if (sizeof(trio_intmax_t) == sizeof(trio_ulonglong_t))
		    flags |= FLAGS_QUAD;
		  else if (sizeof(trio_intmax_t) == sizeof(long))
		    flags |= FLAGS_LONG;
		  break;
#endif

#if defined(QUALIFIER_QUAD)
		case QUALIFIER_QUAD:
		  flags |= FLAGS_QUAD;
		  break;
#endif

#if defined(QUALIFIER_FIXED_SIZE)
		case QUALIFIER_FIXED_SIZE:
		  if (flags & FLAGS_FIXED_SIZE)
		    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);

		  if (flags & (FLAGS_ALL_SIZES | FLAGS_LONGDOUBLE |
			       FLAGS_WIDECHAR | FLAGS_VARSIZE_PARAMETER))
		    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);

		  if ((format[index] == '6') &&
		      (format[index + 1] == '4'))
		    {
		      varsize = sizeof(trio_int64_t);
		      index += 2;
		    }
		  else if ((format[index] == '3') &&
			   (format[index + 1] == '2'))
		    {
		      varsize = sizeof(trio_int32_t);
		      index += 2;
		    }
		  else if ((format[index] == '1') &&
			   (format[index + 1] == '6'))
		    {
		      varsize = sizeof(trio_int16_t);
		      index += 2;
		    }
		  else if (format[index] == '8')
		    {
		      varsize = sizeof(trio_int8_t);
		      index++;
		    }
		  else
		    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		  
		  flags |= FLAGS_FIXED_SIZE;
		  break;
#endif

#if defined(QUALIFIER_WIDECHAR)
		case QUALIFIER_WIDECHAR:
		  flags |= FLAGS_WIDECHAR;
		  break;
#endif

#if defined(QUALIFIER_SIZE_T_UPPER)
		case QUALIFIER_SIZE_T_UPPER:
		  break;
#endif

#if defined(QUALIFIER_QUOTE)
		case QUALIFIER_QUOTE:
		  flags |= FLAGS_QUOTE;
		  break;
#endif

#if defined(QUALIFIER_STICKY)
		case QUALIFIER_STICKY:
		  flags |= FLAGS_STICKY;
		  got_sticky = TRUE;
		  break;
#endif
		  
#if defined(QUALIFIER_VARSIZE)
		case QUALIFIER_VARSIZE:
		  flags |= FLAGS_VARSIZE_PARAMETER;
		  parameterPosition++;
		  if (positional)
		    varsize = parameterPosition;
		  else
		    {
		      varsize = currentParam;
		      currentParam = varsize + 1;
		    }
		  if (currentParam > maxParam)
		    maxParam = currentParam;
		  break;
#endif

		default:
		  /* Bail out completely to make the error more obvious */
                  return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		}
	    } /* while qualifier */

	  /*
	   * Parameters only need the type and value. The value is
	   * read later.
	   */
	  if (flags & FLAGS_WIDTH_PARAMETER)
	    {
#if TRIO_ERRORS
	      usedEntries[width] += 1;
#endif
	      parameters[pos].type = FORMAT_PARAMETER;
	      indices[width] = pos;
	      width = pos++;
	    }
	  if (flags & FLAGS_PRECISION_PARAMETER)
	    {
#if TRIO_ERRORS
	      usedEntries[precision] += 1;
#endif
	      parameters[pos].type = FORMAT_PARAMETER;
	      indices[precision] = pos;
	      precision = pos++;
	    }
	  if (flags & FLAGS_BASE_PARAMETER)
	    {
#if TRIO_ERRORS
	      usedEntries[base] += 1;
#endif
	      parameters[pos].type = FORMAT_PARAMETER;
	      indices[base] = pos;
	      base = pos++;
	    }
	  if (flags & FLAGS_VARSIZE_PARAMETER)
	    {
#if TRIO_ERRORS
	      usedEntries[varsize] += 1;
#endif
	      parameters[pos].type = FORMAT_PARAMETER;
	      indices[varsize] = pos;
	      varsize = pos++;
	    }
	  
	  indices[currentParam] = pos;
	  
	  switch (format[index++])
	    {
#if defined(SPECIFIER_CHAR_UPPER)
	    case SPECIFIER_CHAR_UPPER:
	      flags |= FLAGS_WIDECHAR;
	      /* FALLTHROUGH */
#endif
	    case SPECIFIER_CHAR:
	      if (flags & FLAGS_LONG)
		flags |= FLAGS_WIDECHAR;
	      else if (flags & FLAGS_SHORT)
		flags &= ~FLAGS_WIDECHAR;
	      parameters[pos].type = FORMAT_CHAR;
	      break;

#if defined(SPECIFIER_STRING_UPPER)
	    case SPECIFIER_STRING_UPPER:
	      flags |= FLAGS_WIDECHAR;
	      /* FALLTHROUGH */
#endif
	    case SPECIFIER_STRING:
	      if (flags & FLAGS_LONG)
		flags |= FLAGS_WIDECHAR;
	      else if (flags & FLAGS_SHORT)
		flags &= ~FLAGS_WIDECHAR;
	      parameters[pos].type = FORMAT_STRING;
	      break;

	    case SPECIFIER_GROUP:
	      if (TYPE_SCAN == type)
		{
		  int depth = 1;
		  parameters[pos].type = FORMAT_GROUP;
		  if (format[index] == QUALIFIER_CIRCUMFLEX)
		    index++;
		  if (format[index] == SPECIFIER_UNGROUP)
		    index++;
		  if (format[index] == QUALIFIER_MINUS)
		    index++;
		  /* Skip nested brackets */
		  while (format[index] != NIL)
		    {
		      if (format[index] == SPECIFIER_GROUP)
			{
			  depth++;
			}
		      else if (format[index] == SPECIFIER_UNGROUP)
			{
			  if (--depth <= 0)
			    {
			      index++;
			      break;
			    }
			}
		      index++;
		    }
		}
	      break;
	      
	    case SPECIFIER_INTEGER:
	      parameters[pos].type = FORMAT_INT;
	      break;
	      
	    case SPECIFIER_UNSIGNED:
	      flags |= FLAGS_UNSIGNED;
	      parameters[pos].type = FORMAT_INT;
	      break;

	    case SPECIFIER_DECIMAL:
	      /* Disable base modifier */
	      flags &= ~FLAGS_BASE_PARAMETER;
	      base = BASE_DECIMAL;
	      parameters[pos].type = FORMAT_INT;
	      break;

	    case SPECIFIER_OCTAL:
	      flags &= ~FLAGS_BASE_PARAMETER;
	      base = BASE_OCTAL;
	      parameters[pos].type = FORMAT_INT;
	      break;

#if defined(SPECIFIER_BINARY)
	    case SPECIFIER_BINARY_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
	    case SPECIFIER_BINARY:
	      flags |= FLAGS_NILPADDING;
	      flags &= ~FLAGS_BASE_PARAMETER;
	      base = BASE_BINARY;
	      parameters[pos].type = FORMAT_INT;
	      break;
#endif

	    case SPECIFIER_HEX_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
	    case SPECIFIER_HEX:
	      flags |= FLAGS_UNSIGNED;
	      flags &= ~FLAGS_BASE_PARAMETER;
	      base = BASE_HEX;
	      parameters[pos].type = FORMAT_INT;
	      break;

	    case SPECIFIER_FLOAT_E_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
	    case SPECIFIER_FLOAT_E:
	      flags |= FLAGS_FLOAT_E;
	      parameters[pos].type = FORMAT_DOUBLE;
	      break;

	    case SPECIFIER_FLOAT_G_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
	    case SPECIFIER_FLOAT_G:
	      flags |= FLAGS_FLOAT_G;
	      parameters[pos].type = FORMAT_DOUBLE;
	      break;

	    case SPECIFIER_FLOAT_F_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
	    case SPECIFIER_FLOAT_F:
	      parameters[pos].type = FORMAT_DOUBLE;
	      break;

	    case SPECIFIER_POINTER:
	      parameters[pos].type = FORMAT_POINTER;
	      break;

	    case SPECIFIER_COUNT:
	      parameters[pos].type = FORMAT_COUNT;
	      break;

#if defined(SPECIFIER_HEXFLOAT)
# if defined(SPECIFIER_HEXFLOAT_UPPER)
	    case SPECIFIER_HEXFLOAT_UPPER:
	      flags |= FLAGS_UPPER;
	      /* FALLTHROUGH */
# endif
	    case SPECIFIER_HEXFLOAT:
	      base = BASE_HEX;
	      parameters[pos].type = FORMAT_DOUBLE;
	      break;
#endif

#if defined(FORMAT_ERRNO)
	    case SPECIFIER_ERRNO:
	      parameters[pos].type = FORMAT_ERRNO;
	      break;
#endif

#if defined(SPECIFIER_USER_DEFINED_BEGIN)
	    case SPECIFIER_USER_DEFINED_BEGIN:
	      {
		unsigned int max;
		int without_namespace = TRUE;
		
		parameters[pos].type = FORMAT_USER_DEFINED;
		parameters[pos].user_name[0] = NIL;
		tmpformat = (char *)&format[index];
	      
		while ((ch = format[index]))
		  {
		    index++;
		    if (ch == SPECIFIER_USER_DEFINED_END)
		      {
			if (without_namespace)
			  {
			    /* We must get the handle first */
			    parameters[pos].type = FORMAT_PARAMETER;
			    parameters[pos].indexAfterSpecifier = index;
			    parameters[pos].flags = FLAGS_USER_DEFINED;
			    /* Adjust parameters for insertion of new one */
			    pos++;
# if TRIO_ERRORS
			    usedEntries[currentParam] += 1;
# endif
			    parameters[pos].type = FORMAT_USER_DEFINED;
			    currentParam++;
			    indices[currentParam] = pos;
			    if (currentParam > maxParam)
			      maxParam = currentParam;
			  }
			/* Copy the user data */
			max = (unsigned int)(&format[index] - tmpformat);
			if (max > MAX_USER_DATA)
			  max = MAX_USER_DATA;
			StrCopyMax(parameters[pos].user_data,
				   max,
				   tmpformat);
			break; /* while */
		      }
		    if (ch == SPECIFIER_USER_DEFINED_SEPARATOR)
		      {
			without_namespace = FALSE;
			/* Copy the namespace for later looking-up */
			max = (int)(&format[index] - tmpformat);
			if (max > MAX_USER_NAME)
			  max = MAX_USER_NAME;
			StrCopyMax(parameters[pos].user_name,
				   max,
				   tmpformat);
			tmpformat = (char *)&format[index];
		      }
		  }
		if (ch != SPECIFIER_USER_DEFINED_END)
		  return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
	      }
	      break;
#endif /* defined(SPECIFIER_USER_DEFINED_BEGIN) */
	      
	    default:
	      /* Bail out completely to make the error more obvious */
              return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
	    }

#if TRIO_ERRORS
	  /*  Count the number of times this entry has been used */
	  usedEntries[currentParam] += 1;
#endif
	  
	  /* Find last sticky parameters */
	  if (got_sticky && !(flags & FLAGS_STICKY))
	    {
	      for (i = pos - 1; i >= 0; i--)
		{
		  if (parameters[i].type == FORMAT_PARAMETER)
		    continue;
		  if ((parameters[i].flags & FLAGS_STICKY) &&
		      (parameters[i].type == parameters[pos].type))
		    {
		      /* Do not overwrite current qualifiers */
		      flags |= (parameters[i].flags & (unsigned long)~FLAGS_STICKY);
		      if (width == NO_WIDTH)
			width = parameters[i].width;
		      if (precision == NO_PRECISION)
			precision = parameters[i].precision;
		      if (base == NO_BASE)
			base = parameters[i].base;
		      break;
		    }
		}
	    }
	  
	  parameters[pos].indexAfterSpecifier = index;
	  parameters[pos].flags = flags;
	  parameters[pos].width = width;
	  parameters[pos].precision = precision;
	  parameters[pos].base = (base == NO_BASE) ? BASE_DECIMAL : base;
	  parameters[pos].varsize = varsize;
	  pos++;
	  
	  if (! positional)
	    parameterPosition++;
	  
	} /* if identifier */
      
    } /* while format characters left */

  for (num = 0; num <= maxParam; num++)
    {
#if TRIO_ERRORS
      if (usedEntries[num] != 1)
	{
	  if (usedEntries[num] == 0) /* gap detected */
	    return TRIO_ERROR_RETURN(TRIO_EGAP, num);
	  else /* double references detected */
	    return TRIO_ERROR_RETURN(TRIO_EDBLREF, num);
	}
#endif
      
      i = indices[num];

      /*
       * FORMAT_PARAMETERS are only present if they must be read,
       * so it makes no sense to check the ignore flag (besides,
       * the flags variable is not set for that particular type)
       */
      if ((parameters[i].type != FORMAT_PARAMETER) &&
	  (parameters[i].flags & FLAGS_IGNORE))
	continue; /* for all arguments */

      /*
       * The stack arguments are read according to ANSI C89
       * default argument promotions:
       *
       *  char           = int
       *  short          = int
       *  unsigned char  = unsigned int
       *  unsigned short = unsigned int
       *  float          = double
       *
       * In addition to the ANSI C89 these types are read (the
       * default argument promotions of C99 has not been
       * considered yet)
       *
       *  long long
       *  long double
       *  size_t
       *  ptrdiff_t
       *  intmax_t
       */
      switch (parameters[i].type)
	{
	case FORMAT_GROUP:
	case FORMAT_STRING:
#if TRIO_WIDECHAR
	  if (flags & FLAGS_WIDECHAR)
	    {
	      parameters[i].data.wstring = (argarray == NULL)
		? va_arg(arglist, wchar_t *)
		: (wchar_t *)(argarray[num]);
	    }
	  else
#endif
	    {
	      parameters[i].data.string = (argarray == NULL)
		? va_arg(arglist, char *)
		: (char *)(argarray[num]);
	    }
	  break;

	case FORMAT_POINTER:
	case FORMAT_COUNT:
	case FORMAT_USER_DEFINED:
	case FORMAT_UNKNOWN:
	  parameters[i].data.pointer = (argarray == NULL)
	    ? va_arg(arglist, void *)
	    : argarray[num];
	  break;

	case FORMAT_CHAR:
	case FORMAT_INT:
	  if (TYPE_SCAN == type)
	    {
              if (argarray == NULL)
                parameters[i].data.pointer = 
                  (trio_uintmax_t *)va_arg(arglist, void *);
              else
                {
                  if (parameters[i].type == FORMAT_CHAR)
                    parameters[i].data.pointer =
                      (trio_uintmax_t *)((char *)argarray[num]);
                  else if (parameters[i].flags & FLAGS_SHORT)
                    parameters[i].data.pointer =
                      (trio_uintmax_t *)((short *)argarray[num]);
                  else
                    parameters[i].data.pointer =
                      (trio_uintmax_t *)((int *)argarray[num]);
                }
	    }
	  else
	    {
#if defined(QUALIFIER_VARSIZE) || defined(QUALIFIER_FIXED_SIZE)
	      if ((parameters[i].flags & FLAGS_VARSIZE_PARAMETER) ||
		  (parameters[i].flags & FLAGS_FIXED_SIZE))
		{
		  if (parameters[i].flags & FLAGS_VARSIZE_PARAMETER)
		    {
		      /*
		       * Variable sizes are mapped onto the fixed sizes, in
		       * accordance with integer promotion.
		       *
		       * Please note that this may not be portable, as we
		       * only guess the size, not the layout of the numbers.
		       * For example, if int is little-endian, and long is
		       * big-endian, then this will fail.
		       */
		      varsize = (int)parameters[parameters[i].varsize].data.number.as_unsigned;
		    }
		  else
		    {
		      /* Used for the I<bits> modifiers */
		      varsize = parameters[i].varsize;
		    }
		  parameters[i].flags &= ~FLAGS_ALL_VARSIZES;
		  
		  if (varsize <= (int)sizeof(int))
		    ;
		  else if (varsize <= (int)sizeof(long))
		    parameters[i].flags |= FLAGS_LONG;
#if defined(QUALIFIER_INTMAX_T)
		  else if (varsize <= (int)sizeof(trio_longlong_t))
		    parameters[i].flags |= FLAGS_QUAD;
		  else
		    parameters[i].flags |= FLAGS_INTMAX_T;
#else
		  else
		    parameters[i].flags |= FLAGS_QUAD;
#endif
		}
#endif /* defined(QUALIFIER_VARSIZE) */
#if defined(QUALIFIER_SIZE_T) || defined(QUALIFIER_SIZE_T_UPPER)
	      if (parameters[i].flags & FLAGS_SIZE_T)
		parameters[i].data.number.as_unsigned = (argarray == NULL)
		  ? (trio_uintmax_t)va_arg(arglist, size_t)
		  : (trio_uintmax_t)(*((size_t *)argarray[num]));
	      else
#endif
#if defined(QUALIFIER_PTRDIFF_T)
	      if (parameters[i].flags & FLAGS_PTRDIFF_T)
		parameters[i].data.number.as_unsigned = (argarray == NULL)
		  ? (trio_uintmax_t)va_arg(arglist, ptrdiff_t)
		  : (trio_uintmax_t)(*((ptrdiff_t *)argarray[num]));
	      else
#endif
#if defined(QUALIFIER_INTMAX_T)
	      if (parameters[i].flags & FLAGS_INTMAX_T)
		parameters[i].data.number.as_unsigned = (argarray == NULL)
		  ? (trio_uintmax_t)va_arg(arglist, trio_intmax_t)
		  : (trio_uintmax_t)(*((trio_intmax_t *)argarray[num]));
	      else
#endif
	      if (parameters[i].flags & FLAGS_QUAD)
		parameters[i].data.number.as_unsigned = (argarray == NULL)
		  ? (trio_uintmax_t)va_arg(arglist, trio_ulonglong_t)
		  : (trio_uintmax_t)(*((trio_ulonglong_t *)argarray[num]));
	      else if (parameters[i].flags & FLAGS_LONG)
		parameters[i].data.number.as_unsigned = (argarray == NULL)
		  ? (trio_uintmax_t)va_arg(arglist, long)
		  : (trio_uintmax_t)(*((long *)argarray[num]));
	      else
		{
		  if (argarray == NULL)
		    parameters[i].data.number.as_unsigned = (trio_uintmax_t)va_arg(arglist, int);
		  else
		    {
		      if (parameters[i].type == FORMAT_CHAR)
			parameters[i].data.number.as_unsigned = (trio_uintmax_t)(*((char *)argarray[num]));
		      else if (parameters[i].flags & FLAGS_SHORT)
			parameters[i].data.number.as_unsigned = (trio_uintmax_t)(*((short *)argarray[num]));
		      else
			parameters[i].data.number.as_unsigned = (trio_uintmax_t)(*((int *)argarray[num]));
		    }
		}
	    }
	  break;

	case FORMAT_PARAMETER:
	  /*
	   * The parameter for the user-defined specifier is a pointer,
	   * whereas the rest (width, precision, base) uses an integer.
	   */
	  if (parameters[i].flags & FLAGS_USER_DEFINED)
	    parameters[i].data.pointer = (argarray == NULL)
	      ? va_arg(arglist, void *)
	      : argarray[num];
	  else
	    parameters[i].data.number.as_unsigned = (argarray == NULL)
	      ? (trio_uintmax_t)va_arg(arglist, int)
	      : (trio_uintmax_t)(*((int *)argarray[num]));
	  break;

	case FORMAT_DOUBLE:
	  if (TYPE_SCAN == type)
	    {
	      if (parameters[i].flags & FLAGS_LONG)
		parameters[i].data.longdoublePointer = (argarray == NULL)
		  ? va_arg(arglist, long double *)
		  : (long double *)((long double *)argarray[num]);
	      else
                {
                  if (argarray == NULL)
                    parameters[i].data.doublePointer =
                      va_arg(arglist, double *);
                 else
                   {
                     if (parameters[i].flags & FLAGS_SHORT)
                       parameters[i].data.doublePointer =
                         (double *)((float *)argarray[num]);
                     else
                       parameters[i].data.doublePointer =
                         (double *)((double *)argarray[num]);
                   }
                }
	    }
	  else
	    {
	      if (parameters[i].flags & FLAGS_LONG)
		parameters[i].data.longdoubleNumber = (argarray == NULL)
		  ? va_arg(arglist, long double)
		  : (long double)(*((long double *)argarray[num]));
	      else
		{
		  if (argarray == NULL)
		    parameters[i].data.longdoubleNumber = (long double)va_arg(arglist, double);
		  else
		    {
		      if (parameters[i].flags & FLAGS_SHORT)
			parameters[i].data.longdoubleNumber = (long double)(*((float *)argarray[num]));
		      else
			parameters[i].data.longdoubleNumber = (long double)(long double)(*((double *)argarray[num]));
		    }
		}
	    }
	  break;

#if defined(FORMAT_ERRNO)
	case FORMAT_ERRNO:
	  parameters[i].data.errorNumber = errno;
	  break;
#endif

	default:
	  break;
	}
    } /* for all specifiers */
  return num;
}


/*************************************************************************
 *
 * @FORMATTING
 *
 ************************************************************************/


/*************************************************************************
 * TrioWriteNumber
 *
 * Description:
 *  Output a number.
 *  The complexity of this function is a result of the complexity
 *  of the dependencies of the flags.
 */
TRIO_PRIVATE void
TrioWriteNumber(trio_class_t *self,
		trio_uintmax_t number,
		unsigned long flags,
		int width,
		int precision,
		int base)
{
  BOOLEAN_T isNegative;
  char buffer[MAX_CHARS_IN(trio_uintmax_t) * (1 + MAX_LOCALE_SEPARATOR_LENGTH) + 1];
  char *bufferend;
  char *pointer;
  const char *digits;
  int i;
  int length;
  char *p;
  int charsPerThousand;
  int groupingIndex;
  int count;

  assert(VALID(self));
  assert(VALID(self->OutStream));
  assert((base >= MIN_BASE && base <= MAX_BASE) || (base == NO_BASE));

  digits = (flags & FLAGS_UPPER) ? internalDigitsUpper : internalDigitsLower;

  isNegative = (flags & FLAGS_UNSIGNED)
    ? FALSE
    : ((trio_intmax_t)number < 0);
  if (isNegative)
    number = -number;

  if (flags & FLAGS_QUAD)
    number &= (trio_ulonglong_t)-1;
  else if (flags & FLAGS_LONG)
    number &= (unsigned long)-1;
  else
    number &= (unsigned int)-1;
  
  /* Build number */
  pointer = bufferend = &buffer[sizeof(buffer) - 1];
  *pointer-- = NIL;
  charsPerThousand = (int)internalGrouping[0];
  groupingIndex = 1;
  for (i = 1; i < (int)sizeof(buffer); i++)
    {
      *pointer-- = digits[number % base];
      number /= base;
      if (number == 0)
	break;

      if ((flags & FLAGS_QUOTE)
	  && (charsPerThousand != NO_GROUPING)
	  && (i % charsPerThousand == 0))
	{
	  /*
	   * We are building the number from the least significant
	   * to the most significant digit, so we have to copy the
	   * thousand separator backwards
	   */
	  length = StrLength(internalThousandSeparator);
	  if (((int)(pointer - buffer) - length) > 0)
	    {
	      p = &internalThousandSeparator[length - 1];
	      while (length-- > 0)
		*pointer-- = *p--;
	    }

	  /* Advance to next grouping number */
	  switch (internalGrouping[groupingIndex])
	    {
	    case CHAR_MAX: /* Disable grouping */
	      charsPerThousand = NO_GROUPING;
	      break;
	    case 0: /* Repeat last group */
	      break;
	    default:
	      charsPerThousand = (int)internalGrouping[groupingIndex++];
	      break;
	    }
	}
    }

  /* Adjust width */
  width -= (bufferend - pointer) - 1;

  /* Adjust precision */
  if (NO_PRECISION != precision)
    {
      precision -= (bufferend - pointer) - 1;
      if (precision < 0)
	precision = 0;
      flags |= FLAGS_NILPADDING;
    }

  /* Adjust width further */
  if (isNegative || (flags & FLAGS_SHOWSIGN) || (flags & FLAGS_SPACE))
    width--;
  if (flags & FLAGS_ALTERNATIVE)
    {
      switch (base)
	{
	case BASE_BINARY:
	case BASE_HEX:
	  width -= 2;
	  break;
	case BASE_OCTAL:
	  width--;
	  break;
	default:
	  break;
	}
    }

  /* Output prefixes spaces if needed */
  if (! ((flags & FLAGS_LEFTADJUST) ||
	 ((flags & FLAGS_NILPADDING) && (precision == NO_PRECISION))))
    {
      count = (precision == NO_PRECISION) ? 0 : precision;
      while (width-- > count)
	self->OutStream(self, CHAR_ADJUST);
    }

  /* width has been adjusted for signs and alternatives */
  if (isNegative)
    self->OutStream(self, '-');
  else if (flags & FLAGS_SHOWSIGN)
    self->OutStream(self, '+');
  else if (flags & FLAGS_SPACE)
    self->OutStream(self, ' ');

  if (flags & FLAGS_ALTERNATIVE)
    {
      switch (base)
	{
	case BASE_BINARY:
	  self->OutStream(self, '0');
	  self->OutStream(self, (flags & FLAGS_UPPER) ? 'B' : 'b');
	  break;

	case BASE_OCTAL:
	  self->OutStream(self, '0');
	  break;

	case BASE_HEX:
	  self->OutStream(self, '0');
	  self->OutStream(self, (flags & FLAGS_UPPER) ? 'X' : 'x');
	  break;

	default:
	  break;
	} /* switch base */
    }

  /* Output prefixed zero padding if needed */
  if (flags & FLAGS_NILPADDING)
    {
      if (precision == NO_PRECISION)
	precision = width;
      while (precision-- > 0)
	{
	  self->OutStream(self, '0');
	  width--;
	}
    }

  /* Output the number itself */
  while (*(++pointer))
    {
      self->OutStream(self, *pointer);
    }

  /* Output trailing spaces if needed */
  if (flags & FLAGS_LEFTADJUST)
    {
      while (width-- > 0)
	self->OutStream(self, CHAR_ADJUST);
    }
}

/*************************************************************************
 * TrioWriteStringCharacter
 *
 * Description:
 *  Output a single character of a string
 */
TRIO_PRIVATE void
TrioWriteStringCharacter(trio_class_t *self,
			 int ch,
			 unsigned long flags)
{
  if (flags & FLAGS_ALTERNATIVE)
    {
      if (! (isprint(ch) || isspace(ch)))
	{
	  /*
	   * Non-printable characters are converted to C escapes or
	   * \number, if no C escape exists.
	   */
	  self->OutStream(self, CHAR_BACKSLASH);
	  switch (ch)
	    {
	    case '\007': self->OutStream(self, 'a'); break;
	    case '\b': self->OutStream(self, 'b'); break;
	    case '\f': self->OutStream(self, 'f'); break;
	    case '\n': self->OutStream(self, 'n'); break;
	    case '\r': self->OutStream(self, 'r'); break;
	    case '\t': self->OutStream(self, 't'); break;
	    case '\v': self->OutStream(self, 'v'); break;
	    case '\\': self->OutStream(self, '\\'); break;
	    default:
	      self->OutStream(self, 'x');
	      TrioWriteNumber(self, (trio_intmax_t)ch,
			      FLAGS_UNSIGNED | FLAGS_NILPADDING,
			      2, 2, BASE_HEX);
	      break;
	    }
	}
      else if (ch == CHAR_BACKSLASH)
	{
	  self->OutStream(self, CHAR_BACKSLASH);
	  self->OutStream(self, CHAR_BACKSLASH);
	}
      else
	{
	  self->OutStream(self, ch);
	}
    }
  else
    {
      self->OutStream(self, ch);
    }
}

/*************************************************************************
 * TrioWriteString
 *
 * Description:
 *  Output a string
 */
TRIO_PRIVATE void
TrioWriteString(trio_class_t *self,
		const char *string,
		unsigned long flags,
		int width,
		int precision)
{
  int length;
  int ch;

  assert(VALID(self));
  assert(VALID(self->OutStream));

  if (string == NULL)
    {
      string = internalNullString;
      length = sizeof(internalNullString) - 1;
      /* Disable quoting for the null pointer */
      flags &= (~FLAGS_QUOTE);
      width = 0;
    }
  else
    {
      length = StrLength(string);
    }
  if ((NO_PRECISION != precision) &&
      (precision < length))
    {
      length = precision;
    }
  width -= length;

  if (flags & FLAGS_QUOTE)
    self->OutStream(self, CHAR_QUOTE);

  if (! (flags & FLAGS_LEFTADJUST))
    {
      while (width-- > 0)
	self->OutStream(self, CHAR_ADJUST);
    }

  while (length-- > 0)
    {
      /* The ctype parameters must be an unsigned char (or EOF) */
      ch = (int)((unsigned char)(*string++));
      TrioWriteStringCharacter(self, ch, flags);
    }

  if (flags & FLAGS_LEFTADJUST)
    {
      while (width-- > 0)
	self->OutStream(self, CHAR_ADJUST);
    }
  if (flags & FLAGS_QUOTE)
    self->OutStream(self, CHAR_QUOTE);
}

/*************************************************************************
 * TrioWriteWideStringCharacter
 *
 * Description:
 *  Output a wide string as a multi-byte sequence
 */
#if TRIO_WIDECHAR
TRIO_PRIVATE int
TrioWriteWideStringCharacter(trio_class_t *self,
			     wchar_t wch,
			     unsigned long flags,
			     int width)
{
  int size;
  int i;
  int ch;
  char *string;
  char buffer[MB_LEN_MAX + 1];

  if (width == NO_WIDTH)
    width = sizeof(buffer);
  
  size = wctomb(buffer, wch);
  if ((size <= 0) || (size > width) || (buffer[0] == NIL))
    return 0;

  string = buffer;
  i = size;
  while ((width >= i) && (width-- > 0) && (i-- > 0))
    {
      /* The ctype parameters must be an unsigned char (or EOF) */
      ch = (int)((unsigned char)(*string++));
      TrioWriteStringCharacter(self, ch, flags);
    }
  return size;
}
#endif /* TRIO_WIDECHAR */

/*************************************************************************
 * TrioWriteString
 *
 * Description:
 *  Output a wide character string as a multi-byte string
 */
#if TRIO_WIDECHAR
TRIO_PRIVATE void
TrioWriteWideString(trio_class_t *self,
		    const wchar_t *wstring,
		    unsigned long flags,
		    int width,
		    int precision)
{
  int length;
  int size;

  assert(VALID(self));
  assert(VALID(self->OutStream));

#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  mblen(NULL, 0);
#endif
  
  if (wstring == NULL)
    {
      TrioWriteString(self, NULL, flags, width, precision);
      return;
    }
  
  if (NO_PRECISION == precision)
    {
      length = INT_MAX;
    }
  else
    {
      length = precision;
      width -= length;
    }

  if (flags & FLAGS_QUOTE)
    self->OutStream(self, CHAR_QUOTE);

  if (! (flags & FLAGS_LEFTADJUST))
    {
      while (width-- > 0)
	self->OutStream(self, CHAR_ADJUST);
    }

  while (length > 0)
    {
      size = TrioWriteWideStringCharacter(self, *wstring++, flags, length);
      if (size == 0)
	break; /* while */
      length -= size;
    }

  if (flags & FLAGS_LEFTADJUST)
    {
      while (width-- > 0)
	self->OutStream(self, CHAR_ADJUST);
    }
  if (flags & FLAGS_QUOTE)
    self->OutStream(self, CHAR_QUOTE);
}
#endif /* TRIO_WIDECHAR */

/*************************************************************************
 * TrioWriteDouble
 */
TRIO_PRIVATE void
TrioWriteDouble(trio_class_t *self,
		long double longdoubleNumber,
		unsigned long flags,
		int width,
		int precision,
		int base)
{
  int charsPerThousand;
  int length;
  double number;
  double workNumber;
  int integerDigits;
  int fractionDigits;
  int exponentDigits;
  int expectedWidth;
  int exponent;
  unsigned int uExponent = 0;
  double dblBase;
  BOOLEAN_T isNegative;
  BOOLEAN_T isExponentNegative = FALSE;
  BOOLEAN_T isHex;
  const char *digits;
  char numberBuffer[MAX_MANTISSA_DIGITS
		   * (1 + MAX_LOCALE_SEPARATOR_LENGTH) + 1];
  char *numberPointer;
  char exponentBuffer[MAX_EXPONENT_DIGITS + 1];
  char *exponentPointer = NULL;
  int groupingIndex;
  char *work;
  int i;
  BOOLEAN_T onlyzero;
  int zeroes = 0;
  
  assert(VALID(self));
  assert(VALID(self->OutStream));
  assert(base == BASE_DECIMAL || base == BASE_HEX);

  number = (double)longdoubleNumber;
  
  /* Look for infinite numbers and non-a-number first */
  switch (trio_isinf(number))
    {
    case 1:
      /* Positive infinity */
      TrioWriteString(self,
		      (flags & FLAGS_UPPER)
		      ? INFINITE_UPPER
		      : INFINITE_LOWER,
		      flags, width, precision);
      return;

    case -1:
      /* Negative infinity */
      TrioWriteString(self,
		      (flags & FLAGS_UPPER)
		      ? "-" INFINITE_UPPER
		      : "-" INFINITE_LOWER,
		      flags, width, precision);
      return;

    default:
      /* Finitude */
      break;
    }
  if (trio_isnan(number))
    {
      TrioWriteString(self,
		      (flags & FLAGS_UPPER)
		      ? NAN_UPPER
		      : NAN_LOWER,
		      flags, width, precision);
      return;
    }

  /* Normal numbers */
  digits = (flags & FLAGS_UPPER) ? internalDigitsUpper : internalDigitsLower;
  isHex = (base == BASE_HEX);
  dblBase = (double)base;
  
  if (precision == NO_PRECISION)
    precision = FLT_DIG;
  
  isNegative = (number < 0.0);
  if (isNegative)
    number = -number;

  if ((flags & FLAGS_FLOAT_G) || isHex)
    {
      if (precision == 0)
	precision = 1;

      if ((number < 1.0e-4) || (number > pow(10.0, (double)precision)))
	{
	  /* Use scientific notation */
	  flags |= FLAGS_FLOAT_E;
	}
      else if (number < 1.0)
	{
	  /*
	   * Use normal notation. If the integer part of the number is
	   * zero, then adjust the precision to include leading fractional
	   * zeros.
	   */
	  workNumber = fabs(guarded_log10(number));
	  if (workNumber - floor(workNumber) < 0.001)
	    workNumber--;
	  zeroes = (int)floor(workNumber);
	}
    }

  if (flags & FLAGS_FLOAT_E)
    {
      /* Scale the number */
      workNumber = guarded_log10(number);
      if (trio_isinf(workNumber) == -1)
	{
	  exponent = 0;
	  /* Undo setting */
	  if (flags & FLAGS_FLOAT_G)
	    flags &= ~FLAGS_FLOAT_E;
	}
      else
	{
	  exponent = (int)floor(workNumber);
	  number /= pow(10.0, (double)exponent);
	  isExponentNegative = (exponent < 0);
	  uExponent = (isExponentNegative) ? -exponent : exponent;
	  /* No thousand separators */
	  flags &= ~FLAGS_QUOTE;
	}
    }

  /*
   * Truncated number.
   *
   * precision is number of significant digits for FLOAT_G
   * and number of fractional digits for others
   */
  integerDigits = (floor(number) > DBL_EPSILON)
    ? 1 + (int)guarded_log10(floor(number))
    : 1;
  fractionDigits = ((flags & FLAGS_FLOAT_G) && (zeroes == 0))
    ? precision - integerDigits
    : zeroes + precision;
  
  number = floor(0.5 + number * pow(dblBase, (double)fractionDigits));
  workNumber = (isHex
		? guarded_log16(0.5 + number)
		: guarded_log10(0.5 + number));
  if ((int)workNumber + 1 > integerDigits + fractionDigits)
    {
      if (flags & FLAGS_FLOAT_E)
	{
	  /* Adjust if number was rounded up one digit (ie. 0.99 to 1.00) */
	  exponent--;
	  uExponent -= (isExponentNegative) ? 1 : -1;
	  number /= dblBase;
	}
      else
	{
	  /* Adjust if number was rounded up one digit (ie. 99 to 100) */
	  integerDigits++;
	}
    }
  
  /* Build the fraction part */
  numberPointer = &numberBuffer[sizeof(numberBuffer) - 1];
  *numberPointer = NIL;
  onlyzero = TRUE;
  for (i = 0; i < fractionDigits; i++)
    {
      *(--numberPointer) = digits[(int)fmod(number, dblBase)];
      number = floor(number / dblBase);

      if ((flags & FLAGS_FLOAT_G) && !(flags & FLAGS_ALTERNATIVE))
        {
          /* Prune trailing zeroes */
          if (numberPointer[0] != digits[0])
            onlyzero = FALSE;
          else if (onlyzero && (numberPointer[0] == digits[0]))
            numberPointer++;
        }
      else
        onlyzero = FALSE;
    }
  
  /* Insert decimal point */
  if ((flags & FLAGS_ALTERNATIVE) || ((fractionDigits > 0) && !onlyzero))
    {
      i = StrLength(internalDecimalPoint);
      while (i> 0)
	{
	  *(--numberPointer) = internalDecimalPoint[--i];
	}
    }
  /* Insert the integer part and thousand separators */
  charsPerThousand = (int)internalGrouping[0];
  groupingIndex = 1;
  for (i = 1; i < integerDigits + 1; i++)
    {
      *(--numberPointer) = digits[(int)fmod(number, dblBase)];
      number = floor(number / dblBase);
      if (number < DBL_EPSILON)
	break;

      if ((i > 0)
	  && ((flags & (FLAGS_FLOAT_E | FLAGS_QUOTE)) == FLAGS_QUOTE)
	  && (charsPerThousand != NO_GROUPING)
	  && (i % charsPerThousand == 0))
	{
	  /*
	   * We are building the number from the least significant
	   * to the most significant digit, so we have to copy the
	   * thousand separator backwards
	   */
	  length = StrLength(internalThousandSeparator);
	  integerDigits += length;
	  if (((int)(numberPointer - numberBuffer) - length) > 0)
	    {
	      work = &internalThousandSeparator[length - 1];
	      while (length-- > 0)
		*(--numberPointer) = *work--;
	    }

	  /* Advance to next grouping number */
	  if (charsPerThousand != NO_GROUPING)
	    {
	      switch (internalGrouping[groupingIndex])
		{
		case CHAR_MAX: /* Disable grouping */
		  charsPerThousand = NO_GROUPING;
		  break;
		case 0: /* Repeat last group */
		  break;
		default:
		  charsPerThousand = (int)internalGrouping[groupingIndex++];
		  break;
		}
	    }
	}
    }
  
  /* Build the exponent */
  exponentDigits = 0;
  if (flags & FLAGS_FLOAT_E)
    {
      exponentPointer = &exponentBuffer[sizeof(exponentBuffer) - 1];
      *exponentPointer-- = NIL;
      do {
	*exponentPointer-- = digits[uExponent % base];
	uExponent /= base;
	exponentDigits++;
      } while (uExponent);
    }

  /*
   * Calculate expected width.
   *  sign + integer part + thousands separators + decimal point
   *  + fraction + exponent
   */
  expectedWidth = StrLength(numberPointer);
  if (isNegative || (flags & FLAGS_SHOWSIGN))
    expectedWidth += sizeof("-") - 1;
  if (exponentDigits > 0)
    expectedWidth += exponentDigits +
      ((exponentDigits > 1) ? sizeof("E+") : sizeof("E+0")) - 1;
  if (isHex)
    expectedWidth += sizeof("0X") - 1;
  
  /* Output prefixing */
  if (flags & FLAGS_NILPADDING)
    {
      /* Leading zeros must be after sign */
      if (isNegative)
	self->OutStream(self, '-');
      else if (flags & FLAGS_SHOWSIGN)
	self->OutStream(self, '+');
      if (isHex)
	{
	  self->OutStream(self, '0');
	  self->OutStream(self, (flags & FLAGS_UPPER) ? 'X' : 'x');
	}
      if (!(flags & FLAGS_LEFTADJUST))
	{
	  for (i = expectedWidth; i < width; i++)
	    {
	      self->OutStream(self, '0');
	    }
	}
    }
  else
    {
      /* Leading spaces must be before sign */
      if (!(flags & FLAGS_LEFTADJUST))
	{
	  for (i = expectedWidth; i < width; i++)
	    {
	      self->OutStream(self, CHAR_ADJUST);
	    }
	}
      if (isNegative)
	self->OutStream(self, '-');
      else if (flags & FLAGS_SHOWSIGN)
	self->OutStream(self, '+');
      if (isHex)
	{
	  self->OutStream(self, '0');
	  self->OutStream(self, (flags & FLAGS_UPPER) ? 'X' : 'x');
	}
    }
  /* Output number */
  for (i = 0; numberPointer[i]; i++)
    {
      self->OutStream(self, numberPointer[i]);
    }
  /* Output exponent */
  if (exponentDigits > 0)
    {
      self->OutStream(self,
		      isHex
		      ? ((flags & FLAGS_UPPER) ? 'P' : 'p')
		      : ((flags & FLAGS_UPPER) ? 'E' : 'e'));
      self->OutStream(self, (isExponentNegative) ? '-' : '+');

      /* The exponent must contain at least two digits */
      if (exponentDigits == 1)
        self->OutStream(self, '0');

      for (i = 0; i < exponentDigits; i++)
	{
	  self->OutStream(self, exponentPointer[i + 1]);
	}
    }
  /* Output trailing spaces */
  if (flags & FLAGS_LEFTADJUST)
    {
      for (i = expectedWidth; i < width; i++)
	{
	  self->OutStream(self, CHAR_ADJUST);
	}
    }
}

/*************************************************************************
 * TrioFormatProcess
 */
TRIO_PRIVATE int
TrioFormatProcess(trio_class_t *data,
		  const char *format,
		  trio_parameter_t *parameters)

{
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  int charlen;
#endif
  int i;
  const char *string;
  void *pointer;
  unsigned long flags;
  int width;
  int precision;
  int base;
  int index;
  
  index = 0;
  i = 0;
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  mblen(NULL, 0);
#endif
  
  while (format[index])
    {
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
      if (! isascii(format[index]))
	{
	  charlen = mblen(&format[index], MB_LEN_MAX);
	  while (charlen-- > 0)
	    {
	      data->OutStream(data, format[index++]);
	    }
	  continue; /* while */
	}
#endif /* TRIO_COMPILER_SUPPORTS_MULTIBYTE */
      if (CHAR_IDENTIFIER == format[index])
	{
	  if (CHAR_IDENTIFIER == format[index + 1])
	    {
	      data->OutStream(data, CHAR_IDENTIFIER);
	      index += 2;
	    }
	  else
	    {
	      /* Skip the parameter entries */
	      while (parameters[i].type == FORMAT_PARAMETER)
		i++;
	      
	      flags = parameters[i].flags;

	      /* Find width */
	      width = parameters[i].width;
	      if (flags & FLAGS_WIDTH_PARAMETER)
		{
		  /* Get width from parameter list */
		  width = (int)parameters[width].data.number.as_signed;
		}
	      
	      /* Find precision */
	      if (flags & FLAGS_PRECISION)
		{
		  precision = parameters[i].precision;
		  if (flags & FLAGS_PRECISION_PARAMETER)
		    {
		      /* Get precision from parameter list */
		      precision = (int)parameters[precision].data.number.as_signed;
		    }
		}
	      else
		{
		  precision = NO_PRECISION;
		}

	      /* Find base */
	      base = parameters[i].base;
	      if (flags & FLAGS_BASE_PARAMETER)
		{
		  /* Get base from parameter list */
		  base = (int)parameters[base].data.number.as_signed;
		}
	      
	      switch (parameters[i].type)
		{
		case FORMAT_CHAR:
		  if (flags & FLAGS_QUOTE)
		    data->OutStream(data, CHAR_QUOTE);
		  if (! (flags & FLAGS_LEFTADJUST))
		    {
		      while (--width > 0)
			data->OutStream(data, CHAR_ADJUST);
		    }
#if TRIO_WIDECHAR
		  if (flags & FLAGS_WIDECHAR)
		    {
		      TrioWriteWideStringCharacter(data,
						   (wchar_t)parameters[i].data.number.as_signed,
						   flags,
						   NO_WIDTH);
		    }
		  else
#endif
		    {
		      TrioWriteStringCharacter(data,
					       (int)parameters[i].data.number.as_signed,
					       flags);
		    }

		  if (flags & FLAGS_LEFTADJUST)
		    {
		      while(--width > 0)
			data->OutStream(data, CHAR_ADJUST);
		    }
		  if (flags & FLAGS_QUOTE)
		    data->OutStream(data, CHAR_QUOTE);

		  break; /* FORMAT_CHAR */

		case FORMAT_INT:
		  if (base == NO_BASE)
		    base = BASE_DECIMAL;

		  TrioWriteNumber(data,
				  parameters[i].data.number.as_signed,
				  flags,
				  width,
				  precision,
				  base);

		  break; /* FORMAT_INT */

		case FORMAT_DOUBLE:
		  TrioWriteDouble(data,
				  parameters[i].data.longdoubleNumber,
				  flags,
				  width,
				  precision,
				  base);
		  break; /* FORMAT_DOUBLE */

		case FORMAT_STRING:
#if TRIO_WIDECHAR
		  if (flags & FLAGS_WIDECHAR)
		    {
		      TrioWriteWideString(data,
					  parameters[i].data.wstring,
					  flags,
					  width,
					  precision);
		    }
		  else
#endif
		    {
		      TrioWriteString(data,
				      parameters[i].data.string,
				      flags,
				      width,
				      precision);
		    }
		  break; /* FORMAT_STRING */

		case FORMAT_POINTER:
		  {
		    trio_reference_t reference;
		    
		    reference.data = data;
		    reference.parameter = &parameters[i];
		    trio_print_pointer(&reference, parameters[i].data.pointer);
		  }
		  break; /* FORMAT_POINTER */

		case FORMAT_COUNT:
		  pointer = parameters[i].data.pointer;
		  if (NULL != pointer)
		    {
		      /*
		       * C99 paragraph 7.19.6.1.8 says "the number of
		       * characters written to the output stream so far by
		       * this call", which is data->committed
		       */
#if defined(QUALIFIER_SIZE_T) || defined(QUALIFIER_SIZE_T_UPPER)
		      if (flags & FLAGS_SIZE_T)
			*(size_t *)pointer = (size_t)data->committed;
		      else
#endif
#if defined(QUALIFIER_PTRDIFF_T)
		      if (flags & FLAGS_PTRDIFF_T)
			*(ptrdiff_t *)pointer = (ptrdiff_t)data->committed;
		      else
#endif
#if defined(QUALIFIER_INTMAX_T)
		      if (flags & FLAGS_INTMAX_T)
			*(trio_intmax_t *)pointer = (trio_intmax_t)data->committed;
		      else
#endif
		      if (flags & FLAGS_QUAD)
			{
			  *(trio_ulonglong_t *)pointer = (trio_ulonglong_t)data->committed;
			}
		      else if (flags & FLAGS_LONG)
			{
			  *(long int *)pointer = (long int)data->committed;
			}
		      else if (flags & FLAGS_SHORT)
			{
			  *(short int *)pointer = (short int)data->committed;
			}
		      else
			{
			  *(int *)pointer = (int)data->committed;
			}
		    }
		  break; /* FORMAT_COUNT */

		case FORMAT_PARAMETER:
		  break; /* FORMAT_PARAMETER */

#if defined(FORMAT_ERRNO)
		case FORMAT_ERRNO:
		  string = StrError(parameters[i].data.errorNumber);
		  if (string)
		    {
		      TrioWriteString(data,
				      string,
				      flags,
				      width,
				      precision);
		    }
		  else
		    {
		      data->OutStream(data, '#');
		      TrioWriteNumber(data,
				      (trio_intmax_t)parameters[i].data.errorNumber,
				      flags,
				      width,
				      precision,
				      BASE_DECIMAL);
		    }
		  break; /* FORMAT_ERRNO */
#endif /* defined(FORMAT_ERRNO) */

#if defined(FORMAT_USER_DEFINED)
		case FORMAT_USER_DEFINED:
		  {
		    trio_reference_t reference;
		    trio_userdef_t *def = NULL;

		    if (parameters[i].user_name[0] == NIL)
		      {
			/* Use handle */
			if ((i > 0) ||
			    (parameters[i - 1].type == FORMAT_PARAMETER))
			  def = (trio_userdef_t *)parameters[i - 1].data.pointer;
		      }
		    else
		      {
			/* Look up namespace */
			def = TrioFindNamespace(parameters[i].user_name, NULL);
		      }
		    if (def) {
		      reference.data = data;
		      reference.parameter = &parameters[i];
		      def->callback(&reference);
		    }
		  }
		  break;
#endif /* defined(FORMAT_USER_DEFINED) */
		  
		default:
		  break;
		} /* switch parameter type */

	      /* Prepare for next */
	      index = parameters[i].indexAfterSpecifier;
	      i++;
	    }
	}
      else /* not identifier */
	{
	  data->OutStream(data, format[index++]);
	}
    }
  return data->processed;
}

/*************************************************************************
 * TrioFormatRef
 */
TRIO_PRIVATE int
TrioFormatRef(trio_reference_t *reference,
	      const char *format,
	      va_list arglist,
	      void **argarray)
{
  int status;
  trio_parameter_t parameters[MAX_PARAMETERS];

  status = TrioParse(TYPE_PRINT, format, parameters, arglist, argarray);
  if (status < 0)
    return status;

  return TrioFormatProcess(reference->data, format, parameters);
}

/*************************************************************************
 * TrioFormat
 *
 * Description:
 *  This is the main engine for formatting output
 */
TRIO_PRIVATE int
TrioFormat(void *destination,
	   size_t destinationSize,
	   void (*OutStream)(trio_class_t *, int),
	   const char *format,
	   va_list arglist,
	   void **argarray)
{
  int status;
  trio_class_t data;
  trio_parameter_t parameters[MAX_PARAMETERS];

  assert(VALID(OutStream));
  assert(VALID(format));

  memset(&data, 0, sizeof(data));
  data.OutStream = OutStream;
  data.location = destination;
  data.max = destinationSize;

#if defined(USE_LOCALE)
  if (NULL == internalLocaleValues)
    {
      TrioSetLocale();
    }
#endif

  status = TrioParse(TYPE_PRINT, format, parameters, arglist, argarray);
  if (status < 0)
    return status;

  return TrioFormatProcess(&data, format, parameters);
}

/*************************************************************************
 * TrioOutStreamFile
 */
TRIO_PRIVATE void
TrioOutStreamFile(trio_class_t *self,
		  int output)
{
  FILE *file = (FILE *)self->location;

  assert(VALID(self));
  assert(VALID(file));

  self->processed++;
  self->committed++;
  (void)fputc(output, file);
}

/*************************************************************************
 * TrioOutStreamFileDescriptor
 */
TRIO_PRIVATE void
TrioOutStreamFileDescriptor(trio_class_t *self,
			    int output)
{
  int fd = *((int *)self->location);
  char ch;

  assert(VALID(self));

  ch = (char)output;
  (void)write(fd, &ch, sizeof(char));
  self->processed++;
  self->committed++;
}

/*************************************************************************
 * TrioOutStreamString
 */
TRIO_PRIVATE void
TrioOutStreamString(trio_class_t *self,
		    int output)
{
  char **buffer = (char **)self->location;

  assert(VALID(self));
  assert(VALID(buffer));

  **buffer = (char)output;
  (*buffer)++;
  self->processed++;
  self->committed++;
}

/*************************************************************************
 * TrioOutStreamStringMax
 */
TRIO_PRIVATE void
TrioOutStreamStringMax(trio_class_t *self,
		       int output)
{
  char **buffer;

  assert(VALID(self));
  buffer = (char **)self->location;
  assert(VALID(buffer));

  if (self->processed < self->max)
    {
      **buffer = (char)output;
      (*buffer)++;
      self->committed++;
    }
  self->processed++;
}

/*************************************************************************
 * TrioOutStreamStringDynamic
 */
#define DYNAMIC_START_SIZE 32
struct dynamicBuffer {
  char *buffer;
  size_t length;
  size_t allocated;
};

TRIO_PRIVATE void
TrioOutStreamStringDynamic(trio_class_t *self,
			   int output)
{
  struct dynamicBuffer *infop;
  
  assert(VALID(self));
  assert(VALID(self->location));

  infop = (struct dynamicBuffer *)self->location;

  if (infop->buffer == NULL)
    {
      /* Start with a reasonable size */
      infop->buffer = (char *)TRIO_MALLOC(DYNAMIC_START_SIZE);
      if (infop->buffer == NULL)
	return; /* fail */
      
      infop->allocated = DYNAMIC_START_SIZE;
      self->processed = 0;
      self->committed = 0;
    }
  else if (self->committed + sizeof(NIL) >= infop->allocated)
    {
      char *newptr;
      
      /* Allocate increasing chunks */
      newptr = (char *)TRIO_REALLOC(infop->buffer, infop->allocated * 2);
      
      if (newptr == NULL)
	return;

      infop->buffer = newptr;
      infop->allocated *= 2;
    }
  
  infop->buffer[self->committed] = (char)output;
  self->committed++;
  self->processed++;
  
  infop->length = self->committed;
}

/*************************************************************************
 * printf
 */
int
trio_printf(const char *format,
	    ...)
{
  int status;
  va_list args;

  assert(VALID(format));
  
  va_start(args, format);
  status = TrioFormat(stdout, 0, TrioOutStreamFile, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vprintf(const char *format,
	     va_list args)
{
  assert(VALID(format));

  return TrioFormat(stdout, 0, TrioOutStreamFile, format, args, NULL);
}

int
trio_printfv(const char *format,
	     void ** args)
{
  va_list dummy;
  
  assert(VALID(format));

  return TrioFormat(stdout, 0, TrioOutStreamFile, format, dummy, args);
}

/*************************************************************************
 * fprintf
 */
int
trio_fprintf(FILE *file,
	     const char *format,
	     ...)
{
  int status;
  va_list args;

  assert(VALID(file));
  assert(VALID(format));
  
  va_start(args, format);
  status = TrioFormat(file, 0, TrioOutStreamFile, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vfprintf(FILE *file,
	      const char *format,
	      va_list args)
{
  assert(VALID(file));
  assert(VALID(format));
  
  return TrioFormat(file, 0, TrioOutStreamFile, format, args, NULL);
}

int
trio_fprintfv(FILE *file,
	      const char *format,
	      void ** args)
{
  va_list dummy;
  
  assert(VALID(file));
  assert(VALID(format));
  
  return TrioFormat(file, 0, TrioOutStreamFile, format, dummy, args);
}

/*************************************************************************
 * dprintf
 */
int
trio_dprintf(int fd,
	     const char *format,
	     ...)
{
  int status;
  va_list args;

  assert(VALID(format));
  
  va_start(args, format);
  status = TrioFormat(&fd, 0, TrioOutStreamFileDescriptor, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vdprintf(int fd,
	      const char *format,
	      va_list args)
{
  assert(VALID(format));
  
  return TrioFormat(&fd, 0, TrioOutStreamFileDescriptor, format, args, NULL);
}

int
trio_dprintfv(int fd,
	      const char *format,
	      void **args)
{
  va_list dummy;
  
  assert(VALID(format));
  
  return TrioFormat(&fd, 0, TrioOutStreamFileDescriptor, format, dummy, args);
}

/*************************************************************************
 * sprintf
 */
int
trio_sprintf(char *buffer,
	     const char *format,
	     ...)
{
  int status;
  va_list args;

  assert(VALID(buffer));
  assert(VALID(format));
  
  va_start(args, format);
  status = TrioFormat(&buffer, 0, TrioOutStreamString, format, args, NULL);
  *buffer = NIL; /* Terminate with NIL character */
  va_end(args);
  return status;
}

int
trio_vsprintf(char *buffer,
	      const char *format,
	      va_list args)
{
  int status;

  assert(VALID(buffer));
  assert(VALID(format));

  status = TrioFormat(&buffer, 0, TrioOutStreamString, format, args, NULL);
  *buffer = NIL;
  return status;
}

int
trio_sprintfv(char *buffer,
	      const char *format,
	      void **args)
{
  int status;
  va_list dummy;

  assert(VALID(buffer));
  assert(VALID(format));

  status = TrioFormat(&buffer, 0, TrioOutStreamString, format, dummy, args);
  *buffer = NIL;
  return status;
}

/*************************************************************************
 * snprintf
 */
int
trio_snprintf(char *buffer,
	      size_t bufferSize,
	      const char *format,
	      ...)
{
  int status;
  va_list args;

  assert(VALID(buffer));
  assert(VALID(format));

  va_start(args, format);
  status = TrioFormat(&buffer, bufferSize > 0 ? bufferSize - 1 : 0,
		      TrioOutStreamStringMax, format, args, NULL);
  if (bufferSize > 0)
    *buffer = NIL;
  va_end(args);
  return status;
}

int
trio_vsnprintf(char *buffer,
	       size_t bufferSize,
	       const char *format,
	       va_list args)
{
  int status;

  assert(VALID(buffer));
  assert(VALID(format));

  status = TrioFormat(&buffer, bufferSize > 0 ? bufferSize - 1 : 0,
		      TrioOutStreamStringMax, format, args, NULL);
  if (bufferSize > 0)
    *buffer = NIL;
  return status;
}

int
trio_snprintfv(char *buffer,
	       size_t bufferSize,
	       const char *format,
	       void **args)
{
  int status;
  va_list dummy;

  assert(VALID(buffer));
  assert(VALID(format));

  status = TrioFormat(&buffer, bufferSize > 0 ? bufferSize - 1 : 0,
		      TrioOutStreamStringMax, format, dummy, args);
  if (bufferSize > 0)
    *buffer = NIL;
  return status;
}

/*************************************************************************
 * snprintfcat
 * Appends the new string to the buffer string overwriting the '\0'
 * character at the end of buffer.
 */
int
trio_snprintfcat(char *buffer,
		 size_t bufferSize,
		 const char *format,
		 ...)
{
  int status;
  va_list args;
  size_t buf_len;

  va_start(args, format);

  assert(VALID(buffer));
  assert(VALID(format));

  buf_len = strlen(buffer);
  buffer = &buffer[buf_len];

  status = TrioFormat(&buffer, bufferSize - 1 - buf_len,
		      TrioOutStreamStringMax, format, args, NULL);
  va_end(args);
  *buffer = NIL;
  return status;
}

int
trio_vsnprintfcat(char *buffer,
		  size_t bufferSize,
		  const char *format,
		  va_list args)
{
  int status;
  size_t buf_len;
  assert(VALID(buffer));
  assert(VALID(format));

  buf_len = strlen(buffer);
  buffer = &buffer[buf_len];
  status = TrioFormat(&buffer, bufferSize - 1 - buf_len,
		      TrioOutStreamStringMax, format, args, NULL);
  *buffer = NIL;
  return status;
}

/*************************************************************************
 * trio_aprintf
 */

/* Deprecated */
char *
trio_aprintf(const char *format,
	     ...)
{
  va_list args;
  struct dynamicBuffer info;

  assert(VALID(format));
  
  info.buffer = NULL;
  info.length = 0;
  info.allocated = 0;

  va_start(args, format);
  (void)TrioFormat(&info, 0, TrioOutStreamStringDynamic, format, args, NULL);
  va_end(args);
  if (info.length) {
    info.buffer[info.length] = NIL; /* we terminate this with a zero byte */
    return info.buffer;
  }
  else
    return NULL;
}

/* Deprecated */
char *
trio_vaprintf(const char *format,
	      va_list args)
{
  struct dynamicBuffer info;

  assert(VALID(format));
  
  info.buffer = NULL;
  info.length = 0;
  info.allocated = 0;

  (void)TrioFormat(&info, 0, TrioOutStreamStringDynamic, format, args, NULL);
  if (info.length) {
    info.buffer[info.length] = NIL; /* we terminate this with a zero byte */
    return info.buffer;
  }
  else
    return NULL;
}

int
trio_asprintf(char **result,
	      const char *format,
	      ...)
{
  va_list args;
  int status;
  struct dynamicBuffer info;

  assert(VALID(format));

  info.buffer = NULL;
  info.length = 0;
  info.allocated = 0;

  va_start(args, format);
  status = TrioFormat(&info, 0, TrioOutStreamStringDynamic, format, args, NULL);
  va_end(args);
  if (status < 0) {
     *result = NULL;
     return status;
  }
  if (info.length == 0) {
    /*
     * If the length is zero, no characters have been written and therefore
     * no memory has been allocated, but we must to allocate and return an
     * empty string.
     */
    info.buffer = (char *)TRIO_MALLOC(sizeof(char));
    if (info.buffer == NULL) {
      *result = NULL;
      return TRIO_ERROR_RETURN(TRIO_ENOMEM, 0);
    }
  }
  info.buffer[info.length] = NIL; /* we terminate this with a zero byte */
  *result = info.buffer;
  
  return status;
}

int
trio_vasprintf(char **result,
	       const char *format,
	       va_list args)
{
  int status;
  struct dynamicBuffer info;

  assert(VALID(format));

  info.buffer = NULL;
  info.length = 0;
  info.allocated = 0;

  status = TrioFormat(&info, 0, TrioOutStreamStringDynamic, format, args, NULL);
  if (status < 0) {
     *result = NULL;
     return status;
  }
  if (info.length == 0) {
    info.buffer = (char *)TRIO_MALLOC(sizeof(char));
    if (info.buffer == NULL) {
      *result = NULL;
      return TRIO_ERROR_RETURN(TRIO_ENOMEM, 0);
    }
  }
  info.buffer[info.length] = NIL; /* we terminate this with a zero byte */
  *result = info.buffer;
  
  return status;
}


/*************************************************************************
 *
 * @CALLBACK
 *
 ************************************************************************/


/*************************************************************************
 * trio_register [public]
 */
void *
trio_register(trio_callback_t callback,
	      const char *name)
{
  trio_userdef_t *def;
  trio_userdef_t *prev = NULL;

  if (callback == NULL)
    return NULL;

  if (name)
    {
      /* Handle built-in namespaces */
      if (name[0] == ':')
	{
	  if (StrEqual(name, ":enter"))
	    {
	      internalEnterCriticalRegion = callback;
	    }
	  else if (StrEqual(name, ":leave"))
	    {
	      internalLeaveCriticalRegion = callback;
	    }
	  return NULL;
	}
      
      /* Bail out if namespace is too long */
      if (StrLength(name) >= MAX_USER_NAME)
	return NULL;
      
      /* Bail out if namespace already is registered */
      def = TrioFindNamespace(name, &prev);
      if (def)
	return NULL;
    }
  
  def = (trio_userdef_t *)TRIO_MALLOC(sizeof(trio_userdef_t));
  if (def)
    {
      if (internalEnterCriticalRegion)
	(void)internalEnterCriticalRegion(NULL);
      
      if (name)
	{
	  /* Link into internal list */
	  if (prev == NULL)
	    internalUserDef = def;
	  else
	    prev->next = def;
	}
      /* Initialize */
      def->callback = callback;
      def->name = (name == NULL)
	? NULL
	: StrDuplicate(name);
      def->next = NULL;

      if (internalLeaveCriticalRegion)
	(void)internalLeaveCriticalRegion(NULL);
    }
  return def;
}

/*************************************************************************
 * trio_unregister [public]
 */
void
trio_unregister(void *handle)
{
  trio_userdef_t *self = (trio_userdef_t *)handle;
  trio_userdef_t *def;
  trio_userdef_t *prev = NULL;

  assert(VALID(self));

  if (self->name)
    {
      def = TrioFindNamespace(self->name, &prev);
      if (def)
	{
	  if (internalEnterCriticalRegion)
	    (void)internalEnterCriticalRegion(NULL);
	  
	  if (prev == NULL)
	    internalUserDef = NULL;
	  else
	    prev->next = def->next;
	  
	  if (internalLeaveCriticalRegion)
	    (void)internalLeaveCriticalRegion(NULL);
	}
      StrFree(self->name);
    }
  TRIO_FREE(self);
}

/*************************************************************************
 * trio_get_format [public]
 */
const char *
trio_get_format(void *ref)
{
  assert(((trio_reference_t *)ref)->parameter->type == FORMAT_USER_DEFINED);
  
  return (((trio_reference_t *)ref)->parameter->user_data);
}

/*************************************************************************
 * trio_get_argument [public]
 */
void *
trio_get_argument(void *ref)
{
  assert(((trio_reference_t *)ref)->parameter->type == FORMAT_USER_DEFINED);
  
  return ((trio_reference_t *)ref)->parameter->data.pointer;
}

/*************************************************************************
 * trio_get_width / trio_set_width [public]
 */
int
trio_get_width(void *ref)
{
  return ((trio_reference_t *)ref)->parameter->width;
}

void
trio_set_width(void *ref,
	       int width)
{
  ((trio_reference_t *)ref)->parameter->width = width;
}

/*************************************************************************
 * trio_get_precision / trio_set_precision [public]
 */
int
trio_get_precision(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->precision);
}

void
trio_set_precision(void *ref,
		   int precision)
{
  ((trio_reference_t *)ref)->parameter->precision = precision;
}

/*************************************************************************
 * trio_get_base / trio_set_base [public]
 */
int
trio_get_base(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->base);
}

void
trio_set_base(void *ref,
	      int base)
{
  ((trio_reference_t *)ref)->parameter->base = base;
}

/*************************************************************************
 * trio_get_long / trio_set_long [public]
 */
int
trio_get_long(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_LONG);
}

void
trio_set_long(void *ref,
	      int is_long)
{
  if (is_long)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_LONG;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_LONG;
}

/*************************************************************************
 * trio_get_longlong / trio_set_longlong [public]
 */
int
trio_get_longlong(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_QUAD);
}

void
trio_set_longlong(void *ref,
		  int is_longlong)
{
  if (is_longlong)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_QUAD;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_QUAD;
}

/*************************************************************************
 * trio_get_longdouble / trio_set_longdouble [public]
 */
int
trio_get_longdouble(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_LONGDOUBLE);
}

void
trio_set_longdouble(void *ref,
		    int is_longdouble)
{
  if (is_longdouble)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_LONGDOUBLE;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_LONGDOUBLE;
}

/*************************************************************************
 * trio_get_short / trio_set_short [public]
 */
int
trio_get_short(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_SHORT);
}

void
trio_set_short(void *ref,
	       int is_short)
{
  if (is_short)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_SHORT;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_SHORT;
}

/*************************************************************************
 * trio_get_shortshort / trio_set_shortshort [public]
 */
int
trio_get_shortshort(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_SHORTSHORT);
}

void
trio_set_shortshort(void *ref,
	       int is_shortshort)
{
  if (is_shortshort)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_SHORTSHORT;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_SHORTSHORT;
}

/*************************************************************************
 * trio_get_alternative / trio_set_alternative [public]
 */
int
trio_get_alternative(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_ALTERNATIVE);
}

void
trio_set_alternative(void *ref,
		     int is_alternative)
{
  if (is_alternative)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_ALTERNATIVE;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_ALTERNATIVE;
}

/*************************************************************************
 * trio_get_alignment / trio_set_alignment [public]
 */
int
trio_get_alignment(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_LEFTADJUST);
}

void
trio_set_alignment(void *ref,
		   int is_leftaligned)
{
  if (is_leftaligned)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_LEFTADJUST;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_LEFTADJUST;
}

/*************************************************************************
 * trio_get_spacing /trio_set_spacing [public]
 */
int
trio_get_spacing(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_SPACE);
}

void
trio_set_spacing(void *ref,
		 int is_space)
{
  if (is_space)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_SPACE;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_SPACE;
}

/*************************************************************************
 * trio_get_sign / trio_set_sign [public]
 */
int
trio_get_sign(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_SHOWSIGN);
}

void
trio_set_sign(void *ref,
	      int is_sign)
{
  if (is_sign)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_SHOWSIGN;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_SHOWSIGN;
}

/*************************************************************************
 * trio_get_padding / trio_set_padding [public]
 */
int
trio_get_padding(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_NILPADDING);
}

void
trio_set_padding(void *ref,
		 int is_padding)
{
  if (is_padding)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_NILPADDING;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_NILPADDING;
}

/*************************************************************************
 * trio_get_quote / trio_set_quote [public]
 */
int
trio_get_quote(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_QUOTE);
}

void
trio_set_quote(void *ref,
	       int is_quote)
{
  if (is_quote)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_QUOTE;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_QUOTE;
}

/*************************************************************************
 * trio_get_upper / trio_set_upper [public]
 */
int
trio_get_upper(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_UPPER);
}

void
trio_set_upper(void *ref,
	       int is_upper)
{
  if (is_upper)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_UPPER;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_UPPER;
}

/*************************************************************************
 * trio_get_largest / trio_set_largest [public]
 */
#if TRIO_C99
int
trio_get_largest(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_INTMAX_T);
}

void
trio_set_largest(void *ref,
		 int is_largest)
{
  if (is_largest)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_INTMAX_T;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_INTMAX_T;
}
#endif

/*************************************************************************
 * trio_get_ptrdiff / trio_set_ptrdiff [public]
 */
int
trio_get_ptrdiff(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_PTRDIFF_T);
}

void
trio_set_ptrdiff(void *ref,
		 int is_ptrdiff)
{
  if (is_ptrdiff)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_PTRDIFF_T;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_PTRDIFF_T;
}

/*************************************************************************
 * trio_get_size / trio_set_size [public]
 */
#if TRIO_C99
int
trio_get_size(void *ref)
{
  return (((trio_reference_t *)ref)->parameter->flags & FLAGS_SIZE_T);
}

void
trio_set_size(void *ref,
	      int is_size)
{
  if (is_size)
    ((trio_reference_t *)ref)->parameter->flags |= FLAGS_SIZE_T;
  else
    ((trio_reference_t *)ref)->parameter->flags &= ~FLAGS_SIZE_T;
}
#endif

/*************************************************************************
 * trio_print_int [public]
 */
void
trio_print_int(void *ref,
	       int number)
{
  trio_reference_t *self = (trio_reference_t *)ref;

  TrioWriteNumber(self->data,
		  (trio_intmax_t)number,
		  self->parameter->flags,
		  self->parameter->width,
		  self->parameter->precision,
		  self->parameter->base);
}

/*************************************************************************
 * trio_print_uint [public]
 */
void
trio_print_uint(void *ref,
		unsigned int number)
{
  trio_reference_t *self = (trio_reference_t *)ref;

  TrioWriteNumber(self->data,
		  (trio_intmax_t)number,
		  self->parameter->flags | FLAGS_UNSIGNED,
		  self->parameter->width,
		  self->parameter->precision,
		  self->parameter->base);
}

/*************************************************************************
 * trio_print_double [public]
 */
void
trio_print_double(void *ref,
		  double number)
{
  trio_reference_t *self = (trio_reference_t *)ref;

  TrioWriteDouble(self->data,
		  number,
		  self->parameter->flags,
		  self->parameter->width,
		  self->parameter->precision,
		  self->parameter->base);
}

/*************************************************************************
 * trio_print_string [public]
 */
void
trio_print_string(void *ref,
		  char *string)
{
  trio_reference_t *self = (trio_reference_t *)ref;

  TrioWriteString(self->data,
		  string,
		  self->parameter->flags,
		  self->parameter->width,
		  self->parameter->precision);
}

/*************************************************************************
 * trio_print_pointer [public]
 */
void
trio_print_pointer(void *ref,
		   void *pointer)
{
  trio_reference_t *self = (trio_reference_t *)ref;
  unsigned long flags;
  trio_uintmax_t number;

  if (NULL == pointer)
    {
      const char *string = internalNullString;
      while (*string)
	self->data->OutStream(self->data, *string++);
    }
  else
    {
      /*
       * The subtraction of the null pointer is a workaround
       * to avoid a compiler warning. The performance overhead
       * is negligible (and likely to be removed by an
       * optimising compiler). The (char *) casting is done
       * to please ANSI C++.
       */
      number = (trio_uintmax_t)((char *)pointer - (char *)0);
      /* Shrink to size of pointer */
      number &= (trio_uintmax_t)-1;
      flags = self->parameter->flags;
      flags |= (FLAGS_UNSIGNED | FLAGS_ALTERNATIVE |
	        FLAGS_NILPADDING);
      TrioWriteNumber(self->data,
		      (trio_intmax_t)number,
		      flags,
		      POINTER_WIDTH,
		      NO_PRECISION,
		      BASE_HEX);
    }
}

/*************************************************************************
 * trio_print_ref [public]
 */
int
trio_print_ref(void *ref,
	       const char *format,
	       ...)
{
  int status;
  va_list arglist;

  assert(VALID(format));
  
  va_start(arglist, format);
  status = TrioFormatRef((trio_reference_t *)ref, format, arglist, NULL);
  va_end(arglist);
  return status;
}

/*************************************************************************
 * trio_vprint_ref [public]
 */
int
trio_vprint_ref(void *ref,
		const char *format,
		va_list arglist)
{
  assert(VALID(format));
  
  return TrioFormatRef((trio_reference_t *)ref, format, arglist, NULL);
}

/*************************************************************************
 * trio_printv_ref [public]
 */
int
trio_printv_ref(void *ref,
		const char *format,
		void **argarray)
{
  va_list dummy;
  
  assert(VALID(format));
  
  return TrioFormatRef((trio_reference_t *)ref, format, dummy, argarray);
}


/*************************************************************************
 *
 * @SCANNING
 *
 ************************************************************************/


/*************************************************************************
 * TrioSkipWhitespaces
 */
TRIO_PRIVATE int
TrioSkipWhitespaces(trio_class_t *self)
{
  int ch;

  ch = self->current;
  while (isspace(ch))
    {
      self->InStream(self, &ch);
    }
  return ch;
}

/*************************************************************************
 * TrioGetCollation
 */
#if TRIO_EXTENSION
TRIO_PRIVATE void
TrioGetCollation()
{
  int i;
  int j;
  int k;
  char first[2];
  char second[2];

  /* This is computational expensive */
  first[1] = NIL;
  second[1] = NIL;
  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
    {
      k = 0;
      first[0] = (char)i;
      for (j = 0; j < MAX_CHARACTER_CLASS; j++)
	{
	  second[0] = (char)j;
	  if (StrEqualLocale(first, second))
	    internalCollationArray[i][k++] = (char)j;
	}
      internalCollationArray[i][k] = NIL;
    }
}
#endif

/*************************************************************************
 * TrioGetCharacterClass
 *
 * FIXME:
 *  multibyte
 */
TRIO_PRIVATE int
TrioGetCharacterClass(const char *format,
		      int *indexPointer,
		      unsigned long *flagsPointer,
		      int *characterclass)
{
  int index = *indexPointer;
  int i;
  char ch;
  char range_begin;
  char range_end;

  *flagsPointer &= ~FLAGS_EXCLUDE;

  if (format[index] == QUALIFIER_CIRCUMFLEX)
    {
      *flagsPointer |= FLAGS_EXCLUDE;
      index++;
    }
  /*
   * If the ungroup character is at the beginning of the scanlist,
   * it will be part of the class, and a second ungroup character
   * must follow to end the group.
   */
  if (format[index] == SPECIFIER_UNGROUP)
    {
      characterclass[(int)SPECIFIER_UNGROUP]++;
      index++;
    }
  /*
   * Minus is used to specify ranges. To include minus in the class,
   * it must be at the beginning of the list
   */
  if (format[index] == QUALIFIER_MINUS)
    {
      characterclass[(int)QUALIFIER_MINUS]++;
      index++;
    }
  /* Collect characters */
  for (ch = format[index];
       (ch != SPECIFIER_UNGROUP) && (ch != NIL);
       ch = format[++index])
    {
      switch (ch)
	{
	case QUALIFIER_MINUS: /* Scanlist ranges */
	  
	  /*
	   * Both C99 and UNIX98 describes ranges as implementation-
	   * defined.
	   *
	   * We support the following behaviour (although this may
	   * change as we become wiser)
	   * - only increasing ranges, ie. [a-b] but not [b-a]
	   * - transitive ranges, ie. [a-b-c] == [a-c]
	   * - trailing minus, ie. [a-] is interpreted as an 'a'
	   *   and a '-'
	   * - duplicates (although we can easily convert these
	   *   into errors)
	   */
	  range_begin = format[index - 1];
	  range_end = format[++index];
	  if (range_end == SPECIFIER_UNGROUP)
	    {
	      /* Trailing minus is included */
	      characterclass[(int)ch]++;
	      ch = range_end;
	      break; /* for */
	    }
	  if (range_end == NIL)
	    return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
	  if (range_begin > range_end)
	    return TRIO_ERROR_RETURN(TRIO_ERANGE, index);
	    
	  for (i = (int)range_begin; i <= (int)range_end; i++)
	    characterclass[i]++;
	    
	  ch = range_end;
	  break;
	  
#if TRIO_EXTENSION

	case SPECIFIER_GROUP:
	  
	  switch (format[index + 1])
	    {
	    case QUALIFIER_DOT: /* Collating symbol */
	      /*
	       * FIXME: This will be easier to implement when multibyte
	       * characters have been implemented. Until now, we ignore
	       * this feature.
	       */
	      for (i = index + 2; ; i++)
		{
		  if (format[i] == NIL)
		    /* Error in syntax */
		    return -1;
		  else if (format[i] == QUALIFIER_DOT)
		    break; /* for */
		}
	      if (format[++i] != SPECIFIER_UNGROUP)
		return -1;
	      
	      index = i;
	      break;
	  
	    case QUALIFIER_EQUAL: /* Equivalence class expressions */
	      {
		unsigned int j;
		unsigned int k;
	    
		if (internalCollationUnconverted)
		  {
		    /* Lazy evalutation of collation array */
		    TrioGetCollation();
		    internalCollationUnconverted = FALSE;
		  }
		for (i = index + 2; ; i++)
		  {
		    if (format[i] == NIL)
		      /* Error in syntax */
		      return -1;
		    else if (format[i] == QUALIFIER_EQUAL)
		      break; /* for */
		    else
		      {
			/* Mark any equivalent character */
			k = (unsigned int)format[i];
			for (j = 0; internalCollationArray[k][j] != NIL; j++)
			  characterclass[(int)internalCollationArray[k][j]]++;
		      }
		  }
		if (format[++i] != SPECIFIER_UNGROUP)
		  return -1;
		
		index = i;
	      }
	      break;
	  
	    case QUALIFIER_COLON: /* Character class expressions */
	  
	      if (StrEqualMax(CLASS_ALNUM, sizeof(CLASS_ALNUM) - 1,
			      &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isalnum(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_ALNUM) - 1;
		}
	      else if (StrEqualMax(CLASS_ALPHA, sizeof(CLASS_ALPHA) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isalpha(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_ALPHA) - 1;
		}
	      else if (StrEqualMax(CLASS_CNTRL, sizeof(CLASS_CNTRL) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (iscntrl(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_CNTRL) - 1;
		}
	      else if (StrEqualMax(CLASS_DIGIT, sizeof(CLASS_DIGIT) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isdigit(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_DIGIT) - 1;
		}
	      else if (StrEqualMax(CLASS_GRAPH, sizeof(CLASS_GRAPH) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isgraph(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_GRAPH) - 1;
		}
	      else if (StrEqualMax(CLASS_LOWER, sizeof(CLASS_LOWER) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (islower(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_LOWER) - 1;
		}
	      else if (StrEqualMax(CLASS_PRINT, sizeof(CLASS_PRINT) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isprint(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_PRINT) - 1;
		}
	      else if (StrEqualMax(CLASS_PUNCT, sizeof(CLASS_PUNCT) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (ispunct(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_PUNCT) - 1;
		}
	      else if (StrEqualMax(CLASS_SPACE, sizeof(CLASS_SPACE) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isspace(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_SPACE) - 1;
		}
	      else if (StrEqualMax(CLASS_UPPER, sizeof(CLASS_UPPER) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isupper(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_UPPER) - 1;
		}
	      else if (StrEqualMax(CLASS_XDIGIT, sizeof(CLASS_XDIGIT) - 1,
				   &format[index]))
		{
		  for (i = 0; i < MAX_CHARACTER_CLASS; i++)
		    if (isxdigit(i))
		      characterclass[i]++;
		  index += sizeof(CLASS_XDIGIT) - 1;
		}
	      else
		{
		  characterclass[(int)ch]++;
		}
	      break;

	    default:
	      characterclass[(int)ch]++;
	      break;
	    }
	  break;
	  
#endif /* TRIO_EXTENSION */
	  
	default:
	  characterclass[(int)ch]++;
	  break;
	}
    }
  return 0;
}

/*************************************************************************
 * TrioReadNumber
 *
 * We implement our own number conversion in preference of strtol and
 * strtoul, because we must handle 'long long' and thousand separators.
 */
TRIO_PRIVATE BOOLEAN_T
TrioReadNumber(trio_class_t *self,
	       trio_uintmax_t *target,
	       unsigned long flags,
	       int width,
	       int base)
{
  trio_uintmax_t number = 0;
  int digit;
  int count;
  BOOLEAN_T isNegative = FALSE;
  int j;

  assert(VALID(self));
  assert(VALID(self->InStream));
  assert((base >= MIN_BASE && base <= MAX_BASE) || (base == NO_BASE));

  if (internalDigitsUnconverted)
    {
      /* Lazy evaluation of digits array */
      memset(internalDigitArray, -1, sizeof(internalDigitArray));
      for (j = 0; j < (int)sizeof(internalDigitsLower) - 1; j++)
	{
	  internalDigitArray[(int)internalDigitsLower[j]] = j;
	  internalDigitArray[(int)internalDigitsUpper[j]] = j;
	}
      internalDigitsUnconverted = FALSE;
    }
  
  TrioSkipWhitespaces(self);
  
  if (!(flags & FLAGS_UNSIGNED))
    {
      /* Leading sign */
      if (self->current == '+')
	{
	  self->InStream(self, NULL);
	}
      else if (self->current == '-')
	{
	  self->InStream(self, NULL);
	  isNegative = TRUE;
	}
    }
  
  count = self->processed;
  
  if (flags & FLAGS_ALTERNATIVE)
    {
      switch (base)
	{
	case NO_BASE:
	case BASE_OCTAL:
	case BASE_HEX:
	case BASE_BINARY:
	  if (self->current == '0')
	    {
	      self->InStream(self, NULL);
	      if (self->current)
		{
		  if ((base == BASE_HEX) &&
		      (toupper(self->current) == 'X'))
		    {
		      self->InStream(self, NULL);
		    }
		  else if ((base == BASE_BINARY) &&
			   (toupper(self->current) == 'B'))
		    {
		      self->InStream(self, NULL);
		    }
		}
	    }
	  else
	    return FALSE;
	  break;
	default:
	  break;
	}
    }

  while (((width == NO_WIDTH) || (self->processed - count < width)) &&
	 (! ((self->current == EOF) || isspace(self->current))))
    {
      if (isascii(self->current))
	{
	  digit = internalDigitArray[self->current];
	  /* Abort if digit is not allowed in the specified base */
	  if ((digit == -1) || (digit >= base))
	    break;
	}
      else if (flags & FLAGS_QUOTE)
	{
	  /* Compare with thousands separator */
	  for (j = 0; internalThousandSeparator[j] && self->current; j++)
	    {
	      if (internalThousandSeparator[j] != self->current)
		break;

	      self->InStream(self, NULL);
	    }
	  if (internalThousandSeparator[j])
	    break; /* Mismatch */
	  else
	    continue; /* Match */
	}
      else
	break;
            
      number *= base;
      number += digit;

      self->InStream(self, NULL);
    }

  /* Was anything read at all? */
  if (self->processed == count)
    return FALSE;
  
  if (target)
    *target = (isNegative) ? -number : number;
  return TRUE;
}

/*************************************************************************
 * TrioReadChar
 */
TRIO_PRIVATE int
TrioReadChar(trio_class_t *self,
	     char *target,
	     unsigned long flags,
	     int width)
{
  int i;
  char ch;
  trio_uintmax_t number;
  
  assert(VALID(self));
  assert(VALID(self->InStream));

  for (i = 0;
       (self->current != EOF) && (i < width);
       i++)
    {
      ch = (char)self->current;
      self->InStream(self, NULL);
      if ((flags & FLAGS_ALTERNATIVE) && (ch == CHAR_BACKSLASH))
	{
	  switch (self->current)
	    {
	    case '\\': ch = '\\'; break;
	    case 'a': ch = '\007'; break;
	    case 'b': ch = '\b'; break;
	    case 'f': ch = '\f'; break;
	    case 'n': ch = '\n'; break;
	    case 'r': ch = '\r'; break;
	    case 't': ch = '\t'; break;
	    case 'v': ch = '\v'; break;
	    default:
	      if (isdigit(self->current))
		{
		  /* Read octal number */
		  if (!TrioReadNumber(self, &number, 0, 3, BASE_OCTAL))
		    return 0;
		  ch = (char)number;
		}
	      else if (toupper(self->current) == 'X')
		{
		  /* Read hexadecimal number */
		  self->InStream(self, NULL);
		  if (!TrioReadNumber(self, &number, 0, 2, BASE_HEX))
		    return 0;
		  ch = (char)number;
		}
	      else
		{
		  ch = (char)self->current;
		}
	      break;
	    }
	}
      
      if (target)
	target[i] = ch;
    }
  return i + 1;
}

/*************************************************************************
 * TrioReadString
 */
TRIO_PRIVATE BOOLEAN_T
TrioReadString(trio_class_t *self,
	       char *target,
	       unsigned long flags,
	       int width)
{
  int i;
  
  assert(VALID(self));
  assert(VALID(self->InStream));

  TrioSkipWhitespaces(self);
    
  /*
   * Continue until end of string is reached, a whitespace is encountered,
   * or width is exceeded
   */
  for (i = 0;
       ((width == NO_WIDTH) || (i < width)) &&
       (! ((self->current == EOF) || isspace(self->current)));
       i++)
    {
      if (TrioReadChar(self, &target[i], flags, 1) == 0)
	break; /* for */
    }
  if (target)
    target[i] = NIL;
  return TRUE;
}

/*************************************************************************
 * TrioReadWideChar
 */
#if TRIO_WIDECHAR
TRIO_PRIVATE int
TrioReadWideChar(trio_class_t *self,
		 wchar_t *target,
		 unsigned long flags,
		 int width)
{
  int i;
  int j;
  int size;
  int amount = 0;
  wchar_t wch;
  char buffer[MB_LEN_MAX + 1];
  
  assert(VALID(self));
  assert(VALID(self->InStream));

  for (i = 0;
       (self->current != EOF) && (i < width);
       i++)
    {
      if (isascii(self->current))
	{
	  if (TrioReadChar(self, buffer, flags, 1) == 0)
	    return 0;
	  buffer[1] = NIL;
	}
      else
	{
	  /*
	   * Collect a multibyte character, by enlarging buffer until
	   * it contains a fully legal multibyte character, or the
	   * buffer is full.
	   */
	  j = 0;
	  do
	    {
	      buffer[j++] = (char)self->current;
	      buffer[j] = NIL;
	      self->InStream(self, NULL);
	    }
	  while ((j < (int)sizeof(buffer)) && (mblen(buffer, (size_t)j) != j));
	}
      if (target)
	{
	  size = mbtowc(&wch, buffer, sizeof(buffer));
	  if (size > 0)
	    target[i] = wch;
	}
      amount += size;
      self->InStream(self, NULL);
    }
  return amount;
}
#endif /* TRIO_WIDECHAR */

/*************************************************************************
 * TrioReadWideString
 */
#if TRIO_WIDECHAR
TRIO_PRIVATE BOOLEAN_T
TrioReadWideString(trio_class_t *self,
		   wchar_t *target,
		   unsigned long flags,
		   int width)
{
  int i;
  int size;
  
  assert(VALID(self));
  assert(VALID(self->InStream));

  TrioSkipWhitespaces(self);

#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  mblen(NULL, 0);
#endif
  
  /*
   * Continue until end of string is reached, a whitespace is encountered,
   * or width is exceeded
   */
  for (i = 0;
       ((width == NO_WIDTH) || (i < width)) &&
       (! ((self->current == EOF) || isspace(self->current)));
       )
    {
      size = TrioReadWideChar(self, &target[i], flags, 1);
      if (size == 0)
	break; /* for */

      i += size;
    }
  if (target)
    target[i] = WCONST('\0');
  return TRUE;
}
#endif /* TRIO_WIDECHAR */

/*************************************************************************
 * TrioReadGroup
 *
 * FIXME: characterclass does not work with multibyte characters
 */
TRIO_PRIVATE BOOLEAN_T
TrioReadGroup(trio_class_t *self,
	      char *target,
	      int *characterclass,
	      unsigned long flags,
	      int width)
{
  int ch;
  int i;
  
  assert(VALID(self));
  assert(VALID(self->InStream));

  ch = self->current;
  for (i = 0;
       ((width == NO_WIDTH) || (i < width)) &&
       (! ((ch == EOF) ||
	   (((flags & FLAGS_EXCLUDE) != 0) ^ (characterclass[ch] == 0))));
       i++)
    {
      if (target)
	target[i] = (char)ch;
      self->InStream(self, &ch);
    }
  
  if (target)
    target[i] = NIL;
  return TRUE;
}

/*************************************************************************
 * TrioReadDouble
 *
 * FIXME:
 *  add long double
 */
TRIO_PRIVATE BOOLEAN_T
TrioReadDouble(trio_class_t *self,
	       double *target,
	       unsigned long flags,
	       int width)
{
  int ch;
  char doubleString[512] = "";
  int index = 0;
  int start;
  int j;
  BOOLEAN_T isHex = FALSE;

  if ((width == NO_WIDTH) || (width > (int)sizeof(doubleString) - 1))
    width = sizeof(doubleString) - 1;
  
  TrioSkipWhitespaces(self);
  
  /*
   * Read entire double number from stream. StrToDouble requires a
   * string as input, but InStream can be anything, so we have to
   * collect all characters.
   */
  ch = self->current;
  if ((ch == '+') || (ch == '-'))
    {
      doubleString[index++] = (char)ch;
      self->InStream(self, &ch);
      width--;
    }

  start = index;
  switch (ch)
    {
    case 'n':
    case 'N':
      /* Not-a-number */
      if (index != 0)
	break;
      /* FALLTHROUGH */
    case 'i':
    case 'I':
      /* Infinity */
      while (isalpha(ch) && (index - start < width))
	{
	  doubleString[index++] = (char)ch;
	  self->InStream(self, &ch);
	}
      doubleString[index] = NIL;

      /* Case insensitive string comparison */
      if (StrEqual(&doubleString[start], INFINITE_UPPER) ||
	  StrEqual(&doubleString[start], LONG_INFINITE_UPPER))
	{
	  *target = ((start == 1 && doubleString[0] == '-'))
	    ? trio_ninf()
	    : trio_pinf();
	  return TRUE;
	}
      if (StrEqual(doubleString, NAN_LOWER))
	{
	  /* NaN must not have a preceeding + nor - */
	  *target = trio_nan();
	  return TRUE;
	}
      return FALSE;
      
    default:
      break;
    }
  
  if (ch == '0')
    {
      doubleString[index++] = (char)ch;
      self->InStream(self, &ch);
      if (toupper(ch) == 'X')
	{
	  isHex = TRUE;
	  doubleString[index++] = (char)ch;
	  self->InStream(self, &ch);
	}
    }
  while ((ch != EOF) && (index - start < width))
    {
      /* Integer part */
      if (isHex ? isxdigit(ch) : isdigit(ch))
	{
	  doubleString[index++] = (char)ch;
	  self->InStream(self, &ch);
	}
      else if (flags & FLAGS_QUOTE)
	{
	  /* Compare with thousands separator */
	  for (j = 0; internalThousandSeparator[j] && self->current; j++)
	    {
	      if (internalThousandSeparator[j] != self->current)
		break;

	      self->InStream(self, &ch);
	    }
	  if (internalThousandSeparator[j])
	    break; /* Mismatch */
	  else
	    continue; /* Match */
	}
      else
	break; /* while */
    }
  if (ch == '.')
    {
      /* Decimal part */
      doubleString[index++] = (char)ch;
      self->InStream(self, &ch);
      while ((isHex ? isxdigit(ch) : isdigit(ch)) &&
	     (index - start < width))
	{
	  doubleString[index++] = (char)ch;
	  self->InStream(self, &ch);
	}
      if (isHex ? (toupper(ch) == 'P') : (toupper(ch) == 'E'))
	{
	  /* Exponent */
	  doubleString[index++] = (char)ch;
	  self->InStream(self, &ch);
	  if ((ch == '+') || (ch == '-'))
	    {
	      doubleString[index++] = (char)ch;
	      self->InStream(self, &ch);
	    }
	  while ((isHex ? isxdigit(ch) : isdigit(ch)) &&
		 (index - start < width))
	    {
	      doubleString[index++] = (char)ch;
	      self->InStream(self, &ch);
	    }
	}
    }

  if ((index == start) || (*doubleString == NIL))
    return FALSE;
  
  if (flags & FLAGS_LONGDOUBLE)
/*     *longdoublePointer = StrToLongDouble()*/
    return FALSE; /* FIXME: Remove when long double is implemented */
  else
    {
      *target = StrToDouble(doubleString, NULL);
    }
  return TRUE;
}

/*************************************************************************
 * TrioReadPointer
 */
TRIO_PRIVATE BOOLEAN_T
TrioReadPointer(trio_class_t *self,
		void **target,
		unsigned long flags)
{
  trio_uintmax_t number;
  char buffer[sizeof(internalNullString)];

  flags |= (FLAGS_UNSIGNED | FLAGS_ALTERNATIVE | FLAGS_NILPADDING);
  
  if (TrioReadNumber(self,
		     &number,
		     flags,
		     POINTER_WIDTH,
		     BASE_HEX))
    {
      /*
       * The strange assignment of number is a workaround for a compiler
       * warning
       */
      if (target)
	*target = (char *)0 + number;
      return TRUE;
    }
  else if (TrioReadString(self,
			  (flags & FLAGS_IGNORE)
			  ? NULL
			  : buffer,
			  0,
			  sizeof(internalNullString) - 1))
    {  
      if (StrEqualCase(buffer, internalNullString))
	{
	  if (target)
	    *target = NULL;
	  return TRUE;
	}
    }
  return FALSE;
}

/*************************************************************************
 * TrioScan
 */
TRIO_PRIVATE int
TrioScan(const void *source,
	 size_t sourceSize,
	 void (*InStream)(trio_class_t *, int *),
	 const char *format,
	 va_list arglist,
	 void **argarray)
{
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  int charlen;
  int cnt;
#endif
  int status;
  int assignment;
  trio_parameter_t parameters[MAX_PARAMETERS];
  trio_class_t internalData;
  trio_class_t *data;
  int ch;
  int index; /* Index of format string */
  int i; /* Index of current parameter */
  unsigned long flags;
  int width;
  int base;
  void *pointer;

  assert(VALID(InStream));
  assert(VALID(format));

  memset(&internalData, 0, sizeof(internalData));
  data = &internalData;
  data->InStream = InStream;
  data->location = (void *)source;
  data->max = sourceSize;

#if defined(USE_LOCALE)
  if (NULL == internalLocaleValues)
    {
      TrioSetLocale();
    }
#endif
  
  status = TrioParse(TYPE_SCAN, format, parameters, arglist, argarray);
  if (status < 0)
    return status;

  assignment = 0;
  i = 0;
  index = 0;
  data->InStream(data, &ch);

#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
  mblen(NULL, 0);
#endif

  while (format[index])
    {
#if defined(TRIO_COMPILER_SUPPORTS_MULTIBYTE)
      if (! isascii(format[index]))
	{
	  charlen = mblen(&format[index], MB_LEN_MAX);
	  /* Compare multibyte characters in format string */
	  for (cnt = 0; cnt < charlen - 1; cnt++)
	    {
	      if (ch != format[index + cnt])
		{
		  return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
		}
	      data->InStream(data, &ch);
	    }
	  continue; /* while */
	}
#endif /* TRIO_COMPILER_SUPPORTS_MULTIBYTE */
      if (EOF == ch)
	return EOF;
      
      if (CHAR_IDENTIFIER == format[index])
	{
	  if (CHAR_IDENTIFIER == format[index + 1])
	    {
	      /* Two % in format matches one % in input stream */
	      if (CHAR_IDENTIFIER == ch)
		{
		  data->InStream(data, &ch);
		  index += 2;
		  continue; /* while format chars left */
		}
	      else
		return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
	    }

	  /* Skip the parameter entries */
	  while (parameters[i].type == FORMAT_PARAMETER)
	    i++;
	  
	  flags = parameters[i].flags;
	  /* Find width */
	  width = parameters[i].width;
	  if (flags & FLAGS_WIDTH_PARAMETER)
	    {
	      /* Get width from parameter list */
	      width = (int)parameters[width].data.number.as_signed;
	    }
	  /* Find base */
	  base = parameters[i].base;
	  if (flags & FLAGS_BASE_PARAMETER)
	    {
	      /* Get base from parameter list */
	      base = (int)parameters[base].data.number.as_signed;
	    }
	  
	  switch (parameters[i].type)
	    {
	    case FORMAT_INT:
	      {
		trio_uintmax_t number;

		if (0 == base)
		  base = BASE_DECIMAL;

		if (!TrioReadNumber(data,
				    &number,
				    flags,
				    width,
				    base))
		  return assignment;
		assignment++;
		
		if (!(flags & FLAGS_IGNORE))
		  {
		    pointer = parameters[i].data.pointer;
#if defined(QUALIFIER_SIZE_T) || defined(QUALIFIER_SIZE_T_UPPER)
		    if (flags & FLAGS_SIZE_T)
		      *(size_t *)pointer = (size_t)number;
		    else
#endif
#if defined(QUALIFIER_PTRDIFF_T)
		    if (flags & FLAGS_PTRDIFF_T)
		      *(ptrdiff_t *)pointer = (ptrdiff_t)number;
		    else
#endif
#if defined(QUALIFIER_INTMAX_T)
		    if (flags & FLAGS_INTMAX_T)
		      *(trio_intmax_t *)pointer = (trio_intmax_t)number;
		    else
#endif
		    if (flags & FLAGS_QUAD)
		      *(trio_ulonglong_t *)pointer = (trio_ulonglong_t)number;
		    else if (flags & FLAGS_LONG)
		      *(long int *)pointer = (long int)number;
		    else if (flags & FLAGS_SHORT)
		      *(short int *)pointer = (short int)number;
		    else
		      *(int *)pointer = (int)number;
		  }
	      }
	      break; /* FORMAT_INT */
	      
	    case FORMAT_STRING:
#if TRIO_WIDECHAR
	      if (flags & FLAGS_WIDECHAR)
		{
		  if (!TrioReadWideString(data,
					  (flags & FLAGS_IGNORE)
					  ? NULL
					  : parameters[i].data.wstring,
					  flags,
					  width))
		    return assignment;
		}
	      else
#endif
		{
		  if (!TrioReadString(data,
				      (flags & FLAGS_IGNORE)
				      ? NULL
				      : parameters[i].data.string,
				      flags,
				      width))
		    return assignment;
		}
	      assignment++;
	      break; /* FORMAT_STRING */
	      
	    case FORMAT_DOUBLE:
	      if (!TrioReadDouble(data,
				  (flags & FLAGS_IGNORE)
				  ? NULL
				  : parameters[i].data.doublePointer,
				  flags,
				  width))
		return assignment;
	      assignment++;
	      break; /* FORMAT_DOUBLE */

	    case FORMAT_GROUP:
	      {
		int characterclass[MAX_CHARACTER_CLASS + 1];
		int rc;

		/* Skip over modifiers */
		while (format[index] != SPECIFIER_GROUP)
		  {
		    index++;
		  }
		/* Skip over group specifier */
		index++;
		
		memset(characterclass, 0, sizeof(characterclass));
		rc = TrioGetCharacterClass(format,
					   &index,
					   &flags,
					   characterclass);
		if (rc < 0)
		  return rc;

		if (!TrioReadGroup(data,
				   (flags & FLAGS_IGNORE)
				   ? NULL
				   : parameters[i].data.string,
				   characterclass,
				   flags,
				   parameters[i].width))
		  return assignment;
		assignment++;
	      }
	      break; /* FORMAT_GROUP */
	      
	    case FORMAT_COUNT:
	      pointer = parameters[i].data.pointer;
	      if (NULL != pointer)
		{
#if defined(QUALIFIER_SIZE_T) || defined(QUALIFIER_SIZE_T_UPPER)
		  if (flags & FLAGS_SIZE_T)
		    *(size_t *)pointer = (size_t)data->committed;
		  else
#endif
#if defined(QUALIFIER_PTRDIFF_T)
		  if (flags & FLAGS_PTRDIFF_T)
		    *(ptrdiff_t *)pointer = (ptrdiff_t)data->committed;
		  else
#endif
#if defined(QUALIFIER_INTMAX_T)
		  if (flags & FLAGS_INTMAX_T)
		    *(trio_intmax_t *)pointer = (trio_intmax_t)data->committed;
		  else
#endif
		  if (flags & FLAGS_QUAD)
		    {
		      *(trio_ulonglong_t *)pointer = (trio_ulonglong_t)data->committed;
		    }
		  else if (flags & FLAGS_LONG)
		    {
		      *(long int *)pointer = (long int)data->committed;
		    }
		  else if (flags & FLAGS_SHORT)
		    {
		      *(short int *)pointer = (short int)data->committed;
		    }
		  else
		    {
		      *(int *)pointer = (int)data->committed;
		    }
		}
	      break; /* FORMAT_COUNT */
	      
	    case FORMAT_CHAR:
#if TRIO_WIDECHAR
	      if (flags & FLAGS_WIDECHAR)
		{
		  if (TrioReadWideChar(data,
				       (flags & FLAGS_IGNORE)
				       ? NULL
				       : parameters[i].data.wstring,
				       flags,
				       (width == NO_WIDTH) ? 1 : width) > 0)
		    return assignment;
		}
	      else
#endif
		{
		  if (TrioReadChar(data,
				   (flags & FLAGS_IGNORE)
				   ? NULL
				   : parameters[i].data.string,
				   flags,
				   (width == NO_WIDTH) ? 1 : width) > 0)
		    return assignment;
		}
	      assignment++;
	      break; /* FORMAT_CHAR */
	      
	    case FORMAT_POINTER:
	      if (!TrioReadPointer(data,
				   (flags & FLAGS_IGNORE)
				   ? NULL
				   : (void **)parameters[i].data.pointer,
				   flags))
		return assignment;
	      assignment++;
	      break; /* FORMAT_POINTER */
	      
	    case FORMAT_PARAMETER:
	      break; /* FORMAT_PARAMETER */
	      
	    default:
	      return TRIO_ERROR_RETURN(TRIO_EINVAL, index);
	    }
	  ch = data->current;
	  index = parameters[i].indexAfterSpecifier;
	  i++;
	}
      else /* Not an % identifier */
	{
	  if (isspace((int)format[index]))
	    {
	      /* Whitespaces may match any amount of whitespaces */
	      ch = TrioSkipWhitespaces(data);
	    }
	  else if (ch == format[index])
	    {
	      data->InStream(data, &ch);
	    }
	  else
	    return assignment;
	  
	  index++;
	}
    }
  return assignment;
}

/*************************************************************************
 * TrioInStreamFile
 */
TRIO_PRIVATE void
TrioInStreamFile(trio_class_t *self,
		 int *intPointer)
{
  FILE *file = (FILE *)self->location;

  assert(VALID(self));
  assert(VALID(file));

  self->current = fgetc(file);
  self->processed++;
  self->committed++;
  
  if (VALID(intPointer))
    {
      *intPointer = self->current;
    }
}

/*************************************************************************
 * TrioInStreamFileDescriptor
 */
TRIO_PRIVATE void
TrioInStreamFileDescriptor(trio_class_t *self,
			   int *intPointer)
{
  int fd = *((int *)self->location);
  int size;
  unsigned char input;

  assert(VALID(self));

  size = read(fd, &input, sizeof(char));
  self->current = (size == 0) ? EOF : input;
  self->processed++;
  self->committed++;
  
  if (VALID(intPointer))
    {
      *intPointer = self->current;
    }
}

/*************************************************************************
 * TrioInStreamString
 */
TRIO_PRIVATE void
TrioInStreamString(trio_class_t *self,
		   int *intPointer)
{
  unsigned char **buffer;

  assert(VALID(self));
  assert(VALID(self->InStream));
  assert(VALID(self->location));

  buffer = (unsigned char **)self->location;
  self->current = (*buffer)[0];
  if (self->current == NIL)
    self->current = EOF;
  (*buffer)++;
  self->processed++;
  self->committed++;
  
  if (VALID(intPointer))
    {
      *intPointer = self->current;
    }
}

/*************************************************************************
 * scanf
 */
int
trio_scanf(const char *format,
	   ...)
{
  int status;
  va_list args;

  assert(VALID(format));
  
  va_start(args, format);
  status = TrioScan(stdin, 0, TrioInStreamFile, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vscanf(const char *format,
	    va_list args)
{
  assert(VALID(format));
  
  return TrioScan(stdin, 0, TrioInStreamFile, format, args, NULL);
}

int
trio_scanfv(const char *format,
	    void **args)
{
  va_list dummy;
  
  assert(VALID(format));
  
  return TrioScan(stdin, 0, TrioInStreamFile, format, dummy, args);
}

/*************************************************************************
 * fscanf
 */
int
trio_fscanf(FILE *file,
	    const char *format,
	    ...)
{
  int status;
  va_list args;

  assert(VALID(file));
  assert(VALID(format));
  
  va_start(args, format);
  status = TrioScan(file, 0, TrioInStreamFile, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vfscanf(FILE *file,
	     const char *format,
	     va_list args)
{
  assert(VALID(file));
  assert(VALID(format));
  
  return TrioScan(file, 0, TrioInStreamFile, format, args, NULL);
}

int
trio_fscanfv(FILE *file,
	     const char *format,
	     void **args)
{
  va_list dummy;
  
  assert(VALID(file));
  assert(VALID(format));
  
  return TrioScan(file, 0, TrioInStreamFile, format, dummy, args);
}

/*************************************************************************
 * dscanf
 */
int
trio_dscanf(int fd,
	    const char *format,
	    ...)
{
  int status;
  va_list args;

  assert(VALID(format));
  
  va_start(args, format);
  status = TrioScan(&fd, 0, TrioInStreamFileDescriptor, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vdscanf(int fd,
	     const char *format,
	     va_list args)
{
  assert(VALID(format));
  
  return TrioScan(&fd, 0, TrioInStreamFileDescriptor, format, args, NULL);
}

int
trio_dscanfv(int fd,
             const char *format,
             void **args)
{
  va_list dummy;
  
  assert(VALID(format));
  
  return TrioScan(&fd, 0, TrioInStreamFileDescriptor, format, dummy, args);
}

/*************************************************************************
 * sscanf
 */
int
trio_sscanf(const char *buffer,
	    const char *format,
	    ...)
{
  int status;
  va_list args;

  assert(VALID(buffer));
  assert(VALID(format));
  
  va_start(args, format);
  status = TrioScan(&buffer, 0, TrioInStreamString, format, args, NULL);
  va_end(args);
  return status;
}

int
trio_vsscanf(const char *buffer,
	     const char *format,
	     va_list args)
{
  assert(VALID(buffer));
  assert(VALID(format));
  
  return TrioScan(&buffer, 0, TrioInStreamString, format, args, NULL);
}

int
trio_sscanfv(const char *buffer,
	     const char *format,
	     void **args)
{
  va_list dummy;
  
  assert(VALID(buffer));
  assert(VALID(format));
  
  return TrioScan(&buffer, 0, TrioInStreamString, format, dummy, args);
}

