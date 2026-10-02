/* Driver for scripts/thread_sanitize.py, which says why it exists.
 *
 * Every thread runs the same work, against answers computed once on one thread:
 *
 *  typed     every function's double and float batch entry points
 *  abstract  every function looked up by name, its lookback, TA_CallFunc
 *  openers   every function's Open and OpenAndFill, then Close
 *  stream    a handle per thread: Peek and Update to the end, one Advance
 *  readers   one handle per shape shared by every thread, with no writer:
 *            Peek, Value, OutRange and Clone
 *
 * The last two take a spread of shapes rather than every function: a ring, a
 * recursive state with two outputs, a candle window, a period bank, every arm
 * of the MA dispatcher, a long fixed state, and a stream built on sub-streams.
 *
 * Settings are changed once, before any thread starts.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ta_libc.h"
#include "ta_stream_frame.h"

#define NB_BAR     400
#define PREFIX     200
#define NB_THREAD  8
#define NB_REP     2
#define MAX_OUT    4
#define MAX_IN     8
#define MAX_OPT    16

#define EXIT_MISMATCH 10
#define EXIT_VACUOUS  11

static double gOpen[NB_BAR], gHigh[NB_BAR], gLow[NB_BAR], gClose[NB_BAR], gVolume[NB_BAR];
static double gPeriods[NB_BAR];
static float  gOpenF[NB_BAR], gHighF[NB_BAR], gLowF[NB_BAR], gCloseF[NB_BAR], gVolumeF[NB_BAR];
static float  gPeriodsF[NB_BAR];

typedef struct
{
   double real[MAX_OUT][NB_BAR];
   int    integer[MAX_OUT][NB_BAR];
   int    beg, nb, lookback;
   TA_RetCode rc;
} Answer;

/* [0] double, [1] float, [2] abstract, [3] Open, [4] OpenAndFill */
#define NB_LEG 5
static Answer *gRef[NB_LEG];

static const double *series( TA_VInputKind kind )
{
   switch( kind )
   {
   case TA_VIN_OPEN:    return gOpen;
   case TA_VIN_HIGH:    return gHigh;
   case TA_VIN_LOW:     return gLow;
   case TA_VIN_VOLUME:  return gVolume;
   case TA_VIN_PERIODS: return gPeriods;
   case TA_VIN_OPENINTEREST: return gVolume;
   default:             return gClose;
   }
}

static const float *series_float( TA_VInputKind kind )
{
   switch( kind )
   {
   case TA_VIN_OPEN:    return gOpenF;
   case TA_VIN_HIGH:    return gHighF;
   case TA_VIN_LOW:     return gLowF;
   case TA_VIN_VOLUME:  return gVolumeF;
   case TA_VIN_PERIODS: return gPeriodsF;
   case TA_VIN_OPENINTEREST: return gVolumeF;
   default:             return gCloseF;
   }
}

static void bind_outputs( Answer *answer, double *outReal[MAX_OUT], int *outInt[MAX_OUT] )
{
   int k;
   for( k = 0; k < MAX_OUT; k++ )
   {
      outReal[k] = answer->real[k];
      outInt[k] = answer->integer[k];
   }
}

static void run_typed( int f, int useFloat, Answer *answer )
{
   const TA_VariantEntry *e = &TA_VariantTable[f];
   const double *in[MAX_IN];
   const float *inF[MAX_IN];
   double opt[MAX_OPT];
   double *outReal[MAX_OUT];
   int *outInt[MAX_OUT];
   int k;

   memset( answer, 0, sizeof(*answer) );
   for( k = 0; k < e->nbInput; k++ )
   {
      in[k] = series( e->inputKind[k] );
      inF[k] = series_float( e->inputKind[k] );
   }
   for( k = 0; k < e->nbOptInput; k++ )
      opt[k] = e->optInput[k].defValue;
   bind_outputs( answer, outReal, outInt );
   answer->rc = useFloat
      ? e->single( 0, NB_BAR - 1, inF, opt, &answer->beg, &answer->nb, outReal, outInt )
      : e->guarded( 0, NB_BAR - 1, in, opt, &answer->beg, &answer->nb, outReal, outInt );
}

