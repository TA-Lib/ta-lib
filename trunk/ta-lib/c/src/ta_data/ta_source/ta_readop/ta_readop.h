#ifndef TA_READOP_H
#define TA_READOP_H

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_SYSTEM_H
   #include "ta_system.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

/* Provides all the functionality for
 * speed optimized read of ASCII file.
 *
 * Note: the file can be in fact a stream in
 *       memory, but this module won't make the
 *       difference.
 *
 * I have to admit, this is slightly complex and
 * a simpler approach could have been choosen. On
 * the other side, the speed here is everything.
 */

/* Definition used for defining a TA_ReadOp.
 * One read operation is encoded within one 32 bit. The fields
 * of the 32 bit provides the following information:
 *  - Should we read an integer or a real? (TA_IS_REAL_CMD...)
 *  - Is that read operation can be skip because the
 *    user did not request for this field when building the
 *    TA_History? (TA_CMD_SKIP_FLAG)
 *  - Is that read operation can be skip because a value must
 *    be permanently skip in the file? (TA_CMD_PERMANENT_SKIP_FLAG)
 *  - Where do I store the read data? (TA_GET_IDX).
 *  - Is this the last operation needed because the user
 *    already got all its field for its TA_History? (TA_CMD_READ_STOP_FLAG)
 *  - Is this the VERY last read operation? (TA_CMD_LAST_FLAG).
 *  - After that read operation, did we get everything needed
 *    to build and validate the timestamp? (TA_TIMESTAMP_COMPLETE_FLAG).
 */
#define TA_CMD_READ_REAL              0x00000100
#define TA_CMD_READ_INTEGER           0x00000200
#define TA_CMD_READ_MONTH_CHAR        0x00000400
#define TA_CMD_PERMANENT_SKIP_FLAG    0x00000800
#define TA_TIMESTAMP_COMPLETE_FLAG    0x00001000
#define TA_CMD_SKIP_FLAG              0x00002000
#define TA_CMD_READ_STOP_FLAG         0x00004000
#define TA_CMD_LAST_FLAG             (0x00008000|TA_CMD_READ_STOP_FLAG)

#define TA_CLOSE_IDX        0x00000000
#define TA_OPEN_IDX         0x00000001
#define TA_HIGH_IDX         0x00000002
#define TA_LOW_IDX          0x00000003
#define TA_REAL_ARRAY_SIZE  4

#define TA_VOLUME_IDX       0x00000007
#define TA_OPENINTEREST_IDX 0x00000006
#define TA_YEAR_IDX         0x00000005
#define TA_MONTH_IDX        0x00000004
#define TA_DAY_IDX          0x00000003
#define TA_HOUR_IDX         0x00000002
#define TA_MIN_IDX          0x00000001
#define TA_SEC_IDX          0x00000000
#define TA_INTEGER_ARRAY_SIZE 8

#define TA_GET_IDX(v)        ((v)&0x000000FF)
#define TA_SET_IDX(v,idx)    {(v)&=0xFFFFFF00; (v)|=(idx&0x000000FF);}

#define TA_GET_NB_NUMERIC(v)     ((((v)&0xFFFF0000)>>16) & 0x0000FFFF)
#define TA_SET_NB_NUMERIC(v,cmd) {(v)&=0x0000FFFF; (v)|=((cmd<<16)&0xFFFF0000);}

#define TA_SET_SKIP_FLAG(v)  {(v)|=TA_CMD_SKIP_FLAG;}
#define TA_CLR_SKIP_FLAG(v)  {(v)&=~TA_CMD_SKIP_FLAG;}
#define TA_IS_SKIP_SET(v)    ((v)&TA_CMD_SKIP_FLAG)

#define TA_SET_PERMANENT_SKIP_FLAG(v)  {(v)|=TA_CMD_PERMANENT_SKIP_FLAG;}
#define TA_CLR_PERMANENT_SKIP_FLAG(v)  {(v)&=~TA_CMD_PERMANENT_SKIP_FLAG;}
#define TA_IS_PERMANENT_SKIP_SET(v)    ((v)&TA_CMD_PERMANENT_SKIP_FLAG)

