//! Prototype-only corpus harness for the C block kernels, written outside the
//! tree when `TA_CODEGEN_KERN_HARNESS=<path>` is set; `generate` writes nothing
//! otherwise, so no committed file depends on it.

use std::collections::BTreeMap;
use std::fmt::Write as _;

use crate::backends::c_stream::{block_entry_of, kernel_decision, recall_subs, BlockEntry, KernDecline};
use crate::ir::{FuncDef, ParamType};
use crate::streaming::{input_array_names, FuncsLookup};

const MAX_IO: usize = 16;

/// The harness's series slot for one input: 0..5 are open, high, low, close,
/// volume, open interest; 6 is the second walk a second generic real reads.
fn series_slot(name: &str, generic: &mut usize) -> usize {
    match name {
        "inOpen" => 0,
        "inHigh" => 1,
        "inLow" => 2,
        "inClose" => 3,
        "inVolume" => 4,
        "inOpenInterest" => 5,
        _ => {
            *generic += 1;
            if *generic == 1 { 3 } else { 6 }
        }
    }
}

fn opt_args(func: &FuncDef) -> Vec<String> {
    func.optional_inputs
        .iter()
        .map(|p| match (&p.param_type, p.default) {
            (ParamType::Real, Some(d)) => format!("{d:?}"),
            (ParamType::Real, None) => "TA_REAL_DEFAULT".to_string(),
            #[allow(clippy::cast_possible_truncation)]
            (ParamType::Integer, Some(d)) if p.name == "optInTimePeriod" => format!("optPeriod( {} )", d as i64),
            #[allow(clippy::cast_possible_truncation)]
            (_, Some(d)) => format!("{}", d as i64),
            (_, None) => "TA_INTEGER_DEFAULT".to_string(),
        })
        .collect()
}

fn out_casts(func: &FuncDef) -> Vec<String> {
    func.outputs
        .iter()
        .enumerate()
        .map(|(k, o)| {
            let ty = if o.param_type == ParamType::Integer { "int" } else { "double" };
            format!("({ty} *)out[{k}]")
        })
        .collect()
}

fn adapters(s: &mut String, func: &FuncDef) {
    let n = func.name.to_uppercase();
    let ins = input_array_names(func);
    let in_arr: Vec<String> = (0..ins.len()).map(|j| format!("in[{j}]")).collect();
    let in_bar: Vec<String> = (0..ins.len()).map(|j| format!("bar[{j}]")).collect();
    let opts = opt_args(func);
    let out_args = out_casts(func);
    let join = |parts: &[&[String]]| parts.iter().flat_map(|p| p.iter().cloned()).collect::<Vec<_>>().join(", ");
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_batch( int n, const double *const *in, void *const *out, int *beg, int *nb )\n{{\n   return TA_{n}( 0, n - 1, {}, beg, nb, {} );\n}}",
        join(&[&in_arr, &opts]),
        out_args.join(", ")
    );
    let open_args = join(&[&in_arr, &["n".to_string()], &opts, &out_args]);
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_open( void **h, int n, const double *const *in, void *const *out )\n{{\n   TA_{n}_Stream *st = NULL;\n   TA_RetCode rc = TA_{n}_Open( &st, {open_args} );\n   *h = st;\n   return rc;\n}}"
    );
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_block( void *h, const double *const *in, int nb, void *const *out )\n{{\n   return TA_{n}_UpdateBlock( (struct TA_{n}_Stream *)h, {} );\n}}",
        join(&[&in_arr, &["nb".to_string()], &out_args])
    );
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_update( void *h, const double *bar, void *const *out )\n{{\n   return TA_{n}_Update( (TA_{n}_Stream *)h, {} );\n}}",
        join(&[&in_bar, &out_args])
    );
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_value( const void *h, void *const *out )\n{{\n   return TA_{n}_Value( (const TA_{n}_Stream *)h, {} );\n}}",
        out_args.join(", ")
    );
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_range( const void *h, int *beg, int *nb )\n{{\n   return TA_{n}_OutRange( (const TA_{n}_Stream *)h, beg, nb );\n}}"
    );
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_close( void *h )\n{{\n   return TA_{n}_Close( (TA_{n}_Stream *)h );\n}}\n"
    );
    let mut bb_args = in_arr.clone();
    bb_args.extend(opts.iter().cloned());
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_bbatch( int s, int e, const double *const *in, void *const *out, int *beg, int *nb )\n{{\n   return TA_{n}_BlockBatch( s, e, {}, beg, nb, {} );\n}}\n",
        bb_args.join(", "),
        out_args.join(", ")
    );
}

/// A whole-range-or-sub-range batch call with defaults, which `restart` makes
/// for every function, kernel or not.
fn range_adapter(s: &mut String, func: &FuncDef) {
    let n = func.name.to_uppercase();
    let mut args: Vec<String> = (0..input_array_names(func).len()).map(|j| format!("in[{j}]")).collect();
    args.extend(opt_args(func));
    let _ = writeln!(
        s,
        "static TA_RetCode {n}_rbatch( int s, int e, const double *const *in, void *const *out, int *beg, int *nb )\n{{\n   return TA_{n}( s, e, {}, beg, nb, {} );\n}}\n",
        args.join(", "),
        out_casts(func).join(", ")
    );
}

