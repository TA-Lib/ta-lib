#ifndef TA_GLOBAL_H
#define TA_GLOBAL_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

/* This interface is used exclusively INTERNALY to the TA-LIB.
 * There is nothing for the end-user here ;->
 */

/* Provides functionality for managing global ressource
 * throughout the TA-LIB.
 *
 * All global variable are in fact allocated memory. This
 * allow to make the library truly re-entrant.
 *
 * Since not all module are used/link in the application,
 * the ta_common simply provides the mechanism for the module
 * to optionnaly "register" its initialization/shutdown
 * function.
 *
 * A function shall always access its global variable by
 * calling TA_GetGlobal. This function will appropriatly
 * call the initialization function if its global are not
 * yet initialized.
 *
 * The call of the init and shutdown function are guaranteed
 * to be multithread protected. It is also guarantee that
 * these function will always get called in alternance (in
 * other word, following an initialization only a shutdown
 * can get called).
 */

typedef enum
{
   /* Module will be shutdown in the order specified here. */
   	
   TA_ABSTRACTION_GLOBAL_ID,
   TA_FUNC_GLOBAL_ID,
   TA_DATA_GLOBAL_ID,

   TA_NETWORK_GLOBAL_ID,   /* Network/internet support. */
   TA_SYSTEM_GLOBAL_ID,    /* OS specific ressources. */

   TA_TRACE_GLOBAL_ID,  /* Must be before last. */
   TA_MEMORY_GLOBAL_ID, /* Must be last.        */

   TA_NB_GLOBAL_ID
} TA_GlobalModuleId;

typedef TA_RetCode (*TA_GlobalInitFunc)    ( TA_Libc *libHandle, void **globalToAlloc );
typedef TA_RetCode (*TA_GlobalShutdownFunc)( TA_Libc *libHandle, void *globalAllocated );

typedef struct
{
   const TA_GlobalModuleId     id;
   const TA_GlobalInitFunc     init;
   const TA_GlobalShutdownFunc shutdown;
} TA_GlobalControl;

TA_RetCode TA_GetGlobal( TA_Libc *libHandle,
                         const TA_GlobalControl * const control,
                         void **global );

TA_StringCache *TA_GetGlobalStringCache( TA_Libc *libHandle );

/* Occasionaly, code tracing must be disable.
 * Example:
 *  - The memory module needs to know if the tracing is
 *    still enabled or not when freeing memory on shutdown.
 *  - We do not want to recursively trace while the tracing
 *    function themselves gets called ;->
 */
int  TA_IsTraceEnabled( TA_Libc *libHandle );
void TA_TraceEnable   ( TA_Libc *libHandle );
void TA_TraceDisable  ( TA_Libc *libHandle );

/* If enabled by the user, use stdio for debugging.
 * Will return NULL if not enabled.
 */
FILE *TA_GetStdioFilePtr( TA_Libc *libHandle );

/* If enabled by the user, use a local drive
 * for configuration and/or temporary file.
 * TA-LIB must NEVER assume such local drive 
 * is available.
 */
const char *TA_GetLocalCachePath( TA_Libc *libHandle );

#endif

