/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  SD       Steven Davis, UA Program Manager. http://www.csidata.com
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  031904 SD   First version working on WIN32
 *  032704 MF   Integrate in TA-Lib. Make it work on Linux.
 */

/* Description:
 *    Access to CSI file formats. Original code has been donated by CSI,
 *    a commercial data vendor. These files can be created with their
 *    Unfair Advantage product.
 *
 *    Two formats are supported:
 *
 *      TA_CSI : The original CSI format.
 *
 *      TA_CSIM: A format that has the advantage to work with
 *               softwares expecting Metastock format.
 *
 *    Exact definitions of these formats is openly documented.
 *
 *    More info at: http://www.csidata.com
 */

/**** Headers ****/
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ta_memory.h"

#include "ta_csi_files.h"
#include "ta_system.h"
#include "sfl.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define BOOL  int
#define FALSE 0
#define TRUE  1

#ifdef __UNIX__
typedef long long hyper;
#endif

typedef unsigned long msfloat;

#pragma pack(1)
typedef struct {
  unsigned short Reserved;
  unsigned short LastPostedRecord;
  char Unused[28-4];
} DatFileHeaderType;
typedef struct {
  msfloat date, open, high, low, close, vol, oi;
} DatRecordType;
  struct MSMasterHdrType {
    short NumOfSecurities;
    short HighestFileNum;
    char Reserved[49];
  } CSIMMasterHdr;
  struct CSIMMasterRawRecType {
    unsigned char FileNum;
    short FileType;
    unsigned char RecordLength;
    unsigned char NumReals, NumInts, NumBytes;
    char Name[11+5];
    short Reserved1; // should equal 0
    char MinDate[4], MaxDate[4]; // MBF
    char Period;
    short IntraTime; // should be 0
    char Symbol[14];
    char Reserved2[1];
    char Flag[1]; // should be space
    char Reserved3[1];
  } CSIMMasterRawRec;
typedef struct {
  msfloat FileEndRecordPointer;
  msfloat MaximumDatePointer;
  msfloat HighestHigh;
  msfloat LowestLow;
  msfloat FirstDate;
  msfloat LastDate;
  char HighNumberFlag;
  char Unused[4+1+2];
} DtaFileHeaderType;
typedef struct {
  unsigned long FileEndRecordPointer;
  unsigned long MaximumDatePointer;
  long HighestHigh;
  long LowestLow;
  unsigned long FirstDate;
  unsigned long LastDate;
  char HighNumberFlag;
  char Unused[4+1+2+36];
} Dt2FileHeaderType; // 68 byte record
typedef struct {
  msfloat date;
  char dow;
  unsigned short open, high, low, close, noon, cash;
  unsigned short tvol; unsigned char tvole;
  unsigned short toi ; unsigned char toi_e;
  unsigned short  vol; unsigned char  vole;
  unsigned short  oi ; unsigned char  oi_e;
  unsigned char ExtendedBits[3];
} DtaRecordType;
typedef struct {
  unsigned long date, dydm;
  long open, high, low, close, cash, ClosingBid, ClosingAsk, vol, oi, tvol, toi;
  char dow;
  unsigned char Unused1[3];
  unsigned long Unused2[3];
} Dt2RecordType; // 68 byte record
  struct QmastRecType {
                      //DESCRIPTION                                                     POSITION                LENGTH
    char csiid1[4];   //CSI I. D. Number                                                  1-4                     4
    char name[20];    //Commodity Name                                                    5-24                    20
    char period[1];   //File Type (D,W,M)                                                 25                      1
    char dm[2];       //Delivery Month                                                    26-27                   2
    char dy[2];       //Delivery Year  (last two digits)                                  28-29                   2
    char cvf[2];      //Conversion Factor                                                 30-31                   2
    char unit[5];     //Pricing Unit                                                      32-36                   5
    char sym1[2];     //Commodity Symbol                                                  37-38                   2
    char comstock[1]; //Commodity/Stock Flag (C/S)                                        39                      1
    char putcall[1];  //Option Flag                                                       40                      1
                      //  (P=Put, C=Call, N=Normal)
    char strike[5];   //Striking Price                                                    41-45                   5
    char symbol[6];   //Stock or Commodity Symbol                                         46-51                   6
    char deleted[1];  //Deleted/Not deleted (0=not deleted, 1=deleted)                    52                      1
    char century[2];  //Delivery Year (First two digits)                                  53-54                   2
    char Reserved[7]; //                                                                  55-61                   6
    char ocvf[2];     //Optional display conversion factor                                62-63                   2
    char csinum2[1];  //Stock Number Extension Byte                                       64                      1
  } QmastRec;
  struct Qmast2RecType {
    unsigned long csinum;
    unsigned long dydm;
    long strike; // + is call, - is put, 0 is non-option
    short cvf, ocvf;
    char period[1];   //File Type (D,W,M,Q,A)
    char comstock[1]; //Commodity/Stock Flag (C/S)                                        39                      1
    char deleted[1];  //Deleted/Not deleted (0=not deleted, 1=deleted)                    52                      1
    char Unused1[1];
    char name[40];
    char Unused2[15];
    char unit[5];     //Pricing Unit                                                      32-36                   5
    char symbol[8];   //Stock or Commodity Symbol                                         46-51                   6
    char Unused3[40];
  } Qmast2Rec;
