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

/*
 * TODO
 * - StrToLongDouble
 */
 
static const char rcsid[] = "@(#)$Id$";

#if defined(unix) || defined(__xlC__) || defined(__QNX__)
# define PLATFORM_UNIX
#elif defined(WIN32) || defined(_WIN32)
# define PLATFORM_WIN32
#elif defined(AMIGA) && defined(__GNUC__)
# define PLATFORM_UNIX
#endif

#if defined(__STDC__) && (__STDC_VERSION__ >= 199901L)
# define TRIO_C99
#endif

#include "strio.h"
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#ifndef DEBUG
# define NDEBUG
#endif
#include <assert.h>

#ifndef NULL
# define NULL 0
#endif
#define NIL ((char)0)
#ifndef FALSE
# define FALSE (1 == 0)
# define TRUE (! FALSE)
#endif

#define VALID(x) (NULL != (x))
#define INVALID(x) (NULL == (x))

#if defined(PLATFORM_UNIX)
# define USE_STRCASECMP
# define USE_STRNCASECMP
# define USE_STRERROR
# if defined(__QNX__)
#  define strcasecmp(x,y) stricmp(x,y)
#  define strncasecmp(x,y,n) strnicmp(x,y,n)
# endif
#elif defined(PLATFORM_WIN32)
# define USE_STRCASECMP
# define strcasecmp(x,y) strcmpi(x,y)
#endif

/*************************************************************************
 * StrAppendMax
 */
char *StrAppendMax(char *target, size_t max, const char *source)
{
  assert(VALID(target));
  assert(VALID(source));
  assert(max > 0);

  max -= StrLength(target) + 1;
  return (max > 0) ? strncat(target, source, max) : target;
}

/*************************************************************************
 * StrCopyMax
 */
char *StrCopyMax(char *target, size_t max, const char *source)
{
  assert(VALID(target));
  assert(VALID(source));
  assert(max > 0); /* Includes != 0 */

  target = strncpy(target, source, max - 1);
  target[max - 1] = (char)0;
  return target;
}

/*************************************************************************
 * StrDuplicate
 */
char *StrDuplicate(const char *source)
{
  char *target;

  assert(VALID(source));

  target = StrAlloc(StrLength(source) + 1);
  if (target)
    {
      StrCopy(target, source);
    }
  return target;
}

/*************************************************************************
 * StrDuplicateMax
 */
char *StrDuplicateMax(const char *source, size_t max)
{
  char *target;
  size_t len;

  assert(VALID(source));
  assert(max > 0);

  /* Make room for string plus a terminating zero */
  len = StrLength(source) + 1;
  if (len > max)
    {
      len = max;
    }
  target = StrAlloc(len);
  if (target)
    {
      StrCopyMax(target, len, source);
    }
  return target;
}

/*************************************************************************
 * StrEqual
 */
int StrEqual(const char *first, const char *second)
{
  assert(VALID(first));
  assert(VALID(second));

  if (VALID(first) && VALID(second))
    {
#if defined(USE_STRCASECMP)
      return (0 == strcasecmp(first, second));
#else
      while ((*first != NIL) && (*second != NIL))
	{
	  if (toupper(*first) != toupper(*second))
	    {
	      break;
	    }
	  first++;
	  second++;
	}
      return ((*first == NIL) && (*second == NIL));
#endif
    }
  return FALSE;
}

/*************************************************************************
 * StrEqualCase
 */
int StrEqualCase(const char *first, const char *second)
{
  assert(VALID(first));
  assert(VALID(second));

  if (VALID(first) && VALID(second))
    {
      return (0 == strcmp(first, second));
    }
  return FALSE;
}

/*************************************************************************
 * StrEqualCaseMax
 */
int StrEqualCaseMax(const char *first, size_t max, const char *second)
{
  assert(VALID(first));
  assert(VALID(second));

  if (VALID(first) && VALID(second))
    {
      return (0 == strncmp(first, second, max));
    }
  return FALSE;
}

/*************************************************************************
 * StrEqualLocale
 */
