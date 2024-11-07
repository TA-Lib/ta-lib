#!/usr/bin/perl
#
# Run all tests for Swig Perl wrapper module Finance::TA

use Test::Harness;

# Windows workaround - space problem
$^X =~ s/Program Files/PROGRA~1/;

@tests = qw( ta_defs.t ta_common.t ta_func.t ta_abstract.t );
runtests(@tests);