fn io_fields(func: &FuncDef) -> String {
    let n = func.name.to_uppercase();
    let ins = input_array_names(func);
    let mut generic = 0;
    let slots: Vec<String> = ins.iter().map(|a| series_slot(a, &mut generic).to_string()).collect();
    let int_flags: Vec<&str> =
        func.outputs.iter().map(|o| if o.param_type == ParamType::Integer { "1" } else { "0" }).collect();
    format!("\"{n}\", {}, {}, {{ {} }}, {{ {} }}", ins.len(), func.outputs.len(), slots.join(", "), int_flags.join(", "))
}

fn table_row(func: &FuncDef) -> String {
    let n = func.name.to_uppercase();
    format!(
        "   {{ {}, {n}_batch, {n}_open, {n}_block, {n}_update, {n}_value, {n}_range, {n}_close, {n}_rbatch, {n}_bbatch }},\n",
        io_fields(func)
    )
}

fn restart_row(func: &FuncDef) -> String {
    let n = func.name.to_uppercase();
    format!("   {{ {}, NULL, NULL, NULL, NULL, NULL, NULL, NULL, {n}_rbatch, NULL }},\n", io_fields(func))
}

/// The harness source and the census of which functions got a kernel.
pub fn generate(funcs: &[FuncDef]) -> (String, String) {
    let lookup = FuncsLookup(funcs);
    let mut kerneled: Vec<&FuncDef> = Vec::new();
    let mut declined: BTreeMap<KernDecline, Vec<String>> = BTreeMap::new();
    for f in funcs.iter().filter(|f| f.streaming) {
        match kernel_decision(f, &lookup) {
            Ok(()) => kerneled.push(f),
            Err(why) => declined.entry(why).or_default().push(f.name.clone()),
        }
    }
    let mut census = String::new();
    let _ = writeln!(
        census,
        "functions {}, with a stream tier {}, kerneled {}",
        funcs.len(),
        funcs.iter().filter(|f| f.streaming).count(),
        kerneled.len()
    );
    for (why, names) in &declined {
        let _ = writeln!(census, "declined {why:?}: {} ({})", names.len(), names.join(" "));
    }
    let composed: Vec<&str> = kerneled
        .iter()
        .filter(|f| block_entry_of(f, &lookup) == BlockEntry::Composed)
        .map(|f| f.name.as_str())
        .collect();
    let _ = writeln!(census, "composed kernels {}: {}", composed.len(), composed.join(" "));
    let _ = writeln!(census, "blocked batch {}", kerneled.len());
    let recalls: Vec<(String, usize)> = kerneled
        .iter()
        .map(|f| (f.name.to_uppercase(), recall_subs(f, &lookup).len()))
        .filter(|(_, k)| *k > 0)
        .collect();

    let mut s = String::new();
    s.push_str("/* Generated by ta_codegen (TA_CODEGEN_KERN_HARNESS); prototype only, never committed.\n");
    for line in census.lines() {
        let _ = writeln!(s, " * {line}");
    }
    s.push_str(" */\n");
    s.push_str(HEAD);
    for f in funcs {
        assert!(
            input_array_names(f).len() <= MAX_IO && f.outputs.len() <= MAX_IO,
            "{}: more inputs or outputs than the harness carries",
            f.name
        );
        range_adapter(&mut s, f);
    }
    for f in &kerneled {
        assert!(
            input_array_names(f).len() <= MAX_IO && f.outputs.len() <= MAX_IO,
            "{}: more inputs or outputs than the harness carries",
            f.name
        );
        adapters(&mut s, f);
    }
    s.push_str("static const KFunc kfuncs[] = {\n");
    for f in &kerneled {
        s.push_str(&table_row(f));
    }
    s.push_str("};\n#define NKFUNC ((int)(sizeof(kfuncs) / sizeof(kfuncs[0])))\n\n");
    s.push_str("static const KFunc rfuncs[] = {\n");
    for f in funcs {
        s.push_str(&restart_row(f));
    }
    s.push_str("};\n#define NRFUNC ((int)(sizeof(rfuncs) / sizeof(rfuncs[0])))\n\n");
    s.push_str("static const struct {\n   const char *name;\n   int subs;\n} recallSubs[] = {\n");
    for (name, k) in &recalls {
        let _ = writeln!(s, "   {{ \"{name}\", {k} }},");
    }
    if recalls.is_empty() {
        s.push_str("   { NULL, 0 },\n");
    }
    s.push_str("};\n#define NRECALL ((int)(sizeof(recallSubs) / sizeof(recallSubs[0])))\n\n");
    s.push_str(DRIVER);
    (s, census)
}

const HEAD: &str = r#"#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <pthread.h>
#include <pthread/qos.h>
#endif
#include "ta_libc.h"
#include "ta_func_stream_private.h"

