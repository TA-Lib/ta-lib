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

/* Boolean values to indicate which functionality is supported by this data
 * source driver.
 *
 * supportUpdateIndex
 *    Indicates that this data source can be modified for adding, removing
 *    changing the symbols and category information.
 *
 * supportUpdateSymbol
 *    Indicates that this data source can modify the data of a particular
 *    symbol.
 *
 * supportCallback
 *   Indicates that this data source can provides callback functionality when
 *   new data is available.
 *
 * supportMarketData
 *   Indicates that this data source can provide the latest market
 *   data (last trade, bid, ask etc...).
 *
 * slowAccess
 *   Indicates that the retreival of the data cannot be done as fast as a
 *   locally accessible database. This may result into multithreading when
 *   retreiving data. Any data source relying solely on internet should be
 *   characterize as slow access.
 */
typedef struct
{
    unsigned int supportUpdateIndex;
    unsigned int supportUpdateSymbol;
    unsigned int supportCallback;
    unsigned int supportMarketData;
    unsigned int slowAccess;
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
 * GetHistoryData() function only! (see below)
 * It allows to add the data who is going to be used in the merging
 * mechanism for building the TA_History.
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

typedef struct
{
    /* The driver initialization is the VERY first
     * function called. This allows the driver to initialize
     * its own global variables etc...
     */
    TA_RetCode (*initializeSourceDriver)( TA_Libc *libHandle );

    /* Free all ressources. */
    TA_RetCode (*shutdownSourceDriver)( TA_Libc *libHandle );

    /* Allows the data source to indicate which capability
     * are available from it.
     */
    TA_RetCode (*getParameters)( TA_Libc *libHandle,
                                 TA_DataSourceParameters *param );

    /* The data cannot be access from the source without
     * opening it.
     *
     * When a source is open with the access TA_OPEN_READ_ONLY,
     * no modification are going to be attempted on the data. Only
     * the following function are going to be used from this point:
     *      - GetXXXXCategoryHandle
     *      - GetXXXXSymbolHandle
     *      - GetHistoryData
     *      - GetMarketData
     *      - CloseSource
     *
     * A driver should allow to open multiple source simultaneouly.
     * The TA_DataSourceHandle allows to differentiate the instances.
     *
     * TA_AddDataSourceParam is a local copy of all the parameters
     * pass by the library user. The data source can safely keep
     * a pointer on these parameters until CloseSource gets called.
     */
    TA_RetCode (*openSource)( TA_Libc *libHandle,
                              const TA_AddDataSourceParamPriv *param,
                              TA_DataSourceHandle **handle );

    TA_RetCode (*closeSource)( TA_Libc *libHandle,
                               TA_DataSourceHandle *handle );

    /* The followings allows to get the "index" of the category and symbol
     * provided by this data source.
     *
     * The driver can provides 'opaque' information that can be re-feed
     * to the driver for getting further information on a certain entity.
     * This opaque information is hidden in the handles.
     *
     * The categoryHandle and symbolHandle are already allocated, the driver
     * just need to fill up the information.
     *
     * The caller assume the pointers set in the 'handles' are valid
     * the source is closed with CloseSource.
     * The caller will NEVER de-allocate the 'string' found in the handles.
     *
     * Must return TA_END_OF_INDEX when no further elements are available.
     */
    TA_RetCode (*getFirstCategoryHandle)( TA_Libc *libHandle,
                                          TA_DataSourceHandle *handle,
                                          TA_CategoryHandle *categoryHandle );

    TA_RetCode (*getNextCategoryHandle) ( TA_Libc *libHandle,
                                          TA_DataSourceHandle *handle,
                                          TA_CategoryHandle *categoryHandle,
                                          unsigned int index );

    TA_RetCode (*getFirstSymbolHandle)( TA_Libc *libHandle,
                                        TA_DataSourceHandle *handle,
                                        TA_CategoryHandle *categoryHandle,
                                        TA_SymbolHandle *symbolHandle );

    TA_RetCode (*getNextSymbolHandle)( TA_Libc *libHandle,
                                       TA_DataSourceHandle *handle,
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
    TA_RetCode (*getHistoryData)( TA_Libc *libHandle,
                                  TA_DataSourceHandle *handle,
                                  TA_CategoryHandle   *categoryHandle,
                                  TA_SymbolHandle     *symbolHandle,
                                  TA_Period            period,
                                  const TA_Timestamp  *start,
                                  const TA_Timestamp  *end,
                                  TA_Field             fieldToAlloc,
                                  TA_ParamForAddData  *paramForAddData );

    /* !!! Note: Function to update the database not defined. */

    /* !!! Note: Function to setup a callback for realtime feed not define. */

    /* !!! Note: Function to get market data not define. */

} TA_DataSourceDriver;

/* Global variables defined in ta_source.c */
extern const TA_DataSourceDriver  TA_gDataSourceTable[];
extern const unsigned int         TA_gDataSourceTableSize;

#endif

