/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfllang.c
    Title:      Multilingual number/date/time support
    Package:    Standard Function Library (SFL)

    Written:    1997/06/04  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/01/02

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.

    TDL: - Abstract accents; provide function to translate any marked-up
           string into either accented characters or portable equivalents.
           Use a coding like /Ra for a-ring.
         - Translate day names into other languages.
         - Use US English too?
         - Provide certification function.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"                     /*  String functions                 */
#include "sfldate.h"                    /*  Date functions                   */
#include "sfllist.h"                    /*  Memory-allocation functions      */
#include "sflmem.h"                     /*  Memory-allocation functions      */
#include "sfllang.h"                    /*  Function prototypes              */

static char
    *language_str [] = {
        "--",
        "DA", "DE", "EN", "ES", "FB", "FR",
        "IS", "IT", "NL", "NO", "PO", "SV"
    };

static char
    *DA_units [] = {
        "nul", "en", "to", "tre", "fire", "fem", "seks", "syv", "otte",
        "ni", "ti", "elve", "tolv", "tretten", "fjorten", "femten",
        "seksten", "sytten", "atten", "nitten"
    },
    *DE_units [] = {
        "null", "ein", "zwei", "drei", "vier", "fünf", "sechs", "sieben",
        "acht", "neun", "zehn", "elf", "zw/Uolf", "dreizehn", "vierzehn",
        "f/Uunfzehn", "sechzehn", "siebzehn", "achtzehn", "neunzehn"
    },
    *EN_units [] = {
        "zero", "one", "two", "three", "four", "five", "six", "seven",
        "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
        "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
    },
    *ES_units [] = {
        "zero", "uno", "dos", "tres", "cuatro", "cinco", "seis", "siete",
        "ocho", "nueve", "diez", "once", "doce", "trece", "catorce", "quince",
        "dieciseis", "diecisiete", "dieciocho", "diecinueve"
    },
    *FR_units [] = {
        "zero", "un", "deux", "trois", "quatre", "cinq", "six", "sept",
        "huit", "neuf", "dix", "onze", "douze", "treize", "quatorze",
        "quinze", "seize", "dix-sept", "dix-huit", "dix-neuf"
    },
    *IS_units [] = {
        "null", "einn", "tweir", "thrir", "fjorir", "fimm", "sex", "sj/Uo",
        "atta", "niu", "tiu", "ellefu", "tolf", "threttan", "fjortan",
        "fimtan", "sextan", "seytjan", "atjan", "nitjan"
    },
    *IT_units [] = {
        "zero", "uno", "due", "tre", "quattro", "cinque", "sei", "sette",
        "otto", "nove", "dieci", "undici", "dodici", "tredici", "quatrodici",
        "quindici", "sedici", "diciassette", "diciotto", "dicianove"
    },
    *NL_units [] = {
        "nul", "een", "twee", "drie", "vier", "vijf", "zes", "zeven", "acht",
        "negen", "tien", "elf", "twaalf", "dertien", "veertien", "vijftien",
        "zestien", "zeventien", "achttien", "negentien"
    },
    *NO_units [] = {
        "null", "en", "to", "tre", "fire", "fem", "seks", "syv", "/Ratte",
        "ni", "ti", "elleve", "tolv", "tretten", "fjorten", "femten",
        "seksten", "sytten", "atten", "nitten"
    },
    *PO_units [] = {
        "zero", "um", "dois", "tres", "quatro", "cinco", "seis", "sete",
        "oito", "nove", "dez", "onze", "doze", "treze", "catorze",
        "quinze", "dezasseis", "dezassete", "dezoito", "dezanove"
    },
    *SV_units [] = {
        "noll", "en", "tv/Ra", "tre", "fyra", "fem", "sex", "sju", "/Ratta",
        "nio", "tio", "elva", "tolv", "tretton", "fjorton", "femton",
        "sexton", "sjutton", "atton", "nitton"
    };


