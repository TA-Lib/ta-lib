/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmem.h
    Title:      Memory allocation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/12/28

    Synopsis:   Encapsulated memory allocation functions.  Based on an
                article by Jim Schimandle in DDJ August 1990.  Provides
                'safe' versions of malloc(), realloc(), free(), and strdup().
                These functions protect the programmer from errors in calling
                memory allocation/free routines.   When these calls are used,
                the allocation routines in this module add a data structure
                to the top of allocated memory blocks which tags them as legal
                memory blocks.  When the free routine is called, the memory
                block to be freed is checked for legality.  If the block
                is not legal, the memory list is dumped to stderr and the
                program is terminated.  Some of these functions are called
                through macros that add the filename and line number of the
                call, for tracing.  Do not call these functions directly.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLMEM_INCLUDED                /*  Allow multiple inclusions        */
#define SFLMEM_INCLUDED


/*- Type definitions --------------------------------------------------------*/

typedef Bool (*scavenger) (void *);     /*  Memory scavenger function        */

typedef struct _MEMHDR MEMHDR;          /*  Memory block header              */
typedef struct _MEMTRN MEMTRN;          /*  Transaction block identifier     */

/*- Function prototypes -----------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*  General memory allocation functions                                      */

void  *mem_alloc_      (MEMTRN *trn, size_t size,
                        const char *source_file, size_t source_line);
void  *mem_realloc_    (void *block, size_t size,
                        const char *source_file, size_t source_line);
void   mem_free_       (void *block,
                        const char *source_file, size_t source_line);
char  *mem_strdup_     (MEMTRN *trn, const char *string,
                        const char *source_file, size_t source_line);
void   mem_strfree_    (char **string,
                        const char *source_file, size_t source_line);
DESCR *mem_descr_      (MEMTRN *trn, const void *block, size_t size,
                        const char *source_file, size_t source_line);

/*  Functions on transactions                                                */

MEMTRN *mem_new_trans_ (const char *source_file, size_t source_line);
void    mem_commit_    (MEMTRN *trn,
                        const char *source_file, size_t source_line);
void    mem_rollback_  (MEMTRN *trn,
                        const char *source_file, size_t source_line);

/*  Hidden control functions                                                 */

void   mem_checkall_   (const char *source_file, size_t source_line);
void   mem_check_      (const void *block,
                        const char *source_file, size_t source_line);
void   mem_assert_     (const char *source_file, size_t source_line);

/*  Visible control functions                                                */

size_t mem_size_       (const void *block,
                        const char *source_file, size_t source_line);
long   mem_used        (void);
long   mem_allocs      (void);
long   mem_frees       (void);
void   mem_freeall     (void);
void   mem_display     (FILE *save_to);
int    mem_scavenger   (scavenger scav_fct, void *scav_arg);

#ifdef __cplusplus
}
#endif

/*- Define macros to encapsulate calls to the hidden functions --------------*/

#if (defined (DEBUG))

/*  Transaction-based allocation macros                                      */

# define memt_alloc(t,n)    mem_alloc_     ((t),  (n),      __FILE__, __LINE__)
# define memt_strdup(t,s)   mem_strdup_    ((t),  (s),      __FILE__, __LINE__)
# define memt_descr(t,p,n)  mem_descr_     ((t),  (p), (n), __FILE__, __LINE__)

/*  Basic allocation macros                                                  */

# define mem_alloc(n)       mem_alloc_     (NULL, (n),      __FILE__, __LINE__)
# define mem_strdup(s)      mem_strdup_    (NULL, (s),      __FILE__, __LINE__)
# define mem_descr(p,n)     mem_descr_     (NULL, (p), (n), __FILE__, __LINE__)

/*  Other functions requiring __FILE__ & __LINE__ substitution               */

# define mem_new_trans()    mem_new_trans_ (                __FILE__, __LINE__)
# define mem_commit(t)      mem_commit_    ((t),            __FILE__, __LINE__)
# define mem_rollback(t)    mem_rollback_  ((t),            __FILE__, __LINE__)
# define mem_realloc(p,n)   mem_realloc_   ((p), (n),       __FILE__, __LINE__)
# define mem_free(p)        { if ((p)) mem_free_ ((p), __FILE__, __LINE__); }
# define mem_strfree(ps)    mem_strfree_   ((ps),           __FILE__, __LINE__)
# define mem_assert()       mem_assert_    (                __FILE__, __LINE__)
# define mem_checkall()     mem_checkall_  (                __FILE__, __LINE__)
# define mem_check(p)       mem_check_     ((p),            __FILE__, __LINE__)
# define mem_size(p)        mem_size_      ((p),            __FILE__, __LINE__)
#else

/*  Transaction-based allocation macros                                      */

# define memt_alloc(t,n)    mem_alloc_     ((t),  (n),      NULL, 0)
# define memt_strdup(t,s)   mem_strdup_    ((t),  (s),      NULL, 0)
# define memt_descr(t,p,n)  mem_descr_     ((t),  (p), (n), NULL, 0)

/*  Basic allocation macros                                                  */

# define mem_alloc(n)       mem_alloc_     (NULL, (n),      NULL, 0)
# define mem_strdup(s)      mem_strdup_    (NULL, (s),      NULL, 0)
# define mem_descr(p,n)     mem_descr_     (NULL, (p), (n), NULL, 0)

/*  Other functions requiring __FILE__ & __LINE__ substitution               */

# define mem_new_trans()    mem_new_trans_ (                NULL, 0)
# define mem_commit(t)      mem_commit_    ((t),            NULL, 0)
# define mem_rollback(t)    mem_rollback_  ((t),            NULL, 0)
# define mem_realloc(p,n)   mem_realloc_   ((p), (n),       NULL, 0)
# define mem_free(p)        mem_free_      ((p),            NULL, 0)
# define mem_strfree(ps)    mem_strfree_   ((ps),           NULL, 0)
# define mem_assert()       mem_assert_    (                NULL, 0)
# define mem_checkall()     mem_checkall_  (                NULL, 0)
# define mem_check(p)       mem_check_     ((p),            NULL, 0)
# define mem_size(p)        mem_size_      ((p),            NULL, 0)
#endif

#endif
