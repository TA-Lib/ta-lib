#!/bin/usr/perl
# Generate the following files:
#	mrg1.csv
#	mrg2.csv
#	mrg3.csv
#	mrg4.csv
#
# These files contains price bar for the same symbol. They are organise in a away
# that, once merge together, they should form a pre-determine timeserie (pre-defined
# price bars).
#
# If that pre-determine timeserie is not recognize by tha ta_selftest, we can assume
# that there is something broken in the merge logic.

# This script needs the mrg.in file as input.

# When doing the self-test, TA-LIB will expect exactly 1000  price bar
# with the following defined values:
# low	goes from 1000.0001 to 1999.1000
# open goes from 2000.0001 to 2999.1000
# close goes from 3000.0001 to 3999.1000
# high goes from 4000.0001 to 4999.1000
# volume goes from 5000 to 5999
# openInterest goes from 6000 to 6999
#
# As you can see, each price in the price bar is going up in increment of 1.0001
# With this scheme, no two price are identical.
#
# The timestamp can start at any date, but must be in 1 day increment.
#

$nbPriceBar = 1000;

$low    = $nbPriceBar + 0.0001;
$open = $low + $nbPriceBar;
$close = $open + $nbPriceBar;
$high  = $close + $nbPriceBar;
$volume = $high + $nbPriceBar;
$openInterest = $volume + $nbPriceBar;

# Determine starting date
$nbSec = rand 1000000000;

# Open files
open( in, "<mrg.in" ) or die "Can't open mrg.in";
open( out1, ">mrg1.csv" ) or die "Can't create output files";
open( out2, ">mrg2.csv" ) or die "Can't create output files";
open( out3, ">mrg3.csv" ) or die "Can't create output files";
open( out4, ">mrg4.csv" ) or die "Can't create output files";

$nbLine = 0;

while( $line = <in> )
{
   # Build the valid price bar
   ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime $nbSec;
   @timestamp = ($mon,"-",$mday,"-",$year+1900);
   @priceBar = (@timestamp,",",$open,",",$high,",",$low,",",$close,",",$volume,",",$openInterest,"\n");

   # Build the price bar with invalid prices (shall be override in the TA_LIB merge logic).
   @badPriceBar = (@timestamp,",1,2,3,4,5,6\n");
  
   @charStr =  unpack("AAAA", $line);
   @outFile = (out1,out2,out3,out4);

   for( $i=0; $i < 4; $i++ )
   { 
      $out = @outFile[$i];
      if( uc(@charStr[$i]) eq "1" ) {
         print $out @priceBar;
      }

      if( uc(@charStr[$i]) eq "X" ) {
         print $out @badPriceBar;
      }
   }

   $open += 1.0001;
   $high += 1.0001;
   $low += 1.0001;
   $close += 1.0001;
   $volume += 1.0001;
   $openInterest += 1.0001;

   $nbSec += (60*60*24);  
   $nbLine++;
}

if( $nbLine != $nbPriceBar )
{
   print "Error, the mrg.in must contains exactly ",$nbPriceBar," lines\n";
}
