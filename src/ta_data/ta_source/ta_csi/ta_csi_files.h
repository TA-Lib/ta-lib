
#ifndef TA_CSI_FILES_H
#define TA_CSI_FILES_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

typedef struct MasterListRecordType {
    unsigned long csinum;
    unsigned long dydm;
    long strike; // + is call, - is put, 0 is non-option
    short cvf;
    TA_Period Period;
    int IsStock; // Boolean
    int Deleted; // Boolean
    char Name[80];
    char Symbol[10];
    char PricingUnits[5];
} MasterListRecord;

typedef struct {
  long date, dydm, strike, open, high, low, close, ClosingBid, ClosingAsk, vol, oi, tvol, toi;
} SingleContractDataDayCore;

TA_RetCode ReadCSIMaster  (const char *DirectoryName, struct MasterListRecordType **outMasterList, int *outNRec);
TA_RetCode ReadCSIMMaster (const char *DirectoryName, struct MasterListRecordType **outMasterList, int *outNRec);

TA_RetCode ReadCSIData    (const char *DirectoryName, int FileNumber, short cvf, SingleContractDataDayCore **outDataList, int *outNRec);
TA_RetCode ReadCSIMData   (const char *DirectoryName, int FileNumber, short cvf, SingleContractDataDayCore **outDataList, int *outNRec);

#endif
