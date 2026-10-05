/* Probe for the Auto warm-up design (docs/auto-warm-up-design.md). Not part of the library.
 *
 * For every function reachable through ta_abstract, compute the outputs over a
 * series fed from bar 0 and over the same series fed from bar D, and measure how
 * the difference at an aligned bar shrinks with the bar's age, where
 *    age = bar - (D + lookback)
 * is the number of outputs the later-started run has already produced.
 *
 * One TSV row per (shape, function, config, output, D).
 *   S   = largest difference seen on this output (in output units)
 *   SF  = largest S over the function's outputs: the seed discrepancy
 *   R   = max - min of the from-bar-0 output over the compared bars
 *   A<K> = first age from which the difference stays <= e^-K * SF (seed-relative)
 *   C<d> = first age from which the two runs agree to d significant digits of the value:
 *          difference <= 10^-d * max(|a|, |b|)
 *   B<K> = first age from which the difference stays <= e^-K * R  (scale-relative)
 *   Z   = first age from which the two runs are bit-identical (-1: never)
 *   tail = largest difference over the last tenth of the ages, divided by S
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ta_libc.h"

#define MAXOUT 8
#define MAXOPT 16
#define NK 7
static const int KS[NK] = { 7, 10, 14, 16, 19, 21, 28 };

static int N;
static double *O, *H, *L, *C, *V, *OI, *P;
static const char *g_shape;
static const char *g_only;

static unsigned long long rng_s = 0x9E3779B97F4A7C15ULL;
static double rnd(void)
{
   rng_s ^= rng_s >> 12; rng_s ^= rng_s << 25; rng_s ^= rng_s >> 27;
   return (double)((rng_s * 2685821657736338717ULL) >> 11) / 9007199254740992.0;
}
static double gauss(void)
{
   double u = rnd(), v = rnd();
   if( u < 1e-300 ) u = 1e-300;
   return sqrt(-2.0*log(u)) * cos(6.283185307179586*v);
}
static double r2(double x) { return floor(x*100.0 + 0.5) / 100.0; }

static void make_series(const char *shape)
{
   int i;
   double c = 100.0, prev = 100.0;
   for( i = 0; i < N; i++ )
   {
      double o, h, l;
      if( !strcmp(shape, "rw") )
         c = c * exp(0.0002 + 0.012*gauss());
      else if( !strcmp(shape, "zz") )
         c = 100.0 + 5.0*sin(i*6.283185307179586/3000.0) + ((i & 1) ? 0.5 : -0.5) + 0.03*gauss();
      else /* tr: 5000 bars up, 5000 bars down, low noise */
         c = c * exp( (((i/5000) & 1) ? -0.0008 : 0.0008) + 0.0005*gauss() );
      if( c < 1.0 ) c = 1.0;
      o = prev * (1.0 + 0.002*gauss());
      h = (o > c ? o : c) * (1.0 + fabs(0.004*gauss()));
      l = (o < c ? o : c) * (1.0 - fabs(0.004*gauss()));
      O[i] = r2(o); C[i] = r2(c);
      H[i] = r2(h); L[i] = r2(l);
      if( H[i] < O[i] ) H[i] = O[i];
      if( H[i] < C[i] ) H[i] = C[i];
      if( L[i] > O[i] ) L[i] = O[i];
      if( L[i] > C[i] ) L[i] = C[i];
      V[i] = floor(1.0e6 * (0.5 + rnd()));
      OI[i] = floor(1.0e5 * (0.5 + rnd()));
      P[i] = 2.0 + (double)((i*7) % 29);
      prev = c;
   }
}

typedef struct
{
   int beg, nb, lookback;
   double *r[MAXOUT];
   int    *n[MAXOUT];
} Res;

typedef struct
{
   int kind;       /* 0 default, 1 periods x3, 2 MA type */
   int matype;
} Cfg;

static Res g_full, g_cut;
static double g_sf;

