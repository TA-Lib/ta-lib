/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmem.c
    Title:      Memory allocation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/12/28

    Synopsis:   Encapsulated memory allocation functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sfltron.h"                    /*  Tracing functions                */
#include "sflmem.h"                     /*  Prototypes for functions         */


/*- Constants ---------------------------------------------------------------*/

#define MEMTAG      0xa55aU             /*  Value for mh_tag                 */
#define MEMUNTAG    0x5aa5U             /*  Invalidated tag                  */


/*- Macros ------------------------------------------------------------------*/

#define ALIGN_SIZE       sizeof (double)
#define RESERVE_SIZE     (((sizeof (MEMHDR) + (ALIGN_SIZE - 1)) \
                                             / ALIGN_SIZE) * ALIGN_SIZE)
#define MEMTRN_SIZE      sizeof (MEMTRN)
#define CLIENT_2_HDR(a)  ((MEMHDR *) (((char *) (a)) - RESERVE_SIZE))
#define HDR_2_CLIENT(a)  ((void *)   (((char *) (a)) + RESERVE_SIZE))


/*- Put the #define last to enable tracing ----------------------------------*/

#define MEM_TRACE   1                   /*  Enable all trace() calls         */
#undef  MEM_TRACE                       /*  Don't even compile them          */


/*- Local type definitions --------------------------------------------------*/

struct _MEMTRN {                        /*  Transaction info                 */
    MEMTRN  *next;                      /*  Next transaction                 */
    MEMTRN  *prev;                      /*  Previous transaction             */
    char    *file;                      /*  File allocation was from         */
    size_t   line;                      /*  Line allocation was from         */
    LIST     memhdr;                    /*  Header for list of blocks        */
};

struct _MEMHDR {                        /*  Memory block header info         */
    MEMHDR     *next;                   /*  Next memory block                */
    MEMHDR     *prev;                   /*  Previous memory block            */
    word        tag;                    /*  Special ident tag                */
    size_t      size;                   /*  Size of allocation block         */
    const char *file;                   /*  File allocation was from         */
    size_t      line;                   /*  Line allocation was from         */
};

typedef struct _SCAVFCT {               /*  Scavenger function registration  */
    struct _SCAVFCT
        *next, *prev;                   /*    Doubly-linked list             */
    scavenger scav_fct;                 /*    Address of function            */
    void    * scav_arg;                 /*    Argument for function          */
} SCAVFCT;

/*  -------------------------------------------------------------------------
 *  Global variables local to this source
 */

static LIST                             /*  List of scavenger functions      */
    scavfcts = {
        &scavfcts,
        &scavfcts
    };

static long
    mem_total = 0;                      /*  Amount of memory used            */
static MEMTRN                           /*  Dummy transaction, actually the  */
    mem_list = {&mem_list, &mem_list,   /*  main list of committed blocks    */
                NULL, 0,
                {&mem_list.memhdr,
                 &mem_list.memhdr}
               };
static LIST
    tr_list = {&tr_list, &tr_list};     /*  List of transaction blocks       */
static long
    mem_alloc_count = 0,                /*  We keep count of calls to        */
    mem_free_count  = 0;                /*    mem_alloc() and mem_free()     */


/*- Local functions ---------------------------------------------------------*/

static void mem_tag_err      (void *ptr, const char *filename, size_t lineno);
static void mem_scavenge     (void);
static void mem_del_trans    (MEMTRN *trn);
static void mem_display_list (MEMHDR *ptr, FILE *fp);
static void mem_check_list   (MEMHDR *ptr, const char *filename, size_t lineno);
static void mem_free_list    (MEMHDR *ptr);


/*  ---------------------------------------------------------------------[<]-
    Function: mem_alloc_

    Synopsis: Allocates a memory block.  Use the mem_alloc() macro to call
    this function!  Use mem_free_() to free blocks allocated with this
    function.  Returns a pointer to the allocated memory block, or NULL if
    there was not enough memory available.  The supplied source file name
    is assumed to be in a static area.  The requested block size must be
    greater than zero bytes.
    ---------------------------------------------------------------------[>]-*/

