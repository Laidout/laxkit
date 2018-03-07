#!/usr/bin/perl

#This program takes ../ROADMAP
#and converts it to html in roadmap.html.
#
#Just run it, no arguments necessary.


open(ROADMAP,"../ROADMAP");
open(ROADMAPHTML, ">roadmap.html");

#------write header
print ROADMAPHTML << "END_OF_HEADER";
<html>
<head>
<title>Laxkit</title>
<meta content="text/html; charset=ISO-8859-1" http-equiv="Content-Type">
<meta 
content="Tom, Lechner, Laxkit, gui, user, interface, window, toolkit"
name="keywords">
<meta 
content="Loose Amalgamated Xkit, a window toolkit" 
name="description">
<link rel="icon" href="icon.png" type="image/png">
<link rel="stylesheet" href="style.css" type="text/css">


</head>

<body aLink="red" bgColor="#cccccc" link="red" text="olive" vLink="maroon">

<center>
<table border=0 cellpadding=5 cellspacing=0><tr>
<td id="bodytext" class="side" valign=top> 

<!-- <table border=1 "width=100%"  height="100%"><tr><td> -->
  <center><a href="index.html"><img src="logo-131x120.jpg" width="131" height="120" border="0" alt="The Laxkit"></a></center><br>
  <blockqoute><b>
  &nbsp;&nbsp;<a href="index.html">Main</a><br>
  &nbsp;&nbsp;<a href="news.html">News</a><br>
  &nbsp;&nbsp;<a href="faq.html">Faq</a><br>
  &nbsp;&nbsp;<a href="docs.html">Documentation</a><br>
  &nbsp;&nbsp;<a href="docs.html#downloading">Download</a><br>
  &nbsp;&nbsp;<a href="links.html">Links</a><br>
  &nbsp;&nbsp;<a href="#contact">Contact</a><br>
  </b></blockqoute>
<!-- </td></tr></table> -->
</td>
<td id="bodytext">

<h1>Laxkit Roadmap</h1>


END_OF_HEADER

$versions=0;
$linenum=0;
$maybemoredesc=0; #if line indented more than 6 spaces: /^       +(.*)/, append to desc
while (defined($line = <ROADMAP>)) {
	$linenum++;

	 #perhaps there is more description incoming
	if ($maybemoredesc!=0) {
		if ($maybemoredesc==1 && $line =~ /^        +(.*)/) {

			$desc="$desc $1";
			next;
		} elsif ($maybemoredesc==-1 && $line=~ /^ +(.*)/) {
			$desc="$desc $1";
			next;
		}
		 #else no more description, and dump out current stuff
		print ROADMAPHTML "<tr><td align=\"center\" valign=\"top\">$done</td><td>$bugtext$desc</td></tr>\n";

		 ## with column for bug numbers:
		#print ROADMAPHTML "<tr><td>$bug</td><td align=\"center\" valign=\"top\">$done</td><td>$desc</td></tr>\n";

		$bug="";
		$bugtext="";
		$done="";
		$desc="";
		$maybemoredesc=0;
	}

	if ($line =~ /^\s*$/) {
		print "skipping blank line $linenum...\n";
		next;
	}

   	 #skip lines like "  ------------  "
	if ($line =~ /^\s*-*\s*$/) { 
		print "skipping dashed line $linenum...\n";
		next;
   	}

	 #dump out initial description bascially verbatim
	if ($versions==0) {
		if ($line =~/Laxkit/) { next; }
		if ($line =~/^VERSION/) {
			print ROADMAPHTML "<table>\n\n";
			$versions=1;
		} else {
			print ROADMAPHTML $line;
			next;
		}
	}

	 #start new section
	if ($line =~ /^VERSION/) {
		$line =~ /^VERSION\s*(\S*)\s*-*\s*(.*)/;
		print ROADMAPHTML "<tr><td colspan=3><br><h2>Version $1 -- $2</h2></td></tr>\n";
		next;
	}

	 #start old release section
	if ($line =~ /OLD RELEASE/) {
		print ROADMAPHTML "<tr><td colspan=\"3\" align=\"center\"><hr/><br/><h2>Old Releases</h2></td></tr>\n";
		next;
	}

	 #last line should be the svn id line
	if ($line =~/^\$Id/) {
		print ROADMAPHTML "</table>\n<br/>\n<br/>\nLast change: $line\n\n";
		break;
	}

	if ($line =~ /bug #(\d*)/) { $bug=$1; } else { $bug=""; }
	#$bug=$1;
	$bug="";
	$bugtext="";
	if ($bug ne "") { 
		# $bug contains the number of the bug
		# $bugtext="<a title=\"Laxkit bug number $bug\" "
		#			."href=\"http://***************">bug #$bug</a>, ";
	} else {
		$bug="";
		$bugtext="";
	}
	if ($line =~ /^\((done)([^)]*)\)\s*(.*)$/ ) { #match: ^(done *) desc
		$done="($1)&nbsp;";
		if ($2 ne "") { 
			$tmp=$2;
			$desc=$3;
			$tmp =~ /^\W*(.*)/;
			$desc="($1) $desc";
	   	} else { $desc=$3; }
		$maybemoredesc=1;
		print "found: (done) ...\n";
	} else {
		if ($line =~ /^       (.*)$/) { #match: (7 spaces)* 
			$maybemoredesc=1;
			#print "$linenum: found: \"       ...\" :$1\n";
			$desc=$1;
		} else {
			$line =~ /^(\w.*)$/;   #match: ^(word char)*
			$maybemoredesc=-1;
			#print "$linenum: found: \"...\": $1\n";
			$desc=$1;
		}
		
		$done="<span style=\"color:red;font-weight=bold;\">&bull;</span>";
		#print "$linenum: found: \"...\": $1\n";
	}


	print "line:$linenum   bug:$bug   done:$done   desc:$desc\n";
}


print ROADMAPHTML << "END_OF_FOOTER";

</td>
</tr>
</table>


</body>
</html>

END_OF_FOOTER


close (ROADMAP);
close (ROADMAPHTML);

