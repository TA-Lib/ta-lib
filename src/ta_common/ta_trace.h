
#ifndef TA_TRACE_H
#define TA_TRACE_H

#ifndef TA_DICT_H
   #include "ta_dict.h"
#endif

/* This module allows to monitor and log internal error.
 * Most of the implementation is done through macro allowing
 * to enable/disable the feature.
 *
 * More loging and debugging is performed when TA_DEBUG is defined. 
 * TA_DEBUG shall never be defined in the final library.
 */

/* Note: TA-LIB is NEVER doing any console I/O (no putchar, printf etc...) unless
 *       it is specifically requested by the library user (like with the TA_FatalReport
 *       function).
 * 
 *       This is to make possible to integrate the TA-LIB in any GUI without undesirable
 *       console output.
 */

/* TA_FILE_INFO must be placed at the top in the 'c' file, prior to 
 * using any of the tracing or assert macro.
 */
#ifndef TA_FILE_INFO
#define TA_FILE_INFO static const char *theFileNamePtr = __FILE__; \
                     static const char *theFileDatePtr = __DATE__; \
                     static const char *theFileTimePtr = __TIME__;
#endif

/* The TA_FATAL/TA_ASSERT macro are going to be ALWAYS enabled in the
 * final library.
 *
 * Upon failure, these macro will "return" from the function. There is no 
 * "exit" done to prevent to shutdown unpropriatly the application.
 *
 * On a fatal/assertion failure, the software will record information. The
 * function will return the failed function.
 *
 * The user of the library can retreive the recorded information on the failure
 * by using TA_FatalReport.
 */
#define TA_ASSERT(b) {if(!(b)) {TA_PrivError(1,#b,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,0,0); TA_TRACE_RETURN(TA_FATAL_ERR);}}
/* Same as TA_ASSERT, but return the specified value on failure. */
#define TA_ASSERT_RET(b,ret) {if(!(b)) {TA_PrivError(1,#b,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,0,0); return ret;}}
/* Same as TA_ASSERT, but for function returning void. */
#define TA_ASSERT_NO_RET(b) {if(!(b)) {TA_PrivError(1,#b,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,0,0); return;}}

#define TA_FATAL(str,param1,param2) {TA_PrivError(0,str,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,(unsigned int)param1,(unsigned int)param2); TA_TRACE_RETURN( TA_FATAL_ERR );}
/* Same as TA_ASSERT, but return the specified value on failure. */
#define TA_FATAL_RET(str,param1,param2,ret) {TA_PrivError(0,str,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,(unsigned int)param1,(unsigned int)param2); return ret;}
/* Same as TA_FATAL, but for function returning void. */
#define TA_FATAL_NO_RET(str,param1,param2) {TA_PrivError(0,str,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,(unsigned int)param1,(unsigned int)param2); return;}

/* When TA_DEBUG is defined, additional "paranoiac" testing can be performed
 * for checking the integrity of the software.
 *
 * This assert mechanism will NOT be enabled in the final library.
 */
#ifdef TA_DEBUG
   #define TA_DEBUG_ASSERT(b){if(!(b)){TA_PrivError(2,#b,theFileNamePtr,theFileDatePtr,theFileTimePtr,__LINE__,1,1); return TA_FATAL_ERR;}}
#else
   #define TA_DEBUG_ASSERT(b)
#endif

/* Tracing allows to fulfill the following needs:
 *     - Identify the context of execution when a
 *       fatal error is detected.
 *     - Allows to do an estimation of the code coverage
 *       while regression testing is performed.
 *
 * To make this efficient, all internal functions having access to
 * the TA_Libc handle and returning TA_RetCode shall use the
 * tracing MACRO:
 * 
 *   TA_PROLOG:
 *        Must be specified at the very top of all functions
 *        (before the local parameters).
 *
 *   TA_TRACE_BEGIN:
 *        Must be the first line executed in the function.
 *
 *   TA_TRACE_CHECKPOINT:
 *        Shall be put into the code block{} you wish to monitor
 *        for code coverage.
 *
 *   TA_TRACE_RETURN:
 *        Must be called at all exit point of the function.
 *
 * Example:
 *
 *    TA_FILE_INFO;
 *
 *    TA_RetCode myFunction( int param )
 *    {
 *       TA_PROLOG
 *       int i, j;  
 *
 *       TA_TRACE_BEGIN(myFunction);
 *
 *       if( param == 0 )
 *       {
 *          TA_TRACE_CHECKPOINT;
 *          TA_TRACE_RETURN( TA_BAD_PARAM );
 *       }
 *       else
 *       {
 *          TA_TRACE_CHECKPOINT;
 *
 *          for( i=0; i < 10; i++ )
 *             printf( "%d", i );
 *       }
 *
 *       TA_TRACE_RETURN( TA_SUCCESS );
 *    }
 */

#ifdef TA_DEBUG
   #define TA_PROLOG    const char *theTraceFuncName;
   #define TA_TRACE_BEGIN(name) { TA_PrivTraceBegin( #name, theFileNamePtr, __LINE__ ); \
                                  theTraceFuncName = #name; \
								        }

   #define TA_TRACE_CHECKPOINT { TA_PrivTraceCheckpoint( theTraceFuncName, theFileNamePtr, __LINE__ ); }

   #define TA_TRACE_RETURN(x) { return TA_PrivTraceReturn( theTraceFuncName, theFileNamePtr, __LINE__, x ); }
#else
   #define TA_PROLOG
   #define TA_TRACE_BEGIN(x)
   #define TA_TRACE_CHECKPOINT
   #define TA_TRACE_RETURN(x) {return x;}
#endif


/* Never call directly the following functions. Always use the macro define above. */
void TA_PrivTraceCheckpoint( const char *funcname,
							        const char *filename,
						           unsigned int lineNb );
   
TA_RetCode TA_PrivTraceReturn( const char *funcname,
							          const char *filename,
						             unsigned int lineNb,
						             TA_RetCode retCode );

void TA_PrivTraceBegin( const char *funcname,
						      const char *filename, 
						      unsigned int lineNb );

void TA_PrivError( unsigned int type, const char *str,
                   const char *filename, const char *date,
                   const char *time, int lineNb,
                   unsigned int j, unsigned int k  );

#endif

