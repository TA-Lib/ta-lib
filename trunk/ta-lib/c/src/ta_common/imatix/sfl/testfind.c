/*  ----------------------------------------------------------------<Prolog>-
    Name:       testfind.c
    Title:      Test program for search functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/24  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/10  iMatix SFL project team <sfl@imatix.com>

    Synopsis:   Runs repeated random and presupplied cases through the
                search functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

#define BLOCK_MAX     (10000)
#define BLOCK_MIN      (5000)
#define PATTERN_MAX     (500)
#define PATTERN_MIN      (20)
#define MEMFIND_MAX     (999)

static int fill_random (byte *block, int minimum, int maximum);
static void seed_block  (byte *block, int block_len, byte *seed, int seed_len);
static Bool test_memfind_r(const int maxtimes);
static Bool test_memfind_rb(const int maxtimes);
static Bool test_txtfind(void);

int main (int argc, char *argv [])
{
  Bool 
      passed = TRUE;

  printf("Testing sflfind.c functions (memfind_r(), txtfind())\n\n");

  printf("memfind_r(%d): ", MEMFIND_MAX);
  if (! test_memfind_r(MEMFIND_MAX))
    passed = FALSE;
  printf("\n\n");

  printf("memfind_rb(%d): ", MEMFIND_MAX);
  if (! test_memfind_rb(MEMFIND_MAX))
    passed = FALSE;
  printf("\n\n");

  printf("txtfind():   ");
  if (! test_txtfind())
    passed = FALSE;
  printf("\n");

  if (passed)
      printf("All tests passed.\n");
  else
      printf("Some tests failed.\n");

  return (passed ? 0 : 1);
}

/*  Search for random patterns in random blocks                              */
static Bool test_memfind_r(const int maxtimes)
{
    int
        i,
        block_size,
        pattern_size;
    byte
        block   [BLOCK_MAX],
        pattern [PATTERN_MAX];
    byte
        *limit,
        *found,
        *naive_result;
    Bool
        passed = TRUE;

    randomize ();
    for (i = 0; i < maxtimes; i++) {
        block_size   = fill_random (block,   BLOCK_MIN,   BLOCK_MAX);
        pattern_size = fill_random (pattern, PATTERN_MIN, PATTERN_MAX);
        seed_block(block, block_size, pattern, pattern_size);

        /*  Naive search for pattern in block                                */
        limit = block + block_size - pattern_size;
        found = block;
        naive_result = NULL;
        while (found < limit)
          {
            found = memchr (found, pattern [0], (size_t) (limit - found + 1));
            if (found)
              {
                if (memcmp (found, pattern, pattern_size) == 0)
                  {
                    naive_result = found;
                    break;
                  }
                else
                    found++;
              }
            else
                break;
          }

        /*  Fast search for pattern in block                                 */
        found = memfind_r (block, block_size, pattern, pattern_size);

        if (found != naive_result)
          {
            printf ("\nFailed: block=%d, pattern=%d\n",
                     block_size, pattern_size);
            printf ("Naive result=%p, memfind=%p\n", naive_result, found);
            passed = FALSE;

            if (found && found > limit) 
                puts ("The smart search looked too far.");

            if (found && memcmp (found, pattern, pattern_size) == 0) 
                puts ("The naive search failed.");

          }

        if (found)
            printf ("*");
        else
            printf (".");
    }

    return (passed);
}

/*  Look for the same pattern over and over again                           */
static Bool test_memfind_rb(const int maxtimes)
{
    int
        i,
        block_size,
        pattern_size;
    byte
        block   [BLOCK_MAX],
        pattern [PATTERN_MAX];
    byte
        *limit,
        *found,
        *naive_result;
    Bool
        passed = TRUE,
        repeat_find = FALSE;
    size_t
        searchbuf[256];

    randomize ();
    pattern_size     = fill_random (pattern, PATTERN_MIN, PATTERN_MAX);
    
    for (i = 0; i < maxtimes; i++) {
        block_size   = fill_random (block,   BLOCK_MIN,   BLOCK_MAX);
        seed_block(block, block_size, pattern, pattern_size);

        /*  Naive search for pattern in block                                */
        limit = block + block_size - pattern_size;
        found = block;
        naive_result = NULL;
        while (found < limit)
          {
            found = memchr (found, pattern [0], (size_t) (limit - found + 1));
            if (found)
              {
                if (memcmp (found, pattern, pattern_size) == 0)
                  {
                    naive_result = found;
                    break;
                  }
                else
                    found++;
              }
            else
                break;
          }

        /*  Fast search for pattern in block                                 */
        found = memfind_rb (block, block_size, pattern, pattern_size,
                            searchbuf, &repeat_find);

        if (found != naive_result)
          {
            printf ("\nFailed: block=%d, pattern=%d\n",
                     block_size, pattern_size);
            printf ("Naive result=%p, memfind=%p\n", naive_result, found);
            passed = FALSE;

            if (found && found > limit) 
                puts ("The smart search looked too far.");

            if (found && memcmp (found, pattern, pattern_size) == 0) 
                puts ("The naive search failed.");

          }

        if (found)
            printf ("*");
        else
            printf (".");
    }

    return (passed);
}


