#!/usr/bin/python
"""
Simple test of Python wrapper to ta_defs
""" 

import sys
import unittest
sys.path.insert(0,'../../../lib/python')

from TaLib import *


class ta_defs_test(unittest.TestCase):

    def testConstants(self):
        #print 'Testing Constants...'
        self.assert_( TA_INTEGER_MIN )
        self.assert_( TA_INTEGER_MAX )
        self.assertEqual( TA_REAL_MIN, -3e+37 )
        self.assertEqual( TA_REAL_MAX,  3e+37 )
        self.assertEqual( TA_INTEGER_DEFAULT, TA_INTEGER_MIN-1 )
        self.assertEqual( TA_REAL_DEFAULT, -4e+37 )

    def test_TA_RetCode(self):
        #print 'Testing TA_RetCode...'
        self.assertEqual( TA_SUCCESS, 0 )
        self.assertEqual( TA_LIB_NOT_INITIALIZE, 1 )
        self.assertEqual( TA_BAD_PARAM, 2 )
        self.assertEqual( TA_ALLOC_ERR, 3 )
        self.assertEqual( TA_GROUP_NOT_FOUND, 4 )
        self.assertEqual( TA_FUNC_NOT_FOUND, 5 )
        self.assertEqual( TA_INVALID_HANDLE, 6 )
        self.assertEqual( TA_INVALID_PARAM_HOLDER, 7 )
        self.assertEqual( TA_INVALID_PARAM_HOLDER_TYPE, 8 )
        self.assertEqual( TA_INVALID_PARAM_FUNCTION, 9 )
        self.assertEqual( TA_INPUT_NOT_ALL_INITIALIZE, 10 )
        self.assertEqual( TA_OUTPUT_NOT_ALL_INITIALIZE, 11 )
        self.assertEqual( TA_OUT_OF_RANGE_START_INDEX, 12 )
        self.assertEqual( TA_OUT_OF_RANGE_END_INDEX, 13 )
        self.assertEqual( TA_INVALID_LIST_TYPE, 14 )
        self.assertEqual( TA_BAD_OBJECT, 15 )
        self.assertEqual( TA_NOT_SUPPORTED, 16 )
        self.assertEqual( TA_INTERNAL_ERROR, 5000 )
        self.assertEqual( TA_UNKNOWN_ERR, 0xFFFF )


    def test_TA_Compatibility(self):
        #print "Testing TA_Compatibility..."
        self.assertEqual( TA_COMPATIBILITY_DEFAULT, 0 )
        self.assertEqual( TA_COMPATIBILITY_METASTOCK, 1 )

    def test_TA_MAType(self):
        #print "Testing TA_MAType..."
        self.assertEqual( TA_MAType_SMA, 0 )
        self.assertEqual( TA_MAType_EMA, 1 )
        self.assertEqual( TA_MAType_WMA, 2 )
        self.assertEqual( TA_MAType_DEMA, 3 )
        self.assertEqual( TA_MAType_TEMA, 4 )
        self.assertEqual( TA_MAType_TRIMA, 5 )
        self.assertEqual( TA_MAType_KAMA, 6 )
        self.assertEqual( TA_MAType_MAMA, 7 )
        self.assertEqual( TA_MAType_T3, 8 )

    def test_TA_FuncUnstId(self):
        #print "Testing TA_FuncUnstId..."
        self.assertEqual(  0, TA_FUNC_UNST_ADX)
        self.assertEqual(  1, TA_FUNC_UNST_ADXR)
        self.assertEqual(  2, TA_FUNC_UNST_ATR)
        self.assertEqual(  3, TA_FUNC_UNST_CMO)
        self.assertEqual(  4, TA_FUNC_UNST_DX)
        self.assertEqual(  5, TA_FUNC_UNST_EMA)
        self.assertEqual(  6, TA_FUNC_UNST_HT_DCPERIOD)
        self.assertEqual(  7, TA_FUNC_UNST_HT_DCPHASE)
        self.assertEqual(  8, TA_FUNC_UNST_HT_PHASOR)
        self.assertEqual(  9, TA_FUNC_UNST_HT_SINE)
        self.assertEqual( 10, TA_FUNC_UNST_HT_TRENDLINE)
        self.assertEqual( 11, TA_FUNC_UNST_HT_TRENDMODE)
        self.assertEqual( 12, TA_FUNC_UNST_KAMA)
        self.assertEqual( 13, TA_FUNC_UNST_MAMA)
        self.assertEqual( 14, TA_FUNC_UNST_MFI)
        self.assertEqual( 15, TA_FUNC_UNST_MINUS_DI)
        self.assertEqual( 16, TA_FUNC_UNST_MINUS_DM)
        self.assertEqual( 17, TA_FUNC_UNST_NATR)
        self.assertEqual( 18, TA_FUNC_UNST_PLUS_DI)
        self.assertEqual( 19, TA_FUNC_UNST_PLUS_DM)
        self.assertEqual( 20, TA_FUNC_UNST_RSI)
        self.assertEqual( 21, TA_FUNC_UNST_STOCHRSI)
        self.assertEqual( 22, TA_FUNC_UNST_T3)
        self.assert_( TA_FUNC_UNST_ALL > 0 )
        self.assertEqual( -1, TA_FUNC_UNST_NONE)

if __name__ == '__main__':
    print "TA-Lib ", TA_GetVersionString()
    print "Testing ta_defs...";
    unittest.main()