#pragma pack()

typedef enum {
  HundredMillionthsPriceFormat=8,
  TenMillionthsPriceFormat=7,
  MillionthsPriceFormat=6,
  HundredThousandsPriceFormat=5,
  TenThousandsPriceFormat=4,
  ThousandsPriceFormat=3,
  HundrethsPriceFormat=2,
  TenthsPriceFormat=1,
  LiteralPriceFormat=0,
  EightsPriceFormat= -1,
  SixthteensPriceFormat= -2,
  ThirtySecondsPriceFormat= -3,
  SixthFourthsPriceFormat= -4,
  OneTwentyEightsPriceFormat= -5,
  TwoFiftySixthsPriceFormat= -6,
  ThirtySecondsAndHalvesPriceFormat= -7,
  ThirtySecondsAndQuartersPriceFormat= -8,
} PriceFormat;

#define MAXCVF 8
#define MINCVF -8
long PriConv_PntsPerUnit[17]= {
  // -8 -7  -6  -5 -4 -3 -2 -1
		128,64,256,128,64,32,16,8,
	//  0  1   2   3    4      5       6         7          8
      1,10,100,1000,10000,100000L,1000000L,10000000L,100000000L };
long PriConv_SubPntsPerUnit[17]= {
  // -8 -7  -6  -5 -4 -3 -2 -1
		320,320,256,128,64,32,16,8,
	//  0  1   2   3    4      5       6         7          8
      1,10,100,1000,10000,100000L,1000000L,10000000L,100000000L };
long PriConv_NumDigits[17]= {
  // -8    -7   -6   -5  -4  -3  -2 -1
   1000, 1000,1000,1000,100,100,100,10,
	//  0  1   2   3    4      5       6         7          8
      1,10,100,1000,10000,100000L,1000000L,10000000L,100000000L };