static TA_RetCode run(const TA_FuncInfo *fi, const Cfg *cfg, int D, Res *res, char *desc, size_t descsz)
{
   TA_ParamHolder *ph;
   TA_RetCode rc;
   unsigned int i;
   int realSeen = 0, applied = 0;
   size_t used = 0;

   desc[0] = 0;
   rc = TA_ParamHolderAlloc(fi->handle, &ph);
   if( rc != TA_SUCCESS ) return rc;

   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo(fi->handle, i, &ii);
      if( ii->type == TA_Input_Price )
         rc = TA_SetInputParamPricePtr(ph, i, O+D, H+D, L+D, C+D, V+D, OI+D);
      else if( ii->type == TA_Input_Real )
      {
         const double *src = (realSeen == 0) ? C : H;
         if( !strcmp(fi->name, "MAVP") && realSeen == 1 ) src = P;
         rc = TA_SetInputParamRealPtr(ph, i, src + D);
         realSeen++;
      }
      else
         rc = TA_BAD_PARAM;
      if( rc != TA_SUCCESS ) { TA_ParamHolderFree(ph); return rc; }
   }

   for( i = 0; i < fi->nbOptInput && i < MAXOPT; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo(fi->handle, i, &oi);
      {  /* OVR="Name=value,Name=value" in the environment overrides parameters of the default set */
         const char *q = getenv("OVR");
         while( q && *q && cfg->kind == 0 )
         {
            char key[64]; double val; int n = 0;
            if( sscanf(q, "%63[^=]=%lf%n", key, &val, &n) < 2 ) break;
            if( strstr(oi->paramName, key) )
            {
               if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList ) TA_SetOptInputParamReal(ph, i, val);
               else TA_SetOptInputParamInteger(ph, i, (TA_Integer)val);
            }
            q += n; if( *q == ',' ) q++;
         }
      }
      if( cfg->kind == 1 && oi->type == TA_OptInput_IntegerRange && strstr(oi->paramName, "Period") )
      {
         const TA_IntegerRange *rg = (const TA_IntegerRange *)oi->dataSet;
         long v = (long)oi->defaultValue * 3;
         if( v > rg->max ) v = rg->max;
         if( v < rg->min ) v = rg->min;
         rc = TA_SetOptInputParamInteger(ph, i, (TA_Integer)v);
         if( rc != TA_SUCCESS ) { TA_ParamHolderFree(ph); return rc; }
         {
            int w = snprintf(desc+used, descsz-used, "%s%s=%ld", used ? "," : "", oi->paramName+5, v);
            if( w > 0 ) used = ((size_t)w < descsz-used) ? used+(size_t)w : descsz-1;
         }
         applied++;
      }
      else if( cfg->kind == 2 && oi->type == TA_OptInput_IntegerList && strstr(oi->paramName, "MAType") )
      {
         rc = TA_SetOptInputParamInteger(ph, i, cfg->matype);
         if( rc != TA_SUCCESS ) { TA_ParamHolderFree(ph); return rc; }
         applied++;
      }
   }
   if( cfg->kind != 0 && !applied ) { TA_ParamHolderFree(ph); return TA_NOT_SUPPORTED; }
   if( cfg->kind == 2 ) snprintf(desc, descsz, "matype=%d", cfg->matype);
   if( cfg->kind == 0 ) snprintf(desc, descsz, "defaults");

   for( i = 0; i < fi->nbOutput && i < MAXOUT; i++ )
   {
      const TA_OutputParameterInfo *oo;
      TA_GetOutputParameterInfo(fi->handle, i, &oo);
      if( oo->type == TA_Output_Real )
         rc = TA_SetOutputParamRealPtr(ph, i, res->r[i]);
      else
         rc = TA_SetOutputParamIntegerPtr(ph, i, res->n[i]);
      if( rc != TA_SUCCESS ) { TA_ParamHolderFree(ph); return rc; }
   }

   rc = TA_GetLookback(ph, &res->lookback);
   if( rc == TA_SUCCESS )
      rc = TA_CallFunc(ph, 0, N - D - 1, &res->beg, &res->nb);
   TA_ParamHolderFree(ph);
   return rc;
}