#define MAX_IO 16
#define NSERIES 7

/* KERN_PERIOD=<p> replaces every optInTimePeriod default, to reach the
 * identity and mode arms the defaults never select. */
static int gPeriod;

static int optPeriod( int d )
{
   return gPeriod > 0 ? gPeriod : d;
}

typedef TA_RetCode (*BatchFn)( int n, const double *const *in, void *const *out, int *beg, int *nb );
typedef TA_RetCode (*OpenFn)( void **h, int n, const double *const *in, void *const *out );
typedef TA_RetCode (*BlockFn)( void *h, const double *const *in, int nb, void *const *out );
typedef TA_RetCode (*UpdateFn)( void *h, const double *bar, void *const *out );
typedef TA_RetCode (*ValueFn)( const void *h, void *const *out );
typedef TA_RetCode (*RangeFn)( const void *h, int *beg, int *nb );
typedef TA_RetCode (*CloseFn)( void *h );
typedef TA_RetCode (*RBatchFn)( int s, int e, const double *const *in, void *const *out, int *beg, int *nb );

typedef struct {
   const char *name;
   int nIn, nOut;
   int inSeries[MAX_IO];
   int outInt[MAX_IO];
   BatchFn batch;
   OpenFn open;
   BlockFn block;
   UpdateFn update;
   ValueFn value;
   RangeFn range;
   CloseFn close;
   RBatchFn rbatch;
   RBatchFn bbatch;
} KFunc;

"#;

const DRIVER: &str = r#"static unsigned long long rngState;

/* KERN_ONLY=NAME1,NAME2 restricts check and time to those functions. */
static int selected( const char *name )
{
   const char *only = getenv( "KERN_ONLY" ), *p;
   size_t len = strlen( name );
   if( !only || !*only )
      return 1;
   for( p = only; *p; )
   {
      const char *q = strchr( p, ',' );
      size_t l = q ? (size_t)(q - p) : strlen( p );
      if( l == len && strncmp( p, name, len ) == 0 )
         return 1;
      if( !q )
         break;
      p = q + 1;
   }
   return 0;
}

static double rnd( void )
{
   rngState ^= rngState << 13;
   rngState ^= rngState >> 7;
   rngState ^= rngState << 17;
   return (double)(rngState >> 11) * (1.0 / 9007199254740992.0);
}

static double cents( double x )
{
   return floor( x * 100.0 + 0.5 ) / 100.0;
}

/* Mean-reverting walks around 100 (and 50 for the second real), two-decimal,
 * with gapped opens and a positive volume, so no series goes flat. */
static void makeSeries( unsigned long long seed, int n, double *const *s )
{
   int i;
   double c = 100.0, w = 50.0, o, h, l;
   rngState = seed * 2654435761ULL + 88172645463325252ULL;
   for( i = 0; i < n; i++ )
   {
      o = cents( c * (1.0 + 0.004 * (rnd() - 0.5)) );
      c = c * (1.0 + 0.02 * (rnd() - 0.5));
      c += (100.0 - c) * 0.01;
      c = cents( c );
      h = cents( (o > c ? o : c) * (1.0 + 0.003 * rnd()) );
      l = cents( (o < c ? o : c) * (1.0 - 0.003 * rnd()) );
      w = w * (1.0 + 0.03 * (rnd() - 0.5));
      w += (50.0 - w) * 0.01;
      w = cents( w );
      s[0][i] = o;
      s[1][i] = h;
      s[2][i] = l;
      s[3][i] = c;
      s[4][i] = floor( 1000.0 + 9000.0 * rnd() );
      s[5][i] = floor( 5000.0 + 1000.0 * rnd() );
      s[6][i] = w;
   }
}

static size_t esz( const KFunc *f, int k )
{
   return f->outInt[k] ? sizeof(int) : sizeof(double);
}

static void *at( void *buf, const KFunc *f, int k, int i )
{
   return (char *)buf + (size_t)i * esz( f, k );
}

static void printElem( const KFunc *f, int k, const void *p )
{
   if( f->outInt[k] )
      printf( "%d", *(const int *)p );
   else
      printf( "%.17g", *(const double *)p );
}

/* First element of `got` that differs from `ref`, or -1. */
static int firstDiff( const KFunc *f, void *const *ref, void *const *got, int nb, int *outK )
{
   int i, k;
   for( i = 0; i < nb; i++ )
      for( k = 0; k < f->nOut; k++ )
         if( memcmp( at( ref[k], f, k, i ), at( got[k], f, k, i ), esz( f, k ) ) != 0 )
         {
            *outK = k;
            return i;
         }
   return -1;
}

static void reportDiff( const KFunc *f, void *const *ref, void *const *got, int beg, int i, int k, const char *phase )
{
   printf( "%s DIFFER at bar %d output %d: ", f->name, beg + i, k );
   printElem( f, k, at( ref[k], f, k, i ) );
   printf( " vs " );
   printElem( f, k, at( got[k], f, k, i ) );
   printf( " [%s]\n", phase );
}

