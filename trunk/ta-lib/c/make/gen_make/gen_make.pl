#!/usr/local/bin/perl

# This perl script is the "mother of all" making
# all the operations for re-generating all the
# makefiles variant.

# Un-comment the following lines if
# you wish to provide your own TMAKEPATH.
# use Env qw( TMAKEPATH );

$ENV{'TMAKEPATH'} = '.\\template\\win32-msvc';

print "Generating ta_func.pro template...";
chdir "ta_func";
system( "perl make_pro.pl >ta_func.pro" );
chdir "..";
print "done.\n";

print "Generating ta_abstract.pro template...";
chdir "ta_abstract";
system( "perl make_pro.pl >ta_abstract.pro" );
chdir "..";
print "done.\n";

print "Generating ta_libc.pro template...";
chdir "ta_libc";
system( "perl make_pro.pl >ta_libc.pro" );
chdir "..";
print "done.\n";

system( "perl ./make_make.pl cdr .\\template\\win32-msvc .\\template\\* all" );
system( "perl ./make_make.pl cdd .\\template\\win32-msvc .\\template\\* all" );
system( "perl ./make_make.pl cmd .\\template\\win32-msvc .\\template\\* all" );
system( "perl ./make_make.pl cmr .\\template\\win32-msvc .\\template\\* all" );
system( "perl ./make_make.pl csr .\\template\\win32-msvc .\\template\\* all" );
system( "perl ./make_make.pl csd .\\template\\win32-msvc .\\template\\* all" );
