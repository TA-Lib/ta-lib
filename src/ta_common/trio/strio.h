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
 ************************************************************************/

#ifndef TRIO_STRIO_H
#define TRIO_STRIO_H

#if !(defined(DEBUG) || defined(NDEBUG))
# define NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef STRIO_MALLOC
# define STRIO_MALLOC(n) malloc(n)
#endif
#ifndef STRIO_FREE
# define STRIO_FREE(x) free(x)
#endif

/*
 * StrAppend(target, source)
 * StrAppendMax(target, maxsize, source)
 *
 *   Append 'source' to 'target'
 *
 * target = StrAlloc(size)
 *
 *   Allocate a new string
 *
 * StrContains(target, substring)
 *
 *   Find out if the string 'substring' is
 *   contained in the string 'target'
 *
 * StrCopy(target, source)
 * StrCopyMax(target, maxsize, source)
 *
 *   Copy 'source' to 'target'
 *
 * target = StrDuplicate(source)
 * target = StrDuplicateMax(source, maxsize)
 *
 *   Allocate and copy 'source' to 'target'
 *
 * StrEqual(first, second)
 * StrEqualMax(first, maxsize, second)
 *
 *   Compare if 'first' is equal to 'second'.
 *   Case-independent.
 *
 * StrEqualCase(first, second)
 * StrEqualCaseMax(first, maxsize, second)
 *
 *   Compare if 'first' is equal to 'second'
 *   Case-dependent. Please note that the use of the
 *   word 'case' has the opposite meaning as that of
 *   strcasecmp().
 *
 * StrFormat(target, format, ...)
 * StrFormatMax(target, maxsize, format, ...)
 *
 *   Build 'target' according to 'format' and succesive
 *   arguments. This is equal to the sprintf() and
 *   snprintf() functions.
 *
 * StrFormatDate(target, format, ...)
 *
 * StrFree(target)
 *
 *   De-allocates a string
 *
 * StrHash(string, type)
 *
 *   Calculates the hash value of 'string' based on the
 *   'type'.
 *
 * StrIndex(target, character)
 * StrIndexLast(target, character)
 *
 *   Find the first/last occurrence of 'character' in
 *   'target'
 *
 * StrLength(target)
 *
 *   Return the length of 'target'
 *
 * StrMatch(string, pattern)
 * StrMatchCase(string, pattern)
 *
 *   Find 'pattern' within 'string'. 'pattern' may contain
 *   wildcards such as * (asterics) and ? (question mark)
 *   which matches zero or more characters and exactly
 *   on character respectively
 *
 * StrScan(source, format, ...)
 *
 *   Equal to sscanf()
 *
 * StrSubstring(target, substring)
 *
 *   Find the first occurrence of the string 'substring'
 *   within the string 'target'
 *
 * StrTokenize(target, list)
 *
 *   Split 'target' into the first token delimited by
 *   one of the characters in 'list'. If 'target' is
 *   NULL then next token will be returned.
 *
 * StrToUpper(target)
 *
 *   Convert all lower case characters in 'target' into
 *   upper case characters.
 */

enum {
  STRIO_HASH_NONE = 0,
  STRIO_HASH_PLAIN,
  STRIO_HASH_TWOSIGNED
};

#if !defined(DEBUG) || defined(__DECC)
#define StrAlloc(n) (char *)STRIO_MALLOC(n)
#define StrAppend(x,y) strcat((x), (y))
#define StrContains(x,y) (0 != strstr((x), (y)))
#define StrCopy(x,y) strcpy((x), (y))
#define StrIndex(x,y) strchr((x), (y))
#define StrIndexLast(x,y) strrchr((x), (y))
#define StrFree(x) STRIO_FREE(x)
#define StrLength(x) strlen((x))
#define StrSubstring(x,y) strstr((x), (y))
#define StrTokenize(x,y) strtok((x), (y))
#define StrToLong(x,y,n) strtol((x), (y), (n))
#define StrToUnsignedLong(x,y,n) strtoul((x), (y), (n))
#else /* DEBUG */
 /*
  * To be able to use these macros everywhere, including in
  * if() sentences, the assertions are put first in a comma
  * seperated list.
  *
  * Unfortunately the DECC compiler does not seem to like this
  * so it will use the un-asserted functions above for the
  * debugging case too.
  */
#define StrAlloc(n) \
     (assert((n) > 0),\
      (char *)STRIO_MALLOC(n))
#define StrAppend(x,y) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      strcat((x), (y)))
#define StrContains(x,y) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      (0 != strstr((x), (y))))
#define StrCopy(x,y) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      strcpy((x), (y)))
#define StrIndex(x,c) \
     (assert((x) != NULL),\
      strchr((x), (c)))
#define StrIndexLast(x,c) \
     (assert((x) != NULL),\
      strrchr((x), (c)))
#define StrFree(x) \
     (assert((x) != NULL),\
      STRIO_FREE(x))
#define StrLength(x) \
     (assert((x) != NULL),\
      strlen((x)))
#define StrSubstring(x,y) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      strstr((x), (y)))
#define StrTokenize(x,y) \
     (assert((y) != NULL),\
      strtok((x), (y)))
#define StrToLong(x,y,n) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      assert((n) >= 2 && (n) <= 36),\
      strtol((x), (y), (n)))
#define StrToUnsignedLong(x,y,n) \
     (assert((x) != NULL),\
      assert((y) != NULL),\
      assert((n) >= 2 && (n) <= 36),\
      strtoul((x), (y), (n)))
#endif /* DEBUG */

char *StrAppendMax(char *target, size_t max, const char *source);
char *StrCopyMax(char *target, size_t max, const char *source);
char *StrDuplicate(const char *source);
char *StrDuplicateMax(const char *source, size_t max);
int StrEqual(const char *first, const char *second);
int StrEqualCase(const char *first, const char *second);
int StrEqualCaseMax(const char *first, size_t max, const char *second);
int StrEqualLocale(const char *first, const char *second);
int StrEqualMax(const char *first, size_t max, const char *second);
const char *StrError(int);
size_t StrFormatDateMax(char *target, size_t max, const char *format, const struct tm *datetime);
unsigned long StrHash(const char *string, int type);
int StrMatch(char *string, char *pattern);
int StrMatchCase(char *string, char *pattern);
size_t StrSpanFunction(char *source, int (*Function)(int));
char *StrSubstringMax(const char *string, size_t max, const char *find);
float StrToFloat(const char *source, const char **target);
double StrToDouble(const char *source, const char **target);
int StrToUpper(char *target);

#endif /* TRIO_STRIO_H */