static void shiftIn( const KFunc *f, double *const *series, int pos, const double **in )
{
   int j;
   for( j = 0; j < f->nIn; j++ )
      in[j] = series[f->inSeries[j]] + pos;
}

static void slots( const KFunc *f, void *const *buf, int idx, void **out )
{
   int k;
   for( k = 0; k < f->nOut; k++ )
      out[k] = at( buf[k], f, k, idx );
}

/* One BlockBatch call over [s, e] against the batch over the same range. */
static int bbLeg( const KFunc *f, const double *const *in, int s, int e, void *const *ref, void *const *got )
{
   int rb = -1, rn = -1, gb = -2, gn = -2, i, k;
   TA_RetCode rr = f->rbatch( s, e, in, ref, &rb, &rn );
   TA_RetCode gr = f->bbatch( s, e, in, got, &gb, &gn );
   if( rr != gr || rb != gb || rn != gn )
   {
      printf( "%s DIFFER over [%d, %d]: rc %d beg %d nb %d vs rc %d beg %d nb %d [blockbatch]\n", f->name, s, e, (int)rr, rb,
              rn, (int)gr, gb, gn );
      return 1;
   }
   if( rr == TA_SUCCESS && (i = firstDiff( f, ref, got, rn, &k )) >= 0 )
   {
      printf( "%s over [%d, %d]: ", f->name, s, e );
      reportDiff( f, ref, got, rb, i, k, "blockbatch" );
      return 1;
   }
   return 0;
}

/* 0 ok, 1 differ, 2 skipped. */
static int checkOne( const KFunc *f, double *const *series, int n, void *const *ref, void *const *got, void *const *aux )
{
   static const int chunks[4] = { 1, 7, 64, 500 };
   const double *in[MAX_IO];
   void *out[MAX_IO];
   double bar[MAX_IO];
   void *h = NULL;
   TA_RetCode rc;
   int beg = 0, nb = 0, pos, idx, ci, c, i, k, j, half, rb, rn;

   shiftIn( f, series, 0, in );
   rc = f->batch( n, in, ref, &beg, &nb );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "%s skipped: batch returned %d with %d outputs\n", f->name, (int)rc, nb );
      return 2;
   }

   slots( f, got, 0, out );
   rc = f->open( &h, beg + 1, in, out );
   if( rc != TA_SUCCESS )
   {
      printf( "%s skipped: Open refused (%d)\n", f->name, (int)rc );
      return 2;
   }
   for( pos = beg + 1, idx = 1, ci = 0; pos < n; pos += c, idx += c, ci++ )
   {
      c = chunks[ci % 4];
      if( c > n - pos )
         c = n - pos;
      shiftIn( f, series, pos, in );
      slots( f, got, idx, out );
      rc = f->block( h, in, c, out );
      if( rc != TA_SUCCESS )
      {
         printf( "%s DIFFER: UpdateBlock returned %d at bar %d\n", f->name, (int)rc, pos );
         f->close( h );
         return 1;
      }
   }
   f->close( h );
   if( (i = firstDiff( f, ref, got, nb, &k )) >= 0 )
   {
      reportDiff( f, ref, got, beg, i, k, "chunks" );
      return 1;
   }

   for( k = 0; k < f->nOut; k++ )
      memset( got[k], 0, (size_t)n * esz( f, k ) );
   shiftIn( f, series, 0, in );
   slots( f, got, 0, out );
   h = NULL;
   if( f->open( &h, beg + 1, in, out ) != TA_SUCCESS )
   {
      printf( "%s DIFFER: second Open refused\n", f->name );
      return 1;
   }
   half = (n - (beg + 1)) / 2;
   if( half > 0 )
   {
      shiftIn( f, series, beg + 1, in );
      slots( f, got, 1, out );
      rc = f->block( h, in, half, out );
      if( rc != TA_SUCCESS )
      {
         printf( "%s DIFFER: UpdateBlock returned %d\n", f->name, (int)rc );
         f->close( h );
         return 1;
      }
   }
   for( pos = beg + 1 + half, idx = 1 + half; pos < n; pos++, idx++ )
   {
      for( j = 0; j < f->nIn; j++ )
         bar[j] = series[f->inSeries[j]][pos];
      slots( f, got, idx, out );
      rc = f->update( h, bar, out );
      if( rc != TA_SUCCESS )
      {
         printf( "%s DIFFER: Update returned %d at bar %d\n", f->name, (int)rc, pos );
         f->close( h );
         return 1;
      }
   }
   if( (i = firstDiff( f, ref, got, nb, &k )) >= 0 )
   {
      reportDiff( f, ref, got, beg, i, k, "block+update" );
      f->close( h );
      return 1;
   }

   {
      double vbuf[MAX_IO];
      void *vout[MAX_IO];
      for( k = 0; k < f->nOut; k++ )
         vout[k] = &vbuf[k];
      rc = f->value( h, vout );
      for( k = 0; rc == TA_SUCCESS && k < f->nOut; k++ )
         if( memcmp( vout[k], at( ref[k], f, k, nb - 1 ), esz( f, k ) ) != 0 )
         {
            printf( "%s DIFFER at bar %d output %d: ", f->name, beg + nb - 1, k );
            printElem( f, k, at( ref[k], f, k, nb - 1 ) );
            printf( " vs " );
            printElem( f, k, vout[k] );
            printf( " [Value]\n" );
            f->close( h );
            return 1;
         }
      if( rc != TA_SUCCESS )
      {
         printf( "%s DIFFER: Value returned %d\n", f->name, (int)rc );
         f->close( h );
         return 1;
      }
   }
   rb = rn = -1;
   rc = f->range( h, &rb, &rn );
   f->close( h );
   if( rc != TA_SUCCESS || rb != beg || rn != nb )
   {
      printf( "%s DIFFER at bar %d output 0: %d+%d vs %d+%d [OutRange, rc %d]\n", f->name, beg + nb - 1, beg, nb, rb, rn,
              (int)rc );
      return 1;
   }

   shiftIn( f, series, 0, in );
   {
      const int ranges[][2] = { { 0, n - 1 },        { beg + 137, n - 1 },     { beg + 1, beg + 701 }, { beg, beg },
                                { 0, beg },          { beg - 1, beg - 1 },     { n - 1, n - 1 },       { beg + 500, beg + 500 },
                                { beg / 2, beg + 3 }, { n - 300, n - 1 },      { n - 2, n - 1 },       { 2 * beg + 1, n - 1 } };
      int r;
      for( r = 0; r < (int)(sizeof ranges / sizeof ranges[0]); r++ )
      {
         int s = ranges[r][0], e = ranges[r][1];
         if( s < 0 || e > n - 1 || e < s )
            continue;
         if( bbLeg( f, in, s, e, aux, got ) )
            return 1;
      }
   }
   printf( "%s ok\n", f->name );
   return 0;
}