#define TA_SET_LAST_FLAG(v)  {(v)|=TA_CMD_LAST_FLAG;}
#define TA_CLR_LAST_FLAG(v)  {(v)&=~TA_CMD_LAST_FLAG;}
#define TA_IS_LAST_SET(v)    ((v)&TA_CMD_LAST_FLAG)

#define TA_SET_READ_STOP_FLAG(v)    {(v)|=TA_CMD_READ_STOP_FLAG;}
#define TA_CLR_READ_STOP_FLAG(v)    {(v)&=~TA_CMD_READ_STOP_FLAG;}
#define TA_IS_READ_STOP_FLAG_SET(v) ((v)&TA_CMD_READ_STOP_FLAG)

#define TA_SET_TIMESTAMP_COMPLETE(v) {(v)|=TA_TIMESTAMP_COMPLETE_FLAG;}
#define TA_CLR_TIMESTAMP_COMPLETE(v) {(v)&=~TA_TIMESTAMP_COMPLETE_FLAG;}
#define TA_IS_TIMESTAMP_COMPLETE(v)  ((v)&TA_TIMESTAMP_COMPLETE_FLAG)

#define TA_IS_REAL_CMD(v)    ((v)&TA_CMD_READ_REAL)
#define TA_IS_INTEGER_CMD(v) ((v)&TA_CMD_READ_INTEGER)

typedef unsigned int TA_ReadOp;

typedef struct
{
   TA_Libc *libHandle;

   /* The parameter defining the fields. 
    * Example: "[YYYY][MM][DD][O][C][V]"
    */
   const char *sourceInfo;
   TA_Period period;

   /* The following array describes the series of operations that needs
    * to be performed for reading one line of the ASCII files.
    */
   TA_ReadOp *arrayReadOp;
   unsigned int nbReadOp;

   /* Some additional information for correctly processing the input file. */
   unsigned int nbHeaderLineToSkip;
   unsigned int openInterestMult; /* 1, 10, 100, 1000, 10000 */
   unsigned int volumeMult;       /* 1, 10, 100, 1000, 10000 */

   /* Indicates all the fields that can be provided. */
   TA_Field fieldProvided;
} TA_ReadOpInfo;

TA_RetCode TA_ReadOpInfoAlloc( TA_Libc *libHandle,
                               const char *sourceInfo,
                               TA_ReadOpInfo **allocatedInfo );

TA_RetCode TA_ReadOpInfoFree( TA_ReadOpInfo *readOpInfoToBeFreed );

TA_RetCode TA_ReadOp_Optimize( TA_Libc       *libHandle,
                               TA_ReadOpInfo *readOpInfo,
                               TA_Period      period,
                               TA_Field       fieldToAlloc );

TA_RetCode TA_ReadOp_Do( TA_Libc             *libHandle,
                         TA_FileHandle       *fileHandle,
                         const TA_ReadOpInfo *readOpInfo,
                         TA_Period            period,
                         const TA_Timestamp  *start,
                         const TA_Timestamp  *end,
                         unsigned int         minimumNbBar,
                         TA_Field             fieldToAlloc,
                         TA_ParamForAddData  *paramForAddData,
                         unsigned int        *nbTotalBarAdded,
                         TA_Timestamp        *lastBarTimestamp );

unsigned int TA_ReadOpToField( TA_Libc *libHandle, TA_ReadOp readOp );

 /* See ta_readop_estalloc.c */
typedef struct
{
  unsigned int minimumSize;
  unsigned int maximumSize;
} TA_EstimateInfo;

TA_RetCode TA_EstimateAllocInit( const TA_Timestamp *start,
                                 const TA_Timestamp *end,
                                 TA_Period period,
                                 unsigned int minimumSize,
                                 unsigned int maximumSize,
                                 TA_EstimateInfo *estimationInfo,
                                 unsigned int *nbElementToAllocate );

TA_RetCode TA_EstimateAllocNext( TA_EstimateInfo *estimationInfo,
                                 unsigned int *nbElementToAllocate );



#endif

