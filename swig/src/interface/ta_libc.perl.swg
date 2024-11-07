/****************************************************************************/
/* File : ta_libc.perl.swg 
 * Perl typemaps for all TA interfaces
 */

/* Adittional typemap not found in the standard set *************************/

/* Not working with ActivePerl, alas
%typemap(perl5,in) FILE * {
   $1 = IoIFP(sv_2io($input));
   $1 = PerlIO_exportFILE(IoIFP(sv_2io($input)), 0);
}
*/


/* Some TA functions operate on (return) string tables
 * being (pointers to) arrays of pointers to char arrays
 * and having a size field.
 * They are being converted to native Perl lists of strings.
 */
%define STRING_TABLE(TYPE, SIZE_FIELD)

%typemap(in,numinputs=0) TYPE** (TYPE *temp = 0)
    "$1 = &temp;"

%typemap(argout) TYPE** 
{
    /* Copy the strings from **$1 to a Perl array */
    if ( *$1 && (*$1)->SIZE_FIELD > 0) {
        int new_items = argvi + (*$1)->SIZE_FIELD;
        unsigned int elem;
        if (new_items > items) {
            EXTEND(sp,(new_items-items));
        }
        for (elem = 0;  elem < (*$1)->SIZE_FIELD;  elem++) {
            ST(argvi) = sv_newmortal();
            if ((*$1)->string[elem]) {
                sv_setpv((SV*)ST(argvi++), (char *) (*$1)->string[elem]);
            } else {
                sv_setsv((SV*)ST(argvi++), &PL_sv_undef);
            }
        }
    }
    $symnameFree(*$1);
}

/* we don't want just wrapper classes around */
%ignore TYPE;

%enddef /* STRING_TABLE */



/* Some string tables are not dynamically created 
 * but defined at compilation time.
 * They must not be deallocated and use only one indirection.
 */
%define CONST_STRING_TABLE(TYPE, SIZE_FIELD)

%typemap(out) TYPE* 
{
    /* Copy the strings from *$1 to a Perl array */
    if ( $1->SIZE_FIELD > 0) {
        int new_items = argvi + $1->SIZE_FIELD;
        unsigned int elem;
	    if (new_items > items) {
                EXTEND(sp,(new_items-items));
	    }
        for (elem = 0;  elem < $1->SIZE_FIELD;  elem++) {
            ST(argvi) = sv_newmortal();
            if ($1->string[elem]) {
                sv_setpv((SV*)ST(argvi++), (char *) $1->string[elem]);
            } else {
                sv_setsv((SV*)ST(argvi++), &PL_sv_undef);
            }
        }
    }
}

/* we don't want just wrapper classes around */
%ignore TYPE;

%enddef /* CONST_STRING_TABLE */


/* Some other string tables are flat, i.e. there is only one string pointer
 * and all strings (fixed length) are one after another.
 */
%define STRING_BUFFER(TYPE, SIZE_FIELD, LEN_FIELD)

%typemap(in,numinputs=0) TYPE** (TYPE *temp = 0)
    "$1 = &temp;"

%typemap(argout) TYPE** 
{
    /* Copy the strings from **$1 to a Perl array */
    if ( *$1 && (*$1)->SIZE_FIELD > 0) {
        int new_items = argvi + (*$1)->SIZE_FIELD;
        unsigned int elem;
        if (new_items > items) {
            EXTEND(sp,(new_items-items));
        }
        for (elem = 0;  elem < (*$1)->SIZE_FIELD;  elem++) {
            ST(argvi) = sv_newmortal();
            sv_setpv((SV*)ST(argvi++), (char *) (*$1)->buffer+(elem*(*$1)->LEN_FIELD));
        }
    }
    $symnameFree(*$1);
}

/* we don't want just wrapper classes around */
%ignore TYPE;

%enddef /* STRING_BUFFER */



/* Handles are created and released only by appropriate calls to TA-Lib functions
 * They have no accessible fields
 * To obtain a handle in Perl, use references
 * Example: 
 *   $retCode = TA_HandleAlloc(\$handle);
 *   $retCode = TA_HandleFree($handle);
 *
 * However, for all TA handles, Perl shadow classes are defined, simplifying
 * creaction and destruction.
 */
%define HANDLE(TYPE, NAME)

%typemap(in) TYPE **NAME($*1_ltype temp) 
{
    SV *tempsv;
    if (!SvROK($input)) {
        SWIG_croak("Expected a reference as argument $argnum of $symname");
    }
    tempsv = SvRV($input);
    if (SWIG_ConvertPtr(tempsv, (void **) &temp, $*1_descriptor, 0) < 0) {
        SWIG_croak("Type error in argument $argnum of $symname. Expected $*1_mangle");
    }
    if (temp) {
        SWIG_croak("Expected a reference to undef variable as argument $argnum of $symname");
    }
    $1 = &temp;
}

