#ifndef TA_PARALLEL_H
#define TA_PARALLEL_H

#ifndef TA_SYSTEM_H
   #include "ta_system.h"
#endif

#ifndef TA_GLOBAL_H
   #include "ta_global.h"
#endif

/*************************************************************************
 * NOTE: DO NOT USE THAT FUNCTIONALITY YET. IT IS UNDER DEVELOPMENT.
 *       IT WILL NOT WORK UNTIL SOME GLUE CODE ARE ADDED.
 *************************************************************************
 *************************************************************************
 *************************************************************************
 *************************************************************************
 */

/* Very basic parallel support.
 *
 * This interface is not intended to be a "full blown" parallel/multithread
 * support. It is specifically kept simple for internal use in the TA-LIB.
 *
 * The functions/macro defined here can be used even on uni-processor system.
 * The library will attempt to run things in parallel only if TA_PARALLEL
 * is defined and only when it makes sense.
 *
 * When TA_PARALLEL is NOT defined, the functions/macros are non-obstrusive
 * and the processing is done sequentially.
 *
 * There is two kind of support. One for General purpose usage and the other
 * specifically for the TA-FUNC functions. These functions are not I/O bound so
 * they required a slightly different treatment.
 *
 * Interface for general purpose (I/O Bound processing thread usually):
 *   An unlimited number of thread can be spawn with TA_PAR_EXEC.
 *
 *   Example:
 *   This example starts two threads. At the TA_PAR_JOIN point, execution of the
 *   mainThread is block until all thread returned.
 *
 *      void threadedFunction( void *args )
 *      {
 *         puts( (char *)args );
 *      }
 *
 *      void mainThread( void )
 *      {
 *         TA_PAR_VARS;
 *
 *         TA_PAR_INIT;
 *
 *         TA_PAR_EXEC( threadedFunction, "Hello"  );
 *         TA_PAR_EXEC( threadedFunction, "world!" );
 *
 *         ... some processing can be added here too...
 *
 *         TA_PAR_JOIN;
 *
 *         TA_PAR_END;
 *      }
 *
 * Interface specifically for function from the TA-FUNC library:
 *    All technical analysis functions in the TA-FUNC library can be called
 *    for parallel processing. There is no guarantee that the processing
 *    will be done in parallel. This is due to the fact that these technical
 *    analysis function are CPU bound and generally never I/O bound. Consequently
 *    there is generally no advantage to run more thread than there is procesors
 *    reachable for parallel processing.
 *    This interface provides simple and automatic protection against CPU
 *    trashing because of "over-threading".
 *
 *    Example:
 *      void TA_NewIndicator( TA_Libc *libHandle, ... )
 *      {
 *         TA_PAR_VARS;
 *
 *         TA_PAR_INIT( libHandle );
 *
 *         TA_PAR_FUNC( TA_MA( ... ) );
 *         TA_PAR_FUNC( TA_EMA( ... ) );
 *         TA_PAR_JOIN;
 *
 *         TA_PAR_FUNC( TA_RSI( ... ) );
 *         ... some processing can be added here too...
 *         TA_PAR_JOIN;
 *
 *         TA_PAR_END;
 *      }
 *
 *    Note 1: ONLY technical analysis functions that has a prototype in the
 *            "ta_func.h" can be called with TA_PAR_FUNC( ). A link error will
 *            occured if an unsupported function is called.
 *
 *    Note 2: Parallel processing shall be done only on DISTINCTIVE input and
 *            output, else you kill the performance (because of possible memory
 *            cache trashing).
 */

#if defined(TA_PARALLEL) && !defined( TA_SINGLE_THREAD )
   #define TA_PAR_VARS    TA_BarrierSync TA_PAR_BS
   #define TA_PAR_INIT(LH){TA_BarrierSyncInit(LH,&TA_PAR_BS);}
   #define TA_PAR_JOIN    {TA_BarrierSyncWaitAllDone(&TA_PAR_BS);}
   #define TA_PAR_END     {TA_BarrierSyncDestroy(&TA_PAR_BS);}

   /* TA_PAR_FUNC is exclusively for function found in "ta_func.h" */
   #define TA_PAR_FUNC(rc,x) {TA_BarrierSyncThreadAdd(&TA_PAR_BS); rc = MAC_PAR_##x;}

   /* TA_PAR_EXEC is for general purpose parallelism. */
   #define TA_PAR_EXEC(x,w) TA_PAR_EXEC_F( &TA_PAR_BS, x, w )

   /* The TA_BarrierSync is a support tool for the above macro.
    * Should never be called directly.
    */
   typedef struct
   {
      TA_Sema mutexSema; /* Mutex for this structure. */
      volatile unsigned int nbThread; /* Nb thread left to be done. */
      TA_Sema barrierSema;   /* Stay block until all thread are done. */
      TA_Libc *libHandle;
   } TA_BarrierSync;

   TA_RetCode TA_BarrierSyncInit       ( TA_Libc *libHandle, TA_BarrierSync *bs );
   TA_RetCode TA_BarrierSyncDestroy    ( TA_BarrierSync *bs );
   TA_RetCode TA_BarrierSyncThreadAdd  ( TA_BarrierSync *bs );
   TA_RetCode TA_BarrierSyncThreadDone ( TA_BarrierSync *bs );
   TA_RetCode TA_BarrierSyncWaitAllDone( TA_BarrierSync *bs );

   /* TA_PAR_EXEC_F is a support function for the above macros.
    * Should never be called directly.
    */
   void TA_PAR_EXEC_F( TA_BarrierSync *bs, TA_ThreadFunction newThread, void *args );
#else
   #define TA_PAR_VARS
   #define TA_PAR_INIT(x)
   #define TA_PAR_FUNC(rc,x) rc=x
   #define TA_PAR_JOIN
   #define TA_PAR_END
   #define TA_PAR_EXEC(x,w) {x(w);}
#endif


#if 0
The following code works! TRICK TO REMEMBER!!!

void PAR_call( int i, const char *str )
{
   printf( "You made it %d [%s]!", i, str );
}

/* The following macro will be generated by gen_code */
#define D_call(param) PAR_call( z, param )
#define PAR(func) D_##func

/* Call example: */
int main(int argc, char* argv[])
{
int z;

z = 12;

PAR( call( "Bouh!" ) );
getchar();
        return 0;
}

#endif

#endif
