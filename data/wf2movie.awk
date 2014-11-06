#! /bin/awk -f
#
#
#	awk script to convert a "wf" file into an "async" shell script
#	to create snapshots from within partiview

BEGIN {
   print "###  this script automatically created with wf2movie.awk";
   print "#echo center 43 409 -9";
}

{
   print "echo jump ",$0
   print "echo update"
   print "echo snapset snap%03d.gif"
   print "echo snapshot"
}

END {
   print "# All done.";
}