static double vdiff(double a, double b)
{
   if( isnan(a) && isnan(b) ) return 0.0;
   if( isnan(a) || isnan(b) ) return HUGE_VAL;
   if( isinf(a) || isinf(b) ) return (a == b) ? 0.0 : HUGE_VAL;
   return fabs(a - b);
}

static void each(const TA_FuncInfo *fi, void *opaque)
{
   static const int DS[] = { 1, 2, 5, 33, 250, 1000 };
   Cfg cfgs[2 + 16];
   int ncfg = 0, c, k, d, isIndex;
   unsigned int o;
   char desc[256], desc2[256];
   (void)opaque;

   if( g_only && strcmp(g_only, fi->name) ) return;
   isIndex = strstr(fi->name, "INDEX") != NULL;

   cfgs[ncfg].kind = 0; cfgs[ncfg++].matype = 0;
   cfgs[ncfg].kind = 1; cfgs[ncfg++].matype = 0;
   for( k = 0; k < 16; k++ ) { cfgs[ncfg].kind = 2; cfgs[ncfg++].matype = k; }

   for( c = 0; c < ncfg; c++ )
   {
      TA_RetCode rc = run(fi, &cfgs[c], 0, &g_full, desc, sizeof desc);
      if( rc == TA_NOT_SUPPORTED ) continue;      /* config does not apply to this function */
      if( rc != TA_SUCCESS )
      {
         if( cfgs[c].kind != 2 )
            printf("#ERR\t%s\t%s\t%s\trc=%d\n", g_shape, fi->name, desc, (int)rc);
         continue;
      }
      for( d = 0; d < (int)(sizeof DS / sizeof DS[0]); d++ )
      {
         int D = DS[d], first, last;
         rc = run(fi, &cfgs[c], D, &g_cut, desc2, sizeof desc2);
         if( rc != TA_SUCCESS ) { printf("#ERR\t%s\t%s\t%s\tD=%d rc=%d\n", g_shape, fi->name, desc, D, (int)rc); continue; }
         first = D + g_cut.beg;                    /* absolute bar of the later run's first output */
         if( first < g_full.beg ) first = g_full.beg;
         last = g_full.beg + g_full.nb - 1;
         if( D + g_cut.beg + g_cut.nb - 1 < last ) last = D + g_cut.beg + g_cut.nb - 1;
         if( last - first < 100 ) continue;
         {
            double SF = 0.0;
            int t2;
            for( o = 0; o < fi->nbOutput && o < MAXOUT; o++ )
            {
               const TA_OutputParameterInfo *oo;
               TA_GetOutputParameterInfo(fi->handle, o, &oo);
               for( t2 = first; t2 <= last; t2++ )
               {
                  double a, b, df;
                  if( oo->type == TA_Output_Real )
                  {  a = g_full.r[o][t2 - g_full.beg]; b = g_cut.r[o][t2 - D - g_cut.beg]; }
                  else
                  {  a = g_full.n[o][t2 - g_full.beg]; b = g_cut.n[o][t2 - D - g_cut.beg] + (isIndex ? D : 0); }
                  df = vdiff(a, b);
                  if( df > SF ) SF = df;
               }
            }
            g_sf = SF;
         }
         for( o = 0; o < fi->nbOutput && o < MAXOUT; o++ )
         {
            const TA_OutputParameterInfo *oo;
            double S = 0.0, lo = HUGE_VAL, hi = -HUGE_VAL, R, tail = 0.0;
            int A[NK], B[NK], Z = 0, t, n = last - first + 1, ageS = 0, C4 = 0, C6 = 0, C8 = 0;
            TA_GetOutputParameterInfo(fi->handle, o, &oo);
            for( k = 0; k < NK; k++ ) { A[k] = 0; B[k] = 0; }
            for( t = first; t <= last; t++ )
            {
               double a, b, df;
               if( oo->type == TA_Output_Real )
               {  a = g_full.r[o][t - g_full.beg]; b = g_cut.r[o][t - D - g_cut.beg]; }
               else
               {  a = g_full.n[o][t - g_full.beg]; b = g_cut.n[o][t - D - g_cut.beg] + (isIndex ? D : 0); }
               df = vdiff(a, b);
               if( df > S ) { S = df; ageS = t - first; }
               if( isfinite(a) ) { if( a < lo ) lo = a; if( a > hi ) hi = a; }
            }
            R = (hi > lo) ? hi - lo : 0.0;
            for( t = first; t <= last; t++ )
            {
               double a, b, df;
               int age = t - first;
               if( oo->type == TA_Output_Real )
               {  a = g_full.r[o][t - g_full.beg]; b = g_cut.r[o][t - D - g_cut.beg]; }
               else
               {  a = g_full.n[o][t - g_full.beg]; b = g_cut.n[o][t - D - g_cut.beg] + (isIndex ? D : 0); }
               df = vdiff(a, b);
               if( df != 0.0 ) Z = age + 1;
               { double mag = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
                 if( df > 1e-4 * mag ) C4 = age + 1;
                 if( df > 1e-6 * mag ) C6 = age + 1;
                 if( df > 1e-8 * mag ) C8 = age + 1; }
               for( k = 0; k < NK; k++ )
               {
                  double th = exp(-(double)KS[k]);
                  if( df > th * g_sf ) A[k] = age + 1;
                  if( df > th * R ) B[k] = age + 1;
               }
               if( age >= n - n/10 && S > 0.0 && df / S > tail ) tail = df / S;
            }
            if( Z >= n ) Z = -1;
            printf("%s\t%s\t%08x\t%d\t%s\t%u\t%s\t%s\t%d\t%d\t%.6g\t%d\t%.6g\t%.6g",
                   g_shape, fi->name, (unsigned)fi->flags, cfgs[c].kind, desc, o, oo->paramName,
                   oo->type == TA_Output_Real ? "real" : "int", g_cut.lookback, D, S, ageS, R, g_sf);
            for( k = 0; k < NK; k++ ) printf("\t%d", A[k] >= n ? -1 : A[k]);
            for( k = 0; k < NK; k++ ) printf("\t%d", B[k] >= n ? -1 : B[k]);
            printf("\t%d\t%.3g\t%d\t%d\t%d\t%d\n", Z, tail, n, C4 >= n ? -1 : C4, C6 >= n ? -1 : C6, C8 >= n ? -1 : C8);
         }
      }
   }
}