static int
fill_random (byte *block, int minimum, int maximum)
{
    int
        block_size,
        byte_nbr;

    block_size = minimum + random (maximum-minimum) + 1;
    for (byte_nbr = 0; byte_nbr < block_size; byte_nbr++)
        block [byte_nbr] = random (256);
    return (block_size);
}

/*  Randomly decide whether to stick a copy of the seed into the block, and  */
/*  if so, stick it at a random place in the block.                          */
static void seed_block  (byte *block, int block_len, byte *seed, int seed_len)
{
    int position;
    
    if (((random(64) / 4) % 4) >= 2)
    {
       position = random(block_len - seed_len);
       memcpy(block+position, seed, seed_len);  
    }
}

static Bool 
test_txtfind(void)
{
  struct
    {
      char *string;               /*  String to search in                    */
      char *pattern;              /*  String to search for                   */
      int  pos;                   /*  Position we expect to find it (-1=not) */
    }
  static search_patterns[] =      
    {                             /*  (pos = -2 marks end of patterns)       */
      { "",         "",     0 },  /*  Empty string at start of empty string  */
      { "abc",      "",     0 },  /*  Empty string at start of non-empty str */
      { "abc",      "a",    0 },  /*  One character matches                  */
      { "abc",      "b",    1 },
      { "abc",      "c",    2 },
      { "abc",      "A",    0 },  /*  Mixed case, one character matches      */
      { "abc",      "B",    1 },
      { "abc",      "C",    2 },
      { "ABC",      "a",    0 },  /*  Mixed case, one character matches      */
      { "ABC",      "b",    1 },
      { "ABC",      "c",    2 },
      { "aAa",      "A",    0 },  /*  First, ignoring case                   */
      { "ABC",      "ab",   0 },  /*  Matching pairs of characters           */
      { "ABC",      "bc",   1 },
      { "ABC",      "cd",  -1 },
      { "ab",       "abc", -1 },  /*  Search string longer than main string  */
      { "ababab",   "abc", -1 },  /*  All-but last present                   */
      { "ABABAB",   "abc", -1 },      { "ababab",  "ABC",  -1 },
      { "abababc",  "abc",  4 },  /*  Present at the end only                */
      { "abababc",  "Abc",  4 },
      { "Abababc",  "Abc",  4 },  /*  Ignore partial match at start          */
      { "abABcabc", "abc",  2 },  /*  Present in middle and end              */
      { "abABcabc", "Abc",  2 },
      { "AbABcabc", "Abc",  2 },  /*  Ignore partial match at start          */
      { "aaabbabc", "abc",  5 },  /*  Present at end only                    */
      { "abbbbccc", "Abc", -1 },
      { "abcccabc", "cab",  4 },  /*  Different pattern, for variety         */
      { "abcabcab", "cab",  2 },
      { "abcabcab", "BCA",  1 },
      { "aBCAbcab", "bCa",  1 },
      { "a",        "a",    0 },  /*  Strings the same length                */
      { "ab",       "ab",   0 },
      { "abc",      "abc",  0 },  
      { "abcdef\n\n","\n\n",6 },
      { "",          "",   -2 }   /*  ---- END MARKER -----                  */
    };

  Bool
      passed = TRUE;

  int
      no  = 0,                    /*  Number of patterns                     */
      i;
  char
      buffer[200];

  while(search_patterns[no++].pos != -2)
    ;                             /*  Count number of patterns               */

  printf(" %d patterns\n", --no);

  for(i = 0; i < no; i++) 
    {
      char *found = NULL;
      Bool correct = FALSE;

      if (search_patterns[i].pos >= 0)
          snprintf(buffer, sizeof (buffer), "%d", search_patterns[i].pos);
      else
          snprintf(buffer, sizeof (buffer), "ABSENT");

      printf("  %-27s %-27s [%6s/", 
             search_patterns[i].string,
             search_patterns[i].pattern,
             buffer);
      fflush(stdout);

      found = txtfind(search_patterns[i].string, search_patterns[i].pattern);

      /*  Correct if not found, and not supposed to be found, or if found at */
      /*  the expected position.                                             */
      correct = 
             ((found == NULL && search_patterns[i].pos < 0) ||
              (found != NULL && 
               (found - search_patterns[i].string) == search_patterns[i].pos));

      if (found != NULL)
          snprintf(buffer, sizeof (buffer), "%ld", 
		           (long) (found - search_patterns[i].string));
      else
          snprintf(buffer, sizeof (buffer), "ABSENT");

      printf("%6s]  %s\n", 
             buffer, 
             (correct ? "PASS" : "FAIL"));

      if (! correct)
        passed = FALSE;
    }

    return (passed);
}