int StrEqualLocale(const char *first, const char *second)
{
  assert(VALID(first));
  assert(VALID(second));

#if defined(LC_COLLATE)
  return (strcoll(first, second) == 0);
#else
  return StrEqual(first, second);
#endif
}

/*************************************************************************
 * StrEqualMax
 */
int StrEqualMax(const char *first, size_t max, const char *second)
{
  assert(VALID(first));
  assert(VALID(second));

  if (VALID(first) && VALID(second))
    {
#if defined(USE_STRNCASECMP)
      return (0 == strncasecmp(first, second, max));
#else
      /* Not adequately tested yet */
      size_t cnt = 0;
      while ((*first != NIL) && (*second != NIL) && (cnt <= max))
	{
	  if (toupper(*first) != toupper(*second))
	    {
	      break;
	    }
	  first++;
	  second++;
	  cnt++;
	}
      return ((cnt == max) || ((*first == NIL) && (*second == NIL)));
#endif
    }
  return FALSE;
}

/*************************************************************************
 * StrError
 */
const char *StrError(int errorNumber)
{
#if defined(USE_STRERROR)
  return strerror(errorNumber);
#else
  return "unknown";
#endif
}

/*************************************************************************
 * StrFormatDate
 */
size_t StrFormatDateMax(char *target,
			size_t max,
			const char *format,
			const struct tm *datetime)
{
  assert(VALID(target));
  assert(VALID(format));
  assert(VALID(datetime));
  assert(max > 0);
  
  return strftime(target, max, format, datetime);
}

/*************************************************************************
 * StrHash
 */
unsigned long StrHash(const char *string, int type)
{
  unsigned long value = 0L;
  char ch;

  assert(VALID(string));
  
  switch (type)
    {
    case STRIO_HASH_PLAIN:
      while ( (ch = *string++) != NIL )
	{
	  value *= 31;
	  value += (unsigned long)ch;
	}
      break;
    default:
      assert(FALSE);
      break;
    }
  return value;
}

/*************************************************************************
 * StrMatch
 */
int StrMatch(char *string, char *pattern)
{
  assert(VALID(string));
  assert(VALID(pattern));
  
  for (; ('*' != *pattern); ++pattern, ++string)
    {
      if (NIL == *string)
	{
	  return (NIL == *pattern);
	}
      if ((toupper((int)*string) != toupper((int)*pattern))
	  && ('?' != *pattern))
	{
	  return FALSE;
	}
    }
  /* two-line patch to prevent *too* much recursiveness: */
  while ('*' == pattern[1])
    pattern++;

  do
    {
      if ( StrMatch(string, &pattern[1]) )
	{
	  return TRUE;
	}
    }
  while (*string++);
  
  return FALSE;
}

/*************************************************************************
 * StrMatchCase
 */
int StrMatchCase(char *string, char *pattern)
{
  assert(VALID(string));
  assert(VALID(pattern));
  
  for (; ('*' != *pattern); ++pattern, ++string)
    {
      if (NIL == *string)
	{
	  return (NIL == *pattern);
	}
      if ((*string != *pattern)
	  && ('?' != *pattern))
	{
	  return FALSE;
	}
    }
  /* two-line patch to prevent *too* much recursiveness: */
  while ('*' == pattern[1])
    pattern++;

  do
    {
      if ( StrMatchCase(string, &pattern[1]) )
	{
	  return TRUE;
	}
    }
  while (*string++);
  
  return FALSE;
}

/*************************************************************************
 * StrSpanFunction
 *
 * Untested
 */
size_t StrSpanFunction(char *source, int (*Function)(int))
{
  size_t count = 0;

  assert(VALID(source));
  assert(VALID(Function));
  
  while (*source != NIL)
    {
      if (Function(*source))
	break; /* while */
      source++;
      count++;
    }
  return count;
}

/*************************************************************************
 * StrSubstringMax
 */
