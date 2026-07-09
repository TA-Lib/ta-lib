/* ta_abstract_dump.c -- Dump the entire ta_abstract API as structured JSON.
 *
 * Usage: ta_abstract_dump > output.json
 *
 * This program exercises TA_GroupTableAlloc, TA_FuncTableAlloc,
 * TA_ForEachFunc, TA_Get{Input,OptInput,Output}ParameterInfo,
 * TA_ParamHolderAlloc, TA_GetLookback, and TA_ParamHolderFree,
 * producing a single JSON object on stdout.
 */

#include <stdio.h>
#include <string.h>
#include "ta_abstract.h"

/* ---------- JSON string escaping ----------------------------------------- */

static void print_json_string(const char *s)
{
   if (s == NULL) {
      printf("null");
      return;
   }
   putchar('"');
   for (; *s; s++) {
      switch (*s) {
         case '"':  printf("\\\""); break;
         case '\\': printf("\\\\"); break;
         case '\b': printf("\\b");  break;
         case '\f': printf("\\f");  break;
         case '\n': printf("\\n");  break;
         case '\r': printf("\\r");  break;
         case '\t': printf("\\t");  break;
         default:
            if ((unsigned char)*s < 0x20) {
            printf("\\u%04x", (unsigned char)*s);
         } else {
            putchar(*s);
         }
      }
   }
   putchar('"');
}

/* ---------- Per-function callback for TA_ForEachFunc --------------------- */

typedef struct {
   int count;   /* number of functions emitted so far */
   int failed;  /* set to 1 on any TA_RetCode failure  */
} CallbackState;

static void dump_function(const TA_FuncInfo *funcInfo, void *opaqueData)
{
   CallbackState *state = (CallbackState *)opaqueData;
   unsigned int i;
   TA_RetCode rc;

   if (state->failed) return;

   /* comma separator between array elements */
   if (state->count > 0) printf(",\n");

   printf("    {\n");

   /* -- scalar fields ---------------------------------------------------- */
   printf("      \"name\": ");          print_json_string(funcInfo->name);          printf(",\n");
   printf("      \"group\": ");         print_json_string(funcInfo->group);         printf(",\n");
   printf("      \"hint\": ");          print_json_string(funcInfo->hint);          printf(",\n");
   printf("      \"camelCaseName\": "); print_json_string(funcInfo->camelCaseName); printf(",\n");
   printf("      \"flags\": %d,\n",     (int)funcInfo->flags);
   printf("      \"nbInput\": %u,\n",   funcInfo->nbInput);
   printf("      \"nbOptInput\": %u,\n",funcInfo->nbOptInput);
   printf("      \"nbOutput\": %u,\n",  funcInfo->nbOutput);

   /* -- inputs ----------------------------------------------------------- */
   printf("      \"inputs\": [");
   for (i = 0; i < funcInfo->nbInput; i++) {
      const TA_InputParameterInfo *paramInfo;
      rc = TA_GetInputParameterInfo(funcInfo->handle, i, &paramInfo);
      if (rc != TA_SUCCESS) { state->failed = 1; return; }

         if (i > 0) printf(", ");
      printf("\n        {\"type\": %d, \"paramName\": ", (int)paramInfo->type);
      print_json_string(paramInfo->paramName);
      printf(", \"flags\": %d}", (int)paramInfo->flags);
   }
   if (funcInfo->nbInput > 0) printf("\n      ");
   printf("],\n");

   /* -- optional inputs -------------------------------------------------- */
   printf("      \"optInputs\": [");
   for (i = 0; i < funcInfo->nbOptInput; i++) {
      const TA_OptInputParameterInfo *paramInfo;
      rc = TA_GetOptInputParameterInfo(funcInfo->handle, i, &paramInfo);
      if (rc != TA_SUCCESS) { state->failed = 1; return; }

         if (i > 0) printf(",");
      printf("\n        {\"type\": %d, \"paramName\": ", (int)paramInfo->type);
      print_json_string(paramInfo->paramName);
      printf(", \"flags\": %d, \"displayName\": ", (int)paramInfo->flags);
      print_json_string(paramInfo->displayName);
      printf(", \"defaultValue\": %g, \"hint\": ", paramInfo->defaultValue);
      print_json_string(paramInfo->hint);

      /* dataSet details */
      if (paramInfo->type == TA_OptInput_RealRange && paramInfo->dataSet != NULL) {
         const TA_RealRange *range = (const TA_RealRange *)paramInfo->dataSet;
         printf(", \"realRange\": {\"min\": %g, \"max\": %g, \"precision\": %d",
            range->min, range->max, (int)range->precision);
         printf(", \"sugStart\": %g, \"sugEnd\": %g, \"sugInc\": %g}",
            range->suggested_start, range->suggested_end, range->suggested_increment);
      }
      else if (paramInfo->type == TA_OptInput_RealList && paramInfo->dataSet != NULL) {
         const TA_RealList *list = (const TA_RealList *)paramInfo->dataSet;
         unsigned int j;
         printf(", \"realList\": {\"nbElement\": %u, \"data\": [", list->nbElement);
         for (j = 0; j < list->nbElement; j++) {
            if (j > 0) printf(", ");
            printf("{\"value\": %g, \"string\": ", list->data[j].value);
            print_json_string(list->data[j].string);
            printf("}");
         }
         printf("]}");
      }
      else if (paramInfo->type == TA_OptInput_IntegerRange && paramInfo->dataSet != NULL) {
         const TA_IntegerRange *range = (const TA_IntegerRange *)paramInfo->dataSet;
         printf(", \"intRange\": {\"min\": %d, \"max\": %d",
            (int)range->min, (int)range->max);
         printf(", \"sugStart\": %d, \"sugEnd\": %d, \"sugInc\": %d}",
            (int)range->suggested_start, (int)range->suggested_end,
            (int)range->suggested_increment);
      }
      else if (paramInfo->type == TA_OptInput_IntegerList && paramInfo->dataSet != NULL) {
         const TA_IntegerList *list = (const TA_IntegerList *)paramInfo->dataSet;
         unsigned int j;
         printf(", \"intList\": {\"nbElement\": %u, \"data\": [", list->nbElement);
         for (j = 0; j < list->nbElement; j++) {
            if (j > 0) printf(", ");
            printf("{\"value\": %d, \"string\": ", (int)list->data[j].value);
            print_json_string(list->data[j].string);
            printf("}");
         }
         printf("]}");
      }

      printf("}");
   }
   if (funcInfo->nbOptInput > 0) printf("\n      ");
   printf("],\n");

   /* -- outputs ---------------------------------------------------------- */
   printf("      \"outputs\": [");
   for (i = 0; i < funcInfo->nbOutput; i++) {
      const TA_OutputParameterInfo *paramInfo;
      rc = TA_GetOutputParameterInfo(funcInfo->handle, i, &paramInfo);
      if (rc != TA_SUCCESS) { state->failed = 1; return; }

         if (i > 0) printf(", ");
      printf("\n        {\"type\": %d, \"paramName\": ", (int)paramInfo->type);
      print_json_string(paramInfo->paramName);
      printf(", \"flags\": %d}", (int)paramInfo->flags);
   }
   if (funcInfo->nbOutput > 0) printf("\n      ");
   printf("],\n");

   /* -- lookback with default params ------------------------------------- */
   {
      TA_ParamHolder *params = NULL;
      TA_Integer lookback = -1;

      rc = TA_ParamHolderAlloc(funcInfo->handle, &params);
      if (rc != TA_SUCCESS) {
         printf("      \"lookback\": null\n");
      } else {
         rc = TA_GetLookback(params, &lookback);
         if (rc != TA_SUCCESS) {
            printf("      \"lookback\": null\n");
         } else {
            printf("      \"lookback\": %d\n", (int)lookback);
         }
         TA_ParamHolderFree(params);
      }
   }

   printf("    }");

   state->count++;
}

