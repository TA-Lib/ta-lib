#!/usr/bin/perl
#
# Synopsis:
#   func_list [level]
#
# Where:
#   level   : optional number (1..5) specifying detail level
#

use lib "../../../lib/perl";
use Finance::TA;

our $level = shift || 5;

print "TA-Lib ", TA_GetVersionString(), "\n\n";

sub ident($) {
    return "    " x $_[0];
}

sub func_flags {
    my ($flags) = @_;
    my @flags;
    push(@flags, "TA_FUNC_FLG_OVERLAP") if $flags & $TA_FUNC_FLG_OVERLAP;
    push(@flags, "TA_FUNC_FLG_INDICATOR") if $flags & $TA_FUNC_FLG_INDICATOR;
    push(@flags, "TA_FUNC_FLG_VOLUME") if $flags & $TA_FUNC_FLG_VOLUME;
    push(@flags, "TA_FUNC_FLG_UNST_PER") if $flags & $TA_FUNC_FLG_UNST_PER;
    return @flags;
}

sub in_flags {
    my ($flags) = @_;
    my @flags;
    push(@flags, "TA_IN_PRICE_OPEN") if $flags & $TA_IN_PRICE_OPEN;
    push(@flags, "TA_IN_PRICE_HIGH") if $flags & $TA_IN_PRICE_HIGH;
    push(@flags, "TA_IN_PRICE_LOW") if $flags & $TA_IN_PRICE_LOW;
    push(@flags, "TA_IN_PRICE_CLOSE") if $flags & $TA_IN_PRICE_CLOSE;
    push(@flags, "TA_IN_PRICE_VOLUME") if $flags & $TA_IN_PRICE_VOLUME;
    push(@flags, "TA_IN_PRICE_OPENINTEREST") if $flags & $TA_IN_PRICE_OPENINTEREST;
    push(@flags, "TA_IN_PRICE_TIMESTAMP") if $flags & $TA_IN_PRICE_TIMESTAMP;
    return @flags;
}

sub opt_flags {
    my ($flags) = @_;
    my @flags;
    push(@flags, "TA_OPTIN_IS_PERCENT") if $flags & $TA_OPTIN_IS_PERCENT;
    push(@flags, "TA_OPTIN_IS_DEGREE") if $flags & $TA_OPTIN_IS_DEGREE;
    push(@flags, "TA_OPTIN_IS_CURRENCY") if $flags & $TA_OPTIN_IS_CURRENCY;
    push(@flags, "TA_OPTIN_ADVANCED") if $flags & $TA_OPTIN_ADVANCED;
    return @flags;
}

sub out_flags {
    my ($flags) = @_;
    my @flags;
    push(@flags, "TA_OUT_LINE") if $flags & $TA_OUT_LINE;
    push(@flags, "TA_OUT_DOT_LINE") if $flags & $TA_OUT_DOT_LINE;
    push(@flags, "TA_OUT_DASH_LINE") if $flags & $TA_OUT_DASH_LINE;
    push(@flags, "TA_OUT_DOT") if $flags & $TA_OUT_HISTO;
    push(@flags, "TA_OUT_THIN_LINE") if $flags & $TA_OUT_THIN_LINE;
    push(@flags, "TA_OUT_NORM_LINE") if $flags & $TA_OUT_NORM_LINE;
    push(@flags, "TA_OUT_THICK_LINE") if $flags & $TA_OUT_THICK_LINE;
    return @flags;
}

sub in_type {
    my ($type) = @_;
    return "TA_Input_Price" if $type == $TA_Input_Price;
    return "TA_Input_Real" if $type == $TA_Input_Real;
    return "TA_Input_Integer" if $type == $TA_Input_Integer;
}

sub opt_type {
    my ($type) = @_;
    return "TA_OptInput_RealRange" if $type == $TA_OptInput_RealRange;
    return "TA_OptInput_RealList" if $type == $TA_OptInput_RealList;
    return "TA_OptInput_IntegerRange" if $type == $TA_OptInput_IntegerRange;
    return "TA_OptInput_IntegerList" if $type == $TA_OptInput_IntegerList;
}

