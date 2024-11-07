#!/usr/bin/perl -w
#
# Simple test of ta_defs
# 

use strict;
use lib "../../../lib/perl";
use Test;
BEGIN { plan tests => 38 }

use Finance::TA v0.5.0;

print "TA-Lib ", TA_GetVersionString(), "\n";
print "Testing ta_defs...\n";

ok( defined $TA_INTEGER_MIN );
ok( defined $TA_INTEGER_MAX );
ok( $TA_REAL_MIN, -3e+37 );
ok( $TA_REAL_MAX, 3e+37 );
ok( $TA_INTEGER_DEFAULT, $TA_INTEGER_MIN-1 );
ok( $TA_REAL_DEFAULT, -4e+37 );

print "Testing TA_RetCode...\n";

ok( $TA_SUCCESS, 0 );
ok( $TA_LIB_NOT_INITIALIZE, 1 );
ok( $TA_BAD_PARAM, 2 );
ok( $TA_ALLOC_ERR, 3 );
ok( $TA_GROUP_NOT_FOUND, 4 );
ok( $TA_FUNC_NOT_FOUND, 5 );
ok( $TA_INVALID_HANDLE, 6 );
ok( $TA_INVALID_PARAM_HOLDER, 7 );
ok( $TA_INVALID_PARAM_HOLDER_TYPE, 8 );
ok( $TA_INVALID_PARAM_FUNCTION, 9 );
ok( $TA_INPUT_NOT_ALL_INITIALIZE, 10 );
ok( $TA_OUTPUT_NOT_ALL_INITIALIZE, 11 );
ok( $TA_OUT_OF_RANGE_START_INDEX, 12 );
ok( $TA_OUT_OF_RANGE_END_INDEX, 13 );
ok( $TA_INVALID_LIST_TYPE, 14 );
ok( $TA_BAD_OBJECT, 15 );
ok( $TA_NOT_SUPPORTED, 16 );
ok( $TA_INTERNAL_ERROR, 5000 );
ok( $TA_UNKNOWN_ERR, 0xFFFF );

print "Testing TA_Compatibility\n";
ok( $TA_COMPATIBILITY_DEFAULT, 0 );
ok( $TA_COMPATIBILITY_METASTOCK, 1 );

print "Testing TA_MAType\n";
ok( $TA_MAType_SMA, 0 );
ok( $TA_MAType_EMA, 1 );
ok( $TA_MAType_WMA, 2 );
ok( $TA_MAType_DEMA, 3 );
ok( $TA_MAType_TEMA, 4 );
ok( $TA_MAType_TRIMA, 5 );
ok( $TA_MAType_KAMA, 6 );
ok( $TA_MAType_MAMA, 7 );
ok( $TA_MAType_T3, 8 );

# Individual TA_FuncUnstId values note tested because 
# they might change from release to release.
ok( $TA_FUNC_UNST_ALL > 0 );
ok( $TA_FUNC_UNST_NONE, -1 );

print "Done.\n";