%typemap(argout) TYPE **NAME 
{
    SV *tempsv;
    tempsv = SvRV($input);
    SWIG_MakePtr(tempsv, (void *) *$1, $*1_descriptor, SWIG_SHADOW);
}

/* Don't create default constructors/destructors  */
%nodefault TYPE;

%enddef /* HANDLE */



/* For const handles returned, Perl refs are used too, but the objects need not be created */
/* (const handles are used in ta_abstract) */
%define CONST_HANDLE(TYPE, NAME)

%typemap(in) const TYPE **NAME($*1_ltype temp = 0) 
{
    $1 = &temp;
    if (!SvROK($input)) {
        SWIG_croak("Expected a reference as argument $argnum of $symname");
    }
}

%typemap(argout) const TYPE **NAME 
{
    SV *tempsv;
    tempsv = SvRV($input);
    SWIG_MakePtr(tempsv, (void *) *$1, $*1_descriptor, SWIG_SHADOW);
}

/* Don't create default constructors/destructors. 
 */
%nodefault TYPE;

%enddef /* CONST_HANDLE */




/* typemaps for TA objects like TA_History */

/* Objects are created as argout (just to be different than handles ;-)
 * Example:
 * @res = TA_HistoryAlloc($udb, $history_param);
 * $history = $res[1] if $res[0] == $TA_SUCCESS;
 *
 * Clumsy, huh?
 * However in practice, you will use 'new' anyway...
 */

%define OBJECT(TYPE, NAME)

%typemap(in,numinputs=0) TYPE **NAME($*1_ltype temp = 0) 
	"$1 = &temp;";

%typemap(argout) TYPE **NAME 
%{
    if (*$1) {
	if (argvi >= items) {
	    EXTEND(sp,1);
	}
	$result = sv_newmortal();
	SWIG_MakePtr($result, (void *) *$1, $*1_descriptor, SWIG_SHADOW|SWIG_OWNER);
	argvi++;
    }
%}

/* Don't create default constructors/destructors. 
 */
%nodefault TYPE;

%enddef  /* OBJECT */


/* typemaps for "array objects," that is, objects holding arrays as member data,
 * like TA_History, TA_PMArray...
 */

%define ARRAY_OBJECT(TYPE, SIZE_FIELD)

/* Each array object saves its size in array_size
 * This will later be needed when acessing the member arrays
 * This typemap does not perform a conversion, that's why it
 * cannot be type 'in'.
 */
%typemap(check) TYPE* (unsigned int array_size)
    "array_size = ($1 != NULL)? ($1->SIZE_FIELD) : 0;"

/* SIZE_FIELD must not be modified, otherwise segmentation fault may occur */
%immutable SIZE_FIELD;

%enddef /* ARRAY_OBJECT */

/* Is that all?  Not really.  Array objects may have various number
 * of member arrays, of various types.  The following ARRAY typemaps
 * serve to access individual member arrays.
 * But how to figure out how many elements the arrays have? 
 * This and the following three typemaps rely upon the encompassing structure
 * creating a local variable array_size.
 * That's why each encompassing structure has to be defined as ARRAY_OBJECT.
 * If forgotten to define it, a compile error will occur.
 */
%typemap(out) const TA_Real *ARRAY 
{
    AV *av;
    unsigned int idx = 0;
    SV *sv;

    av = newAV();
    av_extend(av, array_size1-1);
    for (idx = 0; idx < array_size1 ; idx++) {
	sv = newSVnv($1[idx]);
        if (av_store(av, idx, sv) == 0) SvREFCNT_dec(sv);	
    };
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;    
}

%typemap(out) const TA_Integer *ARRAY 
{
    AV *av;
    SV *sv;
    unsigned int idx = 0;

    av = newAV();
    av_extend(av, array_size1-1);
    for (idx = 0; idx < array_size1 ; idx++) {
	sv = newSViv($1[idx]);
        if (av_store(av, idx, sv) == 0) SvREFCNT_dec(sv);	
    };
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;    
}


/* There are also arrays of other types used.  
 * They will be defined by the following typemap macro.
 */
%define MEMBER_ARRAY(TYPE)

%typemap(out) const TYPE *ARRAY 
{
    AV *av;
    SV **svs;
    unsigned int idx = 0;

    /* this is not the most efficient way of creating an array
     * it creates much too many mortals
     * I have cloned it from SWIG example but a better way is in 
     * the typemaps above
     * TODO: optimize it here
     */
    svs = (SV **) malloc(array_size1 * sizeof(SV *));
    for (idx = 0; idx < array_size1 ; idx++) {
        svs[idx] = sv_newmortal();
        SWIG_MakePtr((SV*)svs[idx], (void *) &$1[idx], $1_descriptor, SWIG_SHADOW);
    };
    av =  av_make(array_size1,svs);
    free(svs);
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;    
}

