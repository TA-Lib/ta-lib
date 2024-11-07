/****************************************************************************/
/* File : ta_libc.python.swg 
 * Python typemaps for all TA interfaces
 * !!! TODO: UNFINISHED !!!
 */

/* This file has to define the following typemap macros:
 *      STRING_TABLE
 *      CONST_STRING_TABLE
 *      STRING_BUFFER
 *      HANDLE
 *      CONST_HANDLE
 *      OBJECT
 *      ARRAY_OBJECT
 *
 * It has also to define typemaps for:
 *      TA_Real *ARRAY
 *      TA_Integer *ARRAY
 * and generic typemap macros for ARRAYs:
 *      MEMBER_ARRAY
 *      MEMBER_PTRARRAY
 * 
 * Also some typemaps have to be defined for specific TA interface files 
 * (see below).
 */

 
/* In Python we can use exception handling instead TA_RetCode.
 * If you need the old method, you can comment out this typemap.
 */
%typemap(out) TA_RetCode
{
	if( $1 != TA_SUCCESS )
	{
	 	TA_RetCodeInfo info;
		char text[200];
		TA_SetRetCodeInfo( $1, &info );
		snprintf(text, sizeof(text)-1, "Error %d (%s): %s\n", $1, info.enumStr, info.infoStr );
		PyErr_SetString(PyExc_Exception, text);
		SWIG_fail;
	} 
	Py_INCREF(Py_None); 
	$result = Py_None;
}


%define STRING_TABLE(TYPE, SIZE_FIELD)
    %typemap(in,numinputs=0) TYPE** (TYPE *temp = 0)
        "$1 = &temp;"
    %typemap(argout) TYPE**
    {
        /* Copy the strings from **$1 to a Python list */
        PyObject * list = PyList_New((*$1)->SIZE_FIELD);
        int elem;
        for (elem = 0;  elem < (*$1)->SIZE_FIELD;  elem++)
        {
            PyObject * name = PyString_FromString((*$1)->string[elem]);
            PyList_SetItem(list, elem, name);
        }
        $result = SWIG_Python_AppendOutput($result, list);
        $symnameFree(*$1);
    }
    /* we don't want just wrapper classes around */
    %ignore TYPE;
%enddef /* STRING_TABLE */



%define CONST_STRING_TABLE(TYPE, SIZE_FIELD)
/* TODO */
%ignore TYPE;
%enddef /* CONST_STRING_TABLE */



%define STRING_BUFFER(TYPE, SIZE_FIELD, LEN_FIELD)
/* TODO */
%ignore TYPE;
%enddef /* STRING_BUFFER */



%define HANDLE(TYPE, NAME)
/* TODO */
%nodefault TYPE;
%enddef /* HANDLE */



%define CONST_HANDLE(TYPE, NAME)
    %typemap(in, numinputs=0) const TYPE **NAME (TYPE *temp = 0)
        "$1 = &temp;"
    %typemap(argout) const TYPE **NAME
    {
        PyObject *obj = SWIG_NewPointerObj((void *) *$1, $*1_descriptor, 0);
        /*SWIG_croak("Type error in argument $argnum of $symname. Expected $*1_mangle");*/
        $result = SWIG_Python_AppendOutput($result, obj);
    }
    %nodefault TYPE;
%enddef /* CONST_HANDLE */



%define OBJECT(TYPE, NAME)
/* TODO */
%nodefault TYPE;
%enddef  /* OBJECT */



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



%define MEMBER_ARRAY(TYPE)
	%typemap(out) const TYPE *ARRAY
	{
	    int idx;
	    $result = PyList_New(array_size1);
	    Py_INCREF($result);
	    for (idx = 0; idx < array_size1; idx++)
	    {
	        PyObject *o = SWIG_NewPointerObj((void *) &$1[idx], $1_descriptor, 1);
	        PyList_SET_ITEM($result,idx,o);
	    }
	}
%enddef /* MEMBER_ARRAY */



%define MEMBER_PTRARRAY(TYPE)
/* TODO */
%enddef /* MEMBER_PTRARRAY */



/* typemaps for ta_common ***************************************************/

/* complex typemap for TA_FatalReportToBuffer */
/* TODO */



/* typemaps for ta_func *****************************************************/

/* The following typemaps have to be defined:
    START_IDX, END_IDX,
    IN_ARRAY
    OPT_INT, OPT_REAL
    BEG_IDX, OUT_SIZE,
    OUT_ARRAY
*/

/* helper functions for arrays */

%{
static int convert_darray(PyObject *input, double *ptr, int size) {
    int i;
    int len;

    if (!PySequence_Check(input)) {
        PyErr_SetString(PyExc_TypeError,"Expecting a sequence");
        return 0;
    }
    len = PyObject_Length(input);
    if (len > size) len = size;
    for (i =0; i < len; i++) {
        PyObject *o = PySequence_GetItem(input,i);
        if (!PyFloat_Check(o)) {
            PyErr_SetString(PyExc_ValueError,"Expecting a sequence of floats");
            Py_DECREF(o);
            return 0;
        }
        ptr[i] = PyFloat_AsDouble(o);
        Py_DECREF(o);
    }
    return 1;
}
%}


/* START_IDX and END_IDX are always the first two arguments of all functions.
 * They create two local (auto) variables, startIdx1 and endIdx2
 * Other typemaps depend on them to allocate properly sized C arrays.
 */
%typemap(in) int START_IDX (int startIdx)
{
    $1 = (int) PyLong_AsLong($input);
    if ($1 < 0) $1 = 0;                 /* ensure valid array range */
    startIdx = $1;
}

