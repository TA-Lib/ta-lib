/* Census for the Auto-Stabilization rules. Not part of the library.
 *
 * For every function that owns an unstable id: many random series, each run
 * from bar 0 and from a later bar D with every id at 0, and the age from which
 * the two runs stay within e^-K of their largest difference (the need), set
 * against the count the library discards at each level. The probe measures
 * three fixed series; this counts how often a rule is passed, which is how a
 * step in the recursion shows.
 *
 *    census <trials> <seed> [NAME [drift [level tick]]]   one TSV row per (function, parameters, series kind)
 *    census list                       the functions it would run
 *
 * Series kinds: rw, a random walk; tr, a walk whose drift (0.001 per bar unless
 * given) flips at random intervals; rb, a range-bound walk pulled back to its
 * mean. tr starts at 10000 so that a long fall does not rest on the 1.00 floor:
 * a flat run is a limit of its own, and it would be counted as a rule's.
 * With CENSUS_CSV=<file> a fourth kind, csv, takes each trial's bars from a
 * random offset of that file (a header line, then date,high,low,close; the open
 * is the previous close). Its trials overlap, so they are not independent.
 * Prices are near `level` (100 unless given) and rounded to `tick` (0.01): the
 * same walk at 4.5e-8 with a tick of 1e-10 is a coin quoted in satoshis.
 * need<K> p50/p99/max over the trials; over<K> = trials whose need is above the
 * count; never = trials still above e^-7 at the last compared age.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ta_libc.h"

#define MAXOUT 8
#define NMAX 40000
#define NKIND 3
#define NCFG 3

static double O[NMAX], H[NMAX], L[NMAX], C[NMAX], V[NMAX], OI[NMAX], P[NMAX];
static double RA[MAXOUT][NMAX], RB[MAXOUT][NMAX];
static int    IA[MAXOUT][NMAX], IB[MAXOUT][NMAX];
static const char *g_only;
static int g_trials, g_list;
static double g_drift = 0.001, g_level = 100.0, g_tick = 0.01;
static unsigned long long g_seed, rng_s;
static double fH[NMAX], fL[NMAX], fC[NMAX];
static int fN;

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
static double r2(double x) { return floor(x*(g_level/100.0)/g_tick + 0.5) * g_tick; }

static void make_series(int kind, int n)
{
   int i, off = kind == 3 ? (int)(rnd()*(fN - n + 1)) : 0;
   double c = kind == 1 ? 10000.0 : 100.0, prev = c, drift = g_drift, vol = 1.0e6;
   if( kind == 3 ) prev = fC[off > 0 ? off - 1 : 0];
   for( i = 0; i < n; i++ )
   {
      double o, h, l;
      if( kind == 3 ) c = fC[off+i];
      else if( kind == 0 ) c *= exp(0.012*gauss());
      else if( kind == 1 )
      {
         if( rnd() < 1.0/300.0 ) drift = -drift;
         c *= exp(drift + 0.012*gauss());
      }
      else c += 0.05*(100.0 - c) + 0.5*gauss();
      if( kind == 3 ) { o = prev; h = fH[off+i]; l = fL[off+i]; }
      else
      {
         if( c < 1.0 ) c = 1.0;
         o = prev * (1.0 + 0.002*gauss());
         h = (o > c ? o : c) * (1.0 + fabs(0.004*gauss()));
         l = (o < c ? o : c) * (1.0 - fabs(0.004*gauss()));
      }
      O[i] = r2(o); C[i] = r2(c); H[i] = r2(h); L[i] = r2(l);
      if( H[i] < O[i] ) H[i] = O[i];
      if( H[i] < C[i] ) H[i] = C[i];
      if( L[i] > O[i] ) L[i] = O[i];
      if( L[i] > C[i] ) L[i] = C[i];
      vol *= exp(0.02*(13.8 - log(vol)) + 0.2*gauss());
      V[i] = floor(vol);
      OI[i] = floor(1.0e5 * (0.5 + rnd()));
      P[i] = 2.0 + (double)((i*7) % 29);
      prev = c;
   }
}

/* cfg: 0 defaults, 1 every integer period tripled, 2 every integer period at its minimum. */
static TA_RetCode call(const TA_FuncInfo *fi, int cfg, int D, int n, int useB,
                       int *beg, int *nb, int *lookback, int *autoCount, char *desc, size_t descsz)
{
   TA_ParamHolder *ph;
   TA_RetCode rc;
   unsigned int i;
   int realSeen = 0, applied = 0;
   size_t used = 0;

   desc[0] = 0;
   rc = TA_ParamHolderAlloc(fi->handle, &ph);
   if( rc != TA_SUCCESS ) return rc;
   for( i = 0; i < fi->nbInput && rc == TA_SUCCESS; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo(fi->handle, i, &ii);
      if( ii->type == TA_Input_Price )
         rc = TA_SetInputParamPricePtr(ph, i, O+D, H+D, L+D, C+D, V+D, OI+D);
      else if( ii->type == TA_Input_Real )
      {
         rc = TA_SetInputParamRealPtr(ph, i, (realSeen == 0 ? C : H) + D);
         realSeen++;
      }
      else rc = TA_BAD_PARAM;
   }
   for( i = 0; i < fi->nbOptInput && rc == TA_SUCCESS; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo(fi->handle, i, &oi);
      if( cfg != 0 && oi->type == TA_OptInput_IntegerRange && strstr(oi->paramName, "Period") )
      {
         const TA_IntegerRange *rg = (const TA_IntegerRange *)oi->dataSet;
         long v = cfg == 1 ? (long)oi->defaultValue * 3 : rg->min;
         int w;
         if( v > rg->max ) v = rg->max;
         rc = TA_SetOptInputParamInteger(ph, i, (TA_Integer)v);
         w = snprintf(desc+used, descsz-used, "%s%s=%ld", used ? "," : "", oi->paramName+5, v);
         if( w > 0 ) used = ((size_t)w < descsz-used) ? used+(size_t)w : descsz-1;
         applied++;
      }
   }
   if( rc == TA_SUCCESS && cfg != 0 && !applied ) rc = TA_NOT_SUPPORTED;
   if( cfg == 0 ) snprintf(desc, descsz, "defaults");
   for( i = 0; i < fi->nbOutput && i < MAXOUT && rc == TA_SUCCESS; i++ )
   {
      const TA_OutputParameterInfo *oo;
      TA_GetOutputParameterInfo(fi->handle, i, &oo);
      if( oo->type == TA_Output_Real )
         rc = TA_SetOutputParamRealPtr(ph, i, useB ? RB[i] : RA[i]);
      else
         rc = TA_SetOutputParamIntegerPtr(ph, i, useB ? IB[i] : IA[i]);
   }
   if( rc == TA_SUCCESS ) rc = TA_GetLookback(ph, lookback);
   for( i = 0; i < 2 && rc == TA_SUCCESS && autoCount; i++ )
   {
      TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, i ? TA_UNSTABLE_AUTO_PREC_8 : TA_UNSTABLE_AUTO_PREC_4);
      rc = TA_GetLookback(ph, &autoCount[i]);
      autoCount[i] -= *lookback;
      TA_SetUnstablePeriod(TA_FUNC_UNST_ALL, 0);
   }
   if( rc == TA_SUCCESS && n > 0 ) rc = TA_CallFunc(ph, 0, n - D - 1, beg, nb);
   TA_ParamHolderFree(ph);
   return rc;
}

