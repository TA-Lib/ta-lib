/* What a frozen-release serve adds to the generated transport: seeded inputs
 * and hashed outputs on abstract_call, the member's waiver hook, the ref_*
 * methods that hand the member's data to the driver, and a refusal of every
 * function the release lacks. #included by ta_abstract_serve.c under
 * -DTA_REF_SERVE, after the transport's globals.
 *
 * Nothing here may read TA_Globals: the frozen library's layout of it is not
 * the one in the current headers. */

#include "fuzz_data.h"
#include "ta_ref.h"
#include "ta_ref_absent.h"   /* builder-generated: the functions the release lacks */
#include "ta_ref_unst.h"     /* builder-generated: each function's unstable-period ids */

#define TA_REF_MAXN   4096
#define TA_REF_MAXOPT 64

static double g_refGen[6][TA_REF_MAXN];   /* open, high, low, close, volume, OI */
static int    g_refGenN;

double ta_ref_opt( const TaRefCase *c, const char *name, double dflt )
{
   int i;
   for( i = 0; i < c->nbOpt; i++ )
      if( strcmp( c->optName[i], name ) == 0 )
         return c->optValue[i];
   return dflt;
}

static int ta_ref_is_absent( const char *name, int len )
{
   int i;
   for( i = 0; ta_ref_absent[i]; i++ )
      if( (int)strlen( ta_ref_absent[i] ) == len && strncmp( ta_ref_absent[i], name, len ) == 0 )
         return 1;
   return 0;
}

static int ta_ref_json_string( char *resp, int resp_size, int pos, const char *s )
{
   pos = json_appendf( resp, resp_size, pos, "\"" );
   for( ; *s; s++ )
      pos = json_appendf( resp, resp_size, pos, (*s == '"' || *s == '\\') ? "\\%c" : "%c", *s );
   return json_appendf( resp, resp_size, pos, "\"" );
}

static const char *ta_ref_mode_name( TaRefTolMode m )
{
   switch( m )
   {
   case TA_REF_TOL_ABS:             return "abs";
   case TA_REF_TOL_REL_IN:          return "rel_in";
   case TA_REF_TOL_REL_OUT:         return "rel_out";
   case TA_REF_TOL_REL_OUT_INFLOOR: return "rel_out_infloor";
   case TA_REF_TOL_NAN_TO:          return "nan_to";
   }
   return "?";
}

static int ta_ref_count_excluded( void )
{
   int n = 0;
   if( ta_ref_member.excluded )
      while( ta_ref_member.excluded[n] ) n++;
   return n;
}

/* Answers the request and returns 1 when it is a ref_* method or names a
 * function the release lacks; returns 0 to let the transport dispatch it. The
 * refusal comes first because some handlers act before they call the
 * function: an unstable-period id that names the absent function in the
 * current enum can name another, or all of them, in the frozen one. */
