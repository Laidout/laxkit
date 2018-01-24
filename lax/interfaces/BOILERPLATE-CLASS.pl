#!/usr/bin/perl

##
## ./BOILERPLATE-CLASS.pl Name
##   -> makes nameinterface.h, nameinterface.cc
##


$Name=$ARGV[0];
if ($Name eq "") {
	print "Usage: ./BOILERPLATE-CLASS.pl NameOfThing\n";
	exit;
}

$Name =~ s/Interface$//;

$name=lc($Name);
$NAME=uc($name);


$hfile="${name}interface.h";
$ccfile="${name}interface.cc";

$NAME_H=uc($hfile);
$NAME_H =~ s/\./_/;

###--------- header file -----------

open (HFILE, ">$hfile") or die "can't open $hfile";
open (BOILER_H, "<aninterface-BOILERPLATE.h") or die "can't open aninterface-BOILERPLATE.h!";

print "Creating $hfile\n";

while (defined($line = <BOILER_H>)) {
	$line =~ s/boilerplate/$name/g;
	$line =~ s/BoilerPlate/$Name/g;
	$line =~ s/BOILERPLATE/$NAME/g;

	print HFILE $line
}


close(BOILER_H);
close(HFILE);





###--------- cc file -----------

open (CCFILE, ">$ccfile") or die "can't open $ccfile";
open (BOILER_CC, "<aninterface-BOILERPLATE.cc") or die "can't open aninterface-BOILERPLATE.cc!";

print "Creating $ccfile\n";

while (defined($line = <BOILER_CC>)) {
	$line =~ s/boilerplate/$name/g;
	$line =~ s/BoilerPlate/$Name/g;
	$line =~ s/BOILERPLATE/$NAME/g;

	print CCFILE $line
}


close(BOILER_CC);
close(CCFILE);


