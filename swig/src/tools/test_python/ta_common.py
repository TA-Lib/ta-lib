#!/usr/bin/python
"""
Simple test of Python wrapper to ta_common
""" 

import sys
import re
import unittest
sys.path.insert(0,'../../../lib/python')

from TaLib import *



class ta_common_test(unittest.TestCase):

    def test_TA_GetVersion(self):
        self.assertNotEqual( TA_GetVersionString(), '' )
        r1 = re.compile(r'^\d+$')
        self.assert_( r1.match( TA_GetVersionMajor() ) )
        self.assert_( r1.match( TA_GetVersionMinor() ) )
        self.assert_( r1.match( TA_GetVersionBuild() ) )
        r2 = re.compile(r'^\w+\s+\d+\s+\d+$' )
        self.assert_( r2.match( TA_GetVersionDate() ) )
        r3 = re.compile( r'^\d\d:\d\d:\d\d$' )
        self.assert_( r3.match( TA_GetVersionTime() ) )

    def test_TA_SetRetCodeInfo(self):
        rci = TA_RetCodeInfo()
        TA_SetRetCodeInfo(0, rci)
        self.assertEqual( rci.enumStr, 'TA_SUCCESS' )
        self.assertEqual( rci.infoStr, 'No error' )
        TA_SetRetCodeInfo(1, rci )
        self.assertEqual( rci.enumStr, 'TA_LIB_NOT_INITIALIZE' )
        self.assertEqual( rci.infoStr, 'TA_Initialize was not sucessfully called' )

        # Using constructor parameter
        self.assertEqual( TA_RetCodeInfo(2).enumStr, 'TA_BAD_PARAM' )
        self.assertEqual( TA_RetCodeInfo(2).infoStr, 'A parameter is out of range' )

    def test_TA_Initialize(self):
        self.assertEqual( TA_Initialize(), None )
        self.assertEqual( TA_Initialize(), None )      # implicit call to TA_Shutdown
        self.assertEqual( TA_Shutdown(), 0 )
        self.assertEqual( TA_Shutdown(), 0 )            # accepted, no-op


if __name__ == '__main__':
    print "TA-Lib ", TA_GetVersionString()
    print "Testing ta_common...";
    unittest.main()
