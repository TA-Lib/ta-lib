/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflconv.h
    Title:      Conversion functions
    Package:    Standard Function Library (SFL)

    Written:    1995/12/17  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/10/19

    Synopsis:   These functions provide conversion between a set of datatypes
                (dates, times, numbers, Booleans) and external strings that
                represent the values.  The objective is to format datatypes
                for display or printing, and to validate and convert strings
                supplied by the user.  Conversion is controlled by a set of
                options specific to each datatype.  Additionally, dates and
                times may be formatted using picture strings.  The functions
                were written for use in an interactive 'forms' environment.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLCONV_INCLUDED               /*  Allow multiple inclusions        */
#define SFLCONV_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

char  *conv_number_str  (const char *number, int flags, char point,
                         int decimals, int decimal_format, int width,
                         int sign_format);
char  *conv_str_number  (const char *string, int flags, char point,
                         int decimals, int decimal_format, int width);
char  *conv_date_str    (long date, int flags, int format, int order,
                         char datesep, int width);
long   conv_str_date    (const char *string, int flags, int format, int order);
int    conv_str_day     (const char *day_name);

char  *conv_time_str    (long time, int flags, char timesep, int width);
long   conv_str_time    (const char *string);

char  *conv_bool_str    (Bool boolean, int format);
int    conv_str_bool    (const char *string);

char  *conv_time_pict   (long time, const char *picture);
char  *conv_date_pict   (long date, const char *picture);

/** Not Yet Implemented **/
char  *conv_float_str   (double number, int flags, int sign_format, char
                         point, int decimals, int decimal_format, int width
                        /*  Srcdoc: IGNORE                                   */
                        );
/** Not Yet Implemented **/
double conv_str_float   (char *string, int flags, int sign_format,
                         char point, int decimals, int decimal_format
                        /*  Srcdoc: IGNORE                                   */
                        );

#ifdef __cplusplus
}
#endif

#define FORMAT_MAX              80      /*  Max. size of formatted field     */
#define CONV_MAX_DECS           100     /*  Up to 100 decimal positions      */


/*  Global variables for error reporting                                     */

extern int   conv_reason;               /*  Reason for last conversion error */
                                        /*  0 = okay; >0 = error             */
extern char *conv_reason_text [];       /*  Array of error messages 1..n     */
                                        /*  Index using conv_reason          */

/*  Possible values for conv_reason                                          */

#define CONV_NO_ERRORS             0    /*  No errors                        */

#define CONV_ERR_INVALID_INPUT     1    /*  Unrecognised char in input       */
#define CONV_ERR_OUT_OF_RANGE      2    /*  Value out of valid range         */

/*  conv_str_bool ()                                                         */

#define CONV_ERR_NOT_BOOLEAN       3    /*  Not a yes/no or true/false value */

/*  conv_str_time ()                                                         */

#define CONV_ERR_MULTIPLE_AM       4    /*  More than one 'am' or 'pm'       */

/*  conv_date_str ()                                                         */

#define CONV_ERR_DATE_OVERFLOW     5    /*  Result too large for output      */

/*  conv_str_date ()                                                         */

#define CONV_ERR_DATE_SIZE         6    /*  Too few or too many digits       */
#define CONV_ERR_MULTIPLE_DELIM    7    /*  Too many delimiters              */
#define CONV_ERR_BAD_MONTH         8    /*  Unknown month name               */
#define CONV_ERR_REJECT_3_5        9    /*  3/5 digits in a row not allowed  */
#define CONV_ERR_MULTIPLE_MONTH   10    /*  More than one month name         */

/*  conv_number_str ()                                                       */

#define CONV_ERR_DECS_MISSING     11    /*  Not enough decimals supplied     */
#define CONV_ERR_NUM_OVERFLOW     12    /*  Result too large for output      */

/*  conv_str_number ()                                                       */

#define CONV_ERR_MULTIPLE_SIGN    13    /*  More than one sign character     */
#define CONV_ERR_SIGN_REJECTED    14    /*  Sign not allowed if unsigned     */
#define CONV_ERR_SIGN_BAD_FIN     15    /*  Malformed financial negative     */
#define CONV_ERR_MULTIPLE_POINT   16    /*  More than one decimal point      */
#define CONV_ERR_DECS_REJECTED    17    /*  Decimals not allowed if integer  */
#define CONV_ERR_DECS_HIDDEN      18    /*  Decimals not allowed if hidden   */
#define CONV_ERR_DECS_OVERFLOW    19    /*  Too many decimal positions       */
#define CONV_ERR_TOO_MANY_DIGITS  20    /*  Too many digits for number       */


/*  Constants used for dedicated formatting functions                        */

#define DATE_ORDER_FIRST       1       /*  Values for date_order            */
#define DATE_ORDER_YMD         1
#define DATE_ORDER_DMY         2
#define DATE_ORDER_MDY         3
#define DATE_ORDER_LAST        3

#define FLAG_N_SIGNED           1       /*  Number field flags               */
#define FLAG_N_DECIMALS         2
#define FLAG_N_LEFT             4
#define FLAG_N_ZERO_FILL        8
#define FLAG_N_ZERO_BLANK      16
#define FLAG_N_THOUSANDS       32

#define SIGN_NEG_TRAIL          1       /*  Number field formatting          */
#define SIGN_ALL_TRAIL          2
#define SIGN_NEG_LEAD           3
#define SIGN_ALL_LEAD           4
#define SIGN_FINANCIAL          5

#define DECS_SHOW_ALL           1
#define DECS_DROP_ZEROS         2
#define DECS_HIDE_ALL           3
#define DECS_SCIENTIFIC         4

#define DATE_FORMAT_FIRST      0       /*  Date field formatting            */
#define DATE_YMD_COMPACT        0
#define DATE_YMD_DELIM          1
#define DATE_YMD_SPACE          2
#define DATE_YMD_COMMA          3
#define DATE_YMD_LAST          3
#define DATE_YM_COMPACT         4
#define DATE_YM_DELIM           5
#define DATE_YM_SPACE           6
#define DATE_YM_LAST           6
#define DATE_MD_COMPACT         7
#define DATE_MD_DELIM           8
#define DATE_MD_SPACE           9
#define DATE_MD_LAST           9
#define DATE_FORMAT_LAST       9

#define FLAG_D_DD_AS_D          1       /*  Date field flags                 */
#define FLAG_D_MM_AS_M          2
#define FLAG_D_MONTH_ABC        4
#define FLAG_D_CENTURY          8
#define FLAG_D_UPPER           16
#define FLAG_D_ORDER_YMD       32
#define FLAG_D_ORDER_DMY       64
#define FLAG_D_ORDER_MDY      128

#define FLAG_T_HH_AS_H          1       /*  Time field flags                 */
#define FLAG_T_MM_AS_M          2
#define FLAG_T_SS_AS_S          4
#define FLAG_T_CC_AS_C          8
#define FLAG_T_COMPACT         16
#define FLAG_T_12_HOUR         32

#define BOOL_YES_NO             0       /*  Boolean field formatting         */
#define BOOL_Y_N                1
#define BOOL_TRUE_FALSE         2
#define BOOL_T_F                3
#define BOOL_1_0                4

#endif
