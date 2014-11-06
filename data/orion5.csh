#! /bin/csh -f
#
#    this is an example script that produces some orbit-around Orion.
#    Typical steps:
#   	$ orion5.csh > orion5.wf
#	$ wf2movie.awk orion5.wf > orion5.sh
#	$ chmod +x orion5.sh
#	$ partiview hipbright
#	Cmd: winsize 300 300
#	Cmd: center 43 410 -9
#	Cmd: async orion5.sh
#	... (recording window, do not mess with screen or overlay other windows here)
#	<ESC> to quit partiview when all snapshots have been recorded
#	$ gifsicle -d 10 --colors 256 snap*gif > orion5a.gif
#
#	See http://bima.astro.umd.edu/nemo/amnh/movies/ for the results
#
# Note: You need partiview, starlab, NEMO and ImageMagic, at least to complete it
#       the way the example was written.
#
# History:    
#	3-sep-2001	Added to CVS 				Peter Teuben


#    first we first manually record the beginning and ending viewpoint, and 
#    linearly  interpolate. There are other methods where you record a number
#    of points and spline interpolate/smooth etc.etc. See numerical recipes
#    for methods. The (a) and (b) vector are x,y,z,Rx,Ry,Rz for those two
#    viewpoints, obtained with the "jump" command in partiview

set a=(0.1 -1.5 -0.1  80 -100 -100)
set b=(43 -3.8 -8.2   85.3 -127.6 -127.6)

#    field of view
set fov=60

#    note you need NEMO for the nemoinp/tabmath programs....
#    401 frames

nemoinp 0:400 |\
  tabmath - - "$a[1]+%1/200*($b[1]-$a[1]),$a[2]+%1/200*($b[2]-$a[2]),$a[3]+%1/200*($b[3]-$a[3]),$a[4]+%1/200*($b[4]-$a[4]),$a[5]+%1/200*($b[5]-$a[5]),$a[6]+%1/200*($b[6]-$a[6]),$fov" 1