static void allocSeries( double **series, size_t len )
{
   int s;
   for( s = 0; s < NSERIES; s++ )
   {
      series[s] = malloc( len * sizeof(double) );
      if( !series[s] )
      {
         fprintf( stderr, "out of memory\n" );
         exit( 2 );
      }
   }
}

static void allocOuts( void **buf, size_t len )
{
   int k;
   for( k = 0; k < MAX_IO; k++ )
   {
      buf[k] = calloc( len, sizeof(double) );
      if( !buf[k] )
      {
         fprintf( stderr, "out of memory\n" );
         exit( 2 );
      }
   }
}

static int runCheck( void )
{
   const int n = 3000;
   double *series[NSERIES];
   void *ref[MAX_IO], *got[MAX_IO], *aux[MAX_IO];
   int fi, r, ok = 0, differ = 0, skipped = 0, tried = 0;

   allocSeries( series, (size_t)n );
   allocOuts( ref, (size_t)n );
   allocOuts( got, (size_t)n );
   allocOuts( aux, (size_t)n );
   makeSeries( 1, n, series );
   for( fi = 0; fi < NKFUNC; fi++ )
   {
      if( !selected( kfuncs[fi].name ) )
         continue;
      r = checkOne( &kfuncs[fi], series, n, ref, got, aux );
      tried++;
      ok += r == 0;
      differ += r == 1;
      skipped += r == 2;
   }
   printf( "check: %d ok, %d DIFFER, %d skipped of %d\n", ok, differ, skipped, tried );
   {
      int total = 0, k = 0;
      for( r = 0; r < NRECALL; r++ )
         if( recallSubs[r].name && selected( recallSubs[r].name ) )
         {
            total += recallSubs[r].subs;
            k++;
         }
      printf( "recall subs: %d in %d functions:", total, k );
      for( r = 0; r < NRECALL; r++ )
         if( recallSubs[r].name && selected( recallSubs[r].name ) )
            printf( " %s:%d", recallSubs[r].name, recallSubs[r].subs );
      printf( "\n" );
   }
   return differ ? 1 : 0;
}