char *StrSubstringMax(const char *string, size_t max, const char *find)
{
  size_t count;
  size_t size;
  char *result = NULL;

  assert(VALID(string));
  assert(VALID(find));
  
  size = StrLength(find);
  if (size <= max)
    {
      for (count = 0; count <= max - size; count++)
	{
	  if (StrEqualMax(find, size, &string[count]))
	    {
	      result = (char *)&string[count];
	      break;
	    }
	}
    }
  return result;
}

/*************************************************************************
 * StrToDouble
 *
 * double ::= [ <sign> ]
 *            ( <number> |
 *              <number> <decimal_point> <number> |
 *              <decimal_point> <number> )
 *            [ <exponential> [ <sign> ] <number> ]
 * number ::= 1*( <digit> )
 * digit ::= ( '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' )
 * exponential ::= ( 'e' | 'E' )
 * sign ::= ( '-' | '+' )
 * decimal_point ::= '.'
 */
double StrToDouble(const char *source, const char **endp)
{
#if defined(TRIO_C99)
  return strtod(source, (char **)endp);
#else
  /* Preliminary code */
  int isNegative = FALSE;
  int isExponentNegative = FALSE;
  unsigned long integer = 0;
  unsigned long fraction = 0;
  unsigned long fracdiv = 1;
  unsigned long exponent = 0;
  double value = 0.0;

  /* First try hex-floats */
  if ((source[0] == '0') && ((source[1] == 'x') || (source[1] == 'X')))
    {
      source += 2;
      while (isxdigit((int)*source))
	{
	  integer *= 16;
	  integer += (isdigit((int)*source)
		      ? (*source - '0')
		      : 10 + (toupper((int)*source) - 'A'));
	  source++;
	}
      if (*source == '.')
	{
	  source++;
	  while (isxdigit((int)*source))
	    {
	      fraction *= 16;
	      fraction += (isdigit((int)*source)
			   ? (*source - '0')
			   : 10 + (toupper((int)*source) - 'A'));
	      fracdiv *= 16;
	      source++;
	    }
	  if ((*source == 'p') || (*source == 'P'))
	    {
	      source++;
	      if ((*source == '+') || (*source == '-'))
		{
		  isExponentNegative = (*source == '-');
		  source++;
		}
	      while (isdigit((int)*source))
		{
		  exponent *= 10;
		  exponent += (*source - '0');
		  source++;
		}
	    }
	}
    }
  else /* Then try normal decimal floats */
    {
      isNegative = (*source == '-');
      /* Skip sign */
      if ((*source == '+') || (*source == '-'))
	source++;

      /* Integer part */
      while (isdigit((int)*source))
	{
	  integer *= 10;
	  integer += (*source - '0');
	  source++;
	}

      if (*source == '.')
	{
	  source++; /* skip decimal point */
	  while (isdigit((int)*source))
	    {
	      fraction *= 10;
	      fraction += (*source - '0');
	      fracdiv *= 10;
	      source++;
	    }
	}
      if ((*source == 'e') || (*source == 'E'))
	{
	  source++; /* Skip exponential indicator */
	  isExponentNegative = (*source == '-');
	  if ((*source == '+') || (*source == '-'))
	    source++;
	  while (isdigit((int)*source))
	    {
	      exponent *= 10;
	      exponent += (*source - '0');
	      source++;
	    }
	}
    }
  
  value = (double)integer;
  if (fraction != 0)
    {
      value += (double)fraction / (double)fracdiv;
    }
  if (exponent != 0)
    {
      if (isExponentNegative)
	value /= pow((double)10, (double)exponent);
      else
	value *= pow((double)10, (double)exponent);
    }
  if (isNegative)
    value = -value;

  if (endp)
    *endp = source;
  return value;
#endif
}

/*************************************************************************
 * StrToFloat
 */
float StrToFloat(const char *source, const char **endp)
{
#if defined(TRIO_C99)
  return strtof(source, (char **)endp);
#else
  return (float)StrToDouble(source, endp);
#endif
}

/*************************************************************************
 * StrToUpper
 */
int StrToUpper(char *target)
{
  int i = 0;

  assert(VALID(target));
  
  while (NIL != *target)
    {
      *target = toupper((int)*target);
      target++;
      i++;
    }
  return i;
}
