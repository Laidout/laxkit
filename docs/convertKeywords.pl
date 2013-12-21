#!/usr/bin/perl

#
# Use to convert $Date$ and $Id$ to relevant lines, now that we are on git instead of svn:
# > convertKeywords [files...]
# This makes all lines from * like:
#     $Date$
#     $Id$
# to:
#     $Date: 2013 Dec 12 8:00$
#     $Id: filename, last commit: 2013 Dec 12 8:00$
#
#



use File::stat;

$numargs=@ARGV;
die "$numargs: No files to parse!\n" if ($numargs == 0);


foreach $file (@ARGV) {
	$st=stat($file);

	$date=localtime($st->mtime);
	$commit=lastcommit($file);
	print "commit: $commit\n";
	$id="$file, last commit $commit";
	print "mod time: ".localtime($st->mtime)."\n";

	if (open (INFILE, "<$file")) {
		print "Converting keywords in $file\n";
		
		open (OUTFILE, ">$file-hidden");
		while ($line = <INFILE>) {
			$line =~ s/\$Date[^\$]*\$/\$Date: $date\$/; 
			$line =~ s/\$Id[^\$]*\$/\$Id: $id\$/; 

			print OUTFILE "$line";
		}
		close(INFILE);
		close(OUTFILE);
	}
	system("mv -f $file-hidden $file");
}

sub lastcommit
{
	my $file=$_[0];
	my $command="git log $file | grep Date:";

	my $output=`$command`;
	print "output before: \"".$output."\"\n";
	$output =~ s/\s+$//; #trim trailing whitespace
	$output =~ s/Date: *(.*)$/\1/;
	print "output after: \"".$output."\"\n";
	return $output;
}

