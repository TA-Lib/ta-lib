/****************************************************************************/
/* File : ta_libc.swg
 * This is the main SWIG file for the TA-Lib wrapper.
 * It includes other swg files, if needed.
 */


%header %{
#include "ta_libc.h"
%}

/* include standard SWIG typemaps */
%include "typemaps.i"


/* include typemaps for all TA interfaces */
#ifdef SWIGPERL
  %include "ta_libc.perl.swg"
#endif
#ifdef SWIGPYTHON
  %include "ta_libc.python.swg"
#endif


/* Strip all const qualifies, they are not relevant to scripts,
 * but may prevent SWIG release memory in proxy classes.
 * This is safe because TA-Lib never sets returned const char pointers
 * to anything in static (data) area by itself (except in ta_abstract).
 * If needed, const members are made explicitly immutable.
 */
%clear const char *;

/* hiddenData should not be accessible for client code */
%ignore hiddenData;



/** ta_defs *****************************************************************/

/* The constants below are pulled out from <limits.h> and used by ta_defs.h 
 * to define some TA-specific constants.
 * They have to be redefined here otherwise SWIG would not be able
 * to determine the values of some TA constants and would skip them.
 * The limits.h constants themselves do not need to be exported by the module;
 * that's why they are tagged %ignore
 */

%ignore INT_MAX;
%ignore INT_MIN;
#define INT_MAX 2147483647
#define INT_MIN -INT_MAX-1


%include "ta_defs.h"


/** ta_common ***************************************************************/

/* TA_StringTable** is used as output in TA_XxxAlloc() functions */
STRING_TABLE(TA_StringTable, size)

/* Sometimes it is used as output to constant strings */
CONST_STRING_TABLE(TA_StringTable, size)

%include "ta_common.h"

/** ta_func *****************************************************************/

/* ta_func typemaps are target language specific, see ta_libc.XXX.swg */

/* Treat OPT_MATYPE as OPT_INT */
%apply int OPT_INT { TA_MAType OPT_MATYPE };

%include "ta_func.swg"


/** ta_abstract *************************************************************/

%rename(TA_GroupTable) TA_GroupTableAlloc;
%rename(TA_FuncTable)  TA_FuncTableAlloc;
%ignore TA_FuncTableFree;
%ignore TA_GroupTableFree;

CONST_HANDLE(TA_FuncHandle, handle)
CONST_HANDLE(TA_FuncInfo, funcInfo)
CONST_HANDLE(TA_InputParameterInfo, info)
CONST_HANDLE(TA_OptInputParameterInfo, info)
CONST_HANDLE(TA_OutputParameterInfo, info)

/* Fool SWIG that TA_FuncHandle is a struct.  SWIG creates shadow classes only
 * for structs and classes.
 */
typedef struct {} TA_FuncHandle;

/* similarly other const structs here */

/* TA_ForEachFunc is not needed, use 'map' or 'grep' on the list (in Perl) */
%ignore TA_ForEachFunc;

/* None of TA_FuncInfo fields may be modified, especially const char* */
/* The same applies to other structs */
%immutable name;
%immutable group;
%immutable hint;
%immutable camelCaseName;
%immutable flags;
%immutable nbInput;
%immutable nbOptInput;
%immutable nbOutput;
%immutable handle;
%immutable type;
%immutable paramName;
%immutable displayName;
%immutable dataSet;
%immutable defaultValue;

/* The field 'dataSet' is a tricky one; it has to be recast from const void*
 * to something sensible depending on the value of 'type'
 * It is target language dependant; typemap defined in la_libc.XXX.swg
 */

/* The following structs are never created; 
 * Only pointers to const structs can be obtained through 'dataSet' access
 */
%nodefault TA_RealRange;
%nodefault TA_IntegerRange;
%nodefault TA_RealDataPair;
%nodefault TA_IntegerDataPair;
%nodefault TA_RealList;
%nodefault TA_IntegerList;


/* Again, the structs are read only */
%immutable min;
%immutable max;
%immutable precision;
%immutable suggested_start;
%immutable suggested_end;
%immutable suggested_increment;
%immutable value;
%immutable string;
%immutable data;
%immutable nbElement;


/* Accessing 'data' of TA_RealList and TA_IntegerList is similar to 
 * array access in TA_History 
 */
ARRAY_OBJECT(TA_RealList, nbElement)
ARRAY_OBJECT(TA_IntegerList, nbElement)
MEMBER_ARRAY(TA_RealDataPair)
MEMBER_ARRAY(TA_IntegerDataPair)
%apply const TA_RealDataPair *ARRAY { const TA_RealDataPair *data };
%apply const TA_IntegerDataPair *ARRAY { const TA_IntegerDataPair *data };

/* The functions supporting abstract calling are not wrapped.
 * There is no need for that; in Perl, you can already call 
 * any function by name and with any parameter list you like
 * Any decent scripting language supports the same.
 */

%ignore TA_ParamHolder;
%ignore TA_ParamHolderAlloc;
%ignore TA_ParamHolderFree;
%ignore TA_SetInputParamIntegerPtr;
%ignore TA_SetInputParamRealPtr;
%ignore TA_SetInputParamPricePtr;
%ignore TA_SetOptInputParamInteger;
%ignore TA_SetOptInputParamReal;
%ignore TA_SetOutputParamIntegerPtr;
%ignore TA_SetOutputParamRealPtr;
%ignore TA_GetLookback;
%ignore TA_CallFunc;

%include "ta_abstract.h"

/* ta_abstract is the last module, so we do not have to clean up 
 * the definitions specific to this file.
 * I hope that some day there will be a SWIG version that is able to process
 * several interface files for one library.
 */

/****************************************************************************/