static int ta_ref_handle( const char *json, const char *method, int methodLen,
                          char *resp, int resp_size )
{
   const TaRefMember *m = &ta_ref_member;
   int i, pos;

   if( methodLen > 3 && strncmp( method, "TA_", 3 ) == 0 )
   {
      int len = methodLen - 3;
      if( len > 9 && strncmp( method + methodLen - 9, "_Lookback", 9 ) == 0 )
         len -= 9;
      if( ta_ref_is_absent( method + 3, len ) )
      {
         snprintf( resp, resp_size, "{\"error\":\"Unknown method: %.*s\"}", methodLen, method );
         return 1;
      }
      return 0;
   }

   if( (methodLen == 13 && strncmp( method, "abstract_call", 13 ) == 0)
       || (methodLen == 21 && strncmp( method, "abstract_get_lookback", 21 ) == 0) )
   {
      int nameLen = 0;
      const char *name = json_find_string( json, "funcName", &nameLen );
      if( name && ta_ref_is_absent( name, nameLen ) )
      {
         snprintf( resp, resp_size, "{\"error\":\"Unknown function: %.*s\"}", nameLen, name );
         return 1;
      }
      return 0;
   }

   if( methodLen == 8 && strncmp( method, "ref_info", 8 ) == 0 )
   {
      pos = json_appendf( resp, resp_size, 0,
               "{\"version\":\"%s\",\"commit\":\"%s\",\"libVersion\":\"%s.%s.%s\","
               "\"nbFunctions\":%d,\"intFloor\":%d,\"maTypeMax\":%d,"
               "\"nbExcluded\":%d,\"nbTol\":%d,\"nbWaivers\":%d}",
               TA_REF_VERSION, m->commit ? m->commit : "",
               TA_GetVersionMajor(), TA_GetVersionMinor(), TA_GetVersionPatch(),
               m->nbFunctions, m->intFloor, TA_REF_MATYPE_MAX,
               ta_ref_count_excluded(), m->tol ? m->nbTol : 0,
               m->waivers ? m->nbWaivers : 0 );
      (void)pos;
      return 1;
   }

   i = json_find_int( json, "index" );
   if( methodLen == 12 && strncmp( method, "ref_excluded", 12 ) == 0 )
   {
      if( i < 0 || i >= ta_ref_count_excluded() )
         snprintf( resp, resp_size, "{\"error\":\"index out of range\"}" );
      else
      {
         pos = json_appendf( resp, resp_size, 0, "{\"name\":" );
         pos = ta_ref_json_string( resp, resp_size, pos, m->excluded[i] );
         json_appendf( resp, resp_size, pos, "}" );
      }
      return 1;
   }
   if( methodLen == 7 && strncmp( method, "ref_tol", 7 ) == 0 )
   {
      if( !m->tol || i < 0 || i >= m->nbTol )
         snprintf( resp, resp_size, "{\"error\":\"index out of range\"}" );
      else
      {
         pos = json_appendf( resp, resp_size, 0, "{\"func\":" );
         pos = ta_ref_json_string( resp, resp_size, pos, m->tol[i].func );
         json_appendf( resp, resp_size, pos, ",\"mode\":\"%s\",\"tol\":%.17g,\"cap\":%.17g}",
                       ta_ref_mode_name( m->tol[i].mode ), m->tol[i].tol, m->tol[i].cap );
      }
      return 1;
   }
   if( methodLen == 10 && strncmp( method, "ref_waiver", 10 ) == 0 )
   {
      if( !m->waivers || i < 0 || i >= m->nbWaivers )
         snprintf( resp, resp_size, "{\"error\":\"index out of range\"}" );
      else
      {
         pos = json_appendf( resp, resp_size, 0, "{\"id\":" );
         pos = ta_ref_json_string( resp, resp_size, pos, m->waivers[i].id );
         pos = json_appendf( resp, resp_size, pos, ",\"why\":" );
         pos = ta_ref_json_string( resp, resp_size, pos, m->waivers[i].why );
         json_appendf( resp, resp_size, pos, ",\"maxFraction\":%.17g}", m->waivers[i].maxFraction );
      }
      return 1;
   }
   return 0;
}

/* Generates the case's series from (gen_shape, gen_seed, gen_n) into the input
 * buffers, mapping real inputs the way the driver does: close, then volume.
 * Returns 1, 0 when the request carries no seed, -1 on an unusable size. */
static int ta_ref_gen_inputs( const char *json, const TA_FuncHandle *handle,
                              const TA_FuncInfo *fi )
{
   double *const bufs[6] = { g_inBuf0, g_inBuf1, g_inBuf2, g_inBuf3, g_inBuf4, g_inBuf5 };
   unsigned int i;
   int k, realIdx = 0, n;

   if( !json_find_int( json, "gen_present" ) ) return 0;
   n = json_find_int( json, "gen_n" );
   if( n < 1 || n > TA_REF_MAXN || n > MAX_ARRAY_SIZE ) return -1;

   fuzz_gen( json_find_int( json, "gen_shape" ), json_find_int( json, "gen_seed" ), n,
             g_refGen[0], g_refGen[1], g_refGen[2], g_refGen[3], g_refGen[4], g_refGen[5] );
   g_refGenN = n;
   for( k = 0; k < 6; k++ )
      memcpy( bufs[k], g_refGen[k], (size_t)n * sizeof(double) );

   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo( handle, i, &ii );
      if( ii->type != TA_Input_Real ) continue;
      memcpy( bufs[realIdx < 4 ? realIdx : 3], g_refGen[realIdx == 1 ? 4 : 3],
              (size_t)n * sizeof(double) );
      realIdx++;
   }
   return 1;
}

