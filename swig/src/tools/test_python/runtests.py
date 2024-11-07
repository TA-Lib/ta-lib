#!/usr/bin/python
"""
Run all regression tests of Python wrapper to TaLib
""" 

import unittest
from ta_defs import ta_defs_test
from ta_common import ta_common_test
from ta_func import ta_func_test


if __name__ == '__main__':
    unittest.main()