static void run_abstract( int f, Answer *answer )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo *info;
   TA_ParamHolder *holder;
   unsigned int k;
   int nbReal = 0;

   memset( answer, 0, sizeof(*answer) );
   answer->rc = TA_GetFuncHandle( TA_VariantTable[f].name, &handle );
   if( answer->rc != TA_SUCCESS ) return;
   answer->rc = TA_GetFuncInfo( handle, &info );
   if( answer->rc != TA_SUCCESS ) return;
   answer->rc = TA_ParamHolderAlloc( handle, &holder );
   if( answer->rc != TA_SUCCESS ) return;
   for( k = 0; k < info->nbInput; k++ )
   {
      const TA_InputParameterInfo *in;
      TA_GetInputParameterInfo( handle, k, &in );
      if( in->type == TA_Input_Price )
         TA_SetInputParamPricePtr( holder, k, gOpen, gHigh, gLow, gClose, gVolume, NULL );
      else if( in->type == TA_Input_Real )
         TA_SetInputParamRealPtr( holder, k, nbReal++ == 0 ? gClose : gPeriods );
   }
   for( k = 0; k < info->nbOptInput; k++ )
   {
      const TA_OptInputParameterInfo *opt;
      TA_GetOptInputParameterInfo( handle, k, &opt );
      if( opt->type == TA_OptInput_RealRange || opt->type == TA_OptInput_RealList )
         TA_SetOptInputParamReal( holder, k, opt->defaultValue );
      else
         TA_SetOptInputParamInteger( holder, k, (TA_Integer)opt->defaultValue );
   }
   for( k = 0; k < info->nbOutput && k < MAX_OUT; k++ )
   {
      const TA_OutputParameterInfo *out;
      TA_GetOutputParameterInfo( handle, k, &out );
      if( out->type == TA_Output_Real )
         TA_SetOutputParamRealPtr( holder, k, answer->real[k] );
      else
         TA_SetOutputParamIntegerPtr( holder, k, answer->integer[k] );
   }
   answer->rc = TA_GetLookback( holder, &answer->lookback );
   if( answer->rc == TA_SUCCESS )
      answer->rc = TA_CallFunc( holder, 0, NB_BAR - 1, &answer->beg, &answer->nb );
   if( TA_ParamHolderFree( holder ) != TA_SUCCESS && answer->rc == TA_SUCCESS )
      answer->rc = TA_BAD_PARAM;
}

static void run_opener( int f, int fill, Answer *answer )
{
   const TA_StreamEntry *e = &TA_StreamTable[f];
   const double *in[MAX_IN];
   double opt[MAX_OPT];
   double *outReal[MAX_OUT];
   int *outInt[MAX_OUT];
   void *handle = NULL;
   int k;

   memset( answer, 0, sizeof(*answer) );
   for( k = 0; k < e->nbInput; k++ )
      in[k] = series( e->inputKind[k] );
   for( k = 0; k < e->nbOptInput; k++ )
      opt[k] = e->optInput[k].defValue;
   bind_outputs( answer, outReal, outInt );
   answer->rc = fill
      ? e->openAndFill( &handle, in, PREFIX, opt, &answer->beg, &answer->nb, outReal, outInt )
      : e->open( &handle, in, PREFIX, opt, outReal, outInt );
   if( handle && e->close( handle ) != TA_SUCCESS && answer->rc == TA_SUCCESS )
      answer->rc = TA_BAD_PARAM;
}

/* ---- the stream shapes ---- */
enum { S_SMA, S_MAMA, S_CDL, S_MAVP, S_HT, S_BBANDS, S_MA_FIRST, S_COUNT = S_MA_FIRST + TA_MATYPE_MAX + 1 };

typedef struct { double update[NB_BAR][3], peek[NB_BAR][3]; int beg, nb, failed; } StreamAnswer;
typedef struct { double peek[3], value[3], cloned[3]; int beg, nb, failed; } ReaderAnswer;
static StreamAnswer gStreamRef[S_COUNT];
static ReaderAnswer gReaderRef[S_COUNT];
static void *gShared[S_COUNT];

