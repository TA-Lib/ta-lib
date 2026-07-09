/* ta_abstract_serve.c — Generic JSON-RPC handlers for the ta_abstract API.
 *
 * These handlers use the public ta_abstract API (TA_GetFuncHandle, TA_CallFunc,
 * etc.) to dispatch calls generically — no per-function code needed.
 * This tests the ta_abstract layer itself, not just the indicator functions.
 *
 * This file is #included by the generated server (ta_codegen_serve.c).
 * It depends on:
 *   - JSON helpers: json_find_string, json_find_int, json_find_double,
 *     json_find_double_array, json_write_double_array, json_write_int_array
 *   - Global buffers: g_inBuf0..5, g_outBuf0..2, g_outIntBuf0..1, MAX_ARRAY_SIZE
 *   - ta_abstract.h (included below)
 *
 * When porting to other languages, rewrite this file in the target language
 * using that language's ta_abstract implementation.
 */

#include "ta_abstract.h"

/* ---- abstract_for_each_func ----
 * Mirrors: TA_ForEachFunc
 * Returns: list of all functions with metadata
 */

typedef struct {
   char *buf;
   int   buf_size;
   int   pos;
   int   count;
} ForEachCtx;

static void abstract_for_each_cb(const TA_FuncInfo *funcInfo, void *opaque) {
   ForEachCtx *ctx = (ForEachCtx *)opaque;
   const char *comma = ctx->count > 0 ? "," : "";
   ctx->pos += snprintf(ctx->buf + ctx->pos, ctx->buf_size - ctx->pos,
      "%s{\"name\":\"%s\",\"group\":\"%s\",\"nbInput\":%u,\"nbOptInput\":%u,\"nbOutput\":%u}",
      comma, funcInfo->name, funcInfo->group,
      funcInfo->nbInput, funcInfo->nbOptInput, funcInfo->nbOutput);
   ctx->count++;
}

static void handle_abstract_for_each_func(const char *json, char *resp, int resp_size) {
   (void)json;
   ForEachCtx ctx;
   ctx.buf = resp;
   ctx.buf_size = resp_size;
   ctx.pos = snprintf(resp, resp_size, "{\"functions\":[");
   ctx.count = 0;
   TA_ForEachFunc(abstract_for_each_cb, &ctx);
   snprintf(resp + ctx.pos, resp_size - ctx.pos, "]}");
}

/* ---- Helper: parse funcName from JSON ---- */

static int abstract_parse_func_name(const char *json, char *funcName, int funcNameSize,
   const TA_FuncHandle **handle, char *resp, int resp_size)
{
   int nameLen = 0;
   const char *nameRaw = json_find_string(json, "funcName", &nameLen);
   if( !nameRaw ) {
      snprintf(resp, resp_size, "{\"error\":\"Missing funcName\"}");
      return 0;
   }
   if( nameLen >= funcNameSize ) {
      snprintf(resp, resp_size, "{\"error\":\"funcName too long\"}");
      return 0;
   }
   memcpy(funcName, nameRaw, nameLen);
   funcName[nameLen] = '\0';

   if( TA_GetFuncHandle(funcName, handle) != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"error\":\"Unknown function: %s\"}", funcName);
      return 0;
   }
   return 1;
}

/* ---- abstract_get_lookback ----
 * Mirrors: TA_GetFuncHandle + TA_ParamHolderAlloc + TA_SetOptInput* + TA_GetLookback
 * Params:  funcName, optional param values (by paramName)
 * Returns: lookback value
 */

static void handle_abstract_get_lookback(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   TA_ParamHolder *params;
   if( TA_ParamHolderAlloc(handle, &params) != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"error\":\"ParamHolderAlloc failed\"}");
      return;
   }

   const TA_FuncInfo *fi;
   TA_GetFuncInfo(handle, &fi);

   /* Set optional params from JSON (by paramName) */
   for( unsigned int i = 0; i < fi->nbOptInput; i++ ) {
      const TA_OptInputParameterInfo *optInfo;
      TA_GetOptInputParameterInfo(handle, i, &optInfo);
      switch( optInfo->type ) {
         case TA_OptInput_RealRange:
            case TA_OptInput_RealList: {
            double v = json_find_double(json, optInfo->paramName);
            TA_SetOptInputParamReal(params, i, v);
            break;
         }
         case TA_OptInput_IntegerRange:
            case TA_OptInput_IntegerList: {
            int v = json_find_int(json, optInfo->paramName);
            TA_SetOptInputParamInteger(params, i, v);
            break;
         }
      }
   }

   TA_Integer lookback = 0;
   TA_GetLookback(params, &lookback);
   TA_ParamHolderFree(params);

   snprintf(resp, resp_size, "{\"lookback\":%d}", (int)lookback);
}