int main(int argc, char **argv)
{
   static const char *shapes[] = { "rw", "zz", "tr" };
   int s, i;
   N = (argc > 1) ? atoi(argv[1]) : 40000;
   g_only = (argc > 2) ? argv[2] : NULL;
   O = malloc(sizeof(double)*N); H = malloc(sizeof(double)*N); L = malloc(sizeof(double)*N);
   C = malloc(sizeof(double)*N); V = malloc(sizeof(double)*N); OI = malloc(sizeof(double)*N);
   P = malloc(sizeof(double)*N);
   for( i = 0; i < MAXOUT; i++ )
   {
      g_full.r[i] = malloc(sizeof(double)*N); g_full.n[i] = malloc(sizeof(int)*N);
      g_cut.r[i]  = malloc(sizeof(double)*N); g_cut.n[i]  = malloc(sizeof(int)*N);
   }
   if( TA_Initialize() != TA_SUCCESS ) return 2;
   printf("shape\tfunc\tflags\tcfgkind\tcfg\tout\toutname\ttype\tlookback\tD\tS\tageS\tR\tSF\tA7\tA10\tA14\tA16\tA19\tA21\tA28\tB7\tB10\tB14\tB16\tB19\tB21\tB28\tZ\ttail\tn\tC4\tC6\tC8\n");
   for( s = 0; s < 3; s++ )
   {
      g_shape = shapes[s];
      rng_s = 0x9E3779B97F4A7C15ULL + (unsigned long long)s;
      make_series(g_shape);
      TA_ForEachFunc(each, NULL);
   }
   TA_Shutdown();
   return 0;
}