static double nowNs( void )
{
   struct timespec ts;
   clock_gettime( CLOCK_MONOTONIC, &ts );
   return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int stable( const double *t, int used )
{
   double lo1 = 1e300, lo2 = 1e300;
   int j;
   if( used < 2 )
      return 0;
   for( j = 0; j < used; j++ )
   {
      if( t[j] < lo1 )
      {
         lo2 = lo1;
         lo1 = t[j];
      }
      else if( t[j] < lo2 )
         lo2 = t[j];
   }
   return lo2 <= lo1 * 1.01;
}

static double lowest( const double *t, int used )
{
   double lo = 1e300;
   int j;
   for( j = 0; j < used; j++ )
      if( t[j] < lo )
         lo = t[j];
   return lo;
}

static volatile double sink;

static long minorFaults( void )
{
   struct rusage ru;
   getrusage( RUSAGE_SELF, &ru );
   return ru.ru_minflt;
}

static double repBatch( const KFunc *f, double *const *series, int points, int R, int inner, void *const *out, long long *call )
{
   const double *in[MAX_IO];
   int j, beg, nb;
   double t0 = nowNs();
   for( j = 0; j < inner; j++ )
   {
      shiftIn( f, series, (int)((*call)++ % R) * points, in );
      nb = 0;
      f->batch( points, in, out, &beg, &nb );
      if( nb > 0 )
         sink += f->outInt[0] ? (double)((int *)out[0])[nb - 1] : ((double *)out[0])[nb - 1];
   }
   return (nowNs() - t0) / inner / points;
}

static double repKern( const KFunc *f, double *const *series, int points, int R, int inner, int beg, int block,
                       void *const *out, long long *call )
{
   const double *in[MAX_IO];
   void *o[MAX_IO];
   void *h;
   int j, pos, c, so;
   double t0 = nowNs();
   for( j = 0; j < inner; j++ )
   {
      so = (int)((*call)++ % R) * points;
      shiftIn( f, series, so, in );
      slots( f, out, 0, o );
      h = NULL;
      if( f->open( &h, beg + 1, in, o ) != TA_SUCCESS )
         continue;
      for( pos = beg + 1; pos < points; pos += c )
      {
         c = block > 0 && block < points - pos ? block : points - pos;
         shiftIn( f, series, so + pos, in );
         slots( f, out, pos - beg, o );
         f->block( h, in, c, o );
      }
      f->close( h );
      sink += f->outInt[0] ? (double)((int *)out[0])[points - beg - 1] : ((double *)out[0])[points - beg - 1];
   }
   return (nowNs() - t0) / inner / points;
}

static double repBB( const KFunc *f, double *const *series, int points, int R, int inner, void *const *out, long long *call )
{
   const double *in[MAX_IO];
   int j, beg, nb;
   double t0 = nowNs();
   for( j = 0; j < inner; j++ )
   {
      shiftIn( f, series, (int)((*call)++ % R) * points, in );
      nb = 0;
      f->bbatch( 0, points - 1, in, out, &beg, &nb );
      if( nb > 0 )
         sink += f->outInt[0] ? (double)((int *)out[0])[nb - 1] : ((double *)out[0])[nb - 1];
   }
   return (nowNs() - t0) / inner / points;
}

static int runTime( int points, int block, int maxreps )
{
   int R = points <= 4000 ? 64 : 1;
   int inner = 2000000 / points > 1 ? 2000000 / points : 1;
   double *series[NSERIES];
   void *out[MAX_IO];
   double *tb, *tk, *tq, warmMs = getenv( "KERN_WARM_MS" ) ? atof( getenv( "KERN_WARM_MS" ) ) : 300.0;
   int fi, r, s, ub, uk, uq, beg, nb;
   long long call = 0;
   int faults = getenv( "KERN_FAULTS" ) && atoi( getenv( "KERN_FAULTS" ) ) == 1;

   if( points < 2 || maxreps < 2 )
   {
      fprintf( stderr, "time: points >= 2 and maxreps >= 2\n" );
      return 2;
   }
   allocSeries( series, (size_t)points * (size_t)R );
   allocOuts( out, (size_t)points );
   tb = malloc( (size_t)maxreps * sizeof(double) );
   tk = malloc( (size_t)maxreps * sizeof(double) );
   tq = malloc( (size_t)maxreps * sizeof(double) );
   if( !tb || !tk || !tq )
      return 2;
   for( s = 0; s < R; s++ )
   {
      double *sub[NSERIES];
      int q;
      for( q = 0; q < NSERIES; q++ )
         sub[q] = series[q] + (size_t)s * (size_t)points;
      makeSeries( (unsigned long long)s + 1, points, sub );
   }
   printf( "time: points=%d block=%d maxreps=%d inner=%d rotate=%d warm=%gms\n", points, block, maxreps, inner, R, warmMs );
   for( fi = 0; fi < NKFUNC; fi++ )
   {
      const KFunc *f = &kfuncs[fi];
      const double *in[MAX_IO];
      double warmUntil;
      if( !selected( f->name ) )
         continue;
      shiftIn( f, series, 0, in );
      beg = nb = 0;
      if( f->batch( points, in, out, &beg, &nb ) != TA_SUCCESS || nb <= 0 )
      {
         printf( "%s skipped: batch returned no output\n", f->name );
         continue;
      }
      warmUntil = nowNs() + 1e6 * warmMs;
      while( nowNs() < warmUntil )
      {
         repBatch( f, series, points, R, inner, out, &call );
         repKern( f, series, points, R, inner, beg, block, out, &call );
         repBB( f, series, points, R, inner, out, &call );
      }
      ub = uk = uq = 0;
      {
         long bFaults = 0, bbFaults = 0, f0;
         for( r = 0; r < maxreps; r++ )
         {
            int sb = stable( tb, ub ), sk = stable( tk, uk ), sq = stable( tq, uq );
            if( sb && sk && sq )
               break;
            if( !sb )
            {
               f0 = minorFaults();
               tb[ub++] = repBatch( f, series, points, R, inner, out, &call );
               bFaults += minorFaults() - f0;
            }
            if( !sk )
               tk[uk++] = repKern( f, series, points, R, inner, beg, block, out, &call );
            if( !sq )
            {
               f0 = minorFaults();
               tq[uq++] = repBB( f, series, points, R, inner, out, &call );
               bbFaults += minorFaults() - f0;
            }
         }
         printf( "%s batch=%.3f kern=%.3f ratio=%.3f reps=%d/%d,%d/%d bb=%.3f bbratio=%.3f bbreps=%d/%d", f->name,
                 lowest( tb, ub ), lowest( tk, uk ), lowest( tk, uk ) / lowest( tb, ub ), ub, maxreps, uk, maxreps,
                 lowest( tq, uq ), lowest( tq, uq ) / lowest( tb, ub ), uq, maxreps );
         if( faults )
            printf( " bflt=%.3f bbflt=%.3f", (double)bFaults / ((double)ub * inner), (double)bbFaults / ((double)uq * inner) );
         printf( "\n" );
      }
      fflush( stdout );
   }
   return 0;
}

/* restart: is a batch call over [s, e] bitwise the same bars as one call over
 * the whole range? */
typedef struct {
   int found, start, bar, k, count;
   double maxRel;
   char refTxt[64], gotTxt[64];
} RDiff;

static void elemText( const KFunc *f, int k, const void *p, char *buf, size_t len )
{
   if( f->outInt[k] )
      snprintf( buf, len, "%d", *(const int *)p );
   else
      snprintf( buf, len, "%.17g", *(const double *)p );
}

static double relDiff( const KFunc *f, int k, const void *a, const void *b )
{
   double x, y, m;
   if( f->outInt[k] )
   {
      x = *(const int *)a;
      y = *(const int *)b;
   }
   else
   {
      x = *(const double *)a;
      y = *(const double *)b;
   }
   if( x != x || y != y )
      return x != x && y != y ? 0.0 : HUGE_VAL;
   m = fabs( x ) > fabs( y ) ? fabs( x ) : fabs( y );
   return m > 0.0 ? fabs( x - y ) / m : 0.0;
}

static void noteRange( RDiff *d, int s, const char *what )
{
   d->count++;
   if( !d->found )
   {
      d->found = 1;
      d->start = s;
      d->bar = s;
      d->k = 0;
      snprintf( d->refTxt, sizeof d->refTxt, "%s", "range" );
      snprintf( d->gotTxt, sizeof d->gotTxt, "%s", what );
   }
   d->maxRel = HUGE_VAL;
}

/* Calls [s, e] and compares it with ref, whose first bar is refBeg. */
static void restartCall( const KFunc *f, const double *const *in, int s, int e, void *const *ref, int refBeg,
                         void *const *got, RDiff *d )
{
   char what[64];
   int beg = -1, nb = -1, i, k, bad;
   double r;
   TA_RetCode rc = f->rbatch( s, e, in, got, &beg, &nb );
   if( rc != TA_SUCCESS || beg != s || nb != e - s + 1 )
   {
      snprintf( what, sizeof what, "rc %d outBegIdx %d outNBElement %d", (int)rc, beg, nb );
      noteRange( d, s, what );
      return;
   }
   for( i = 0; i < nb; i++ )
   {
      bad = 0;
      for( k = 0; k < f->nOut; k++ )
      {
         const void *a = at( ref[k], f, k, s - refBeg + i ), *b = at( got[k], f, k, i );
         if( memcmp( a, b, esz( f, k ) ) == 0 )
            continue;
         r = relDiff( f, k, a, b );
         if( r > d->maxRel )
            d->maxRel = r;
         if( !d->found )
         {
            d->found = 1;
            d->start = s;
            d->bar = s + i;
            d->k = k;
            elemText( f, k, a, d->refTxt, sizeof d->refTxt );
            elemText( f, k, b, d->gotTxt, sizeof d->gotTxt );
         }
         bad = 1;
      }
      d->count += bad;
   }
}

/* 0 exact, 1 differs, 2 skipped. */
static int restartOne( const KFunc *f, double *const *series, int n, void *const *ref, void *const *got, const char *shape )
{
   static const int offs[8] = { 1, 2, 7, 63, 64, 65, 500, 1000 };
   const double *in[MAX_IO];
   RDiff d;
   int beg = 0, nb = 0, j, s, tried = 0;
   TA_RetCode rc;

   memset( &d, 0, sizeof d );
   shiftIn( f, series, 0, in );
   rc = f->rbatch( 0, n - 1, in, ref, &beg, &nb );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "%s: %s skipped: batch returned %d with %d outputs\n", shape, f->name, (int)rc, nb );
      return 2;
   }
   if( beg + nb != n )
   {
      printf( "%s: %s skipped: whole-range output ends at bar %d, not %d\n", shape, f->name, beg + nb - 1, n - 1 );
      return 2;
   }
   for( j = 0; j < 9; j++ )
   {
      s = j < 8 ? beg + offs[j] : n - 2;
      if( s <= beg || s > n - 1 || (j == 8 && s == beg + offs[7]) )
         continue;
      restartCall( f, in, s, n - 1, ref, beg, got, &d );
      tried++;
   }
   s = beg + 100;
   if( s + 511 <= n - 1 )
   {
      restartCall( f, in, s, s + 511, ref, beg, got, &d );
      tried++;
   }
   if( !tried )
   {
      printf( "%s: %s skipped: no restart start after outBegIdx %d\n", shape, f->name, beg );
      return 2;
   }
   if( !d.found )
   {
      printf( "%s: %s exact\n", shape, f->name );
      return 0;
   }
   printf( "%s: %s differs at start %d bar %d output %d: %s vs %s (first of %d differing bars over all starts; max |rel "
           "diff| %.3g)\n",
           shape, f->name, d.start, d.bar, d.k, d.refTxt, d.gotTxt, d.count, d.maxRel );
   return 1;
}