static char
    *DA_tens [] = {
        "ti", "tyve", "tredive", "fyrre", "halvtreds",
        "treds", "halvfjerds", "firs", "halvfems"
    },
    *DE_tens [] = {
        "zehn", "zwanzig", "drei/Bsig", "vierzig", "f/Uunfzig",
        "sechzig", "siebzig", "achtzig", "neunzig"
    },
    *EN_tens [] = {
        "ten", "twenty", "thirty", "forty", "fifty",
        "sixty", "seventy", "eighty", "ninety"
    },
    *ES_tens [] = {
        "diez", "veinti", "treinta", "cuarenta", "cincuenta",
        "sesenta", "setenta", "ochenta", "noventa"
    },
    *FB_tens [] = {
        "dix", "vingt", "trente", "quarante", "cinquante",
        "soixante", "septante", "quatre-vingt", "nonante"
    },
    *FR_tens [] = {
        "dix", "vingt", "trente", "quarante", "cinquante",
        "soixante", "soixante", "quatre-vingt", "quatre-vingt"
    },
    *IS_tens [] = {
        "tiu", "tuttugu", "thrjatiu", "fj/Uorutiu", "fumtiu",
        "sextiu", "sj/Uotiu", "attatiu", "niutiu"
    },
    *IT_tens [] = {
        "dieci", "venti", "trenta", "quaranta", "cinquanta",
        "sessanta", "settanta", "ottanta", "novanta"
    },
    *NL_tens [] = {
        "tien", "twintig", "dertig", "veertig", "vijftig",
        "zestig", "zeventig", "tachtig", "negentig"
    },
    *NO_tens [] = {
        "ti", "tyve", "tredve", "f/Sorti", "femti",
        "seksti", "sytti", "/Ratti", "nitti"
    },
    *PO_tens [] = {
        "dez", "vinte", "trinta", "quarenta", "cinquenta",
        "sessenta", "setenta", "oitenta", "noventa"
    },
    *SV_tens [] = {
        "tio", "tjugo", "trettio", "fyrtio", "femtio",
        "sextio", "sjuttio", "/Rattio", "nittio"
    };

static char
    *DA_months [] = {
        "januar", "februar", "marts", "april", "maj", "juni",
        "juli", "august", "september", "oktober", "november", "december"
    },
    *DE_months [] = {
        "Januar", "Februar", "Marsch", "April", "Mai", "Juni",
        "Juli", "August", "September", "Oktober", "November", "Dezember"
    },
    *EN_months [] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    },
    *ES_months [] = {
        "enero", "febrero", "marzo", "abril", "mayo", "junio",
        "julio", "agosto", "setiembre", "octubre", "noviembre", "diciembre"
    },
    *FR_months [] = {
        "janvier", "fevrier", "mars", "avril", "mai", "juin",
        "juillet", "ao/Cut", "septembre", "octobre", "novembre", "decembre"
    },
    *IS_months [] = {
        "januar", "februar", "marz", "april", "mai", "juni",
        "juli", "agust", "september", "oktober", "november", "desember"
    },
    *IT_months [] = {
        "gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno",
        "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre"
    },
    *NL_months [] = {
        "januari", "februari", "mars", "april", "mei", "juni",
        "juli", "augustus", "september", "oktober", "november", "december"
    },
    *NO_months [] = {
        "januar", "februar", "mars", "april", "mai", "juni",
        "juli", "august", "september", "oktober", "november", "desember"
    },
    *PO_months [] = {
        "janeiro", "fevereiro", "março", "abril", "maio", "junho",
        "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"
    },
    *SV_months [] = {
        "januari", "februari", "mars", "april", "maj", "juni",
        "juli", "augusti", "september", "oktober", "november", "december"
    };