void *
mem_alloc_ (
    MEMTRN     *trn,                    /*  Associated transaction           */
    size_t      size,                   /*  Desired size of memory block     */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    MEMHDR
       *ptr;                            /*  Allocated memory block           */

    /*  Allocate block with extra space for the header                       */
    ASSERT (size > 0);                  /*  Cannot allocate zero bytes!      */

    ptr = malloc (RESERVE_SIZE + size);
    if (ptr == NULL)                    /*  If nothing free, do a hunt       */
      {                                 /*    and try again...               */
        mem_scavenge ();
        ptr = malloc (RESERVE_SIZE + size);
        if (ptr == NULL)
            return (NULL);              /*  Really in trouble now!           */
      }
#   if (defined (MEM_TRACE))
    trace ("%s (%ld): alloc %d bytes->%p", 
            filename? filename: "-", (long) lineno, size, ptr);
#   endif

    ptr-> tag  = MEMTAG;                /*  Initialise block header          */
    ptr-> size = size;                  /*  Size of block                    */
    ptr-> file = filename;              /*  Who allocated it                 */
    ptr-> line = lineno;                /*    and where                      */

    if (!trn)                           /*  If no transaction then use the   */
        trn = &mem_list;                /*  main block list                  */

    list_reset (ptr);                   /*  Set up new block as list         */
    list_relink_before (ptr,            /*  Add to list of blocks            */
                        &trn-> memhdr);

    mem_total += size;                  /*  Keep count of space used         */
    mem_alloc_count += 1;               /*    and number of allocations      */

    return (HDR_2_CLIENT (ptr));        /*   and return client address       */
}


/* ---------------------------------------------------------------------------
 *  mem_scavenge -- internal
 *
 *  Calls all registered scavenger functions.
 */

static void
mem_scavenge (void)
{
    SCAVFCT
        *entry;                         /*  Entry in list of functions       */

    for (entry  = scavfcts.next;
         entry != (SCAVFCT *) &scavfcts;
         entry  = entry-> next)
        (*entry-> scav_fct) (entry-> scav_arg);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_realloc_

    Synopsis: Reallocates a memory block, which remains part of the same
    transaction.  Use the mem_realloc() macro to call this function!
    Accepts a pointer to a memory block and the desired size of the new
    memory block.  Returns the address of the new memory block, or NULL if
    there was not enough memory available.  If the specified block was not
    correctly allocated, dumps the memory allocation list and exits.  The
    desired size must be greater than zero.
    ---------------------------------------------------------------------[>]-*/

void *
mem_realloc_ (
    void       *client_ptr,             /*  Block of memory to reallocate    */
    size_t      size,                   /*  Desired size of memory block     */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    MEMHDR
        *ptr,
        *next;

    ASSERT (client_ptr);
    ASSERT (size > 0);

    /*  Check that block is valid                                            */
    ptr = CLIENT_2_HDR (client_ptr);
    if (ptr-> tag != MEMTAG)
        mem_tag_err (ptr, filename, lineno);

    /*  Invalidate header                                                    */
    ptr-> tag = MEMUNTAG;

    mem_total -= ptr-> size;
    mem_free_count += 1;
    
    next = ptr-> next;                  /*  Save where we were linked        */
    list_unlink (ptr);                  /*     and unlink                    */
    
    /*  Reallocate memory block                                              */
    ptr = (MEMHDR *) realloc (ptr, RESERVE_SIZE + size);
    if (ptr == NULL)                    /*  If nothing free, do a hunt       */
      {                                 /*    and try again...               */
        mem_scavenge ();
        ptr = (MEMHDR *) realloc (ptr, RESERVE_SIZE + size);
        if (ptr == NULL)
            return (NULL);              /*  Really in trouble now!           */
      }

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): realloc %d bytes ->%p", 
            filename? filename: "-", (long) lineno, size, ptr);
#   endif

    /*  Update header                                                        */
    ptr-> tag  = MEMTAG;
    ptr-> size = size;
    ptr-> file = filename;
    ptr-> line = lineno;

    list_reset (ptr);                   /*  Set up block as list             */
    list_relink_before (ptr, next);     /*  And link where old block was     */

    mem_total += size;                  /*  Keep count of space used         */
    mem_alloc_count += 1;               /*    and number of allocations      */

    return (HDR_2_CLIENT (ptr));
}