/* Every price and the volume of one bar in 97 times 1000, still in cents. */
static void spikeSeries( int n, double *const *s )
{
   int i, q;
   for( i = 48; i < n; i += 97 )
      for( q = 0; q < NSERIES; q++ )
         s[q][i] = cents( s[q][i] * 1000.0 );
}

static int runRestart( void )
{
   static const char *const verdict[3] = { "exact", "differs", "skipped" };
   const int n = 3000;
   double *series[NSERIES];
   void *ref[MAX_IO], *got[MAX_IO];
   int *res[2], sh, fi, tot[2][3], both = 0, anyDiff = 0, skippedBoth = 0;
   const char *shapes[2] = { "walk", "spike" };

   allocSeries( series, (size_t)n );
   allocOuts( ref, (size_t)n );
   allocOuts( got, (size_t)n );
   memset( tot, 0, sizeof tot );
   for( sh = 0; sh < 2; sh++ )
   {
      res[sh] = calloc( (size_t)NRFUNC, sizeof(int) );
      if( !res[sh] )
         return 2;
      makeSeries( 1, n, series );
      if( sh == 1 )
         spikeSeries( n, series );
      for( fi = 0; fi < NRFUNC; fi++ )
      {
         res[sh][fi] = restartOne( &rfuncs[fi], series, n, ref, got, shapes[sh] );
         tot[sh][res[sh][fi]]++;
      }
      printf( "%s: %d exact, %d differs, %d skipped of %d\n", shapes[sh], tot[sh][0], tot[sh][1], tot[sh][2], NRFUNC );
      fflush( stdout );
   }
   printf( "\nverdicts:\n" );
   for( fi = 0; fi < NRFUNC; fi++ )
   {
      int w = res[0][fi], p = res[1][fi], k, ints = 0;
      for( k = 0; k < rfuncs[fi].nOut; k++ )
         ints += rfuncs[fi].outInt[k];
      printf( "%s walk: %s, spike: %s [%s]\n", rfuncs[fi].name, verdict[w], verdict[p],
              ints == 0 ? "real" : ints == rfuncs[fi].nOut ? "integer" : "mixed" );
      both += w == 0 && p == 0;
      anyDiff += w == 1 || p == 1;
      skippedBoth += w != 1 && p != 1 && (w == 2 || p == 2);
   }
   printf( "restart: %d exact on both shapes, %d differ on at least one, %d skipped (no difference seen) of %d\n", both,
           anyDiff, skippedBoth, NRFUNC );
   return 0;
}

