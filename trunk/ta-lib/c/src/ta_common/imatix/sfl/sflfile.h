/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflfile.h
    Title:      File-access functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/11/08

    Synopsis:   Provides functions to read and write files with explicit
                new-line/carriage-return control; to find files on a path;
                to copy files, check files' protection, etc.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLFILE_INCLUDED               /*  Allow multiple inclusions        */
#define SFLFILE_INCLUDED


/*  System-specific definitions                                              */

#if (defined (__MSDOS__))
#   define FOPEN_READ_TEXT      "rt"    /*  Under DOS we can be explict      */
#   define FOPEN_READ_BINARY    "rb"    /*    and use 't' or 'b' in fopen    */
#   define FOPEN_WRITE_TEXT     "wt"
#   define FOPEN_WRITE_BINARY   "wb"
#   define FOPEN_APPEND_TEXT    "at"
#   define FOPEN_APPEND_BINARY  "ab"
#elif (defined (__VMS__))
#   define FOPEN_READ_TEXT      "r"     /*  Dec C does not like 't' or 'b'   */
#   define FOPEN_READ_BINARY    "r"
#   define FOPEN_WRITE_TEXT     "w"
#   define FOPEN_WRITE_BINARY   "w"
#   define FOPEN_APPEND_TEXT    "a"
#   define FOPEN_APPEND_BINARY  "a"
#elif (defined (__UNIX__))
#   define FOPEN_READ_TEXT      "rt"    /*  Under UNIX we can be explict     */
#   define FOPEN_READ_BINARY    "rb"    /*    and use 't' or 'b' in fopen    */
#   define FOPEN_WRITE_TEXT     "wt"
#   define FOPEN_WRITE_BINARY   "wb"
#   define FOPEN_APPEND_TEXT    "at"
#   define FOPEN_APPEND_BINARY  "ab"
#elif (defined (__OS2__))
#   define FOPEN_READ_TEXT      "rt"    /*  Under OS/2 we can be explict     */
#   define FOPEN_READ_BINARY    "rb"    /*    and use 't' or 'b' in fopen    */
#   define FOPEN_WRITE_TEXT     "wt"
#   define FOPEN_WRITE_BINARY   "wb"
#   define FOPEN_APPEND_TEXT    "at"
#   define FOPEN_APPEND_BINARY  "ab"
#else
#   error "No definitions for FOPEN constants"
#endif


/*  Constants                                                                */

enum {
    CYCLE_ALWAYS  = 0,                  /*  Cycle file unconditionally       */
    CYCLE_HOURLY  = 1,                  /*  Cycle file if hour has changed   */
    CYCLE_DAILY   = 2,                  /*  Cycle file if day has changed    */
    CYCLE_WEEKLY  = 3,                  /*  Cycle file if week has changed   */
    CYCLE_MONTHLY = 4,                  /*  Cycle file if month has changed  */
    CYCLE_NEVER   = 5                   /*  Don't cycle the file             */
};

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

FILE  *file_open           (TA_Libc *libHandle, const char *filename, char mode);

#if 0
!!! Not needed within Ta-Lib
FILE  *file_locate         (TA_Libc *libHandle, const char *path, const char *name,
                            const char *ext);
#endif

int    file_close          (TA_Libc *libHandle, FILE *stream);
Bool   file_read           (TA_Libc *libHandle, FILE *stream, char *string);
Bool   file_readn          (TA_Libc *libHandle, FILE *stream, char *string, int line_max);
char  *file_write          (TA_Libc *libHandle, FILE *stream, const char *string);
int    file_copy           (TA_Libc *libHandle, const char *dest, const char *src, char mode);
int    file_concat         (TA_Libc *libHandle, const char *dest, const char *src);
int    file_rename         (TA_Libc *libHandle, const char *oldname, const char *newname);
int    file_delete         (TA_Libc *libHandle, const char *filename);
char  *file_where          (TA_Libc *libHandle, char mode, const char *path, const char *name,
                            const char *ext);
char  *file_where_ext      (TA_Libc *libHandle, char mode, const char *path, const char *name,
                            const char **ext);
Bool   file_exists         (TA_Libc *libHandle, const char *filename);
Bool   file_cycle          (TA_Libc *libHandle, const char *filename, int how);
Bool   file_cycle_needed   (TA_Libc *libHandle, const char *filename, int how);
Bool   file_has_changed    (TA_Libc *libHandle, const char *filename, long old_date, long old_time);
Bool   safe_to_extend      (TA_Libc *libHandle, const char *filename);
char  *default_extension   (TA_Libc *libHandle, char *dest, const char *src, const char *ext);
char  *fixed_extension     (TA_Libc *libHandle, char *dest, const char *src, const char *ext);
char  *strip_extension     (TA_Libc *libHandle, char *filename);
char  *add_extension       (TA_Libc *libHandle, char *dest, const char *src, const char *ext);
char  *strip_file_path     (TA_Libc *libHandle, char *filename);
char  *strip_file_name     (TA_Libc *libHandle, char *filename);

/* Will put a NULL between the path and the file adn return
 * a ptr on the file.
 * If there is no split occuring, NULL is returned.
 */
char  *split_path_and_file (TA_Libc *libHandle, char *name );

#if 0
!!! Not needed within TA-Lib
char  *get_new_filename    (TA_Libc *libHandle, const char *filename);
#endif

Bool   file_is_readable    (TA_Libc *libHandle, const char *filename);
Bool   file_is_writeable   (TA_Libc *libHandle, const char *filename);
Bool   file_is_executable  (TA_Libc *libHandle, const char *filename);
Bool   file_is_directory   (TA_Libc *libHandle, const char *filename);
Bool   file_is_program     (TA_Libc *libHandle, const char *filename);
Bool   file_is_legal       (TA_Libc *libHandle, const char *filename);
char  *file_exec_name      (TA_Libc *libHandle, const char *filename);
long   get_file_size       (TA_Libc *libHandle, const char *filename);
time_t get_file_time       (TA_Libc *libHandle, const char *filename);
long   get_file_lines      (TA_Libc *libHandle, const char *filename);

#if 0
!!! Not needed within TA-Lib
DESCR *file_slurp          (TA_Libc *libHandle, const char *filename);
DESCR *file_slurpl         (TA_Libc *libHandle, const char *filename);
#endif

dbyte  file_set_eoln       (TA_Libc *libHandle, char *dest, const char *src, dbyte src_size,
                            Bool add_cr);
#if 0
!!! Not needed within TA-Lib
char  *get_tmp_file_name   (TA_Libc *libHandle, const char *path, qbyte *index, const char *ext);
int    file_fhredirect     (TA_Libc *libHandle, int source, int dest);
void   file_fhrestore      (TA_Libc *libHandle, int source, int dest);
FILE  *ftmp_open           (TA_Libc *libHandle, char **pathname);
void   ftmp_close          (TA_Libc *libHandle, FILE *tempstream);
#endif

#ifdef __cplusplus
}
#endif


/*  Symbols, macros                                                          */

#define FILE_NAME_MAX   160             /*  Max size of filename             */
#define FILE_DIR_MAX    64              /*  Max size of directory name       */
#define file_lines(f)   get_file_lines(f)   /*  Changed 98/07/23             */

/*  External variables                                                       */

extern Bool  file_crlf;                 /*  TRUE or FALSE                    */

#endif
