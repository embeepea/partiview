#! /bin/csh -f
#
#    this is an example script that produces a time animation
#    Typical steps: 
#   	$ primbin16.csh > primbin16.sh
#	$ chmod +x primbin16.sh
#	$ primbin16.cf
#	Cmd: async primbin16.sh
#	... (recording window, do not mess with screen or overlay other windows here)
#	<ESC> to quit partiview when all snapshots have been recorded
#	$ gifsicle -d 10 --colors 256 snap*gif > primbin16.gif
#
#	See http://bima.astro.umd.edu/nemo/amnh/movies/ for the results
#
# Note: You need partiview, and ImageMagic, at least to complete it
#       the way the example was written.
#
# History:    
#	17-oct-2001	Added to CVS 				Peter Teuben
#       

#  set time, timestep and number of frames
set t=0.0
set dt=0.05
set n=201



echo "echo jump -4.63412 13.4097 4.79856  -63.5511 -44.0014 14.4626"
echo "echo fov 20"

loop:
   @ n--
   if ($n < 0) exit
   echo "echo step $t"
   echo "echo update"
   echo "echo snapset snap%03d.gif"
   echo "echo snapshot"
   set t=`echo $t+$dt|bc -l`
   goto loop

