/* ta_sql_mysql.h
 * Interface definition to MySQL minidriver
 * See ta_sql_minidriver.h for description of the interface
 */

#ifndef TA_SQL_MYSQL_H
#define TA_SQL_MYSQL_H

#ifndef TA_DEFS_H
#include "ta_defs.h"
#endif

#ifndef TA_COMMON_H
#include "ta_common.h"
#endif


TA_RetCode TA_SQL_MySQL_OpenConnection(const char database[], 
                                        const char host[], 
                                        const char username[], 
                                        const char password[],
                                        unsigned int port,
                                        void **connection);
TA_RetCode TA_SQL_MySQL_ExecuteQuery(void *connection, 
                                      const char sql_query[], 
                                      void **query_result);
TA_RetCode TA_SQL_MySQL_GetNumColumns(void *query_result, int *num);
TA_RetCode TA_SQL_MySQL_GetNumRows(void *query_result, int *num);
TA_RetCode TA_SQL_MySQL_GetColumnName(void *query_result, int column, const char **name);
TA_RetCode TA_SQL_MySQL_GetRowString(void *query_reslut, int row, int column, const char **value);
TA_RetCode TA_SQL_MySQL_GetRowReal(void *query_reslut, int row, int column, TA_Real *value);
TA_RetCode TA_SQL_MySQL_GetRowInteger(void *query_reslut, int row, int column, TA_Integer *value);
TA_RetCode TA_SQL_MySQL_ReleaseQuery(void *query_result);
TA_RetCode TA_SQL_MySQL_CloseConnection(void *connection);

#endif