/* ---- abstract_call ----
 * Mirrors: Full ta_abstract call path (GetFuncHandle → ParamHolderAlloc →
 *          SetInput* → SetOptInput* → SetOutput* → CallFunc → GetLookback → Free)
 * Params:  funcName, startIdx, endIdx, input arrays, optional params
 * Returns: retCode, outBegIdx, outNBElement, lookback, output arrays
 */

static void handle_abstract_call(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   TA_ParamHolder *params;
   if( TA_ParamHolderAlloc(handle, &params) != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"error\":\"ParamHolderAlloc failed\"}");
      return;
   }

   const TA_FuncInfo *fi;
   TA_GetFuncInfo(handle, &fi);

   int startIdx = json_find_int(json, "startIdx");
   int endIdx   = json_find_int(json, "endIdx");

   /* Handle unstable period if provided */
   int funcUnstId = json_find_int(json, "funcUnstId");
   int unstPeriod = json_find_int(json, "unstablePeriod");
   if( unstPeriod > 0 || funcUnstId > 0 ) {
      TA_SetUnstablePeriod((TA_FuncUnstId)funcUnstId, (unsigned int)unstPeriod);
   }

   /* Set inputs based on ta_abstract metadata */
   int totalRealInputs = 0;
   for( unsigned int i = 0; i < fi->nbInput; i++ ) {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo(handle, i, &ii);
      if( ii->type == TA_Input_Real ) totalRealInputs++;
   }

   int realInputCount = 0;
   for( unsigned int i = 0; i < fi->nbInput; i++ ) {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo(handle, i, &ii);

      switch( ii->type ) {
         case TA_Input_Price: {
            double *pOpen = NULL, *pHigh = NULL, *pLow = NULL;
            double *pClose = NULL, *pVol = NULL, *pOI = NULL;
            if( ii->flags & TA_IN_PRICE_OPEN ) {
               json_find_double_array(json, "inOpen", g_inBuf0, MAX_ARRAY_SIZE);
               pOpen = g_inBuf0;
            }
            if( ii->flags & TA_IN_PRICE_HIGH ) {
               json_find_double_array(json, "inHigh", g_inBuf1, MAX_ARRAY_SIZE);
               pHigh = g_inBuf1;
            }
            if( ii->flags & TA_IN_PRICE_LOW ) {
               json_find_double_array(json, "inLow", g_inBuf2, MAX_ARRAY_SIZE);
               pLow = g_inBuf2;
            }
            if( ii->flags & TA_IN_PRICE_CLOSE ) {
               json_find_double_array(json, "inClose", g_inBuf3, MAX_ARRAY_SIZE);
               pClose = g_inBuf3;
            }
            if( ii->flags & TA_IN_PRICE_VOLUME ) {
               json_find_double_array(json, "inVolume", g_inBuf4, MAX_ARRAY_SIZE);
               pVol = g_inBuf4;
            }
            if( ii->flags & TA_IN_PRICE_OPENINTEREST ) {
               json_find_double_array(json, "inOpenInterest", g_inBuf5, MAX_ARRAY_SIZE);
               pOI = g_inBuf5;
            }
            TA_SetInputParamPricePtr(params, i, pOpen, pHigh, pLow, pClose, pVol, pOI);
            break;
         }
         case TA_Input_Real: {
            char key[32];
            int bufIdx;
            if( totalRealInputs > 1 ) {
               snprintf(key, sizeof(key), "inReal%d", realInputCount);
               bufIdx = realInputCount;
            } else {
               snprintf(key, sizeof(key), "inReal");
               bufIdx = 0;
            }
            /* Map to g_inBuf0..5 */
            double *buf = (bufIdx == 0) ? g_inBuf0 :
            (bufIdx == 1) ? g_inBuf1 :
            (bufIdx == 2) ? g_inBuf2 : g_inBuf3;
            json_find_double_array(json, key, buf, MAX_ARRAY_SIZE);
            TA_SetInputParamRealPtr(params, i, buf);
            realInputCount++;
            break;
         }
         case TA_Input_Integer:
            /* No current TA functions use integer inputs */
            break;
      }
   }

   /* Set optional inputs from JSON (by paramName) */
   for( unsigned int i = 0; i < fi->nbOptInput; i++ ) {
      const TA_OptInputParameterInfo *optInfo;
      TA_GetOptInputParameterInfo(handle, i, &optInfo);
      switch( optInfo->type ) {
         case TA_OptInput_RealRange:
            case TA_OptInput_RealList: {
            double v = json_find_double(json, optInfo->paramName);
            TA_SetOptInputParamReal(params, i, v);
            break;
         }
         case TA_OptInput_IntegerRange:
            case TA_OptInput_IntegerList: {
            int v = json_find_int(json, optInfo->paramName);
            TA_SetOptInputParamInteger(params, i, v);
            break;
         }
      }
   }

   /* Set output buffers based on ta_abstract metadata */
   int outputIsInteger[3] = {0, 0, 0};
   int realOutIdx = 0, intOutIdx = 0;
   for( unsigned int i = 0; i < fi->nbOutput && i < 3; i++ ) {
      const TA_OutputParameterInfo *oi;
      TA_GetOutputParameterInfo(handle, i, &oi);
      if( oi->type == TA_Output_Integer ) {
         TA_SetOutputParamIntegerPtr(params, i,
            intOutIdx == 0 ? g_outIntBuf0 : g_outIntBuf1);
         outputIsInteger[i] = 1;
         intOutIdx++;
      } else {
         TA_SetOutputParamRealPtr(params, i,
            realOutIdx == 0 ? g_outBuf0 :
            realOutIdx == 1 ? g_outBuf1 : g_outBuf2);
         realOutIdx++;
      }
   }

   /* Call the function via ta_abstract */
   int outBegIdx = 0, outNBElement = 0;
   TA_RetCode rc = TA_CallFunc(params, startIdx, endIdx, &outBegIdx, &outNBElement);

   /* Get lookback */
   TA_Integer lookback = 0;
   TA_GetLookback(params, &lookback);

   TA_ParamHolderFree(params);

   /* Build response */
   int pos = snprintf(resp, resp_size,
      "{\"retCode\":%d,\"outBegIdx\":%d,\"outNBElement\":%d,\"lookback\":%d",
      (int)rc, outBegIdx, outNBElement, (int)lookback);

   /* Serialize outputs with correct key naming (per-type counters) */
   int realKeyIdx = 0, intKeyIdx = 0;
   for( unsigned int i = 0; i < fi->nbOutput && i < 3; i++ ) {
      if( outputIsInteger[i] ) {
         const char *key = intKeyIdx == 0 ? "outInteger" : "outInteger1";
         int *buf = intKeyIdx == 0 ? g_outIntBuf0 : g_outIntBuf1;
         pos += snprintf(resp + pos, resp_size - pos, ",\"%s\":", key);
         pos += json_write_int_array(resp + pos, resp_size - pos, buf, outNBElement);
         intKeyIdx++;
      } else {
         const char *key = realKeyIdx == 0 ? "outReal" :
         realKeyIdx == 1 ? "outReal1" : "outReal2";
         double *buf = realKeyIdx == 0 ? g_outBuf0 :
         realKeyIdx == 1 ? g_outBuf1 : g_outBuf2;
         pos += snprintf(resp + pos, resp_size - pos, ",\"%s\":", key);
         pos += json_write_double_array(resp + pos, resp_size - pos, buf, outNBElement);
         realKeyIdx++;
      }
   }

   snprintf(resp + pos, resp_size - pos, "}");
}

