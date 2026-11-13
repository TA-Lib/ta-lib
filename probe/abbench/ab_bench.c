/* Throwaway A/B timer: one shipped TA function by name through the abstract
 * API, default parameters, synthetic OHLCV. Built once per library (build.sh).
 * usage: ab_bench spin <secs> | ab_bench time <FUNC> <n> <maxreps>
 */
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1   /* struct rusage's ru_minflt */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <pthread.h>
#include <pthread/qos.h>
#endif
#include "ta_libc.h"

static double nowNs( void )
{
   struct timespec ts;
   clock_gettime( CLOCK_MONOTONIC, &ts );
   return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
static long minflt( void )
{
   struct rusage ru;
   getrusage( RUSAGE_SELF, &ru );
   return ru.ru_minflt;
}

static unsigned rng = 12345u;
static double urand( void ) { rng = rng * 1103515245u + 12345u; return ((rng >> 8) & 0xFFFF) / 65536.0; }
static double r2( double x ) { return (double)(long long)(x * 100.0 + (x >= 0 ? 0.5 : -0.5)) / 100.0; }

static double *walk( int n, double start )
{
   double *a = malloc( sizeof(double) * (size_t)n ), v = start;
   int i;
   for( i = 0; i < n; i++ ) { v += (urand() - 0.5) * 2.0; if( v < 20 ) v = 20; a[i] = r2( v ); }
   return a;
}

static uint64_t fnv( uint64_t h, const void *p, size_t len )
{
   const unsigned char *b = p;
   size_t i;
   for( i = 0; i < len; i++ ) { h ^= b[i]; h *= 1099511628211ULL; }
   return h;
}

#define MAXOUT 16

static int runTime( const char *name, int n, int maxreps )
{
   const TA_FuncHandle *h;
   const TA_FuncInfo *fi;
   TA_ParamHolder *p;
   void *outs[MAXOUT];
   int outIsReal[MAXOUT];
   double *tNs, lo1, lo2, tol = 0.01, warmUntil, t0;
   volatile double sink = 0.0;
   const char *tolEnv = getenv( "AB_TOL" ), *warmEnv = getenv( "AB_WARM_MS" );
   TA_Integer beg = 0, nb = 0;
   TA_RetCode rc = TA_SUCCESS;
   long faults = 0, f0;
   int i, r, j, inner, used = 0, nreal = 0;
   unsigned k;
   uint64_t hash;

   if( n < 1 || maxreps < 2 ) { fprintf( stderr, "time: bad arguments\n" ); return 2; }
   if( TA_GetFuncHandle( name, &h ) || TA_GetFuncInfo( h, &fi ) ) { fprintf( stderr, "no func %s\n", name ); return 2; }
   if( fi->nbOutput > MAXOUT ) { fprintf( stderr, "too many outputs\n" ); return 2; }

   double *o = malloc( sizeof(double) * (size_t)n ), *hi = malloc( sizeof(double) * (size_t)n ),
          *lo = malloc( sizeof(double) * (size_t)n ), *c = walk( n, 100.0 ), *vol = malloc( sizeof(double) * (size_t)n ),
          *oi = malloc( sizeof(double) * (size_t)n ), *c2 = walk( n, 100.0 ), *per = malloc( sizeof(double) * (size_t)n );
   int *iper = malloc( sizeof(int) * (size_t)n );
   for( i = 0; i < n; i++ )
   {
      o[i] = i ? c[i - 1] : c[0];
      double mx = o[i] > c[i] ? o[i] : c[i], mn = o[i] < c[i] ? o[i] : c[i];
      hi[i] = r2( mx + urand() * 1.5 ); lo[i] = r2( mn - urand() * 1.5 );
      vol[i] = r2( 1000.0 + urand() * 9000.0 ); oi[i] = r2( 5000.0 + urand() * 100.0 );
      per[i] = 10 + (i % 21); iper[i] = 10 + (i % 21);
   }

   if( TA_ParamHolderAlloc( h, &p ) ) { fprintf( stderr, "alloc\n" ); return 2; }
   for( k = 0; k < fi->nbInput; k++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo( h, k, &ii );
      if( ii->type == TA_Input_Price ) rc = TA_SetInputParamPricePtr( p, k, o, hi, lo, c, vol, oi );
      else if( ii->type == TA_Input_Integer ) rc = TA_SetInputParamIntegerPtr( p, k, iper );
      else if( strstr( ii->paramName, "eriod" ) ) rc = TA_SetInputParamRealPtr( p, k, per );
      else rc = TA_SetInputParamRealPtr( p, k, nreal++ == 0 ? c : c2 );
      if( rc ) { fprintf( stderr, "setin %u rc=%d\n", k, (int)rc ); return 2; }
   }
   for( k = 0; k < fi->nbOutput; k++ )
   {
      const TA_OutputParameterInfo *oinf;
      TA_GetOutputParameterInfo( h, k, &oinf );
      outIsReal[k] = oinf->type == TA_Output_Real;
      if( outIsReal[k] )
      {
         double *b = malloc( sizeof(double) * (size_t)n ); memset( b, 0, sizeof(double) * (size_t)n );
         rc = TA_SetOutputParamRealPtr( p, k, b ); outs[k] = b;
      }
      else
      {
         int *b = malloc( sizeof(int) * (size_t)n ); memset( b, 0, sizeof(int) * (size_t)n );
         rc = TA_SetOutputParamIntegerPtr( p, k, b ); outs[k] = b;
      }
      if( rc ) { fprintf( stderr, "setout %u rc=%d\n", k, (int)rc ); return 2; }
   }

   inner = 2000000 / n;
   if( inner < 1 )
      inner = 1;
   if( tolEnv && atof( tolEnv ) > 0.0 )
      tol = atof( tolEnv );
   tNs = malloc( sizeof(double) * (size_t)maxreps );
   /* Warm-up by time (a process not yet at speed agrees with itself too),
    * then stop once the two lowest reps agree within tol, or at maxreps. */
   warmUntil = nowNs() + 1e6 * (warmEnv ? atof( warmEnv ) : 300.0);
   for( r = -1; r < maxreps; r++ )
   {
      f0 = minflt();
      t0 = nowNs();
      for( j = 0; j < inner; j++ )
      {
         nb = 0;
         rc = TA_CallFunc( p, 0, n - 1, &beg, &nb );
         if( nb > 0 )
            sink += outIsReal[0] ? ((double *)outs[0])[nb - 1] : ((int *)outs[0])[nb - 1];
      }
      if( r < 0 )
      {
         if( nowNs() < warmUntil )
            r = -2;
         continue;
      }
      tNs[r] = (nowNs() - t0) / inner;
      faults += minflt() - f0;
      used = r + 1;
      if( used < 2 )
         continue;
      lo1 = lo2 = 1e300;
      for( j = 0; j < used; j++ )
      {
         if( tNs[j] < lo1 ) { lo2 = lo1; lo1 = tNs[j]; }
         else if( tNs[j] < lo2 ) lo2 = tNs[j];
      }
      if( lo2 <= lo1 * (1.0 + tol) )
         break;
   }
   lo1 = lo2 = 1e300;
   for( j = 0; j < used; j++ )
   {
      if( tNs[j] < lo1 ) { lo2 = lo1; lo1 = tNs[j]; }
      else if( tNs[j] < lo2 ) lo2 = tNs[j];
   }

   hash = 14695981039346656037ULL;
   hash = fnv( hash, &beg, sizeof beg );
   hash = fnv( hash, &nb, sizeof nb );
   for( k = 0; k < fi->nbOutput; k++ )
      if( nb > 0 )
         hash = fnv( hash, outs[k], (size_t)nb * (outIsReal[k] ? sizeof(double) : sizeof(int)) );

   printf( "%s %d min=%.0f second=%.0f reps=%d/%d faults=%.2f hash=%016llx rc=%d nb=%d\n",
           name, n, lo1, lo2, used, maxreps, (double)faults / ((double)used * inner),
           (unsigned long long)hash, (int)rc, (int)nb );
   if( sink == 12345.678 )
      fprintf( stderr, "sink\n" );
   TA_ParamHolderFree( p );
   for( k = 0; k < fi->nbOutput; k++ )
      free( outs[k] );
   free( o ); free( hi ); free( lo ); free( c ); free( vol ); free( oi ); free( c2 ); free( per ); free( iper ); free( tNs );
   return 0;
}

int main( int argc, char **argv )
{
   if( TA_Initialize() != TA_SUCCESS )
      return 2;
#ifdef __APPLE__
   /* No affinity on Apple Silicon: the QoS class is what keeps the thread off the E-cores. */
   pthread_set_qos_class_self_np( QOS_CLASS_USER_INITIATED, 0 );
#endif
   if( argc >= 3 && strcmp( argv[1], "spin" ) == 0 )
   {
      double until = nowNs() + 1e9 * atof( argv[2] );
      volatile double x = 0.0;
      while( nowNs() < until )
         x += 1.0;
      return 0;
   }
   if( argc >= 5 && strcmp( argv[1], "time" ) == 0 )
      return runTime( argv[2], atoi( argv[3] ), atoi( argv[4] ) );
   fprintf( stderr, "usage: ab_bench spin <secs> | ab_bench time <FUNC> <n> <maxreps>\n" );
   return 2;
}