static TA_RetCode s_open( int s, void **h, int len, double out[3] )
{
   int io = 0;
   TA_RetCode rc;

   out[0] = out[1] = out[2] = 0.0;
   switch( s )
   {
   case S_SMA:  return TA_SMA_Open( (TA_SMA_Stream **)h, gClose, len, 14, &out[0] );
   case S_MAMA: return TA_MAMA_Open( (TA_MAMA_Stream **)h, gClose, len, 0.5, 0.05, &out[0], &out[1] );
   case S_CDL:
      rc = TA_CDLHAMMER_Open( (TA_CDLHAMMER_Stream **)h, gOpen, gHigh, gLow, gClose, len, &io );
      out[0] = io;
      return rc;
   case S_MAVP: return TA_MAVP_Open( (TA_MAVP_Stream **)h, gClose, gPeriods, len, 2, 30, TA_MAType_SMA, &out[0] );
   case S_HT:   return TA_HT_SINE_Open( (TA_HT_SINE_Stream **)h, gClose, len, &out[0], &out[1] );
   case S_BBANDS:
      return TA_BBANDS_Open( (TA_BBANDS_Stream **)h, gClose, len, 20, 2.0, 2.0, TA_MAType_EMA,
                             &out[0], &out[1], &out[2] );
   default:     return TA_MA_Open( (TA_MA_Stream **)h, gClose, len, 10, (TA_MAType)(s - S_MA_FIRST), &out[0] );
   }
}

#define BAR_CALL( VERB, QUAL ) \
static TA_RetCode s_##VERB( int s, QUAL void *h, int t, double out[3] ) \
{ \
   int io = 0; \
   TA_RetCode rc; \
   out[0] = out[1] = out[2] = 0.0; \
   switch( s ) \
   { \
   case S_SMA:  return TA_SMA_##VERB( h, gClose[t], &out[0] ); \
   case S_MAMA: return TA_MAMA_##VERB( h, gClose[t], &out[0], &out[1] ); \
   case S_CDL: \
      rc = TA_CDLHAMMER_##VERB( h, gOpen[t], gHigh[t], gLow[t], gClose[t], &io ); \
      out[0] = io; \
      return rc; \
   case S_MAVP: return TA_MAVP_##VERB( h, gClose[t], gPeriods[t], &out[0] ); \
   case S_HT:   return TA_HT_SINE_##VERB( h, gClose[t], &out[0], &out[1] ); \
   case S_BBANDS: return TA_BBANDS_##VERB( h, gClose[t], &out[0], &out[1], &out[2] ); \
   default:     return TA_MA_##VERB( h, gClose[t], &out[0] ); \
   } \
}
BAR_CALL( Update, )
BAR_CALL( Peek, const )

static TA_RetCode s_Value( int s, const void *h, double out[3] )
{
   int io = 0;
   TA_RetCode rc;

   out[0] = out[1] = out[2] = 0.0;
   switch( s )
   {
   case S_SMA:  return TA_SMA_Value( h, &out[0] );
   case S_MAMA: return TA_MAMA_Value( h, &out[0], &out[1] );
   case S_CDL:
      rc = TA_CDLHAMMER_Value( h, &io );
      out[0] = io;
      return rc;
   case S_MAVP: return TA_MAVP_Value( h, &out[0] );
   case S_HT:   return TA_HT_SINE_Value( h, &out[0], &out[1] );
   case S_BBANDS: return TA_BBANDS_Value( h, &out[0], &out[1], &out[2] );
   default:     return TA_MA_Value( h, &out[0] );
   }
}

#define HANDLE_CALL( VERB, ARGS ) \
   switch( s ) \
   { \
   case S_SMA:  return TA_SMA_##VERB ARGS; \
   case S_MAMA: return TA_MAMA_##VERB ARGS; \
   case S_CDL:  return TA_CDLHAMMER_##VERB ARGS; \
   case S_MAVP: return TA_MAVP_##VERB ARGS; \
   case S_HT:   return TA_HT_SINE_##VERB ARGS; \
   case S_BBANDS: return TA_BBANDS_##VERB ARGS; \
   default:     return TA_MA_##VERB ARGS; \
   }
static TA_RetCode s_OutRange( int s, const void *h, int *beg, int *nb ) { HANDLE_CALL( OutRange, ( h, beg, nb ) ) }
static TA_RetCode s_Clone( int s, const void *h, void **clone ) { HANDLE_CALL( Clone, ( h, (void *)clone ) ) }
static TA_RetCode s_Advance( int s, void *h ) { HANDLE_CALL( Advance, ( h ) ) }
static TA_RetCode s_Close( int s, void *h ) { HANDLE_CALL( Close, ( h ) ) }

/* `failed` counts the calls that did not answer TA_SUCCESS: a shape whose
 * calls refuse compares equal on every thread and hands the sanitizer nothing. */