sub out_type {
    my ($type) = @_;
    return "TA_Output_Real" if $type == $TA_Output_Real;
    return "TA_Output_Integer" if $type == $TA_Output_Integer;
}

sub display_range {
    my ($data, $type, $ident) = @_;
    return unless $level > 4;
    print ident($ident), "Min: $data->{min}\n";
    print ident($ident), "Max: $data->{max}\n";
    print ident($ident), "Precision: $data->{precision}\n" if $type == $TA_OptInput_RealRange;
    print ident($ident), "Start: $data->{suggested_start}\n";
    print ident($ident), "End: $data->{suggested_end}\n";
    print ident($ident), "Step: $data->{suggested_increment}\n";
}

sub display_list {
    my ($data, $ident) = @_;
    return unless $level > 4;
    for (my $i = 0;  $i < $data->{nbElement};  $i++) {
        print ident($ident), $data->{data}[$i]{string}, ": ", $data->{data}[$i]{value}, "\n";
    }
}

sub display_inpar {
    my ($fh, $i, $ident) = @_;
    $info = $fh->GetInputParameterInfo($i);
    print ident($ident++), "Input: $info->{paramName}\n";
    return unless $level > 3;
    my @flags = in_flags($info->{flags});
    print ident($ident), "Flags: @flags\n" if @flags > 0;
    print ident($ident), "Type: ", in_type($info->{type}), "\n";
}

sub display_optpar {
    my ($fh, $i, $ident) = @_;
    $info = $fh->GetOptInputParameterInfo($i);
    print ident($ident++), "Option: $info->{paramName}\n"; 
    return unless $level > 3;
    print ident($ident), "Name: $info->{displayName}\n"; 
    print ident($ident), "Hint: $info->{hint}\n"; 
    my @flags = opt_flags($info->{flags});
    print ident($ident), "Flags: @flags\n" if @flags > 0;
    print ident($ident), "Type: ", opt_type($info->{type}), "\n";
    print ident($ident), "Default: $info->{defaultValue}\n";
    display_range($info->{dataSet}, $info->{type}, $ident+1) 
        if ($info->{type} == $TA_OptInput_RealRange) || ($info->{type} == $TA_OptInput_IntegerRange);
    display_list($info->{dataSet}, $ident+1) 
        if ($info->{type} == $TA_OptInput_RealList) || ($info->{type} == $TA_OptInput_IntegerList);
}

sub display_outpar {
    my ($fh, $i, $ident) = @_;
    $info = $fh->GetOutputParameterInfo($i);
    print ident($ident++), "Output: $info->{paramName}\n"; 
    return unless $level > 3;
    my @flags = out_flags($info->{flags});
    print( ident($ident), "Flags: @flags\n") if @flags > 0;
    print ident($ident), "Type: ", out_type($info->{type}), "\n";
}


@groups = TA_GroupTable();
shift @groups;
foreach $group (@groups) {
    print "\n" if $level > 1; 
    print "Group: $group\n";
    next unless $level > 1;
    print "=" x 70, "\n";
    @functions = TA_FuncTable($group);
    shift @functions;
    foreach $function (@functions) {
        $fh = new TA_FuncHandle($function);
        $fi = $fh->GetFuncInfo();
        print "\n" if $level > 2; 
        print ident(1), "Function: $function ($fi->{hint})\n";
        next unless $level > 2;
        print ident(1), "-" x 66, "\n";
        local $" = ' | ';
        my @fflags = func_flags($fi->{flags});
        print ident(2), "Flags: @fflags\n";
        for ($i = 0; $i < $fi->{nbInput};  $i++) {
            display_inpar($fh, $i, 2);
        }
        for ($i = 0; $i < $fi->{nbOptInput};  $i++) {
            display_optpar($fh, $i, 2);
        }
        for ($i = 0; $i < $fi->{nbOutput};  $i++) {
            display_outpar($fh, $i, 2);
        }
    }
}
