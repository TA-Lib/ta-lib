/* ta_sql_odbc.h
 * Call the following function to enable the ODBC minidriver on a WIN32 platform.
 */

#ifndef TA_SQL_ODBC_H
#define TA_SQL_ODBC_H

#ifdef WIN32
   TA_RetCode TA_SQL_ODBC_Initialize(void);
#endif

#endif