/* ---------------------------------------------------------------------------
 *  mem_tag_err -- internal
 *
 *  Display memory tag error
 */

static void
mem_tag_err (void *ptr, const char *filename, size_t lineno)
{
    fprintf (stdout, "Memory tag error - %p - %s (%ld)\n",
                     ptr, filename? filename: "<Unknown>", (long) lineno);
    fflush  (stdout);
    abort ();
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_strdup_

    Synopsis: Saves a string in dynamic memory.  Use the mem_strdup macro
    to call this function!  The caller is responsible for freeing the space
    allocated when it is no longer needed.  Returns a pointer to the
    allocated string, which holds a copy of the parameter string.  Returns
    NULL if there was insufficient heap storage available to allocate the
    string, or if the original string was itself NULL.
    ---------------------------------------------------------------------[>]-*/

char *
mem_strdup_ (
    MEMTRN     *trn,                    /*  Associated transaction           */
    const char *string,                 /*  String to copy                   */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    char *copy;
    size_t str_len;

    if (string)                         /*  If string not null, copy it      */
      {
        str_len = strlen (string) + 1;
        copy = mem_alloc_ (trn, str_len, filename, lineno);
        if (copy)
            strncpy (copy, string, str_len);
      }
    else
        copy = NULL;                    /*  Just pass-through a NULL         */

    return (copy);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_strfree_

    Synopsis: Releases memory occupied by a string.  Use the mem_strfree()
    macro to call this function!  Call this function to free strings
    allocated using mem_strdup_().  Accepts the address of a char pointer
    as argument: if the pointer is not null, the string is freed, and the
    pointer is set to null.  Returns the address of the modified pointer.

    Examples:
    char
        *string1 = NULL,
        *string2 = NULL;
    string1 = mem_strdup ("This is a string");
    mem_strfree (&string1);
    mem_strfree (&string2);
    ---------------------------------------------------------------------[>]-*/

void
mem_strfree_ (
    char **string,                      /*  Address of string to free        */
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    ASSERT (string);
    if (*string)
      {
        mem_free_ (*string, filename, lineno);
        *string = NULL;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_free_

    Synopsis: Releases memory previously allocated by mem_alloc_(),
    mem_realloc_(), or mem_strdup_().  Use the mem_free() macro to call
    this function!  If the specified block was not correctly allocated,
    dumps the memory allocation list and exits.  If you specify a null
    address, does nothing.
    ---------------------------------------------------------------------[>]-*/

void
mem_free_ (
    void *client_ptr,                   /*  Block of memory to free          */
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    MEMHDR
       *ptr;

    if (client_ptr == NULL)             /*  Do nothing if address is null    */
        return;

    /*  Check for valid block                                                */
    ptr = CLIENT_2_HDR (client_ptr);
    if (ptr-> tag != MEMTAG)
        mem_tag_err (ptr, filename, lineno);

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): free=%p", 
           filename? filename: "-", (long) lineno, ptr);
#   endif
#   if (defined (DEBUG))
    memset (client_ptr, 0, ptr-> size);  
#   endif

    /*  Invalidate header                                                    */
    ptr-> tag = MEMUNTAG;
    mem_total -= ptr-> size;
    mem_free_count += 1;

    /*  Remove block from list, inlining code from list_unlink()             */
    ((LIST *) ((LIST *) ptr)-> prev)-> next = ((LIST *) ptr)-> next;
    ((LIST *) ((LIST *) ptr)-> next)-> prev = ((LIST *) ptr)-> prev;

    free (ptr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_assert_

    Synopsis: Checks that all allocated memory was freed.  Use the
    mem_assert macro to call this function!  If any memory is still left
    allocated, displays the memory list on stderr and aborts.  Generally
    we use this function at the end of a program, after deallocating all
    memory.  If any memory has not been allocated, we get a nice list and
    an abort.  Our principle is that any memory allocation must be matched
    by a free somewhere in the code.
    ---------------------------------------------------------------------[>]-*/

void
mem_assert_ (
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    FILE
        *trace_file;

    if (mem_total != 0
    ||  !list_empty (&tr_list))
      {
        fflush  (stdout);
        fprintf (stderr, "Clean-memory assertion failed - %s (%ld)\n",
                          filename? filename: "<Unknown>", (long) lineno);
        fprintf (stderr, "Details are in memtrace.lst\n");
        trace_file = fopen ("memtrace.lst", "w");
        mem_display (trace_file);
        fclose (trace_file);
        abort ();
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_checkall_

    Synopsis: Checks all allocated memory blocks; if any block was corrupted,
    aborts with an error message, else does nothing.
    ---------------------------------------------------------------------[>]-*/

void
mem_checkall_ (
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    MEMTRN
        *trn;

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): check all memory", 
           filename? filename: "-", (long) lineno);
#   endif

    mem_check_list ((MEMHDR *) &mem_list.memhdr, filename, lineno);
    trn = tr_list.next;
    while (trn != (MEMTRN *) &tr_list)
      {
        mem_check_list ((MEMHDR *) &trn-> memhdr, filename, lineno);
        trn = trn-> next;
      }
}


/* ---------------------------------------------------------------------------
 *  mem_check_list -- internal
 *
 *  Checks memory blocks in one transaction; if any block was corrupted,
 *  aborts with an error message, else does nothing.
 */

static void
mem_check_list (
    MEMHDR     *lst,                    /*  List of memory allocations       */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    MEMHDR
        *ptr;

    ptr = mem_list.memhdr.next;
    ptr = lst-> next;
    while (ptr != lst)
      {
        if (ptr-> tag != MEMTAG)
          {
            mem_display (stderr);
            mem_tag_err (ptr, filename, lineno);
          }
        ptr = ptr-> next;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_check_

    Synopsis: Checks that a block of memory has not been corrupted.  If the
    block is corrupted, aborts with an error message, else does nothing.
    ---------------------------------------------------------------------[>]-*/

void
mem_check_ (
    const void *client_ptr,             /*  Block of memory to free          */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    MEMHDR
       *ptr;

    if (client_ptr == NULL)             /*  Do nothing if address is null    */
        return;

    /*  Check for valid block                                                */
    ptr = CLIENT_2_HDR (client_ptr);
    if (ptr-> tag != MEMTAG)
        mem_tag_err (ptr, filename, lineno);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_descr_

    Synopsis: Allocates a DESCR block for a specified block of data.  Use the
    mem_descr macro to call this function!  Returns a pointer to an allocated
    DESCR block, or NULL if there was not enough memory.  The DESCR block is
    allocated as a single block, consisting of the DESCR block plus the data.
    To free the entire block you need one call to mem_free().  If the
    data_block argument is not null, its contents are copied into the newly
    allocated memory.
    ---------------------------------------------------------------------[>]-*/

DESCR *
mem_descr_ (
    MEMTRN     *trn,                    /*  Associated transaction           */
    const void *data_block,             /*  Block of memory to copy          */
    size_t      data_size,              /*  Size of memory block             */
    const char *filename,               /*  Name of source file making call  */
    size_t      lineno                  /*  Line number in calling source    */
)
{
    DESCR
        *descr;

    descr = mem_alloc_ (trn, data_size + sizeof (DESCR), filename, lineno);
    if (descr == NULL)
        return (NULL);

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): allocate descr=%p", 
           filename? filename: "-", (long) lineno, descr);
#   endif

    /*  Fill-in descriptor block unless it is NULL                           */
    descr-> size = data_size;
    descr-> data = (byte *) descr + sizeof (DESCR);
    if (data_block)
        memcpy (descr-> data, data_block, data_size);

    return (descr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_new_trans_

    Synopsis: Allocates a transaction block.  Use the mem_new_trans()
    macro to call this function.  Use mem_commit to save or mem_rollback() to
    delete the transaction.  Returns a pointer to the allocated transaction
    block, or NULL if there was not enough memory available.  The supplied
    source file name is assumed to be in a static area.
    ---------------------------------------------------------------------[>]-*/

MEMTRN *
mem_new_trans_(
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    MEMTRN
       *trn;                            /*  Allocated transaction block      */

    /*  Allocate block                                                       */
    trn = malloc (MEMTRN_SIZE);
    if (trn == NULL)
        return (NULL);

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): new transaction", 
           filename? filename: "-", (long) lineno);
#   endif

    trn-> file = (char *) filename;     /*  Who allocated it                 */
    trn-> line = lineno;                /*    and where                      */
    list_reset (&trn-> memhdr);         /*  No memory blocks yet             */

    list_reset (trn);                   /*  Only 1 item in list              */
    list_relink_before (trn, &tr_list);  /*  Add to list of transactions      */
    return (trn);                       /*   and return address              */
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_commit_

    Synopsis: Commits all blocks allocated to a given transaction.
    ---------------------------------------------------------------------[>]-*/

void
mem_commit_ (
    MEMTRN *trn,
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    LIST
       *ptr;

#   if (defined (MEM_TRACE))
    trace ("%s (%ld): commit transaction", 
           filename? filename: "-", (long) lineno);
#   endif

    ptr = &trn-> memhdr;
    if (!list_empty (ptr))              /*  Are there any blocks to commit?  */
      {
        list_relink_before (ptr,        /*  Relink list into main list       */
                            &mem_list. memhdr);
        list_unlink (ptr);
      }

    mem_del_trans (trn);
}


/*  --------------------------------------------------------------------------
 *  mem_del_trans - internal
 *
 *  Deletes a transaction block.
 */

static void
mem_del_trans(
    MEMTRN *trn
)
{
    if (trn == NULL)                    /*  Do nothing if address is null    */
        return;

    ASSERT (list_empty (&trn-> memhdr));

    list_unlink (trn);                  /*  Remove transaction from list     */
    free (trn);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_rollback_

    Synopsis: Rolls back allocations for a particular transaction.  This
    frees up all blocks allocated by calls to mem_alloc, mem_realloc and
    mem_strdup since the last call to mem_commit.  Note that for blocks
    allocated with mem_realloc, this is not really a rollback but a free.
    The mem_rollback() function must be used with some care... if you
    forget to do a mem_commit(), a later mem_rollback() will do damage
    to your memory space.  The general rule is to start your processing
    with mem_commit(), then do work, and call mem_rollback() when there
    is an error.  Finally, call mem_commit() at the end.
    ---------------------------------------------------------------------[>]-*/

void
mem_rollback_ (
    MEMTRN *trn,
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
#   if (defined (MEM_TRACE))
    trace ("%s (%ld): rollback transaction", 
           filename? filename: "-", (long) lineno);
#   endif

    mem_free_list ((MEMHDR *) &trn-> memhdr);
    mem_del_trans (trn);
}


/* ---------------------------------------------------------------------------
 *  mem_free_list -- internal
 *
 */

static void
mem_free_list (
    MEMHDR *list                        /*  List of memory allocations       */
)
{
    MEMHDR
        *ptr;

    while (!list_empty (list))
      {
        ptr = list-> next;
        ptr-> tag  = MEMUNTAG;
        mem_total -= ptr-> size;
        mem_free_count += 1;
        list_unlink (ptr);              /*  Remove block from list           */
        free (ptr);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_size_

    Synopsis: Returns the size in bytes of a memory block.
    ---------------------------------------------------------------------[>]-*/

size_t
mem_size_ (
    const void *client_ptr,             /*  Block of memory to free          */
    const char *filename,               /*  Name of source file making call  */
    size_t lineno                       /*  Line number in calling source    */
)
{
    MEMHDR
       *ptr;

    if (client_ptr == NULL)             /*  Do nothing if address is null    */
        return 0;

    /*  Check for valid block                                                */
    ptr = CLIENT_2_HDR (client_ptr);
    if (ptr-> tag != MEMTAG)
        mem_tag_err (ptr, filename, lineno);

    return (ptr-> size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_used

    Synopsis: Returns the number of bytes currently allocated using the
    memory management system. The value returned is simply the sum of the
    size requests to allocation routines.  It does not reflect any overhead
    required by the memory management system.
    ---------------------------------------------------------------------[>]-*/

long
mem_used (void)
{
    return (mem_total);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_allocs

    Synopsis: Returns the number of blocks allocated in total.  Use this
    to get an idea of the activity of the memory management system.  When
    program ends cleanly, mem_allocs () should be equal to mem_frees().
    ---------------------------------------------------------------------[>]-*/

long
mem_allocs (void)
{
    return (mem_alloc_count);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_frees

    Synopsis: Returns the number of blocks freed in total.  Use this
    to get an idea of the activity of the memory management system.  When
    program ends cleanly, mem_allocs () should be equal to mem_frees().
    ---------------------------------------------------------------------[>]-*/

long
mem_frees (void)
{
    return (mem_free_count);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_display

    Synopsis: Displays the contents of the memory allocation list.
    ---------------------------------------------------------------------[>]-*/

void
mem_display (
    FILE *fp                            /*  File to dump display to          */
)
{
    MEMTRN
        *trn;

    fprintf (fp, "Index   Size  File(Line) - total size %lu\n", mem_total);
    mem_display_list ((MEMHDR *) &mem_list.memhdr, fp);

    trn = tr_list.next;
    while (trn != (MEMTRN *) &tr_list)
      {
        fprintf (fp, "* Transaction %s (%ld)",
                 trn-> file? trn-> file: "<Unknown>", (long) trn-> line);
        fprintf (fp, "\n");
        mem_display_list ((MEMHDR *) &trn-> memhdr, fp);

        trn = trn-> next;
      }
    fflush (fp);
}

/* ---------------------------------------------------------------------------
 *  mem_display_list -- internal
 *
 *  Displays memory allocations attached to a particular list
 */

static void
mem_display_list (
    MEMHDR *lst,                        /*  List of memory allocations       */
    FILE   *fp                          /*  File to dump display to          */
)
{
    MEMHDR
       *ptr;
    int
        index;

    index = 0;
    ptr = lst-> next;
    while (ptr != lst)
      {
        fprintf (fp, "%-5d %6ld", index++, (long) ptr-> size);
        fprintf (fp, "  %s (%ld)",
                 ptr-> file? ptr-> file: "<Unknown>", (long) ptr-> line);
        if (ptr-> tag != MEMTAG)
            fprintf (fp, " INVALID");

        fprintf (fp, " [%p]\n", HDR_2_CLIENT (ptr));
        ptr = ptr-> next;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_scavenger

    Synopsis: Registers a memory scavenger function.  A memory scavenger
    function is an application function that is invoked by mem_alloc_() when
    memory is exhausted, so that unused application objects can be released.
    This allows you to allocate large amounts of memory -- for instance for
    caches -- and then release them when memory runs short.  When you
    register a scavenger function you may provide a void * argument; this is
    passed back to the scavenger if it is ever invoked.  The scavenger
    function returns TRUE if it could release some memory, otherwise it
    returns FALSE.  Note that there is no way to unregister such a function.
    Furthermore, a scavenger function should not itself allocate any new
    memory, unless it can definitely free excess memory first.  Scavenger
    functions are called in an unspecified order.  Returns 0 if the scavenger
    function could be registered, -1 if not.  There is no limit to the number
    of scavenger functions you can register, except available memory.  The
    same scavenger function can be registered several times.
 ---------------------------------------------------------------------[>]-*/

int
mem_scavenger (
    scavenger scav_fct,                 /*  File to dump display to          */
    void    * scav_arg
)
{
    SCAVFCT
        *scavfct;                       /*  Allocated registry function      */

    /*  Allocate an SCAVFCT block and attach it to the scavfcts list         */
    list_create (scavfct, sizeof (SCAVFCT));
    if (scavfct == NULL)
        return (-1);
    list_relink_before (scavfct, &scavfcts);

    scavfct-> scav_fct = scav_fct;
    scavfct-> scav_arg = scav_arg;
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: mem_freeall

    Synopsis: Frees all allocated memory.  This function is rather brutal and
    can do strange things to an application.  It can be useful when you are
    trying to recover control in a crashed application, and need to free all
    allocated memory before restarting it.
    ---------------------------------------------------------------------[>]-*/

void
mem_freeall (void)
{
    MEMTRN
        *trn;

    mem_free_list ((MEMHDR *) &mem_list.memhdr);
    while (!list_empty (&tr_list))
      {
        trn = tr_list.next;
        mem_free_list ((MEMHDR *) &trn-> memhdr);
        mem_del_trans (trn);
      }
}


