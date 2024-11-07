#!/usr/bin/perl -w
#
# Test of ta_common
# 

use strict;
use lib "../../../lib/perl";
use Test;
BEGIN { plan tests => 17 }

use Finance::TA v0.5.0;

print "TA-Lib ", TA_GetVersionString(), "\n";
print "Testing ta_common...\n";

ok( defined TA_GetVersionString() );
ok( TA_GetVersionMajor(), qr/^\d+$/ );
ok( TA_GetVersionMinor(), qr/^\d+$/ );
ok( TA_GetVersionBuild(), qr/^\d+$/ );
ok( TA_GetVersionDate(), qr/^\w+\s+\d+\s+\d+$/ );
ok( TA_GetVersionTime(), qr/^\d\d:\d\d:\d\d$/ );

print "Testing TA_SetRetCodeInfo()...\n";
{
    my $rci = new TA_RetCodeInfo;
    TA_SetRetCodeInfo(0, $rci);
    ok( $rci->{enumStr}, 'TA_SUCCESS' );
    ok( $rci->{infoStr}, 'No error' );
    TA_SetRetCodeInfo(1, $rci );
    ok( $rci->{enumStr}, 'TA_LIB_NOT_INITIALIZE' );
    ok( $rci->{infoStr}, 'TA_Initialize was not sucessfully called' );

    # Using constructor parameter
    ok( new TA_RetCodeInfo(2)->{enumStr}, 'TA_BAD_PARAM' );
    ok( new TA_RetCodeInfo(2)->{infoStr}, 'A parameter is out of range' );
}

print "Testing TA_Initialize and TA_Shutdown...\n";
ok( TA_Initialize(), $TA_SUCCESS );
ok( TA_Initialize(), $TA_SUCCESS );       # implicit call to &TA_Shutdown
ok( TA_Initialize(), $TA_SUCCESS );
ok( TA_Shutdown(), $TA_SUCCESS );
ok( TA_Shutdown(), $TA_SUCCESS );          # accepted, no-op

