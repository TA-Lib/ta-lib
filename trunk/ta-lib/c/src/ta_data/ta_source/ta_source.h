#ifndef TA_SOURCE_H
#define TA_SOURCE_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_ADDDATASOURCEPARAMPRIV_H
   #include "ta_adddatasourceparam_priv.h"
#endif

/* Flags to indicate which functionality is supported by this data
 * source driver.
 *
 * TA_SLOW_ACCESS
 *   Indicates that the retreival of the data cannot be done as fast as a
 *   locally accessible database. This may result into multithreading when
 *   retreiving data. Any data source relying solely on internet should be
 *   characterize as slow access.
 *
 * TA_LOCATION_IS_PATH
 *   Indicates that the "location" parameter when doing TA_AddDataSource
 *   is expected to be a filesystem path. In that case, TA-Lib might adapt
 *   the location (depending of the platform on which it is running) before
 *   passing it to the driver.
 *
 * The list of flags here are not accessible to the TA-Lib user.
 * See ta_defs.h for the flags accessible to the TA-Lib user.
 */

/* Note: for the 16 less significant bits, see ta_defs.h */
#define TA_SLOW_ACCESS      (1<<16)
#define TA_LOCATION_IS_PATH (1<<17)

typedef struct
{
   unsigned int flags;
} TA_DataSourceParameters;

/* The following handles allow the data source to pass some needed
 * information back to the TA-LIB.
 * But the most important aspect of these handle is to provide a way
 * to hide driver specific information into the 'opaque' pointer.
 */
typedef struct
{
   /* Public value that must be initialize by 'openSource()'. */
   unsigned int nbCategory;  /* Number of category in that data source. */

   /* Private opaque data. The data source driver can set that
    * pointer to whatever they need.
    */
   void *opaqueData;
} TA_DataSourceHandle;

typedef struct
{
   /* Public value that must be initialize by 'getXXXXXCategoryHandle()'. */
   TA_String *string;     /* String of this category. */
   unsigned int nbSymbol; /* Number of symbol under that category. */

   /* Private opaque data. The data source driver can set that
    * pointer to whatever they need.
    */
   void *opaqueData;
} TA_CategoryHandle;

typedef struct
{
   /* Public value that must be initialize by 'getXXXXXSymbolHandle()'. */
   TA_String *string;  /* String for the symbol (ticker). */

   /* Private opaque data. The data source driver can set that
    * pointer to whatever they need.
    */
   void *opaqueData;
} TA_SymbolHandle;

/* The following function can be called by the driver from the
 * getHistoryData() function only! (see below)
 * It allows to add the data who is going to be used in the merging
 * mechanism for building the TA_History.
 *
 * The added data must have been allocated with TA_Malloc.
 * Once the pointer is pass to TA_HistoryData, the driver
 * is not the owner of that data anymore, and it should never
 * attempt to free it.
 */
typedef unsigned int TA_ParamForAddData; /* hidden implementation. */

TA_RetCode TA_HistoryAddData( TA_ParamForAddData *paramForAddData,
                              unsigned int nbBarAdded,
                              TA_Period period,
                              TA_Timestamp *timestamp,
                              TA_Real *open,
                              TA_Real *high,
                              TA_Real *low,
                              TA_Real *close,
                              TA_Integer *volume,
                              TA_Integer *openInterest );

/* The following functions can be called by the driver from the
 * GetHistoryData() function only! (see below)
 *
 * It allows to add split/value adjustment for building
 * the TA_History.
 *
 * In other word, the driver adds the raw data and the adjustment
 * are done by TA-Lib (as needed).
 */
TA_RetCode TA_HistoryAddSplitAdjust( TA_ParamForAddData *paramForAddData, TA_Timestamp *when, double factor );
TA_RetCode TA_HistoryAddValueAdjust( TA_ParamForAddData *paramForAddData, TA_Timestamp *when, double amount );

/* Allows the data source driver to cancel all the data
 * who was added up to now. This might be useful if
 * the data source driver needs to restart the processing
 * of adding the data.
 */
TA_RetCode TA_HistoryAddDataReset( TA_ParamForAddData *paramForAddData );

