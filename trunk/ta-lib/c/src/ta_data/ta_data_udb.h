#ifndef TA_DATA_UDB_H
#define TA_DATA_UDB_H

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_DICT_H
   #include "ta_dict.h"
#endif

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_SYSTEM_H
   #include "ta_system.h"
#endif

/* Declarations used only within the TA_DATA module. Should never
 * be accessible by the library end-user.
 */

/* The unified database is represented in memory by using mainly
 * the following structures:
 *
 *   TA_UDB_Category
 *      One exist for each distinct category in the unified database.
 *
 *   TA_UDB_Symbol
 *      One exist for each distinct symbol in a given category in the
 *      unified database.
 *
 *   TA_UDB_Driver
 *      One or more exist for a given symbol in the unified database.
 *      That structure point to all the information needed for retreiving
 *      the data from a particular data source.
 */
typedef struct
{
   TA_String *string;
   TA_Dict *dictUDBSymbol; /* Dictionary of TA_UDB_Symbol for that category. */

   TA_List listUDBDriverArray; /* List of array of TA_UDB_Driver. An array
                                * will exist for each data source added who
                                * contributes some symbol to this category.
                                */

   /* Allocation Strategy: This structure owns all its sub-elements. */
} TA_UDB_Category;

typedef struct
{
   TA_String *string;
   TA_List listDriverHandle; /* List of TA_DriverHandle for this symbol. */

   /* Allocation Strategy: This structure owns all its sub-elements. */
} TA_UDB_Symbol;

typedef struct
{
   /* This structure keep tracks of everything needed to retreive
    * a symbol from a particular data source drivers.
    */
   TA_SourceId          index;
   TA_DataSourceHandle *sourceHandle;
   TA_CategoryHandle   *categoryHandle;
   TA_SymbolHandle      symbolHandle;

   /* Pre-allocated list node used allowing to easily include 
    * this structure into the listDriverHandle in the TA_UDB_Symbol.
    */
   TA_ListNode node;

   /* Allocation Strategy: This structure does not owns the pointed elements. */
} TA_UDB_Driver;

/* The hidden implementation of an "unified database".
 * From the user point of view, this is a TA_UDBase.
 */
typedef struct
{
  unsigned int magicNb;

  #if !defined( TA_SINGLE_THREAD )
  TA_Sema sema; /* To protect this structure integrity. */
  #endif
  TA_List *listDataSource; /* List of TA_DataSource   */
  TA_Dict *dictCategory;   /* Dict of TA_UDB_Category */

  /* Keep a pointer on the default category. */
  TA_UDB_Category *defaultCategory;

  /* The structure has been allocated for this library. */
  TA_Libc *libHandle;
} TA_UDBasePriv;

#endif
