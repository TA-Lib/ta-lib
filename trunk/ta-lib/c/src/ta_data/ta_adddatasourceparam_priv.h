#ifndef TA_ADDDATASOURCEPARAM_PRIV_H
#define TA_ADDDATASOURCEPARAM_PRIV_H

/* The following is a private copy of the user provided
 * parameters for a TA_AddDataSource call.
 *
 * Code is in 'ta_data_interface.c'
 */
typedef struct
{
  TA_SourceId   id;
  TA_SourceFlag flags;
  TA_Period     period;

  TA_String *location;
  TA_String *info;
  TA_String *username;
  TA_String *password;
  TA_String *category;
  TA_String *country;
  TA_String *exchange;
  TA_String *type;
  TA_String *symbol;
  TA_String *name;
} TA_AddDataSourceParamPriv;

#endif
