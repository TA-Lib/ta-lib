import xml.etree.ElementTree as ET
import os

def generate_pg_wrappers(xml_file, c_file, sql_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()

    with open(c_file, 'w') as c_out, open(sql_file, 'w') as sql_out:
        c_out.write('''
#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include "catalog/pg_type.h"
#include "utils/lsyscache.h"
#include "funcapi.h"

#include "ta_func.h"

PG_MODULE_MAGIC;
''')

        for func in root.findall('FinancialFunction'):
            abbreviation = func.find('Abbreviation').text
            
            required_inputs = func.find('RequiredInputArguments').findall('RequiredInputArgument')
            optional_inputs_element = func.find('OptionalInputArguments')
            optional_inputs = optional_inputs_element.findall('OptionalInputArgument') if optional_inputs_element is not None else []
            outputs = func.find('OutputArguments').findall('OutputArgument')

            c_func_name = f"pg_{abbreviation.lower()}"
            sql_func_name = f"ta_{abbreviation.lower()}"

            # C function signature
            c_out.write(f"PG_FUNCTION_INFO_V1({c_func_name});\n")
            c_out.write(f"Datum {c_func_name}(PG_FUNCTION_ARGS);\n")
            c_out.write(f"Datum\n{c_func_name}(PG_FUNCTION_ARGS)\n{{\n")

            # Get arguments
            arg_index = 0
            for j, input_arg in enumerate(required_inputs):
                c_out.write(f"    ArrayType *inArray{j} = PG_GETARG_ARRAYTYPE_P({arg_index});\n")
                arg_index += 1

            for j, input_arg in enumerate(optional_inputs):
                input_type = input_arg.find('Type').text
                default_value = input_arg.find('DefaultValue').text
                if 'Integer' in input_type:
                    c_out.write(f"    int optIn{j} = PG_ARGISNULL({arg_index}) ? {default_value} : PG_GETARG_INT32({arg_index});\n")
                elif 'Double' in input_type:
                    c_out.write(f"    double optIn{j} = PG_ARGISNULL({arg_index}) ? {default_value} : PG_GETARG_FLOAT8({arg_index});\n")
                else: # MA Type
                    c_out.write(f"    TA_MAType optIn{j} = PG_ARGISNULL({arg_index}) ? {default_value} : (TA_MAType) PG_GETARG_INT32({arg_index});\n")
                arg_index += 1

            # Check array lengths
            if len(required_inputs) > 0:
                c_out.write("    int num_elements = ARR_DIMS(inArray0)[0];\n")
                for j in range(1, len(required_inputs)):
                    c_out.write(f"    if (num_elements != ARR_DIMS(inArray{j})[0])\n")
                    c_out.write("    {\n")
                    c_out.write("        ereport(ERROR,\n")
                    c_out.write("                (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),\n")
                    c_out.write("                 errmsg(\"Input arrays must have the same length\")));\n")
                    c_out.write("    }\n")
            else:
                c_out.write("    int num_elements = 1;\n")

            # Get data pointers
            for j, input_arg in enumerate(required_inputs):
                input_type = input_arg.find('Type').text
                if 'Double' in input_type or 'Price' in input_type or 'High' in input_type or 'Low' in input_type or 'Close' in input_type or 'Open' in input_type or 'Volume' in input_type:
                    c_out.write(f"    double *inReal{j} = (double *) ARR_DATA_PTR(inArray{j});\n")
                elif 'Integer' in input_type:
                    c_out.write(f"    int *inInt{j} = (int *) ARR_DATA_PTR(inArray{j});\n")
            
            # Allocate output arrays
            for j, output_arg in enumerate(outputs):
                output_type = output_arg.find('Type').text
                if 'Double' in output_type:
                    c_out.write(f"    double *outReal{j} = (double *) palloc(sizeof(double) * num_elements);\n")
                elif 'Integer' in output_type:
                    c_out.write(f"    int *outInt{j} = (int *) palloc(sizeof(int) * num_elements);\n")

            c_out.write("    int outBegIdx, outNBElement;\n")

            # Call TA-Lib function
            c_out.write(f"    TA_RetCode retCode = TA_{abbreviation}(0, num_elements - 1, ")
            
            input_params = []
            for j, input_arg in enumerate(required_inputs):
                input_type = input_arg.find('Type').text
                if 'Double' in input_type or 'Price' in input_type or 'High' in input_type or 'Low' in input_type or 'Close' in input_type or 'Open' in input_type or 'Volume' in input_type:
                    input_params.append(f"inReal{j}")
                elif 'Integer' in input_type:
                    input_params.append(f"inInt{j}")
            
            for j, input_arg in enumerate(optional_inputs):
                input_params.append(f"optIn{j}")

            c_out.write(', '.join(input_params))
            if len(input_params) > 0:
                c_out.write(", ")

            c_out.write("&outBegIdx, &outNBElement, ")

            output_params = []
            for j, output_arg in enumerate(outputs):
                output_type = output_arg.find('Type').text
                if 'Double' in output_type:
                    output_params.append(f"outReal{j}")
                elif 'Integer' in output_type:
                    output_params.append(f"outInt{j}")
            c_out.write(', '.join(output_params))

            c_out.write(");\n")

            # Check return code
            c_out.write("    if (retCode != TA_SUCCESS)\n")
            c_out.write("    {\n")
            c_out.write(f"        ereport(ERROR,\n")
            c_out.write(f"                (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),\n")
            c_out.write(f"                 errmsg(\"TA-Lib TA_{abbreviation} function failed with error code: %d\", retCode)));\n")
            c_out.write("    }\n")

            if len(outputs) > 1:
                # Build the return tuple
                c_out.write(f"    TupleDesc tupdesc;\n")
                c_out.write(f"    HeapTuple tuple;\n")
                c_out.write(f"    Datum values[{len(outputs)}];\n")
                c_out.write(f"    bool nulls[{len(outputs)}];\n")
                
                c_out.write("    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)\n")
                c_out.write("        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg(\"function returning record called in context that cannot accept type record\")));\n")

                c_out.write("    BlessTupleDesc(tupdesc);\n")

                for j, output_arg in enumerate(outputs):
                    output_type = output_arg.find('Type').text
                    oid = "FLOAT8OID" if "Double" in output_type else "INT4OID"
                    out_var = f"outReal{j}" if "Double" in output_type else f"outInt{j}"
                    
                    c_out.write(f"    int16 elmlen{j};\n")
                    c_out.write(f"    bool elmbyval{j};\n")
                    c_out.write(f"    char elmalign{j};\n")
                    c_out.write(f"    get_typlenbyvalalign({oid}, &elmlen{j}, &elmbyval{j}, &elmalign{j});\n")
                    c_out.write(f"    int dims{j}[1] = {{outNBElement}};\n")
                    c_out.write(f"    int lbs{j}[1] = {{1}};\n")
                    c_out.write(f"    ArrayType *result{j} = construct_md_array((void *){out_var}, NULL, 1, dims{j}, lbs{j}, {oid}, elmlen{j}, elmbyval{j}, elmalign{j});\n")
                    c_out.write(f"    values[{j}] = PointerGetDatum(result{j});\n")
                    c_out.write(f"    nulls[{j}] = false;\n")

                c_out.write("    tuple = heap_form_tuple(tupdesc, values, nulls);\n")
                c_out.write("    PG_RETURN_DATUM(HeapTupleGetDatum(tuple));\n")
            else:
                # Create output array
                output_type = outputs[0].find('Type').text
                oid = "FLOAT8OID" if "Double" in output_type else "INT4OID"
                c_out.write(f"    int16 elmlen;\n")
                c_out.write(f"    bool elmbyval;\n")
                c_out.write(f"    char elmalign;\n")
                c_out.write(f"    get_typlenbyvalalign({oid}, &elmlen, &elmbyval, &elmalign);\n")
                c_out.write(f"    int dims[1] = {{outNBElement}};\n")
                c_out.write(f"    int lbs[1] = {{1}};\n")
                out_var = "outReal0" if "Double" in output_type else "outInt0"
                c_out.write(f"    ArrayType *result = construct_md_array((void *){out_var}, NULL, 1, dims, lbs, {oid}, elmlen, elmbyval, elmalign);\n")
                c_out.write(f"    PG_RETURN_ARRAYTYPE_P(result);\n")

            c_out.write("}\n\n")

            # SQL function signature
            if len(outputs) > 1:
                type_name = f"ta_{abbreviation.lower()}_type"
                sql_out.write(f"CREATE TYPE {type_name} AS (\n")
                type_cols = []
                for j, output_arg in enumerate(outputs):
                    output_name = output_arg.find('Name').text.lower().replace('outreal', '').replace('outinteger', '')
                    if output_name == '':
                        output_name = f"out{j}"
                    output_type = output_arg.find('Type').text
                    pg_type = "double precision[]" if "Double" in output_type else "integer[]"
                    type_cols.append(f"    {output_name} {pg_type}")
                sql_out.write(',\n'.join(type_cols))
                sql_out.write("\n);\n\n")

            sql_out.write(f"CREATE OR REPLACE FUNCTION {sql_func_name}(\n")
            
            all_args = []
            # Required inputs
            for j, input_arg in enumerate(required_inputs):
                input_type = input_arg.find('Type').text
                name = input_arg.find('Name').text.lower().replace(' ','_').replace('-','_').replace('-','_')
                arg_str = ""
                if 'Price' in input_type or 'Real' in input_type or 'Double' in input_type or 'High' in input_type or 'Low' in input_type or 'Close' in input_type or 'Open' in input_type or 'Volume' in input_type:
                    arg_str = f"    {name} double precision[]"
                elif input_type == 'Integer':
                    arg_str = f"    {name} integer[]"
                all_args.append(arg_str)

            # Optional inputs
            for j, input_arg in enumerate(optional_inputs):
                input_type = input_arg.find('Type').text
                name = input_arg.find('Name').text.lower().replace(' ','_').replace('-','_').replace('-','_')
                default_value = input_arg.find('DefaultValue').text
                arg_str = ""
                if 'Integer' in input_type:
                    arg_str = f"    {name} integer DEFAULT {default_value}"
                elif 'Double' in input_type:
                    arg_str = f"    {name} double precision DEFAULT {default_value}"
                else: # MA Type
                    arg_str = f"    {name} integer DEFAULT {default_value}"
                all_args.append(arg_str)

            if all_args:
                sql_out.write(',\n'.join(all_args))
                
            sql_out.write(")\n")
            
            if len(outputs) > 1:
                sql_out.write(f"RETURNS {type_name}\n")
            else:
                output_type = outputs[0].find('Type').text
                if 'Double' in output_type:
                    sql_out.write("RETURNS double precision[]\n")
                elif 'Integer' in output_type:
                    sql_out.write("RETURNS integer[]\n")

            sql_out.write(f"AS 'MODULE_PATHNAME', '{c_func_name}'\n")
            sql_out.write("LANGUAGE C STRICT;\n\n")


if __name__ == "__main__":
    generate_pg_wrappers('./ta_func_api.xml', './src/ta_pg/ta_pg.c', './src/ta_pg/ta_pg--1.0.sql')
