/* ta_sql_odbc.h
 * Interface definition to ODBC minidriver
 * See ta_sql_minidriver.h for description of the interface
 */

#ifndef TA_SQL_ODBC_H
#define TA_SQL_ODBC_H

#ifndef TA_DEFS_H
#include "ta_defs.h"
#endif

#ifndef TA_COMMON_H
#include "ta_common.h"
#endif


TA_RetCode TA_SQL_ODBC_OpenConnection(const char database[], 
                                      const char host[], 
                                      const char username[], 
                                      const char password[],
                                      unsigned int port,
                                      void **connection);
TA_RetCode TA_SQL_ODBC_ExecuteQuery(void *connection, 
                                    const char sql_query[], 
                                    void **query_result);
TA_RetCode TA_SQL_ODBC_GetNumColumns(void *query_result, int *num);
TA_RetCode TA_SQL_ODBC_GetNumRows(void *query_result, int *num);
TA_RetCode TA_SQL_ODBC_GetColumnName(void *query_result, int column, const char **name);
TA_RetCode TA_SQL_ODBC_GetRowString(void *query_reslut, int row, int column, const char **value);
TA_RetCode TA_SQL_ODBC_GetRowReal(void *query_reslut, int row, int column, TA_Real *value);
TA_RetCode TA_SQL_ODBC_GetRowInteger(void *query_reslut, int row, int column, TA_Integer *value);
TA_RetCode TA_SQL_ODBC_ReleaseQuery(void *query_result);
TA_RetCode TA_SQL_ODBC_CloseConnection(void *connection);

#endif
