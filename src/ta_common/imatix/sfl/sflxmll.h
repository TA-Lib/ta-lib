/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflxmll.h
    Title:      XML serialisation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/21

    Synopsis:   Provides functions to load and save XML files.  An XML file
                is held in memory as a tree of nodes, of type XML_ITEM.  The
                XML functions do not currently accept DTDs in the XML data.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SLFXMLL_INCLUDED               /*  Allow multiple inclusions        */
#define SLFXMLL_INCLUDED

/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/*  Error values  */

#define XML_NOERROR         0           /*  No errors                        */
#define XML_FILEERROR       1           /*  Error in file i/o                */
#define XML_LOADERROR       2           /*  Error loading XML                */

/*  Function prototypes  */

int       xml_save_file      (XML_ITEM   *item, const char *filename);
char     *xml_save_string    (XML_ITEM   *item);
char     *xml_error          (void);
int       xml_seems_to_be    (const char *path,
                              const char *filename);
int       xml_load_file      (XML_ITEM   **item,
                              const char *path,
                              const char *filename,
                              Bool  allow_extended);
int       xml_load_string    (XML_ITEM   **item,
                              const char *xmlstring,
                              Bool  allow_extended);

/*  Macros  */

#define xml_load_extended(item, path, filename)                               \
    xml_load_file (item, path, filename, TRUE)
#define xml_load(item, path, filename)                                        \
    xml_load_file (item, path, filename, FALSE)


#ifdef __cplusplus
}
#endif

#endif                                  /*  Included                         */