%enddef /* MEMBER_ARRAY */



/* Some member arrays contain just pointers to objects, 
 * not the objects themselves.
 * Therefore the typemap has to be modified slightly.
 */
%define MEMBER_PTRARRAY(TYPE)

%typemap(out) const TYPE **ARRAY 
{
    AV *av;
    SV **svs;
    unsigned int idx = 0;

    /* as in the previous case
     * this is not the most efficient way of creating an array
     * it creates much too many mortals
     * I have cloned it from SWIG example but a better way is in 
     * the typemaps for TA_Integer and TA_Real
     * TODO: optimize it here
     */
    svs = (SV **) malloc(array_size1 * sizeof(SV *));
    for (idx = 0; idx < array_size1 ; idx++) {
        svs[idx] = sv_newmortal();
        SWIG_MakePtr((SV*)svs[idx], (void *) $1[idx], $*1_descriptor, SWIG_SHADOW);
    };
    av =  av_make(array_size1,svs);
    free(svs);
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;
}

%enddef /* MEMBER_PTRARRAY */



/* typemaps for ta_common ***************************************************/


/* typemaps for TA_FatalReportToBuffer */

%typemap(default) ( char *buffer, unsigned int bufferSize )
{
    $1 = NULL;
    $2 = TA_FATAL_ERROR_BUF_SIZE; /* default */
}

%typemap(in) ( char *buffer, unsigned int bufferSize )
{
    $2 = ($2_ltype) SvUV($input);
}

%typemap(check) ( char *buffer, unsigned int bufferSize )
{
    $1 = ($1_ltype) malloc($2);
}

%typemap(argout) ( char *buffer, unsigned int bufferSize )
{
    if (argvi >= items) {
        EXTEND(sp,1);
    }
    $result = sv_newmortal();
    sv_setpv($result, $1);
    argvi++;
}

%typemap(freearg) ( char *buffer, unsigned int bufferSize )
{
    free($1);
}




/* typemaps for ta_func *****************************************************/

/* The following typemaps are defined:
    START_IDX, END_IDX,
    IN_ARRAY
    OPT_INT, OPT_REAL
    BEG_IDX, OUT_SIZE,
    OUT_ARRAY
*/

/* START_IDX and END_IDX are always the first two arguments of all functions.
 * They create two local (auto) variables, startIdx1 and endIdx2
 * Other typemaps depend on them to allocate properly sized C arrays.
 */
%typemap(in) int START_IDX (int startIdx) 
{
    $1 = (int) SvIV($input);
    if ($1 < 0) $1 = 0;                 /* ensure valid array range */
    startIdx = $1;
}

%typemap(in) int END_IDX (int endIdx)
{
    $1 = (int) SvIV($input);
    if ($1 < startIdx1) $1 = startIdx1;  /* ensure valid array range */
    endIdx = $1;
}

/* Input arrays are passed by references, otherwise they would loose
 * their identity.  Besides, it is more effiecient.
 * As it is implemented now, startIdx and endIdx values overrule the actual
 * array size; eventual missing elements are created and set to zero.
 */
%typemap(in) const double *IN_ARRAY, const float *IN_ARRAY 
{
    int array_size = endIdx2 + 1;
    AV *av;
    I32 len;
    int idx;
    SV **sv;

    if (!SvROK($input) || SvTYPE(SvRV($input)) != SVt_PVAV) {
        SWIG_croak("Expected an array reference as argument $argnum of $symname");
    }
    av = (AV*)SvRV($input);
    $1 = ($1_ltype) calloc(array_size, sizeof($*1_ltype));
    len = av_len(av);
    if (len >= array_size) len = array_size - 1;
    for (idx = 0; idx <= len; idx++) {
        sv = av_fetch(av, idx, 0);	
        $1[idx] = ($*1_ltype) SvNV(*sv);
    }
}

%typemap(freearg) const double *IN_ARRAY, const float *IN_ARRAY
    "free($1);";

/* After the input arrays, optional parameters come.
 * If not provided or given as 'undef', they will be translated to
 * TA_INTEGER_DEFAULT or TA_REAL_DEFAULT, depending on the type.
 */
%typemap(default) int OPT_INT
    "$1 = TA_INTEGER_DEFAULT;"

%typemap(in) int OPT_INT 
%{
    if (SvOK($input)) {
        $1 = ($1_ltype) SvIV($input);
    }
%}

%typemap(default) double OPT_REAL
    "$1 = TA_REAL_DEFAULT;"