static char
    *EN_days [] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    },
    *FR_days [] = {
        "Dimanche", "Lundi", "Mardi", "Mercredi",
        "Jeudi", "Vendredi", "Samedi"
    },
    *NL_days [] = {
        "Zondag", "Maandag", "Dinsdag", "Woensdag",
        "Donderdag", "Vrijdag", "Zaterdag"
    };


static int
    user_language = 0;                  /*  0 = default language             */

static Bool
    use_accents = TRUE;

static char
    **units_table  = EN_units,
    **tens_table   = EN_tens,
    **day_table    = EN_days,
    **month_table  = EN_months;


/*  Local function prototypes                                                */

static char *handle_accents (char *string);


/*  ---------------------------------------------------------------------[<]-
    Function: set_userlang

    Synopsis: Sets language used for date and numeric translation.
    The valid user languages are:
    <TABLE>
    USERLANG_DEFAULT    Default language (use hard-coded values)
    USERLANG_DA         Danish
    USERLANG_DE         German
    USERLANG_EN         English
    USERLANG_ES         Castillian Spanish
    USERLANG_FB         Belgian or Swiss French
    USERLANG_FR         French
    USERLANG_IS         Icelandic
    USERLANG_IT         Italian
    USERLANG_NL         Dutch
    USERLANG_NO         Norwegian
    USERLANG_PO         Portuguese
    USERLANG_SV         Swedish
    </TABLE>
    Returns 0 if okay, -1 if an unsupported language was specified.
    ---------------------------------------------------------------------[>]-*/