/* ---- TA_GetFuncInfo ----
 * Mirrors: TA_GetFuncHandle + TA_GetFuncInfo
 * Params:  funcName
 * Returns: name, group, hint, camelCaseName, flags, nbInput, nbOptInput, nbOutput
 */

static void handle_TA_GetFuncInfo(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   const TA_FuncInfo *fi;
   TA_GetFuncInfo(handle, &fi);

   snprintf(resp, resp_size,
      "{\"name\":\"%s\",\"group\":\"%s\",\"hint\":\"%s\","
      "\"camelCaseName\":\"%s\",\"flags\":%d,"
      "\"nbInput\":%u,\"nbOptInput\":%u,\"nbOutput\":%u}",
      fi->name, fi->group,
      fi->hint ? fi->hint : "",
      fi->camelCaseName ? fi->camelCaseName : "",
      (int)fi->flags,
      fi->nbInput, fi->nbOptInput, fi->nbOutput);
}

/* ---- TA_GetInputParameterInfo ----
 * Mirrors: TA_GetInputParameterInfo
 * Params:  funcName, paramIndex
 * Returns: type, paramName, flags
 */

static void handle_TA_GetInputParameterInfo(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   int paramIndex = json_find_int(json, "paramIndex");

   const TA_InputParameterInfo *info;
   TA_RetCode rc = TA_GetInputParameterInfo(handle, (unsigned int)paramIndex, &info);
   if( rc != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"retCode\":%d}", (int)rc);
      return;
   }

   snprintf(resp, resp_size,
      "{\"type\":%d,\"paramName\":\"%s\",\"flags\":%d}",
      (int)info->type,
      info->paramName ? info->paramName : "",
      (int)info->flags);
}