static int cmpInt(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

static void each(const TA_FuncInfo *fi, void *opaque)
{
   static const int KS[4] = { 7, 10, 16, 19 };
   int cfg, kind, t, k;
   unsigned int o;
   int *need[2];
   char desc[256];
   (void)opaque;

   if( !(fi->flags & TA_FUNC_FLG_UNST_PER) ) return;
   if( g_only && strcmp(g_only, fi->name) ) return;
   if( g_list ) { printf("%s\n", fi->name); return; }
   need[0] = malloc(sizeof(int)*g_trials); need[1] = malloc(sizeof(int)*g_trials);

   for( cfg = 0; cfg < NCFG; cfg++ )
   {
      int lookback, autoCount[2], beg, nb, M;
      if( call(fi, cfg, 0, 0, 0, &beg, &nb, &lookback, autoCount, desc, sizeof desc) != TA_SUCCESS ) continue;
      if( autoCount[0] <= 0 ) continue;
      M = 3*autoCount[1];
      if( M < 300 ) M = 300;
      for( kind = 0; kind < NKIND + (fN > 0); kind++ )
      {
         static const char *kinds[NKIND+1] = { "rw", "tr", "rb", "csv" };
         long over[4] = {0,0,0,0}, never = 0, live = 0;
         int worstT[2] = {-1,-1}, worst[2] = {0,0};
         for( t = 0; t < g_trials; t++ )
         {
            int D, n, begB, nbB, lb2, first, last, age, nd[4] = {0,0,0,0}, ok;
            double SF = 0.0, R[MAXOUT];
            char d2[256];
            need[0][t] = 0; need[1][t] = 0;
            rng_s = g_seed*0x9E3779B97F4A7C15ULL + (unsigned long long)t*1000003ULL + (unsigned long long)(kind*31 + cfg*7 + 1);
            for( k = 0; k < 8; k++ ) rnd();
            D = 50 + (int)(rnd()*autoCount[1]);
            n = D + lookback + M;
            if( n > NMAX ) n = NMAX;
            if( kind == 3 && n > fN ) n = fN;
            make_series(kind, n);
            ok = call(fi, cfg, 0, n, 0, &beg, &nb, &lb2, NULL, d2, sizeof d2) == TA_SUCCESS
              && call(fi, cfg, D, n, 1, &begB, &nbB, &lb2, NULL, d2, sizeof d2) == TA_SUCCESS;
            if( !ok ) continue;
            first = D + begB; if( first < beg ) first = beg;
            last = beg + nb - 1;
            if( last - first < 50 ) continue;
            for( o = 0; o < fi->nbOutput && o < MAXOUT; o++ )
            {
               const TA_OutputParameterInfo *oo;
               double lo = HUGE_VAL, hi = -HUGE_VAL;
               TA_GetOutputParameterInfo(fi->handle, o, &oo);
               for( age = 0; age <= last - first; age++ )
               {
                  double a = oo->type == TA_Output_Real ? RA[o][first+age-beg] : IA[o][first+age-beg];
                  double b = oo->type == TA_Output_Real ? RB[o][first+age-D-begB] : IB[o][first+age-D-begB];
                  double df = fabs(a - b);
                  if( df > SF ) SF = df;
                  if( a < lo ) lo = a;
                  if( a > hi ) hi = a;
               }
               R[o] = hi > lo ? hi - lo : 0.0;
            }
            if( !(SF > 0.0) ) continue;
            live++;
            for( o = 0; o < fi->nbOutput && o < MAXOUT; o++ )
            {
               const TA_OutputParameterInfo *oo;
               TA_GetOutputParameterInfo(fi->handle, o, &oo);
               for( age = 0; age <= last - first; age++ )
               {
                  double a = oo->type == TA_Output_Real ? RA[o][first+age-beg] : IA[o][first+age-beg];
                  double b = oo->type == TA_Output_Real ? RB[o][first+age-D-begB] : IB[o][first+age-D-begB];
                  double df = fabs(a - b);
                  if( df <= 1e-10*R[o] ) continue;        /* rounding, not the seed */
                  for( k = 0; k < 4; k++ )
                     if( df > exp(-(double)KS[k])*SF && age + 1 > nd[k] ) nd[k] = age + 1;
               }
            }
            need[0][t] = nd[1]; need[1][t] = nd[3];
            if( nd[1] > autoCount[0] ) over[1]++;
            if( nd[0] > autoCount[0] ) over[0]++;
            if( nd[3] > autoCount[1] ) over[3]++;
            if( nd[2] > autoCount[1] ) over[2]++;
            if( nd[0] >= last - first - 2 ) never++;
            if( nd[1] > worst[0] ) { worst[0] = nd[1]; worstT[0] = t; }
            if( nd[3] > worst[1] ) { worst[1] = nd[3]; worstT[1] = t; }
         }
         qsort(need[0], g_trials, sizeof(int), cmpInt);
         qsort(need[1], g_trials, sizeof(int), cmpInt);
         printf("%s\t%s\t%s\t%d\t%ld\t%d\t%d\t%d\t%d\t%ld\t%ld\t%d\t%d\t%d\t%d\t%ld\t%ld\t%ld\t%d\t%d\n",
                fi->name, desc, kinds[kind], lookback, live,
                autoCount[0], need[0][g_trials/2], need[0][g_trials - 1 - g_trials/100], need[0][g_trials-1], over[1], over[0],
                autoCount[1], need[1][g_trials/2], need[1][g_trials - 1 - g_trials/100], need[1][g_trials-1], over[3], over[2],
                never, worstT[0], worstT[1]);
         fflush(stdout);
      }
   }
   free(need[0]); free(need[1]);
}

int main(int argc, char **argv)
{
   g_list = argc > 1 && !strcmp(argv[1], "list");
   g_trials = (argc > 1 && !g_list) ? atoi(argv[1]) : 1000;
   g_seed = (argc > 2) ? strtoull(argv[2], NULL, 10) : 1;
   g_only = (argc > 3) ? argv[3] : NULL;
   if( argc > 4 ) g_drift = atof(argv[4]);
   if( argc > 6 ) { g_level = atof(argv[5]); g_tick = atof(argv[6]); }
   if( getenv("CENSUS_CSV") )
   {
      char line[256];
      FILE *f = fopen(getenv("CENSUS_CSV"), "r");
      if( !f || !fgets(line, sizeof line, f) ) return 2;
      while( fN < NMAX && fgets(line, sizeof line, f) )
         if( sscanf(line, "%*[^,],%lf,%lf,%lf", &fH[fN], &fL[fN], &fC[fN]) == 3 ) fN++;
      fclose(f);
      if( fN < 400 ) return 2;
   }
   if( TA_Initialize() != TA_SUCCESS ) return 2;
   if( !g_list && !g_only )
      printf("func\tcfg\tkind\tlookback\tlive\tauto4\tp50_10\tp99_10\tmax_10\tover10\tover7\tauto8\tp50_19\tp99_19\tmax_19\tover19\tover16\tnever\tworstT10\tworstT19\n");
   TA_ForEachFunc(each, NULL);
   TA_Shutdown();
   return 0;
}