/* The source driver can get some info who is derived from all
 * the added data up to now.
 */
typedef struct
{
   /* These reflect the cummulation of ALL bar added up to now */    
   TA_Timestamp highestTimestamp;
   TA_Timestamp lowestTimestamp;

   /* These are reset EVERYTIME TA_GetInfoFromAddedData is called.
    * In other word, these reflect the cummulation of what
    * happen since the las TA_GetInfoFromAddedData was called.
    * Take note that this function is reserved exclusively
    * for being called by the data source driver.
    */
   unsigned int barAddedSinceLastCall; /* boolean */
   TA_Timestamp lowestTimestampAddedSinceLastCall;
   TA_Timestamp highestTimestampAddedSinceLastCall; 
} TA_InfoFromAddedData;

TA_RetCode TA_GetInfoFromAddedData( TA_ParamForAddData *paramForAddData,
                                    TA_InfoFromAddedData *info );


typedef struct
{
    const char *defaultName;

    /* The driver initialization is the VERY first
     * function called. This allows the driver to initialize
     * its own global variables etc...
     */
    TA_RetCode (*initializeSourceDriver)( void );

    /* Free all ressources. Call only if the initialization
     * succeed.
     */
    TA_RetCode (*shutdownSourceDriver)( void );

    /* Allows the data source to indicate which capability
     * are available from it. Can be called before the open
     * but only after the initialization.
     */
    TA_RetCode (*getParameters)( TA_DataSourceParameters *param );

    /* The data cannot be access from the source without
     * opening it.
     *
     * A driver should allow to open multiple source simultaneouly.
     * The TA_DataSourceHandle allows to differentiate the instances.
     *
     * TA_AddDataSourceParam is a local copy of all the parameters
     * pass by the library user. The data source can safely keep
     * a pointer on these parameters until closeSource gets called.
     */
    TA_RetCode (*openSource)( const TA_AddDataSourceParamPriv *param,
                              TA_DataSourceHandle **handle );

    TA_RetCode (*closeSource)( TA_DataSourceHandle *handle );

    /* The followings allows to get the "index" of the category and symbol
     * provided by this data source.
     *
     * The driver can set 'opaque' information to each category/symbol.
     *
     * The categoryHandle and symbolHandle are already allocated, the driver
     * just need to fill up the information.
     *
     * The pointers in the 'handles' must stay valid until the source is 
     * closed with closeSource.
     *
     * The driver is responsible to de-allocate the 'string' found
     * in the handles when closeSource will get called.
     *
     * Must return TA_END_OF_INDEX if no further elements are available.
     */
    TA_RetCode (*getFirstCategoryHandle)( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle *categoryHandle );

    TA_RetCode (*getNextCategoryHandle) ( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle *categoryHandle,
                                          unsigned int index );

    TA_RetCode (*getFirstSymbolHandle)( TA_DataSourceHandle *handle,
                                        TA_CategoryHandle *categoryHandle,
                                        TA_SymbolHandle *symbolHandle );

    TA_RetCode (*getNextSymbolHandle)( TA_DataSourceHandle *handle,
                                       TA_CategoryHandle *categoryHandle,
                                       TA_SymbolHandle *symbolHandle,
                                       unsigned int index );

    /* GetHistoryData allows the driver to provides the data used to
     * build the TA_History.
     *
     * The main responsibility of this function is to retreive the data
     * corresponding to the symbol and pass it to the library by
     * doing one or many call to TA_HistoryAddData().
     */
    TA_RetCode (*getHistoryData)( TA_DataSourceHandle *handle,
                                  TA_CategoryHandle   *categoryHandle,
                                  TA_SymbolHandle     *symbolHandle,
                                  TA_Period            period,
                                  const TA_Timestamp  *start,
                                  const TA_Timestamp  *end,
                                  TA_Field             fieldToAlloc,
                                  TA_ParamForAddData  *paramForAddData );

} TA_DataSourceDriver;

/* Global variables defined in ta_source.c */
extern const TA_DataSourceDriver  TA_gDataSourceTable[];
extern const unsigned int         TA_gDataSourceTableSize;

#endif