int
set_userlang (int language)
{
    /*  Order of this table is not critical                                  */
    static struct {
        int  language;
        char **units;
        char **tens;
        char **days;
        char **months;
    } languages [] =
    {
        { USERLANG_DEFAULT, EN_units, EN_tens, EN_days, EN_months },
        { USERLANG_DA,      DA_units, DA_tens, EN_days, DA_months },
        { USERLANG_DE,      DE_units, DE_tens, EN_days, DE_months },
        { USERLANG_EN,      EN_units, EN_tens, EN_days, EN_months },
        { USERLANG_ES,      ES_units, ES_tens, EN_days, ES_months },
        { USERLANG_FB,      FR_units, FB_tens, FR_days, FR_months },
        { USERLANG_FR,      FR_units, FR_tens, FR_days, FR_months },
        { USERLANG_IS,      IS_units, IS_tens, EN_days, IS_months },
        { USERLANG_IT,      IT_units, IT_tens, EN_days, IT_months },
        { USERLANG_NL,      NL_units, NL_tens, NL_days, NL_months },
        { USERLANG_NO,      NO_units, NO_tens, EN_days, NO_months },
        { USERLANG_PO,      PO_units, PO_tens, EN_days, PO_months },
        { USERLANG_SV,      SV_units, SV_tens, EN_days, SV_months },
        { -1,               NULL,     NULL,    NULL,    NULL      }
    };

    int
        index;

    for (index = 0; languages [index].language != -1; index++)
        if (languages [index].language == language)
          {
            user_language = language;
            units_table   = languages [index].units;
            tens_table    = languages [index].tens;
            day_table     = languages [index].days;
            month_table   = languages [index].months;
            return (0);
          }
    return (-1);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_userlang_str

    Synopsis: Sets language used for date and numeric translation, using a
    string representation of the language. The valid user languages are:
    <TABLE>
    ""      Default language (use hard-coded values)
    "--"    Alternative form for default language
    "DA"    Danish
    "DE"    German
    "EN"    English
    "ES"    Castillian Spanish
    "FB"    Belgian or Swiss French
    "FR"    French
    "IS"    Icelandic
    "IT"    Italian
    "NL"    Dutch
    "NO"    Norwegian
    "PO"    Portuguese
    "SV"    Swedish
    </TABLE>
    Returns 0 if okay, -1 if an unsupported language was specified.
    ---------------------------------------------------------------------[>]-*/

int
set_userlang_str (const char *language)
{
    int
        index;

    if (strnull (language))
        return (set_userlang (USERLANG_DEFAULT));

    for (index = 0; index < USERLANG_TOP; index++)
        if (streq (language, language_str [index]))
            return (set_userlang (index));

    return (-1);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_userlang

    Synopsis: Returns the current user language code.
    ---------------------------------------------------------------------[>]-*/

int
get_userlang (void)
{
    return (user_language);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_userlang_str

    Synopsis: Returns the current user language as a 2-character string.
    ---------------------------------------------------------------------[>]-*/

char *
get_userlang_str (void)
{
    return (language_str [user_language]);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_accents

    Synopsis: Enables or disables native-language accents.  If enabled,
    accented characters in translated words are produced in the current
    system character set, if possible.  Otherwise, suitable translations
    are made into the 26-letter English alphabet.  By default, accents are
    enabled.
    ---------------------------------------------------------------------[>]-*/

int
set_accents (Bool accents)
{
    use_accents = accents;
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_accents

    Synopsis: Returns TRUE if accents are enabled, FALSE if not.
    ---------------------------------------------------------------------[>]-*/

Bool
get_accents (void)
{
    return (use_accents);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_units_name

    Synopsis: Returns the name for the specified units, which is a value from
    zero to 19.  Accented characters are formatted according to the current
    accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_units_name (int units)
{
    ASSERT (units >= 0 && units <= 19);
    return (handle_accents (units_table [units]));
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_tens_name

    Synopsis: Returns the name for the specified tens, which is a value
    from 10 to 90; it is rounded as required.  Accented characters are
    formatted according to the current accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_tens_name (int tens)
{
    ASSERT (tens >= 10 && tens < 100);
    return (handle_accents (tens_table [tens / 10 - 1]));
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_day_name

    Synopsis: Returns the name for the specified day, which must be a
    value from 0 (Sunday) to 6 (Saturday).  Accented characters are
    formatted according to the current accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_day_name (int day)
{
    ASSERT (day >= 0 && day <= 6);
    return (handle_accents (day_table [day]));
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_day_abbrev

    Synopsis: Returns the abbreviation for the specified day, which must be
    a value from 0 (Sunday) to 6 (Saturday).  The abbreviation (3 letters)
    is converted into uppercase if the 'upper' argument is true.  Accented
    characters are formatted according to the current accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_day_abbrev (int day, Bool upper)
{
    char
        abbrev [4];

    ASSERT (day >= 0 && day <= 6);

    strncpy (abbrev, day_table [day], 3);
    abbrev [3] = '\0';
    if (upper)
        strupc (abbrev);
    return (handle_accents (abbrev));
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_month_name

    Synopsis: Returns the name for the specified month, which must be a
    value from 1 to 12.  Accented characters are handled as per the current
    accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_month_name (int month)
{
    ASSERT (month >= 1 && month <= 12);
    return (handle_accents (month_table [month - 1]));
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_month_abbrev

    Synopsis: Returns the abbreviation for the specified month, which must
    be a value from 1 to 12.  The abbreviation (3 letters) is converted into
    uppercase if the 'upper' argument is true.  Accented characters are
    formatted according to the current accents setting.
    ---------------------------------------------------------------------[>]-*/

char *
get_month_abbrev (int month, Bool upper)
{
    char
        abbrev [4];

    ASSERT (month >= 1 && month <= 12);

    strncpy (abbrev, month_table [month - 1], 3);
    abbrev [3] = '\0';
    if (upper)
        strupc (abbrev);

    return (handle_accents (abbrev));
}


/*  handle_accents -- internal
 *
 *  Accepts string containing escaped accented characters and converts into
 *  local values.  Currently handles DOS and Unix character sets and this
 *  (reduced) set of accented characters:
 *
 *      /Uu     u umlaut
 *      /UU     U umlaut
 *      /Uo     o umlaut
 *      /UO     O umlaut
 *      /Ra     a ring
 *      /RA     A ring
 *      /So     slash o
 *      /SO     slash O
 *      /Bs     scharfes S
 *      /BS     scharfes S
 *      /Cu     circumflex u
 *      /CU     circumflex U
 */

static char *
handle_accents (char *string)
{
#if (defined (__UNIX__))
#   define CHAR_a_ring    '\345'
#   define CHAR_A_ring    '\305'
#   define CHAR_o_uml     '\366'
#   define CHAR_O_uml     '\326'
#   define CHAR_u_circ    '\373'
#   define CHAR_u_uml     '\374'
#   define CHAR_U_uml     '\334'
#elif (defined (__MSDOS__))
#   define CHAR_a_ring    '\206'
#   define CHAR_A_ring    '\217'
#   define CHAR_o_uml     '\224'
#   define CHAR_O_uml     '\231'
#   define CHAR_u_circ    '\226'
#   define CHAR_u_uml     '\201'
#   define CHAR_U_uml     '\232'
#else
#   define CHAR_a_ring    'a'
#   define CHAR_A_ring    'A'
#   define CHAR_o_uml     'o'
#   define CHAR_O_uml     'O'
#   define CHAR_u_circ    'u'
#   define CHAR_u_uml     'u'
#   define CHAR_U_uml     'U'
#endif
    return (string);                    /*  To be implemented                */
}


/*  ---------------------------------------------------------------------[<]-
    Function: timestamp_string

    Synopsis: Formats a timestamp according to a user-supplied pattern.  The
    result is returned in a buffer supplied by the caller; if this argument
    is NULL, allocates a buffer and returns that (the caller must then free
    the buffer using mem_free()).  The pattern consists of arbitrary text
    mixed with insertion symbols indicated by '%':
    <TABLE>
        %y        day of year, 001-366
        %yy       year 2 digits, 00-99
        %yyyy     year 4 digits, 0100-9999
        %mm       month, 01-12
        %mmm      month, Jan
        %mmmm     month, January
        %MMM      month, JAN
        %MMMM     month, JANUARY
        %dd       day, 01-31
        %ddd      day of week, Sun
        %dddd     day of week, Sunday
        %DDD      day of week, SUN
        %DDDD     day of week, SUNDAY
        %w        day of week, 1-7 (1=Sunday)
        %ww       week of year, 01-53
        %q        year quarter, 1-4
        %h        hour, 00-23
        %m        minute, 00-59
        %s        second, 00-59
        %c        centisecond, 00-99
        %%        literal character %
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

char *
timestamp_string (char *buffer, const char *pattern)
{
    long
        date,                           /*  Current date                     */
        time;                           /*    and time                       */
    int
        century,                        /*  Century component of date        */
        year,                           /*  Year component of date           */
        month,                          /*  Month component of date          */
        day,                            /*  Day component of date            */
        hour,                           /*  Hour component of time           */
        minute,                         /*  Minute component of time         */
        second,                         /*  Second component of time         */
        centi,                          /*  1/100 sec component of time      */
        cursize;                        /*  Size of current component        */
    char
       *dest,                           /*  Store formatted data here        */
        ch;                             /*  Next character in picture        */

    date = date_now ();
    time = time_now ();

    century = GET_CENTURY (date);
    year    = GET_YEAR    (date);
    month   = GET_MONTH   (date);
    day     = GET_DAY     (date);
    hour    = GET_HOUR    (time);
    minute  = GET_MINUTE  (time);
    second  = GET_SECOND  (time);
    centi   = GET_CENTI   (time);

    if (buffer == NULL)
        buffer = mem_alloc (strlen (pattern) * 2);

    /*  Scan through picture, converting each component                      */
    dest = buffer;
    *dest = 0;                          /*  String is empty                  */
    while (*pattern)
      {
        ch = *pattern++;
        if (ch == '%' && *pattern)
          {
            ch = *pattern++;            /*  Count size of pattern after %    */
            for (cursize = 1; *pattern == ch; cursize++)
                pattern++;
          }
        else
          {
            *dest++ = ch;               /*  Something else - store and next  */
            *dest = 0;                  /*  Terminate the string nicely      */
            continue;
          }

        /*  Now process pattern itself                                       */
        switch (ch)
          {
            case 'y':
                if (cursize == 1)       /*  y     day of year, 001-366       */
                    sprintf (dest, "%03d", julian_date (date));
                else
                if (cursize == 2)       /*  yy    year 2 digits, 00-99       */
                    sprintf (dest, "%02d", year);
                else
                if (cursize == 4)       /*  yyyy  year 4 digits, 0100-9999   */
                    sprintf (dest, "%02d%02d", century, year);
                break;

            case 'm':
                if (cursize == 1)       /*  m     minute, 00-59              */
                    sprintf (dest, "%02d", minute);
                else
                if (cursize == 2)       /*  mm    month, 01-12               */
                    sprintf (dest, "%02d", month);
                else
                if (cursize == 3)       /*  mmm   month, 3 letters           */
                    strcpy (dest, get_month_abbrev (month, FALSE));
                else
                if (cursize == 4)       /*  mmmm  month, full name           */
                    strcpy (dest, get_month_name (month));
                break;

            case 'M':
                if (cursize == 3)       /*  MMM   month, 3-letters, ucase    */
                    strcpy (dest, get_month_abbrev (month, TRUE));
                else
                if (cursize == 4)       /*  MMMM  month, full name, ucase    */
                  {
                    strcpy (dest, get_month_name (month));
                    strupc (dest);
                  }
                break;

            case 'd':
                if (cursize == 2)       /*  dd    day, 01-31                 */
                    sprintf (dest, "%02d", day);
                else
                if (cursize == 3)       /*  ddd   day of week, Sun           */
                    strcpy (dest, get_day_abbrev (day_of_week (date), FALSE));
                else
                if (cursize == 4)       /*  dddd  day of week, Sunday        */
                    strcpy (dest, get_day_name (day_of_week (date)));
                break;

            case 'D':
                if (cursize == 3)       /*  DDD   day of week, SUN           */
                    strcpy (dest, get_day_abbrev (day_of_week (date), TRUE));
                else
                if (cursize == 4)       /*  DDDD  day of week, SUNDAY        */
                  {
                    strcpy (dest, get_day_name (day_of_week (date)));
                    strupc (dest);
                  }
                break;

            case 'w':
                if (cursize == 1)       /*  w     day of week, 1-7 (1=Sun)   */
                    sprintf (dest, "%d", day_of_week (date) + 1);
                else
                if (cursize == 2)       /*  ww    week of year, 01-53        */
                    sprintf (dest, "%d", week_of_year (date));
                break;

            case 'q':
                if (cursize == 1)       /*  q     year quarter, 1-4          */
                    sprintf (dest, "%d", year_quarter (date));
                break;

            case 'h':
                if (cursize == 1)       /*  h     hour, 00-23                */
                    sprintf (dest, "%02d", hour);
                break;

            case 's':
                if (cursize == 1)       /*  s     second, 00-59              */
                    sprintf (dest, "%02d", second);
                break;

            case 'c':
                if (cursize == 1)       /*  c     centisecond, 00-99         */
                    sprintf (dest, "%02d", centi);
                break;

            case '%':
                if (cursize == 1)       /*  %     literal '%'                */
                    strcpy (dest, "%");
                break;
        }
        if (*dest)                      /*  If something was output,         */
            while (*dest)               /*    skip to end of string          */
                dest++;
        else
          {
            while (cursize--)           /*  Else output ch once or more      */
                *dest++ = ch;           /*    and bump dest pointer          */
            *dest = 0;                  /*  Terminate the string nicely      */
          }
    }
    return (buffer);
}