int main( int argc, char **argv )
{
   if( TA_Initialize() != TA_SUCCESS )
      return 2;
   if( getenv( "KERN_PERIOD" ) )
      gPeriod = atoi( getenv( "KERN_PERIOD" ) );
#ifdef __APPLE__
   pthread_set_qos_class_self_np( QOS_CLASS_USER_INITIATED, 0 );
#endif
   if( argc >= 2 && strcmp( argv[1], "check" ) == 0 )
      return runCheck();
   if( argc >= 2 && strcmp( argv[1], "restart" ) == 0 )
      return runRestart();
   if( argc >= 5 && strcmp( argv[1], "time" ) == 0 )
      return runTime( atoi( argv[2] ), atoi( argv[3] ), atoi( argv[4] ) );
   if( argc >= 3 && strcmp( argv[1], "spin" ) == 0 )
   {
      double until = nowNs() + 1e9 * atof( argv[2] );
      volatile double x = 0.0;
      while( nowNs() < until )
         x += 1.0;
      return 0;
   }
   printf( "usage: kern_harness check | restart | time <points> <block> <maxreps> | spin <secs>\n"
           "env: KERN_PERIOD (every optInTimePeriod), KERN_WARM_MS (time; default 300),\n"
           "     KERN_ONLY=NAME1,NAME2 (check and time), KERN_FAULTS=1 (time: minor faults per call)\n" );
   return 2;
}
"#;