%typemap(in) int END_IDX (int endIdx)
{
    $1 = (int) PyLong_AsLong($input);
    if ($1 < startIdx1) $1 = startIdx1;  /* ensure valid array range */
    endIdx = $1;
}


/* Input arrays are passed as lists.
 * As it is implemented now, startIdx and endIdx values overrule the actual
 * array size; eventual missing elements are created and set to zero.
 */
%typemap(in) const double *IN_ARRAY
{
    int array_size = endIdx2 + 1;

    $1 = ($1_ltype) calloc(array_size, sizeof($*1_ltype));
    if (!convert_darray($input, $1, array_size))  goto fail;
}

%typemap(in) const float *IN_ARRAY
{
    PyErr_SetString(PyExc_TypeError,"float arrays are not supported in Python; use double");
    goto fail;
}

%typemap(in) const int *IN_ARRAY
{
    int array_size = endIdx2 + 1;
    int i;
    int pylen;

    if (!PySequence_Check($input)) {
        PyErr_SetString(PyExc_TypeError,"Expecting a sequence");
        return 0;
    }
    pylen = PyObject_Length($input);

    $1 = ($1_ltype) calloc(array_size, sizeof($*1_ltype));
    for (i=0; i<min(pylen, array_size); i++)
    {
        PyObject *obj = PySequence_GetItem($input,i);
        long value = PyInt_AsLong(obj);
        $1[i] = value;
        Py_DECREF(obj);
    }
}

%typemap(freearg) const double *IN_ARRAY
    "free($1);";

%typemap(freearg) const int *IN_ARRAY
    "free($1);";

/* After the input arrays, optional parameters come.
 * If not provided or given as 'undef', they will be translated to
 * TA_INTEGER_DEFAULT or TA_REAL_DEFAULT, depending on the type.
 */
%typemap(default) int OPT_INT
    "$1 = TA_INTEGER_DEFAULT;"

%typemap(in) int OPT_INT 
%{
    if (PyInt_Check($input)) {
        $1 = ($1_ltype) PyInt_AsLong($input);
    }
%}

%typemap(default) double OPT_REAL
    "$1 = TA_REAL_DEFAULT;"

%typemap(in) double OPT_REAL 
%{
    if (PyFloat_Check($input)) {
        $1 = ($1_ltype) PyFloat_AsDouble($input);
    }
%}

/* outBegIdx is handled just as a regular OUTPUT parameter, except that
 * if the function failed, None is returned
 */
%typemap(in,numinputs=0) int *BEG_IDX(int temp = 0)
    "$1 = &temp;";

%typemap(argout) int *BEG_IDX
{
    if (result == TA_SUCCESS) {
        PyObject *o = PyInt_FromLong((long) (*$1));
        $result = SWIG_Python_AppendOutput($result, o);
    }
}


/* Parameter outNbElement is an out value, but is not passed back to Python.
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
 * given so that they do not have to be provided in the function call.
 * However, in such case, they are placed at the very top of the wrapper 
 * function, before other inputs (SWIG bug?), making startIdx and endIdx
 * not accessible in the 'in' typemap.
 * Therefore the 'in' typemap does not generate any code, 
 * but the array allocation is done in the 'check' typemap.
 */
%typemap(in,numinputs=0) double *OUT_ARRAY, int *OUT_ARRAY
    "/* $1 ignored on input */";

%typemap(check) double *OUT_ARRAY, int *OUT_ARRAY
{
    int array_size = endIdx2 - startIdx1 + 1;
    $1 = ($1_ltype) calloc(array_size, sizeof($*1_ltype));
}

/* On output, the arrays are converted to Python arrays and returned as
 * elements oth a tuple
 */
%typemap(argout) double *OUT_ARRAY
{
    if ( result == TA_SUCCESS ) {
        int idx;
        PyObject *list = PyList_New(outNbElement);

        for (idx = 0; idx < outNbElement; idx++) {
            PyObject *o = PyFloat_FromDouble($1[idx]);
            PyList_SET_ITEM(list,idx,o);
        }
        $result = SWIG_Python_AppendOutput($result, list);
    }
}

%typemap(argout) int *OUT_ARRAY
{
    if ( result == TA_SUCCESS ) {
        int idx;
        PyObject *list = PyList_New(outNbElement);

        for (idx = 0; idx < outNbElement; idx++) {
            PyObject *o = PyInt_FromLong($1[idx]);
            PyList_SET_ITEM(list,idx,o);
        }
        $result = SWIG_Python_AppendOutput($result, list);
    }
}

/* It is safer to have a separate typemap for cleanup rather than doing it
 * at the end of the 'argout' typemap.
 * If there were 'goto fail' between the inlined typemap 'check' and 
 * the typemap 'argout', memory leak could occur.
 * The code of the typemap 'freearg' will always be executed.
 */
%typemap(freearg) double *OUT_ARRAY, int *OUT_ARRAY
    "free($1);";





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





/* typemaps for ta_abstract *************************************************/
%typemap(out) const void *dataSet
%{
    if ($1)
    {
        swig_type_info *retType = NULL;
        switch (arg1->type)
        {
            case TA_OptInput_RealRange    : retType = SWIGTYPE_p_TA_RealRange;    break;
            case TA_OptInput_RealList     : retType = SWIGTYPE_p_TA_RealList;     break;
            case TA_OptInput_IntegerRange : retType = SWIGTYPE_p_TA_IntegerRange; break;
            case TA_OptInput_IntegerList  : retType = SWIGTYPE_p_TA_IntegerList;  break;
        }
        if (retType)
        {
            $result = SWIG_NewPointerObj((void *) $1, retType, 1);
            Py_INCREF($result);
        }
    }
%}