/* Applies the request's unstablePeriod to the ids the function's own handler
 * sets, which the builder checked name the same functions in the release.
 * Returns -1 for a non-zero period on a function that has none. */
static int ta_ref_set_unstable( const char *json, const char *func, int *set )
{
   int k = json_find_int( json, "unstablePeriod" ), i, j;
   *set = 0;
   if( k == 0 ) return 0;
   for( i = 0; ta_ref_unst[i].func; i++ )
      if( strcmp( ta_ref_unst[i].func, func ) == 0 )
      {
         for( j = 0; j < ta_ref_unst[i].nb; j++ )
            TA_SetUnstablePeriod( (TA_FuncUnstId)ta_ref_unst[i].ids[j], (unsigned int)k );
         *set = 1;
         return 0;
      }
   return -1;
}

static void ta_ref_reset_unstable( const char *func, int set )
{
   int i, j;
   if( !set ) return;
   for( i = 0; ta_ref_unst[i].func; i++ )
      if( strcmp( ta_ref_unst[i].func, func ) == 0 )
         for( j = 0; j < ta_ref_unst[i].nb; j++ )
            TA_SetUnstablePeriod( (TA_FuncUnstId)ta_ref_unst[i].ids[j], 0 );
}

/* The member's verdict on a seeded case, or -1 to compare it. */
static int ta_ref_waived( const char *json, const TA_FuncHandle *handle, const TA_FuncInfo *fi,
                          TA_ParamHolder *params, int startIdx, int endIdx )
{
   static const char *names[TA_REF_MAXOPT];
   static double values[TA_REF_MAXOPT];
   TaRefCase c;
   TA_Integer lookback = 0;
   unsigned int i;
   int w;

   if( !ta_ref_member.waive ) return -1;
   for( i = 0; i < fi->nbOptInput && i < TA_REF_MAXOPT; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo( handle, i, &oi );
      names[i]  = oi->paramName;
      values[i] = (oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList)
                  ? json_find_double( json, oi->paramName )
                  : (double)json_find_int( json, oi->paramName );
   }
   c.func     = fi->name;
   c.open     = g_refGen[0]; c.high = g_refGen[1]; c.low = g_refGen[2];
   c.close    = g_refGen[3]; c.volume = g_refGen[4];
   c.n        = g_refGenN;
   c.startIdx = startIdx;
   c.endIdx   = endIdx;
   c.lookback = (TA_GetLookback( params, &lookback ) == TA_SUCCESS) ? (int)lookback : -1;
   c.nbOpt    = (int)i;
   c.optName  = names;
   c.optValue = values;
   w = ta_ref_member.waive( &c );
   return (w >= 0 && w < ta_ref_member.nbWaivers) ? w : -1;
}

/* Appends the 64-bit digest of the raw outputs, in logical output order, and
 * closes the response. */
static void ta_ref_append_hash( char *resp, int resp_size, int pos, const TA_FuncInfo *fi,
                                const int *outputIsInteger, TA_RetCode rc, int outNBElement )
{
   unsigned long long h = fuzz_hash_init();
   unsigned int o;
   int r = 0, n = 0;

   if( rc == TA_SUCCESS && outNBElement > 0 )
      for( o = 0; o < fi->nbOutput && o < TA_SERVE_MAX_OUTPUT; o++ )
      {
         if( outputIsInteger[o] )
            h = fuzz_hash_bytes( h, g_outIntBufV[n++], (unsigned long)outNBElement * sizeof(int) );
         else
            h = fuzz_hash_bytes( h, g_outBufV[r++], (unsigned long)outNBElement * sizeof(double) );
      }
   h = fuzz_hash_fin( h );
   json_appendf( resp, resp_size, pos, ",\"out_hash\":\"%016llx\"}", h );
}