/* ---- TA_GetOptInputParameterInfo ----
 * Mirrors: TA_GetOptInputParameterInfo
 * Params:  funcName, paramIndex
 * Returns: type, paramName, flags, displayName, defaultValue, min, max (if range)
 */

static void handle_TA_GetOptInputParameterInfo(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   int paramIndex = json_find_int(json, "paramIndex");

   const TA_OptInputParameterInfo *info;
   TA_RetCode rc = TA_GetOptInputParameterInfo(handle, (unsigned int)paramIndex, &info);
   if( rc != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"retCode\":%d}", (int)rc);
      return;
   }

   int pos = snprintf(resp, resp_size,
      "{\"type\":%d,\"paramName\":\"%s\",\"flags\":%d,"
      "\"displayName\":\"%s\",\"defaultValue\":%.15g",
      (int)info->type,
      info->paramName ? info->paramName : "",
      (int)info->flags,
      info->displayName ? info->displayName : "",
      info->defaultValue);

   /* Include range/list extended data if available */
   if( info->dataSet ) {
      if( info->type == TA_OptInput_RealRange ) {
         const TA_RealRange *r = (const TA_RealRange *)info->dataSet;
         pos += snprintf(resp + pos, resp_size - pos,
            ",\"min\":%.15g,\"max\":%.15g,\"precision\":%d,"
            "\"suggestedStart\":%.15g,\"suggestedEnd\":%.15g,\"suggestedIncrement\":%.15g",
            r->min, r->max, (int)r->precision,
            r->suggested_start, r->suggested_end, r->suggested_increment);
      } else if( info->type == TA_OptInput_IntegerRange ) {
         const TA_IntegerRange *r = (const TA_IntegerRange *)info->dataSet;
         pos += snprintf(resp + pos, resp_size - pos,
            ",\"min\":%d,\"max\":%d,"
            "\"suggestedStart\":%d,\"suggestedEnd\":%d,\"suggestedIncrement\":%d",
            (int)r->min, (int)r->max,
            (int)r->suggested_start, (int)r->suggested_end, (int)r->suggested_increment);
      } else if( info->type == TA_OptInput_IntegerList ) {
         const TA_IntegerList *l = (const TA_IntegerList *)info->dataSet;
         unsigned int vi;
         pos += snprintf(resp + pos, resp_size - pos, ",\"valueList\":\"");
         for( vi = 0; vi < l->nbElement; vi++ ) {
            pos += snprintf(resp + pos, resp_size - pos, "%s%d=%s",
               vi ? ";" : "", (int)l->data[vi].value,
               l->data[vi].string ? l->data[vi].string : "");
         }
         pos += snprintf(resp + pos, resp_size - pos, "\"");
      }
   }

   snprintf(resp + pos, resp_size - pos, "}");
}

/* ---- TA_GetOutputParameterInfo ----
 * Mirrors: TA_GetOutputParameterInfo
 * Params:  funcName, paramIndex
 * Returns: type, paramName, flags
 */

static void handle_TA_GetOutputParameterInfo(const char *json, char *resp, int resp_size) {
   char funcName[64];
   const TA_FuncHandle *handle;
   if( !abstract_parse_func_name(json, funcName, sizeof(funcName), &handle, resp, resp_size) )
      return;

   int paramIndex = json_find_int(json, "paramIndex");

   const TA_OutputParameterInfo *info;
   TA_RetCode rc = TA_GetOutputParameterInfo(handle, (unsigned int)paramIndex, &info);
   if( rc != TA_SUCCESS ) {
      snprintf(resp, resp_size, "{\"retCode\":%d}", (int)rc);
      return;
   }

   snprintf(resp, resp_size,
      "{\"type\":%d,\"paramName\":\"%s\",\"flags\":%d}",
      (int)info->type,
      info->paramName ? info->paramName : "",
      (int)info->flags);
}

/* ---- TA_FunctionDescriptionXML ----
 * Mirrors: TA_FunctionDescriptionXML
 * Returns: length and order-independent checksum (byte sum) of the XML string
 */

static void handle_TA_FunctionDescriptionXML(const char *json, char *resp, int resp_size) {
   (void)json;
   const char *xml = TA_FunctionDescriptionXML();
   if( !xml ) {
      snprintf(resp, resp_size, "{\"error\":\"TA_FunctionDescriptionXML returned NULL\"}");
      return;
   }
   int len = 0;
   unsigned long long checksum = 0;
   while( xml[len] != '\0' && len < 1000000 ) {
      checksum += (unsigned char)xml[len];
      len++;
   }
   snprintf(resp, resp_size, "{\"length\":%d,\"checksum\":%llu}", len, checksum);
}
