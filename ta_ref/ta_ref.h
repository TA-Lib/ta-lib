/* The contract between the serve glue (ta_ref_serve.c) and one frozen-release
 * member (ta_ref_<X>_<Y>_<Z>.c). Zero or NULL in a member field means "no
 * carve-out", so a field added here never touches an existing member. */
#ifndef TA_REF_H
#define TA_REF_H

/* The bound one tolerance row puts on |current - frozen| for an output element. */
typedef enum
{
   TA_REF_TOL_ABS,             /* tol                                              */
   TA_REF_TOL_REL_IN,          /* tol * max|close| over the case, capped at cap    */
   TA_REF_TOL_REL_OUT,         /* tol * max(|current|, |frozen|)                   */
   TA_REF_TOL_REL_OUT_INFLOOR, /* tol * max(|current|, |frozen|, max|close|)       */
   TA_REF_TOL_NAN_TO           /* frozen is NaN and current is exactly tol         */
} TaRefTolMode;

typedef struct
{
   const char  *func;   /* a function name, or "*" for every function with no row */
   TaRefTolMode mode;
   double       tol;
   double       cap;    /* TA_REF_TOL_REL_IN only; 0 is uncapped */
} TaRefTol;

/* A class of cases the frozen release is not an oracle for. */
typedef struct
{
   const char *id;
   const char *why;
   double      maxFraction; /* ceiling on the share of one function's cases */
} TaRefWaiver;

/* One fuzz case, as the waiver hook sees it. The series are the seed-generated
 * ones, before any real input is mapped onto them; lookback is the frozen
 * library's, taken after the case's unstable periods are set, or -1 when it
 * cannot be computed. */
typedef struct
{
   const char   *func;
   const double *open, *high, *low, *close, *volume;
   int           n, startIdx, endIdx, lookback;
   int           nbOpt;
   const char *const *optName;
   const double *optValue;
} TaRefCase;

/* Opt param `name` of the case, or `dflt` when the function has none. */
double ta_ref_opt( const TaRefCase *c, const char *name, double dflt );

typedef struct
{
   const char        *commit;      /* the release tag's commit, 40 hex digits    */
   int                nbFunctions; /* functions in that release                  */
   int                intFloor;    /* lowest IntegerRange value swept; 0 = the declared minimum */
   const char *const *excluded;    /* NULL-terminated                            */
   const TaRefTol    *tol;
   int                nbTol;
   const TaRefWaiver *waivers;
   int                nbWaivers;
   int              (*waive)( const TaRefCase *c ); /* index into waivers, or -1 */
} TaRefMember;

extern const TaRefMember ta_ref_member;

#endif