static void run_stream( int s, StreamAnswer *answer )
{
   void *h = NULL;
   double first[3];
   int t;

   memset( answer, 0, sizeof(*answer) );
   if( s_open( s, &h, PREFIX, first ) != TA_SUCCESS )
   {
      answer->failed = 1;
      return;
   }
   for( t = PREFIX; t < NB_BAR; t++ )
   {
      answer->failed += s_Peek( s, h, t, answer->peek[t] ) != TA_SUCCESS;
      answer->failed += s_Update( s, h, t, answer->update[t] ) != TA_SUCCESS;
   }
   answer->failed += s_Advance( s, h ) != TA_SUCCESS;
   answer->failed += s_OutRange( s, h, &answer->beg, &answer->nb ) != TA_SUCCESS;
   answer->failed += s_Close( s, h ) != TA_SUCCESS;
}

static void run_readers( int s, ReaderAnswer *answer )
{
   void *clone = NULL;

   memset( answer, 0, sizeof(*answer) );
   answer->failed += s_Peek( s, gShared[s], PREFIX, answer->peek ) != TA_SUCCESS;
   answer->failed += s_Value( s, gShared[s], answer->value ) != TA_SUCCESS;
   answer->failed += s_OutRange( s, gShared[s], &answer->beg, &answer->nb ) != TA_SUCCESS;
   if( s_Clone( s, gShared[s], &clone ) != TA_SUCCESS )
   {
      answer->failed++;
      return;
   }
   answer->failed += s_Update( s, clone, PREFIX, answer->cloned ) != TA_SUCCESS;
   answer->failed += s_Close( s, clone ) != TA_SUCCESS;
}

static void run_leg( int leg, int f, Answer *answer )
{
   switch( leg )
   {
   case 0:  run_typed( f, 0, answer ); break;
   case 1:  run_typed( f, 1, answer ); break;
   case 2:  run_abstract( f, answer ); break;
   case 3:  run_opener( f, 0, answer ); break;
   default: run_opener( f, 1, answer ); break;
   }
}

typedef struct { long calls, stream, readers, mismatches; } Tally;

static void *worker( void *arg )
{
   Tally *tally = arg;
   Answer *answer = malloc( sizeof(*answer) );
   StreamAnswer *stream = malloc( sizeof(*stream) );
   ReaderAnswer readers;
   int rep, leg, k;

   if( !answer || !stream )
   {
      tally->calls = -1;
      return NULL;
   }
   for( rep = 0; rep < NB_REP; rep++ )
   {
      for( leg = 0; leg < NB_LEG; leg++ )
         for( k = 0; k < TA_VARIANT_TABLE_SIZE; k++ )
         {
            run_leg( leg, k, answer );
            tally->calls++;
            if( memcmp( answer, &gRef[leg][k], sizeof(*answer) ) != 0 ) tally->mismatches++;
         }
      for( k = 0; k < S_COUNT; k++ )
      {
         run_stream( k, stream );
         tally->stream++;
         if( memcmp( stream, &gStreamRef[k], sizeof(*stream) ) != 0 ) tally->mismatches++;
      }
      for( k = 0; k < S_COUNT; k++ )
      {
         run_readers( k, &readers );
         tally->readers++;
         if( memcmp( &readers, &gReaderRef[k], sizeof(readers) ) != 0 ) tally->mismatches++;
      }
   }
   free( answer );
   free( stream );
   return NULL;
}

/* Two-decimal prices: every value is decided by integer arithmetic. */
static void build_series( void )
{
   unsigned int seed = 12345u;
   int cents = 10037;
   int i;

   for( i = 0; i < NB_BAR; i++ )
   {
      int open, top, bottom;

      seed = seed * 1103515245u + 12345u;
      cents += (int)((seed >> 16) % 201u) - 100;
      if( cents < 500 ) cents = 513;
      seed = seed * 1103515245u + 12345u;
      open = cents + (int)((seed >> 16) % 61u) - 30;
      top = open > cents ? open : cents;
      bottom = open < cents ? open : cents;
      gOpen[i] = open / 100.0;
      gClose[i] = cents / 100.0;
      seed = seed * 1103515245u + 12345u;
      gHigh[i] = ( top + (int)((seed >> 16) % 50u) ) / 100.0;
      seed = seed * 1103515245u + 12345u;
      gLow[i] = ( bottom - (int)((seed >> 16) % 50u) ) / 100.0;
      seed = seed * 1103515245u + 12345u;
      gVolume[i] = 1000.0 + (double)((seed >> 16) % 9000u);
      gPeriods[i] = 2.0 + (double)((seed >> 20) % 29u);
      gOpenF[i] = (float)gOpen[i];
      gHighF[i] = (float)gHigh[i];
      gLowF[i] = (float)gLow[i];
      gCloseF[i] = (float)gClose[i];
      gVolumeF[i] = (float)gVolume[i];
      gPeriodsF[i] = (float)gPeriods[i];
   }
}