%typemap(in) double OPT_REAL 
%{
    if (SvOK($input)) {
        $1 = ($1_ltype) SvNV($input);
    }
%}


/* outBegIdx is handled just as a regular OUTPUT parameter, except that
 * if the function failed, undef is returned
 */
%typemap(in,numinputs=0) int *BEG_IDX(int temp = 0)
    "$1 = &temp;";

%typemap(argout) int *BEG_IDX
{
    if (argvi >= items) {
        EXTEND(sp,1);
    }
    $result = sv_newmortal();
    if (result == TA_SUCCESS) {
        sv_setiv($result,(IV) *($1));
    } else {
        sv_setsv($result, &PL_sv_undef);
    }
    argvi++;
}


/* Parameter outNbElement is an out value, but is not passed back to Perl.
 * It is used to construct OUT_ARRAY of proper length.
 * Its position within the parameter list varies, so we cannot use
 * local variables declared in the same way as for startIdx and endIdx.
 * Taking advantage of that 'arginit' typemaps are the very first ones in
 * the wrapper function, we declare an explicitly named local variable.
 */
%typemap(arginit) int *OUT_SIZE
    "int outNbElement = 0;";

%typemap(in,numinputs=0) int *OUT_SIZE
    "$1 = &outNbElement;";

/* As last, output arrays come.  Since they are only output, numinputs=0 is
 * given so that they do not have to be provided in the function call from Perl.
 * However, in such case, they are placed at the very top of the wrapper 
 * function, before other inputs (SWIG bug?), making startIdx and endIdx
 * not accessible in the 'in' typemap.
 * Therefore the 'in' typemap does not generate any code, 
 * but the array allocation is done in the 'check' typemap.
 */
%typemap(in,numinputs=0) double *OUT_ARRAY, float *OUT_ARRAY, int *OUT_ARRAY
    "/* $1 ignored on input */";

%typemap(check) double *OUT_ARRAY, float *OUT_ARRAY, int *OUT_ARRAY
{
    int array_size = endIdx2 - startIdx1 + 1;
    $1 = ($1_ltype) calloc(array_size, sizeof($*1_ltype));
}

/* On output, the arrays are converted to Perl arrays and returned as
 * Perl references on the output stack
 */
%typemap(argout) double *OUT_ARRAY, float *OUT_ARRAY
{
    AV *av;
    int idx;
    SV *sv;

    av = newAV();
    if ( result == TA_SUCCESS && outNbElement > 0) {
        av_extend(av, outNbElement-1);
        for (idx = 0; idx < outNbElement; idx++) {
            sv = newSVnv($1[idx]);
            if (av_store(av, idx, sv) == 0) SvREFCNT_dec(sv);	
        }
    }
    if (argvi >= items) {
        EXTEND(sp,1);
    }
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;
}

%typemap(argout) int *OUT_ARRAY
{
    AV *av;
    int idx;
    SV *sv;

    av = newAV();
    if ( result == TA_SUCCESS && outNbElement > 0) {
        av_extend(av, outNbElement-1);
        for (idx = 0; idx < outNbElement; idx++) {
            sv = newSViv($1[idx]);
            if (av_store(av, idx, sv) == 0) SvREFCNT_dec(sv);	
        }
    }
    if (argvi >= items) {
        EXTEND(sp,1);
    }
    $result = sv_2mortal(newRV_noinc((SV*)av));
    argvi++;
}

/* It is safer to have a separate typemap for cleanup rather than doing it
 * at the end of the 'argout' typemap.
 * If there were SWIG_croak() between the inlined typemap 'check' and 
 * the typemap 'argout', memory leak could occur.
 * The code of the typemap 'freearg' will always be executed.
 */
%typemap(freearg) double *OUT_ARRAY, float *OUT_ARRAY, int *OUT_ARRAY
    "free($1);";



/* typemaps for ta_abstract *************************************************/

/* The field 'dataSet' is a tricky one; it has to be recast from const void*
 * to something sensible depending on the value of 'type'
 */
%typemap(out) const void *dataSet %{
    if ($1) {
        swig_type_info *retType = NULL;

        switch (arg1->type) {
            case TA_OptInput_RealRange    : retType = SWIGTYPE_p_TA_RealRange;    break;
            case TA_OptInput_RealList     : retType = SWIGTYPE_p_TA_RealList;     break;
            case TA_OptInput_IntegerRange : retType = SWIGTYPE_p_TA_IntegerRange; break;
            case TA_OptInput_IntegerList  : retType = SWIGTYPE_p_TA_IntegerList;  break;
        }
        if (retType) {
	    $result = sv_newmortal();
	    SWIG_MakePtr($result, (void *) $1, retType, SWIG_SHADOW);
	    argvi++;
        }
    }
%}

/****************************************************************************/