/**** Local functions.    ****/
static int msbintoieee( msfloat *src4, float *dst4 );
static unsigned long msbintoint( msfloat *src );
static float msbintofloat( msfloat *src );
static void AddTrailingBackSlash(char *PathName);
static void CopyPStrToZStr(char *dest, short destsz, char *src, short srcsz, BOOL Stripping);
static char Between(long v, long min_value, long max_value);
static hyper chDTP(short cvf, double dec);
static long cDTP(short cvf, double dec);

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode ReadCSIMaster(const char *DirectoryName, struct MasterListRecordType **outMasterList, int *outNRec)
{
  char FileName[PATH_MAX];
  int QmasterVersion;
  FILE *fp;
  int rc, nrec, i;
  char buffer[81];
  struct MasterListRecordType *List;
  struct QmastRecType rec;
  struct MasterListRecordType *mr;
  int dm, dy, ce;

  struct Qmast2RecType rec2;

  *outMasterList = NULL; *outNRec = 0; List = NULL;
  QmasterVersion = -1; 

  {
    strcpy(FileName,DirectoryName);
    AddTrailingBackSlash(FileName);
    strcat(FileName,"Qmaster2");
    if(file_exists(FileName)) {
      QmasterVersion = 2;
    } else {
      FileName[strlen(FileName)-1]='\0';
      if(!file_exists(FileName))
        return TA_CSI_UNABLE_FIND_QMASTER_FILE; // no QMaster file found
      QmasterVersion = 1;
    }
  }

  fp = fopen(FileName,"rb");
  if(fp==NULL)
  {
    if( QmasterVersion == 2 )
       return TA_CSI_UNABLE_OPEN_QMASTER2_FILE;

    return TA_CSI_UNABLE_OPEN_QMASTER_FILE;
  }

  rc = TA_SUCCESS;

  switch(QmasterVersion) {
    case 1: {
      
      fseek(fp,0,SEEK_END);
      nrec = ftell(fp)/sizeof(rec);
      fseek(fp,0,SEEK_SET);
      List = (struct MasterListRecordType*)TA_Malloc(sizeof(struct MasterListRecordType)*nrec);
      if(List==NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated memory for Qmaster contents
      memset(List,0,sizeof(struct MasterListRecordType)*nrec);
      *outMasterList = (struct MasterListRecordType*)List;
      *outNRec = nrec;
      for(i=1;i<=nrec;i++) {
        if(fread(&rec,sizeof(rec),1,fp)!=1) { rc = TA_CSI_QMASTER_READ_ERROR; goto ErrorReturn; } // read error on Qmaster file
        mr = &List[i-1];
        switch(rec.deleted[0]) {
          case '1': mr->Deleted = 1; break;
          case '0': mr->Deleted = 0; break;
          case '9': mr->Deleted = 9; break;
          default:
            { rc = TA_CSI_QMASTER_INVALID_DELETED_FIELD; goto ErrorReturn; } // invalid Deleted field in Qmaster record
        }
        if(mr->Deleted==9) {
          memset(mr,0,sizeof(mr));
          nrec = i-1;
          break;
        }
        buffer[0]=rec.csinum2[0];
        CopyPStrToZStr(buffer+1,sizeof(buffer)-1, rec.csiid1,sizeof(rec.csiid1), 1);
        mr->csinum = atol(buffer);
        CopyPStrToZStr(mr->Name,sizeof(mr->Name), rec.name,sizeof(rec.name), 1);
        switch(rec.period[0]) {
          case 'D': mr->Period = TA_DAILY; break;
          case 'W': mr->Period = TA_WEEKLY; break;
          case 'M': mr->Period = TA_MONTHLY; break;
          default:
            { rc = TA_CSI_QMASTER_BAD_PERIOD; goto ErrorReturn; } // invalid period in Qmaster record
        }
        {
          buffer[2] = '\0';
          buffer[0]=rec.dm[0]; buffer[1]=rec.dm[1]; dm=atoi(buffer);
          buffer[0]=rec.dy[0]; buffer[1]=rec.dy[1]; dy=atoi(buffer);
          buffer[0]=rec.century[0]; buffer[1]=rec.century[1]; ce=atoi(buffer);
          if(dm>12)
            mr->dydm = dm;
          else {
            if(ce==99)
              ce=0;
            if(ce==0) {
              if(dy<20)
                ce=20;
              else
                ce=19;
            }
            mr->dydm = (ce*100L+dy)*100L+dm;
          }
        }
        CopyPStrToZStr(buffer,sizeof(buffer), rec.cvf,sizeof(rec.cvf), 1);
        mr->cvf = atoi(buffer);
        CopyPStrToZStr(mr->PricingUnits,sizeof(mr->PricingUnits), rec.unit,sizeof(rec.unit), 1);
        CopyPStrToZStr(mr->Symbol,sizeof(mr->Symbol), rec.sym1,sizeof(rec.sym1), 1);
        switch(rec.comstock[0]) {
          case 'S': mr->IsStock = 1; break;
          case 'C': mr->IsStock = 0; break;
          default:
            { rc = TA_CSI_QMASTER_INVALID_STOCKCOM_FIELD; goto ErrorReturn; } // invalid Stock/Commodity field in Qmaster record
        }
        CopyPStrToZStr(buffer,sizeof(buffer), rec.strike,sizeof(rec.strike), 1);
        mr->strike = atol(buffer);
        switch(rec.putcall[0]) {
          case 'P': mr->strike = -mr->strike; break;
          case 'C': break;
          case 'N': mr->strike = 0; break;
          default:
            { rc = TA_CSI_QMASTER_INVALID_PUTCALL_FIELD; goto ErrorReturn; } // invalid Put/Call field in Qmaster record
        }
      }
    } break;

    case 2: { // Qmaster format 2      
      fseek(fp,0,SEEK_END);
      nrec = ftell(fp)/sizeof(rec2);
      fseek(fp,0,SEEK_SET);
      List = (struct MasterListRecordType*)TA_Malloc(sizeof(struct MasterListRecordType)*nrec);
      if(List==NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated memory for Qmaster2 contents
      memset(List,0,sizeof(struct MasterListRecordType)*nrec);
      *outMasterList = (struct MasterListRecordType*)List;
      *outNRec = nrec;
      for(i=1;i<=nrec;i++) {
        mr = &List[i-1];
        if(fread(&rec2,sizeof(rec2),1,fp)!=1) { rc = TA_CSI_QMASTER2_READ_ERROR; goto ErrorReturn; } // read error on Qmaster2 file
        switch(rec2.deleted[0]) {
          case '1': mr->Deleted = 1; break;
          case '0': mr->Deleted = 0; break;
          case '9': mr->Deleted = 9; break;
          default:
            { rc = TA_CSI_QMASTER2_INVALID_DELETED_FIELD; goto ErrorReturn; } // invalid Deleted field in Qmaster2 record
        }
        if(mr->Deleted==9) {
          memset(&mr,0,sizeof(mr));
          nrec = i-1;
          break;
        }
        mr->csinum = rec2.csinum;
        CopyPStrToZStr(mr->Name,sizeof(mr->Name), rec2.name,sizeof(rec2.name), 1);
        CopyPStrToZStr(mr->Symbol,sizeof(mr->Symbol), rec2.symbol,sizeof(rec2.symbol), 1);
        switch(rec2.period[0]) {
          case 'D': mr->Period = TA_DAILY; break;
          case 'W': mr->Period = TA_WEEKLY; break;
          case 'M': mr->Period = TA_MONTHLY; break;
          case 'Q': mr->Period = TA_QUARTERLY; break;
          case 'A': mr->Period = TA_YEARLY; break;
          default:
            { rc = TA_CSI_QMASTER2_BAD_PERIOD; goto ErrorReturn; } // invalid period in Qmaster2 record
        }
        mr->dydm = rec2.dydm;
        mr->cvf = rec2.cvf;
        mr->strike = rec2.strike;
        switch(rec2.comstock[0]) {
          case 'S': mr->IsStock = 1; break;
          case 'C': mr->IsStock = 0; break;
          default:
            { rc = TA_CSI_QMASTER2_INVALID_STOCKCOM_FIELD; goto ErrorReturn; } // invalid Stock/Commodity field in Qmaster2 record
        }
        CopyPStrToZStr(mr->PricingUnits,sizeof(mr->PricingUnits), rec2.unit,sizeof(rec2.unit), 1);
      }
    } break;
    default: { rc = TA_CSI_UNKNOWN_QMASTER_VERSION; goto ErrorReturn; } // unknown qmaster record version
  }
  *outNRec = nrec;
  fclose(fp);
  return TA_SUCCESS;
ErrorReturn:
  fclose(fp);
  *outMasterList = NULL; *outNRec = 0;
  if( List )
     TA_Free(List);
  return rc;
}

TA_RetCode ReadCSIData(const char *DirectoryName, int Index, short cvf, SingleContractDataDayCore **outDataList, int *outNRec)
{
  char FileName[PATH_MAX];
  int QmasterVersion;
  int FileNumber;
  FILE *fp;
  SingleContractDataDayCore *List;
  int nrec, rc;
  DtaFileHeaderType daht;
  int i, j;
  DtaRecordType dart;
  unsigned long s, t, u;
  Dt2RecordType dart2;
  SingleContractDataDayCore *scdd;
  Dt2FileHeaderType da2ht;

  (void)cvf;

  FileNumber = Index+1; 
  *outDataList = NULL; *outNRec = 0; List = NULL;
  QmasterVersion= -1;

  {
    strcpy(FileName,DirectoryName);
    AddTrailingBackSlash(FileName);
    sprintf(FileName+strlen(FileName),(FileNumber<1000)?"F%03d.dt2":"F%07d.dt2",FileNumber);
    if(file_exists(FileName)) {
      QmasterVersion = 2;
    } else {
      FileName[strlen(FileName)-1]='a';
      if(!file_exists(FileName))
        return TA_CSI_DATA_FILE_MISSING; // no CSI data file file found
      QmasterVersion = 1;
    }
  }

  fp = fopen(FileName,"rb");
  if(fp==NULL)
    return TA_CSI_DATA_FILE_ACCESS_FAILED; // unable to open CSI data file

  nrec = 0;
  rc = TA_SUCCESS;
 
  switch(QmasterVersion) {
    case 1: {
      {        
        if(!fread(&daht,sizeof(daht),1,fp)) { rc = TA_CSI_DATA_FILE_HEADER_READ_ERROR; goto ErrorReturn; } // read failure on CSI data file header
        nrec = msbintoint(&daht.FileEndRecordPointer)-1;
        List = (SingleContractDataDayCore*)TA_Malloc(sizeof(SingleContractDataDayCore)*nrec);
        if(List == NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated CSI data temporary
        memset(List,0,sizeof(SingleContractDataDayCore)*nrec);
        *outDataList = List;
      }

      j=0;
      for(i=0;i<nrec;i++) {        
        if(!fread(&dart,sizeof(dart),1,fp)) { rc = TA_CSI_DATA_FILE_READ_ERROR; goto ErrorReturn; } // read failure on CSI data file
        if(dart.dow == 9)
          continue;
        scdd = &List[j];

        scdd->date = msbintoint(&dart.date)+19000000L;
        //scdd.dow = (char)(dart.dow==9?HOLIDAY_DOW:dart.dow);
          
          s = (dart.ExtendedBits[2]>>6)&0x03; t = (dart.ExtendedBits[0]>>6)&0x03; u = dart.open ; u = u |((t|(s<<2))<<16); scdd->open  = u; //cPTD(cvf,(long)u,FALSE,NULL);
          s = (dart.ExtendedBits[2]>>4)&0x03; t = (dart.ExtendedBits[0]>>4)&0x03; u = dart.high ; u = u |((t|(s<<2))<<16); scdd->high  = u; //cPTD(cvf,(long)u,FALSE,NULL);
          s = (dart.ExtendedBits[2]>>2)&0x03; t = (dart.ExtendedBits[0]>>2)&0x03; u = dart.low  ; u = u |((t|(s<<2))<<16); scdd->low   = u; //cPTD(cvf,(long)u,FALSE,NULL);
          s = (dart.ExtendedBits[2]   )&0x03; t = (dart.ExtendedBits[0]   )&0x03; u = dart.close; u = u |((t|(s<<2))<<16); scdd->close = u; //cPTD(cvf,(long)u,FALSE,NULL);
          //s = (dart.ExtendedBits[1]   )&0x03; t = (dart.ExtendedBits[1]>>4)&0x03; u = dart.cash ; u = u |((t|(s<<2))<<16); scdd.cash  = cPTD(cvf,(long)u,FALSE,NULL);
          scdd->ClosingBid = scdd->ClosingAsk = scdd->close;
          scdd->dydm = 0;
          s = dart.tvol; t = dart.tvole; scdd->tvol = s|(t<<16);
          s = dart.toi ; t = dart.toi_e; scdd->toi  = s|(t<<16);
          s = dart. vol; t = dart. vole; scdd->vol  = s|(t<<16);
          s = dart. oi ; t = dart. oi_e; scdd->oi   = s|(t<<16);

        j++;
      }
      nrec = j++;
    } break;
  case 2: {
      {        
        if(!fread(&da2ht,sizeof(da2ht),1,fp)) { rc = TA_CSI_DATA_FILE_HEADER_READ_ERROR_2; goto ErrorReturn; } // read failure on CSI data file header
        nrec = (da2ht.FileEndRecordPointer)-1;
        List = (SingleContractDataDayCore*)TA_Malloc(sizeof(SingleContractDataDayCore)*nrec);
        if(List == NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated CSI data temporary
        memset(List,0,sizeof(SingleContractDataDayCore)*nrec);
        *outDataList = List;
      }

      j=0;
      for(i=0;i<nrec;i++) {
        if(!fread(&dart2,sizeof(dart2),1,fp)) { rc = TA_CSI_DATA_FILE_READ_ERROR_2; goto ErrorReturn; } // read failure on CSI data file
        if(dart2.dow == 0)
          continue;
        scdd = &List[j];

        scdd->date = dart2.date;
        scdd->open = dart2.open;
        scdd->high = dart2.high;
        scdd->low = dart2.low;
        scdd->close = dart2.close;
        scdd->ClosingBid = dart2.ClosingBid;
        scdd->ClosingAsk = dart2.ClosingAsk;
        scdd->vol = dart2.vol;
        scdd->oi = dart2.oi;
        scdd->tvol = dart2.tvol;
        scdd->toi = dart2.toi;
        j++;
      }
      nrec = j++;
    } break;
  default:
    { rc = TA_CSI_UNKNOWN_DATA_FILE_VERSION; goto ErrorReturn; } // unknown CSI format version number
  }
  *outNRec = nrec;
  fclose(fp);
  return TA_SUCCESS;
ErrorReturn:
  fclose(fp);
  *outDataList = NULL; *outNRec = 0;
  if( List )
     TA_Free(List);
  return rc;
}

TA_RetCode ReadCSIMMaster(const char *DirectoryName, struct MasterListRecordType **outMasterList, int *outNRec)
{
  int rc, nrec;
  char FileName[PATH_MAX];
  //char buffer[81];
  struct MasterListRecordType *List;
  struct MasterListRecordType *mr;
  struct MSMasterHdrType rec;
  FILE *fp;
  int i;
  
  *outMasterList = NULL; *outNRec = 0; List = NULL;

  {
    strcpy(FileName,DirectoryName);
    AddTrailingBackSlash(FileName);
    strcat(FileName,"Master");
    if(!file_exists(FileName)) {
      return TA_CSI_MASTER_FILE_NOT_FOUND; // no master file found
    }
  }

  fp = fopen(FileName,"rb");
  if(fp==NULL)
    return TA_CSI_MASTER_FILE_ACCESS_FAILED; // unable to open QMaster file

  rc = TA_SUCCESS;
  {      
      if(fread(&rec,sizeof(rec),1,fp)!=1) { rc = TA_CSI_HEADER_READ_FAILED_CSIM; goto ErrorReturn; } // CSIM header file too small
      nrec = rec.NumOfSecurities;
      List = (struct MasterListRecordType*)TA_Malloc(sizeof(struct MasterListRecordType)*nrec);
      if(List==NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated memory for Qmaster contents
      memset(List,0,sizeof(struct MasterListRecordType)*nrec);
      *outMasterList = (struct MasterListRecordType*)List;
      *outNRec = nrec;
  }
  for(i=0;i<nrec;i++) {
    struct CSIMMasterRawRecType rec;
    if(fread(&rec,sizeof(rec),1,fp)!=1) {
      nrec = i;
      break;
    }
    mr = &List[i];
    CopyPStrToZStr(mr->Name,sizeof(mr->Name), rec.Name,sizeof(rec.Name), 1);
    CopyPStrToZStr(mr->Symbol,sizeof(mr->Symbol), rec.Symbol,sizeof(rec.Symbol), 1);
    switch(rec.Period) {
      case 'D': mr->Period = TA_DAILY; break;
      case 'W': mr->Period = TA_WEEKLY; break;
      case 'M': mr->Period = TA_MONTHLY; break;
      default:
        { rc = TA_CSI_MASTER_BAD_PERIOD; goto ErrorReturn; } // invalid period in Qmaster record
    }
  }
  *outNRec = nrec;
  fclose(fp);
  return TA_SUCCESS;
ErrorReturn:
  fclose(fp);
  *outMasterList = NULL; *outNRec = 0;
  if( List  )
     TA_Free(List);
  return rc;
}

TA_RetCode ReadCSIMData(const char *DirectoryName, int Index, short cvf, SingleContractDataDayCore **outDataList, int *outNRec)
{
  int FileNumber;
  char FileName[PATH_MAX];
  FILE *fp;
  int nrec, rc;
  SingleContractDataDayCore *List;
  SingleContractDataDayCore *scdd;
  DatFileHeaderType daht;
  DatRecordType dart;
  int i;
  
  *outDataList = NULL; *outNRec = 0; List = NULL;

  FileNumber = Index+1;

  {
    strcpy(FileName,DirectoryName);
    AddTrailingBackSlash(FileName);
    sprintf(FileName+strlen(FileName),"F%d.dat",FileNumber);
    if(!file_exists(FileName))
      return TA_CSI_MISSING_CSIM_DATA_FILE; // no CSIM data file file found
  }

  fp = fopen(FileName,"rb");
  if(fp==NULL)
    return TA_CSI_FAIL_TO_OPEN_CSIM_DATA_FILE; // unable to open CSI data file

  nrec = 0;
  rc = TA_SUCCESS;
  {        
        if(!fread(&daht,sizeof(daht),1,fp)) { rc = TA_CSI_READ_FAIL_CSIM_HEADER_FILE; goto ErrorReturn; } // read failure on CSIM data file header
        nrec = daht.LastPostedRecord-1;
        List = (SingleContractDataDayCore*)TA_Malloc(sizeof(SingleContractDataDayCore)*nrec);
        if(List == NULL) { rc = TA_ALLOC_ERR; goto ErrorReturn; } // unable to allocated CSI data temporary
        memset(List,0,sizeof(SingleContractDataDayCore)*nrec);
        *outDataList = List;
  }

  for(i=0;i<nrec;i++) {        
        if(!fread(&dart,sizeof(dart),1,fp)) { rc = TA_CSI_READ_FAIL_CSIM_DATA_FILE; goto ErrorReturn; } // read failure on CSI data file
        scdd = &List[i];

        scdd->date = 19000000L+msbintoint(&dart.date);
        scdd->open = cDTP(cvf,msbintofloat(&dart.open));
        scdd->high = cDTP(cvf,msbintofloat(&dart.high));
        scdd->low = cDTP(cvf,msbintofloat(&dart.low));
        scdd->close = cDTP(cvf,msbintofloat(&dart.close));
        scdd->vol = msbintoint(&dart.vol);
        scdd->oi = msbintoint(&dart.oi);
  }
  *outNRec = nrec;
  fclose(fp);
  return TA_SUCCESS;
ErrorReturn:
  fclose(fp);
  *outDataList = NULL; *outNRec = 0;
  if( List )
     TA_Free(List);
  return rc;
}

/**** Local functions definitions.     ****/
// XXXX XXXX SMMM MMMM MMMM MMMM MMMM MMMM  (MSBIN)
// SXXX XXXX XMMM MMMM MMMM MMMM MMMM MMMM  (IEEE)
static int msbintoieee( msfloat *src4, float *dst4 )
{
   unsigned char szIeee[4], szExp;
   memcpy( szIeee, src4, 4 );
   if((szIeee[0]==0)&&(szIeee[1]==0)&&(szIeee[2]==0)&&(szIeee[3]==0)) {
     *dst4 = 0.0;
     return 0;
   }
   szExp=szIeee[3];
   szExp = (unsigned char)(szExp - 0x02);
   if( szExp == 0xFF )
    return 1;
   if( szIeee[2] & 0x80 )
     szIeee[3]=0x80;                  //  S   <-    S
   else
     szIeee[3]=0x00;
   szIeee[3]|=(unsigned char)(szExp>>1);             // -> XXXX XXXX
   if( szExp & 0x01 )
     szIeee[2]|=0x80;
   else
     szIeee[2]&=0x7F;
   memcpy( dst4, &szIeee, 4 );
   return 0;
}

static unsigned long msbintoint( msfloat *src ) {
  float t;
  msbintoieee( src,&t);
  return (unsigned long)t;
}


static float msbintofloat( msfloat *src ) {
  float t;
  msbintoieee(src,&t);
  return t;
}

static void AddTrailingBackSlash(char *PathName)
{
  int n = strlen(PathName);
  if(PathName[n-1]=='\\')
    return;
  if(n>=PATH_MAX-1)
    return; //GenExceptionW("PathName has blowed up: "+CString(PathName));
  PathName[n]=TA_SeparatorASCII(); n++;
  PathName[n]='\0';
}
static void CopyPStrToZStr(char *dest, short destsz, char *src, short srcsz, BOOL Stripping)
{
  short i;
  short sz=destsz; sz--;
  if(sz>srcsz)
    sz=srcsz;
  for(i=(short)(sz-1);i>=0;i--)
    dest[i] = src[i];
  dest[sz]='\0';
  if(Stripping) {
    for(sz--;(sz>=0)&&(dest[sz]==' ');sz--) dest[sz]='\0';
  }
}

static char Between(long v, long min_value, long max_value) { return (char)((v>=min_value)&&(v<=max_value)?1:0); }

static hyper chDTP(short cvf, double dec)
{
  hyper p1, p2;  
  BOOL Negative;
  long ppu, n;

  assert((cvf>= -8)||(cvf<=6));
  Negative = (dec<0);

  if(Negative)
  	dec = -dec;
	assert(Between(cvf,MINCVF,MAXCVF));
	ppu = PriConv_PntsPerUnit[cvf-MINCVF];
	dec += (double)1/5/ppu;
	p1 = (hyper)dec;
	p2 = (hyper)(ppu*(dec-p1));
	if(cvf<= -7) {
		hyper p3 = p2 % (cvf == -7?2:4);
		p2 = p2 / (cvf == -7?2:4);
		return (Negative?-1:1)*(p1*1000+p2*10+5*p3/(cvf == -7?1:2));
	}
	n = PriConv_NumDigits[cvf-MINCVF];
	return (Negative?-1:1)*(n*p1+p2);
}

static long cDTP(short cvf, double dec)
{
  hyper rc = chDTP(cvf,dec);
  if(rc>(hyper)0x7FFFFFFF)
    rc = 0x7FFFFFFF;
  else if(rc< (hyper)-0x7FFFFFFF)
    rc = -0x7FFFFFFF;
  return (long)rc;
}


#if 0
int ReadUaApiContract(TCOMIAPI2 ua, long csinum, int IsStock, long dydm, long strike, SingleContractDataDayCore **outDataList, int *outNRec)
{
  int nrec;
  short cvf; 
  SingleContractDataDayCore *List;
  int rc;

  TVariant DateArray, DOWArray, DydmArray;
  TVariant OpenArray, HighArray, LowArray, CloseArray;
  TVariant ClosingBidArray, ClosingAskArray;
  TVariant VolumeArray, OIArray, TotalVolumeArray, TotalOIArray;
  TVariant CashArray;

  long *cDateArray, *cVolArray, *cOiArray;
  float *cOpenArray, *cHighArray, *cLowArray, *cCloseArray;

  *outDataList = NULL; *outNRec = 0;
  List = NULL;

  {
    ua->IsStock = IsStock;
    ua->MarketNumber = csinum;
    if(ua->GetMarketProfile()<0) return 1; // UA Api csinum does not correspond to a current commodity or stock
    if(strike==0)
      cvf = ua->ConversionFactor;
    else
      cvf = ua->OptionConversionFactor;
  }
  
  if(strike==0) {
    if(IsStock) {
      nrec = ua->RetrieveStock(csinum,-1,-1);
    } else {
      nrec = ua->RetrieveContract(csinum, dydm,-1,-1);
    }
  } else {
    nrec = ua->RetrieveOptionSeries(csinum,IsStock,dydm,strike,-1,-1);
  }
  if(!nrec)
    return 2; // UA Api not data found

  rc = 0;

  {
      nrec = ua->CopyRetrievedDataToArray((short)0, // Daily data
        &DateArray, &DOWArray, &DydmArray,
        &OpenArray, &HighArray, &LowArray, &CloseArray,
        &ClosingBidArray, &ClosingAskArray,
        &VolumeArray, &OIArray, &TotalVolumeArray, &TotalOIArray,
        &CashArray);
      if(nrec<0)
        return 3; // unable to copy retrieved data from UA
      
      SafeArrayAccessData(DateArray.parray,(void**)&cDateArray);
      SafeArrayAccessData(OpenArray.parray,(void**)&cOpenArray);
      SafeArrayAccessData(HighArray.parray,(void**)&cHighArray);
      SafeArrayAccessData(LowArray.parray,(void**)&cLowArray);
      SafeArrayAccessData(CloseArray.parray,(void**)&cCloseArray);
      SafeArrayAccessData(VolumeArray.parray,(void**)&cVolArray);
      SafeArrayAccessData(OIArray.parray,(void**)&cOiArray);

  { //SingleContractDataDayCore *List; {
        List = (SingleContractDataDayCore*)TA_Malloc(sizeof(SingleContractDataDayCore)*nrec);
        if(List == NULL) { rc = 3; goto ErrorReturn; } // unable to allocated CSI data temporary
        memset(List,0,sizeof(SingleContractDataDayCore)*nrec);
        *outDataList = List;
  }
  for(int i = 0;i<nrec;i++) {
        SingleContractDataDayCore &scdd = List[i];

        scdd.date = cDateArray[i];
        scdd.open = cDTP(cvf,cOpenArray[i]);
        scdd.high = cDTP(cvf,cHighArray[i]);
        scdd.low = cDTP(cvf,cLowArray[i]);
        scdd.close = cDTP(cvf,cCloseArray[i]);
        scdd.vol = cVolArray[i];
        scdd.oi = cOiArray[i];
  }
      SafeArrayUnaccessData(OIArray.parray);
      SafeArrayUnaccessData(VolumeArray.parray);
      SafeArrayUnaccessData(CloseArray.parray);
      SafeArrayUnaccessData(LowArray.parray);
      SafeArrayUnaccessData(HighArray.parray);
      SafeArrayUnaccessData(OpenArray.parray);
      SafeArrayUnaccessData(DateArray.parray);
  }    
  *outNRec = nrec;
  return 0;
ErrorReturn:
  *outDataList = NULL; *outNRec = 0;
  if(List!=NULL) TA_Free(List);
  return rc;
}
#endif

#if 0
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
  Timer1->Enabled = false;

  SingleContractDataDayCore *DataList;
  struct MasterListRecordType *MasterList;
  int nrec;
  AnsiString t;

  if(1) {
    t = "";  Memo1->Lines->Add(t);
    t = "ReadCSIMaster"; Memo1->Lines->Add(t);
    ReadCSIMaster("testdata",&MasterList,&nrec);
    for(int i=0;i<nrec;i++) {
      struct MasterListRecordType &mr = MasterList[i];
      if(mr.Deleted!=0)
        continue;
      t = mr.Symbol; t += " "; t += mr.dydm; t += " "; t += mr.strike;
      Memo1->Lines->Add(t);
    }
    TA_Free(MasterList);
  }
  if(1) {
    t = "";  Memo1->Lines->Add(t);
    t = "ReadCSIData"; Memo1->Lines->Add(t);
    ReadCSIData("testdata",0,-1,&DataList,&nrec);
    for(int i=0;i<nrec;i+=(nrec-1)) {
      SingleContractDataDayCore &r = DataList[i];
      t = IntToStr(r.date) + " " + IntToStr(r.close);
      Memo1->Lines->Add(t);
    }
    TA_Free(DataList);
  }

  if(1) {
    t = "";  Memo1->Lines->Add(t);
    t = "ReadCSIMMaster"; Memo1->Lines->Add(t);
    ReadCSIMMaster("testdata",&MasterList,&nrec);
    for(int i=0;i<nrec;i++) {
      struct MasterListRecordType &mr = MasterList[i];
      if(mr.Deleted!=0)
        continue;
      t = mr.Symbol; t += "/"; t += mr.Name;
      Memo1->Lines->Add(t);
    }
    TA_Free(MasterList);
  }
  if(1) {
    t = "";  Memo1->Lines->Add(t);
    t = "ReadCSIMData"; Memo1->Lines->Add(t);
    ReadCSIMData("testdata",0,-1,&DataList,&nrec);
    for(int i=0;i<nrec;i+=(nrec-1)) {
      SingleContractDataDayCore &r = DataList[i];
      t = IntToStr(r.date) + " " + IntToStr(r.close);
      Memo1->Lines->Add(t);
    }
    TA_Free(DataList);
  }

  //ReadAsciiMaster("testdata",&MasterList,&nrec); free(MasterList);
  //ReadAsciiData(1,&DataList,&nrec); free(DataList);

  if(1) {
    t = "";  Memo1->Lines->Add(t);
    t = "ReadUaApiContract"; Memo1->Lines->Add(t);
    TCOMIAPI2 ua;
    ua = CoAPI2::Create();
    ua->AddRef();
    ua->WindowState = 1; // hide UA
    ua->ShowDecimalPoint=1;
    ua->IncludeHolidays=0;
    ua->IncludeSaturdays=0;
    ua->IsStock=1;

    ReadUaApiContract(ua,9,0,200412,3600,&DataList,&nrec);
    for(int i=0;i<nrec;i+=(nrec-1)) {
      SingleContractDataDayCore &r = DataList[i];
      t = IntToStr(r.date) + " " + IntToStr(r.close);
      Memo1->Lines->Add(t);
    }
    free(DataList);

    ua->Release();
  }
}
static msfloat inttomsbin( unsigned long isrc ) {
  float src=(float)isrc;
  msfloat dest;
  ieeetomsbin(&src,&dest);
  return dest;
}

static msfloat floattomsbin( float isrc ) {
  float src=isrc;
  msfloat dest;
  ieeetomsbin(&src,&dest);
  return dest;
}

static double cPTD(short cvf, long points, BOOL RoundToNearestTick, const double *TicksPerUnit)
{
  return chPTD(cvf, points, RoundToNearestTick, TicksPerUnit);
}

// SXXX XXXX XMMM MMMM MMMM MMMM MMMM MMMM  (IEEE)
// XXXX XXXX SMMM MMMM MMMM MMMM MMMM MMMM  (MSBIN)
static int ieeetomsbin( float *src4, msfloat *dst4 )
{
   unsigned char szMsbin[4], szExp;
   memcpy( szMsbin, src4, 4 );
   szExp= (unsigned char)(szMsbin[3]<<1);
   szExp|= (unsigned char)(szMsbin[2]>>7);             // XXXX XXXX <-
   szExp= (unsigned char)(szExp +0x02);
   if( szExp == 0xFF )
    return 1;

   if( szMsbin[3] & 0x80 )           // S   ->    S
     szMsbin[2]|=0x80;
   else
     szMsbin[2]&=0x7F;
   szMsbin[3]=szExp;
   memcpy( dst4, &szMsbin, 4 );
   return 0;
}

static double chPTD(short cvf, hyper points, BOOL RoundToNearestTick, const double *TicksPerUnit)
{
  double v;
  hyper p1, p2, p3;
  long n, ppu, InputScale;

  BOOL Negative = (points<0);
  if(Negative)
  	points = -points;
  assert(Between(cvf,MINCVF,MAXCVF));
  if(cvf<= -7) { // -7 is a restricted -8
    p3 = ((points % 10)*10+5)/25; // between 0 and 3
    p2 = (points/10)%100; // between 0 and 31
    p1 = points/1000;
    return ((Negative?-1:1)*((128*p1+4*p2+p3)/(double)128));
  }
  n = PriConv_NumDigits[cvf-MINCVF];
  p2 = points % n; p1 = points / n;
  ppu = PriConv_PntsPerUnit[cvf-MINCVF];
  v = (((Negative?-1:1)*(ppu*p1+p2))/(double)ppu);
  if(RoundToNearestTick&&(TicksPerUnit!=NULL)) {
     InputScale = DecimalOutputScale[cvf-MINCVF];
     v += 4.5/InputScale/10;
     v = round(v*(*TicksPerUnit))/(*TicksPerUnit);
  }
  return v;
}

static long DecimalOutputScale[] =
  // -8 -7
{  10000L, 10000L, 10000L, 10000L,
   1000L, 1000L, 1000L, 100L, 1, 10, 100, 1000, 10000, 100000L, 1000000L, 10000000L, 100000000L };

static long round(double x)
{
  return (long)(x>0?floor(((double)1/2)+x):-floor(((double)1/2)-x));
}

//---------------------------------------------------------------------------
#endif