int main( void )
{
   pthread_t thread[NB_THREAD];
   Tally tally[NB_THREAD], sum = { 0, 0, 0, 0 };
   double first[3];
   int i, k, leg, refused = 0, empty = 0;

   if( TA_VARIANT_TABLE_SIZE != TA_STREAM_TABLE_SIZE )
   {
      printf( "thread probe: %d batch functions, %d streaming\n",
              TA_VARIANT_TABLE_SIZE, TA_STREAM_TABLE_SIZE );
      return EXIT_VACUOUS;
   }
   build_series();
   if( TA_Initialize() != TA_SUCCESS )
      return EXIT_VACUOUS;
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 3 );

   for( k = 0; k < TA_VARIANT_TABLE_SIZE; k++ )
   {
      const TA_VariantEntry *e = &TA_VariantTable[k];
      if( e->nbInput > MAX_IN || e->nbOptInput > MAX_OPT || e->nbOutput > MAX_OUT ||
          strcmp( e->name, TA_StreamTable[k].name ) != 0 )
      {
         printf( "thread probe: TA_%s does not fit the probe's fixtures\n", e->name );
         return EXIT_VACUOUS;
      }
   }
   for( leg = 0; leg < NB_LEG; leg++ )
   {
      gRef[leg] = malloc( sizeof(Answer) * TA_VARIANT_TABLE_SIZE );
      if( !gRef[leg] )
         return EXIT_VACUOUS;
      for( k = 0; k < TA_VARIANT_TABLE_SIZE; k++ )
      {
         run_leg( leg, k, &gRef[leg][k] );
         refused += gRef[leg][k].rc != TA_SUCCESS;
         empty += leg != 3 && gRef[leg][k].nb <= 0;
      }
   }
   for( k = 0; k < S_COUNT; k++ )
   {
      run_stream( k, &gStreamRef[k] );
      refused += gStreamRef[k].failed;
      if( s_open( k, &gShared[k], PREFIX, first ) != TA_SUCCESS )
      {
         printf( "thread probe: stream shape %d did not open\n", k );
         return EXIT_VACUOUS;
      }
   }
   for( k = 0; k < S_COUNT; k++ )
   {
      run_readers( k, &gReaderRef[k] );
      refused += gReaderRef[k].failed;
   }
   if( refused != 0 || empty != 0 )
   {
      printf( "thread probe: on one thread, %d call(s) refused and %d produced no value\n",
              refused, empty );
      return EXIT_VACUOUS;
   }

   memset( tally, 0, sizeof(tally) );
   for( i = 0; i < NB_THREAD; i++ )
      if( pthread_create( &thread[i], NULL, worker, &tally[i] ) != 0 )
         return EXIT_VACUOUS;
   for( i = 0; i < NB_THREAD; i++ )
   {
      pthread_join( thread[i], NULL );
      if( tally[i].calls < 0 )
         return EXIT_VACUOUS;
      sum.calls += tally[i].calls;
      sum.stream += tally[i].stream;
      sum.readers += tally[i].readers;
      sum.mismatches += tally[i].mismatches;
   }
   for( k = 0; k < S_COUNT; k++ )
      s_Close( k, gShared[k] );
   TA_Shutdown();

   printf( "functions=%d shapes=%d threads=%d calls=%ld stream=%ld readers=%ld mismatches=%ld\n",
           TA_VARIANT_TABLE_SIZE, (int)S_COUNT, NB_THREAD,
           sum.calls, sum.stream, sum.readers, sum.mismatches );
   if( sum.mismatches != 0 )
      return EXIT_MISMATCH;
   if( sum.calls != (long)NB_THREAD * NB_REP * NB_LEG * TA_VARIANT_TABLE_SIZE ||
       sum.stream != (long)NB_THREAD * NB_REP * S_COUNT ||
       sum.readers != (long)NB_THREAD * NB_REP * S_COUNT )
      return EXIT_VACUOUS;
   return 0;
}