/* ---------- main --------------------------------------------------------- */

int main(void)
{
   TA_RetCode rc;
   TA_StringTable *groupTable = NULL;
   unsigned int g;
   int totalFunctions = 0;
   CallbackState state;

   /* -- groups ----------------------------------------------------------- */
   rc = TA_GroupTableAlloc(&groupTable);
   if (rc != TA_SUCCESS) {
      fprintf(stderr, "TA_GroupTableAlloc failed: %d\n", (int)rc);
      return 1;
   }

   printf("{\n");

   /* 1. groups array */
   printf("  \"groups\": [");
   for (g = 0; g < groupTable->size; g++) {
      if (g > 0) printf(", ");
      print_json_string(groupTable->string[g]);
   }
   printf("],\n");

   /* 2. functions_by_group */
   printf("  \"functions_by_group\": {\n");
   for (g = 0; g < groupTable->size; g++) {
      TA_StringTable *funcTable = NULL;
      unsigned int f;

      rc = TA_FuncTableAlloc(groupTable->string[g], &funcTable);
      if (rc != TA_SUCCESS) {
         fprintf(stderr, "TA_FuncTableAlloc failed for group '%s': %d\n",
            groupTable->string[g], (int)rc);
         TA_GroupTableFree(groupTable);
         return 1;
      }

      if (g > 0) printf(",\n");
      printf("    ");
      print_json_string(groupTable->string[g]);
      printf(": [");
      for (f = 0; f < funcTable->size; f++) {
         if (f > 0) printf(", ");
         print_json_string(funcTable->string[f]);
         totalFunctions++;
      }
      printf("]");

      TA_FuncTableFree(funcTable);
   }
   printf("\n  },\n");

   TA_GroupTableFree(groupTable);

   /* 3. functions array via TA_ForEachFunc */
   state.count = 0;
   state.failed = 0;

   printf("  \"functions\": [\n");
   rc = TA_ForEachFunc(dump_function, &state);
   if (rc != TA_SUCCESS) {
      fprintf(stderr, "TA_ForEachFunc failed: %d\n", (int)rc);
      return 1;
   }
   if (state.failed) {
      fprintf(stderr, "Error during function enumeration\n");
      return 1;
   }
   printf("\n  ],\n");

   /* 4. total function count */
   printf("  \"totalFunctions\": %d\n", state.count);

   printf("}\n");

   return 0;
}
