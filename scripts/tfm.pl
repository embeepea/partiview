#! /usr/bin/perl

# Perl geometry calculator, with special assistance for astronomical coordinates.
# By Stuart Levy, NCSA, University of Illinois, 1997-2006.  slevy@ncsa.uiuc.edu.
#
# $Id: tfm.pl,v 1.9 2007/02/06 05:22:09 slevy Exp $

$pi = 3.14159265358979323846;
$choplimit = 2e-14;
&init_eq2gal;

sub help {
  print STDERR <<EOF;
Usage for some tfm.pl functions:
Here "T" is a 4x4 matrix as list of 16 numbers
     "v" is a vector (arbitrary length unless specified)
     "q" is a 4-component quaternion, real-part (cos theta/2) first
  tfm(ax,ay,az, angle)=>T  4x4 rot about axis (ax,ay,az) by angle (degrees)
  tfm(tx,ty,tz)	      =>T  4x4 translation
  tfm(s)              =>T  4x4 uniform scaling
  tfm("scale",sx,sy,sz)=>T 4x4 nonuniform scaling
  tfm(ax,ay,az, angle, cx,cy,cz)  4x4 rot about axis, fixing center cx,cy,cz
  transpose( T )	   NxN matrix transpose
  tmul( T1, T2 ) => T1*T2  4x4 (or 3x3) matrix product
  eucinv( T ) => Tinverse  4x4 inverse (assuming T Euclidean rot/trans/uniform scale)
  hls2rgb(h,l,s) => (r,g,b) color conversion
  svmul( s, v ) => s*v	   scalar * vector
  vmmul( v4, T ) => v'	   4-vector * 4x4 matrix => 4-vector
  v3mmul( v3, T ) => v3'   3-D point * (3x3 or 4x4) matrix => 3-D point
  vsub( va, vb ) => va-vb  vector subtraction
  vsadd(s,va, vb) => s*va+vb  vector scaling & addition
  lerp( t, va, vb ) => v   linear interpolation from va to vb: (1-t)*va + t*vb
  dot( va, vb ) => va.vb   dot product
  mag( v )      => |v|     length of vector v
  normalize( v ) => v/|v|  vector v, scaled to unit length (or zero length)
  t2quat( T ) => q	   extract rotation-part of 4x4 T into quaternion
  quat2t( q ) => T	   quaternion to 4x4 matrix T
  quatmul(qa, qb) => qa*qb quaternion multiplication
  qrotbtwn(v3a, v3b) => q  quaternion which rotates 3-vector va into vb
  lookat(from3,to3,up3,roll) construct 4x4 camera->world c2w matrix (pc * c2w = pw).
  t2euler(\"xzy\",T) => X,Z,Y (deg) so rotY*rotZ*rotX = T.  t2euler(\"yxz\",T) = t2aer(T).
  t2meuler(\"yzx\",T) = X,Y,Z, using t2euler(\"xzy\",T).  meuler2t(\"yzx\",X,Y,Z)
  euler2quat(\"xzy\",X,Z,Y) => q;  quat2euler(\"xzy\", q) => angleX,Z,Y, so rotY*rotZ*rotX = q.
  meuler2quat(\"yzx\",X,Y,Z) => q; quat2meuler(\"yzx\",q) => angleX,Y,Z
  aer2t(Ry,Rx,Rz) => T     and  t2aer(T) => Ry,Rx,Rz  Euler angle conversions
  vd2tfm(x,y,z,Rx,Ry,Rz)   and  tfm2vd(T)  4x4 matrix <=> VirDir tx ty tz rx ry rz
  eq2dms(v3) => text "hh:mm.m +dd:mm:ss dist"
  radec2eq(ra,dec,dist)    (ra h:m:s, dec d:m:s, dist) => J2000 3-vector
  radec2eqbasis(ra,dec) => 3x3matrix (ra,dec DEGREES -> XY=sky-plane, +Ynorth)
  Tprecess(fromyr,toyr) => 3x3matrix: Pfrom * Tprecess(fromyear,toyear) = Pto
  \@Tab = 3x3matrix; a,b are: g(alactic) s(upergalactic) e(qJ2000) z(ecliptic) c(C-gal)
  stats( [svar,] val1, ... ) => ( svar, N, mean, SD )  (SD = sqrt(ssq/N))
  list("string")	   converts blank/comma/brace-separated string to list
  put( list )		   print N-vector, or 2x2 or 3x3 or 4x4 matrix
  pt( list )		   print list on one line, full precision (for copy/pasting)
Each inputline is a perl "eval", e.g.: \@a = (1,2,3); print dot(\@a,\@a); sub me {...}
Previous line's answer saved in "\@_"; first scalar saved in \"\$_\".
For multiline input, have a not-yet-closed "{", or use "\\" at end of line.
EOF
  
}

# &smoothstep(t [,vmin,vmax [,tmin,tmax]] )
sub smoothstep {
   local($t, $vmin, $vmax, $tmin, $tmax) = @_;
   $vmin = 0 unless defined($vmin);
   $vmax = 1 unless defined($vmax);
   $t = ($t-$tmin) / ($tmax-$tmin) if $tmax != $tmin;
   return $vmin if($t <= 0);
   return $vmax if($t >= 1);
   return (3 - 2*$t) * $t * $t * ($vmax-$vmin) + $vmin;
}

##  $frac *= $frac*(3 - $frac)/2;   # Smooth start, fast stop
##  $frac *= (3 - $frac*$frac)/2;   # Smooth stop, fast start
##  $frac *= $frac * (3 - 2*$frac); # Smooth start and stop


# &linearstep(t [,vmin,vmax [,tmin,tmax]] )
sub linearstep {
   local($t, $vmin, $vmax, $tmin, $tmax) = @_;
   $vmin = 0 unless defined($vmin);
   $vmax = 1 unless defined($vmax);
   $t = ($t-$tmin) / ($tmax-$tmin) if $tmax != $tmin;
   return $vmin if($t <= 0);
   return $vmax if($t >= 1);
   return $t * ($vmax-$vmin) + $vmin;
}

sub mag {
   local($dot)=0;
   if(@_ == 3) {
	$dot = $_[0]*$_[0] + $_[1]*$_[1] + $_[2]*$_[2];
   } else {
	local($i);
	for($i=0;$i<@_;$i++) {
	    $dot += $_[$i]*$_[$i];
	}
   }
   sqrt($dot);
}

sub normalize {
   local(@v) = @_;
   local($r) = &mag;
   $r=1, $v[0] = 1 if $r == 0;
   return ($v[0]/$r, $v[1]/$r, $v[2]/$r) if @v == 3;
   grep(($_ /= $r) || 1, @v);
}

# Linear interpolation of two vectors:
#  &lerp(frac,  vector0,  vector1)  (vector0 and vector1 of equal length)
sub lerp {
    local($frac) = shift;
    local($dim) = int(($#_+1)/2);
    local(@result, $i);

    for($i = 0; $i < $dim; $i++) {
	push(@result, $_[$i]*(1-$frac) + $_[$i+$dim]*$frac);
    }
    return @result;
}

sub beginpos {
   local($objpos) = @_;
   print "{INST transform {\n", $objpos, "}\ngeom { LIST\n" 
}

sub endpos {
    print "}} #end INST\n";
}

sub tfm {
    local(@t) = (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

    if(@_ == 1) {	# s (scale)
	@t[0,5,10] = @_[0,0,0];

    } elsif($_[0] eq "scale" && @_ == 4) {
	@t[0,5,10] = @_[1,2,3];

    } elsif(@_ == 5 && $_[0] eq "scale") {  # "scale", scaleby, fixedx,fixedy,fixedz
	@t = &tmul(&tfm(-$_[2], -$_[3], -$_[4]),
		   &tfm($_[1]),
		   &tfm(@_[2..4]));

    } elsif(@_ == 3) { # x,y,z (translate)
	@t[12..14] = @_;

    } elsif(@_ == 2) { # axis,degrees  (named axis, angle)
	local($a,$b) = ($_[0],$_[0]);
	$a =~ tr/xyzXYZ/120120/;
	$b =~ tr/xyzXYZ/201201/;
	local($s,$c) = (sin($_[1]*$pi/180), cos($_[1]*$pi/180));
	@t[$a*4+$a, $a*4+$b, $b*4+$a, $b*4+$b] = ($c,$s,-$s,$c);

    } elsif(@_ == 4) {	# x,y,z, degrees  (vector axis, angle)

	local($ax,$ay,$az) = &normalize(@_[0..2]);
	local($s,$c) = (sin($_[3]*$pi/180), cos($_[3]*$pi/180));
	local($v) = 1-$c;
	@t = ($ax*$ax*$v + $c,  $ax*$ay*$v + $az*$s, $ax*$az*$v - $ay*$s, 0,
	      $ax*$ay*$v - $az*$s, $ay*$ay*$v + $c,  $az*$ay*$v + $ax*$s, 0,
	      $ax*$az*$v + $ay*$s, $ay*$az*$v - $ax*$s, $az*$az*$v + $c, 0,
		0, 0, 0, 1);

    } elsif(@_ == 7) { # x,y,z, degrees, fixedx,fixedy,fixedz
        # Translate fixedxyz to origin, rotate, translate back.
        @t = &tmul(&tfm(-$_[4],-$_[5],-$_[6]),
                   &tfm(@_[0..3]),
                   &tfm(@_[4..6]));
    } else {
	print STDERR "&tfm(", join(", ", @_), "): expected 1, 2, 3, 4 or 7 arguments, got ", (@_+0), ": ", join(" ",@_), ".\n";

    }
    return @t;
}

# &begintfm(x,y,z)  translates by x,y,z
# &begintfm(s)		  scales by s
# &begintfm(axisname,degrees) rotates about that axis by that angle
sub begintfm {
    print "{ ";
    if($#_ >= 16) {
	print "INST transforms { TLIST\n";
	while($#_ > 0) {
	    &puttfm;
	    print "\n";
	}
    } elsif($#_ == 15) {
	print "INST transform {\n";
	&puttfm;
    } else {
	print "INST transform { # ", join(" ", @_), "\n";
	&puttfm(&tfm);
    }
    print "   } geom { LIST\n";
}

sub endtfm {
    print "} } # End transformed object\n";
}

# Multiply two (or several) 4x4 matrices, return the product.
sub tmul {
    local(@t,$i,$j);
    if(@_ == 18) {
	return &m3mmul;
    }
    while(@_ >= 32) {
      for($i = 0; $i < 16; $i += 4) {
	for($j = 0; $j < 4; $j++) {
	    $t[$i+$j] =	$_[$i  ] * $_[$j+16] +
			$_[$i+1] * $_[$j+20] +
			$_[$i+2] * $_[$j+24] +
			$_[$i+3] * $_[$j+28];
	}
      }
      splice(@_, 0,32, @t);
    }
    return @t;
}

# 3x3 matrix multiply: &mmul(@a, @b) returns @a * @b
sub m3mmul {
  local($i,$j,$k);
  local(@a) = @_[0..8];
  local(@b) = @_[9..17];
  local(@c);
  for($i = 0; $i < 9; $i+=3) {
    $c[$i  ] = $a[$i]*$b[0] + $a[$i+1]*$b[3] + $a[$i+2]*$b[6];
    $c[$i+1] = $a[$i]*$b[1] + $a[$i+1]*$b[4] + $a[$i+2]*$b[7];
    $c[$i+2] = $a[$i]*$b[2] + $a[$i+1]*$b[5] + $a[$i+2]*$b[8];
  }
  @c;
}


# scalar * vector -> vector
sub svmul {
    local(@t);
    local($s) = shift;
    while($#_ >= 0) {
	push(@t, $s*shift);
    }
    return @t;
}

# 4-vector * 4x4 matrix -> vector
sub vmmul {
    if(@_ == 12) {
	return &vm3mul;	# or, 3-vector * 3x3matrix -> 3-vector
    }
    local(@t) = (shift,shift,shift,shift);
    local(@res, $i);
    for($i = 0; $i < 4; $i++) {
	push(@res, $t[0]*$_[0] + $t[1]*$_[4] + $t[2]*$_[8] + $t[3]*$_[12]);
	shift;
    }
    return @res;
}

# left-multiply 3-row-vector a by 3x3 matrix T: a*T
sub vm3mul {
  if(@_ != 12) {
    print STDERR "vm3mul: expected 3-vector and 3x3 matrix, not these ", (0+@_), ":\n",
	join(" ", @_), "\n";
    return (0,0,0);
  }
  local(@a) = splice(@_,0,3);
  local($i,@v);
  return (
	$a[0]*$_[0] + $a[1]*$_[3] + $a[2]*$_[6],
	$a[0]*$_[1] + $a[1]*$_[4] + $a[2]*$_[7],
	$a[0]*$_[2] + $a[1]*$_[5] + $a[2]*$_[8]);
}

# &v3mmul(x,y,z, transform)
# Multiply a 3-D point by a 3x3 or 4x4 matrix as returned by e.g. &tfm()
sub v3mmul {
    if(@_ == 12) {
	return &vm3mul;
    }
    local(@res) = &vmmul( @_[0..2], 1, @_[3..18] );
    ($res[0]/$res[3], $res[1]/$res[3], $res[2]/$res[3]);
}

# &vsub(@a, @b) returns @a - @b
sub vsub {
  return &svmul( -1, &vsadd( -1, @_ ) );
}

# &vsadd($s, @a, @b) returns $s*@a + @b, where @a and @b are equal-length vectors
sub vsadd {
    local($s) = shift;
    local($dim) = int(($#_+1)/2);
    local(@result, $i);

    for($i = 0; $i < $dim; $i++) {
	push(@result, $s * $_[$i] + $_[$i+$dim]);
    }
    return @result;
}

# &vcomb(sa, @a, sb, @b) returns $sa*@a + $sb*@b
sub vcomb {
    local($n1) = (@_/2);
    local($b) = $_[$n1];
    local($a) = shift;
    local(@result);
    while(@_ > $n1) {
	push(@result, $a*$_[0] + $b*$_[$n1]);
	shift(@_);
    }
    @result;
}

sub inverse {
    local($cmd) = join(" ", "echo", @_, "| matrixinvert");
    return split(" ", `$cmd`);
}

# Matrix inverse, assuming (without checking!) that the 4x4 matrix
# is a Euclidean similarity (isometry plus possibly uniform scaling).
sub eucinv {
  if(@_ != 16) {
    printf STDERR "eucinv: expected 4x4 matrix (16 elements), not these %d:\n",
	0+@_;
    &puttfm;
    return (0) x 16;
  }
  local($i,$j);
  local($s) = &dot(@_[0..2], @_[0..2]);
  local(@trans, @dst);
  for($i = 0; $i < 3; $i++) {
    for($j = 0; $j < 3; $j++) {
        $dst[$i*4+$j] = $_[$j*4+$i] / $s;
    }
    $dst[$i*4+3] = 0;
  }
  @dst[12..15] = (0,0,0,1);
  @dst[12..14] = &vmmul( &svmul(-1, @_[12..14]),0, @dst );
  $dst[3*4+3] = 1;
  return @dst;
}

# lookat(from[3], to[3], upvector[3], roll[1])
# returns camera-to-world matrix which puts camera at "from"
# looking toward "to" with +Y aligned with "upvector"
# rolled counterclockwise by "roll".
sub lookat {
  my(@c2w);
  my(@from) = @_[0..2];
  my(@to) = @_[3..5];
  my(@up) = @_[6..8];
  my($roll) = $_[9];

  @from = (0,0,1) unless defined $from[2];
  @to = (0,0,0) unless defined $to[2];
  @up = (0,1,0) unless defined $up[2];
  @c2w = &m4( &basis(3, 2, &vsub(@from, @to), 1, @up) );
  @c2w = &tmul( &tfm('z', $roll), @c2w ) if $roll != 0;
  @c2w[12..14] = @from;
  @c2w;
}

sub dms2rad {
  &dms2d * $pi/180;
}

sub dms2d {
  local($d,$m,$s) = @_;
  if($d =~ /:/) {
    ($d,$m,$s) = split(/:/, $d);
  }
  local($sign) = ($d =~ s/^\s*-//) ? -1 : 1;
  $sign * ($d + ($m + $s/60)/60);
}

sub hms2d {
  &dms2d * 15;
}

# &rad2dms( radians ) => [-]dd:mm.m
sub rad2dms {
  local($d) = @_;
  local($sign) = $_[1] || " ";
  $sign = "-", $d = -$d if $d<0;
  $d *= 180/$pi;
  local($m) = 60*($d - int($d));
  local($s) = 60*($m - int($m));
  return sprintf("%s%02d:%02d:%04.1f", $sign, int($d), int($m), $s);
}

# &eqd2vec(ra(decimaldegrees), dec(decimaldegrees), dist) => J2000 3-vector
sub eqd2vec {
  local($ra, $dec, $r) = @_;	# Both RA and DEC in degrees!
  $r = 1 if $r eq "";
  $ra *= $pi/180;
  $dec *= $pi/180;
  local($cdec) = cos($dec);
  return ( $r*cos($ra)*$cdec, $r*sin($ra)*$cdec, $r*sin($dec) );
}

# &radec2eq(ra(hh:mm:ss), dec(dd:mm:ss), dist) => J2000 3-vector
sub radec2eq {
  local($ra, $dec, $r) = @_;
  $ra = &hms2d($ra);
  $dec = &dms2d($dec);
  &eqd2vec($ra, $dec, $r);
}

# Supergalactic (sgx, sgy, sgz) to J2000 equatorial (RA, Dec, Dist)
sub sg2eq {
  &eq2dms( &vm3mul( @_, @Tse ) );
}

# Equatorial (x, y, z) to supergalactic (sgx, sgy, sgz)
sub eq2sg {
  &vm3mul( @_, @Tes );
}

sub radec2eqbasis {
  local($ra, $dec) = @_;
  $ra = &hms2d($ra) if $ra =~ /:/;
  $dec = &dms2d($dec) if $dec =~ /:/;
  $ra *= $pi/180;
  $dec *= $pi/180;
  		# vector to galaxy (equatorial coords)
  local(@z) = (cos($ra)*cos($dec), sin($ra)*cos($dec), sin($dec));
  # project @z out of (0,0,1)
  local(@x) = &normalize( -$z[0]*$z[2], -$z[1]*$z[2], 1 - $z[2]*$z[2] );
  local(@y) = &cross( @z, @x );
  (@x, @y, @z);
}

# &eq2dms( J2000 3-vector ) => ( hh:mm.m, dd:mm.m, dist )
sub eq2dms {
  local($r, $rxy);
  local(@eq) = @_;
  local($ra, $dec);
  $rxy = sqrt($eq[0]*$eq[0] + $eq[1]*$eq[1]);
  $dec = atan2($eq[2], $rxy);
  $ra = atan2($eq[1], $eq[0]);
  $ra += 2*$pi if($ra < 0);
  return(&rad2dms($ra/15), &rad2dms($dec,"+"), sqrt(&dot(@eq,@eq)));
}

# &gal2dms( Galactic 3-vector ) => ( londd:mm:ss.s, latdd:mm:ss.s, dist )
sub gal2dms {
  local($r, $rxy);
  local(@eq) = @_;
  local($lon, $lat);
  $rxy = sqrt($eq[0]*$eq[0] + $eq[1]*$eq[1]);
  $lat = atan2($eq[2], $rxy);
  $lon = atan2($eq[1], $eq[0]);
  $lon += 2*$pi if($lon < 0);
  return(&rad2dms($lon), &rad2dms($lat,"+"), sqrt(&dot(@eq,@eq)));
}

sub init_eq2gal {

  $pi = 3.14159265358979;

  # Both the following matrices are taken from SLALIB routines:
  # 
  # Each column of this matrix is a direction vector in the
  # J2000 equatorial system, expressed in (L2,B2) galactic coordinates.
  # Equivalently, each row is a galactic (L2,B2) direction vector
  # expressed in J2000 equatorial coordinates.
  #
  @Tge = (
	-0.054875539726,-0.873437108010,-0.483834985808,
	+0.494109453312,-0.444829589425,+0.746982251810,
	-0.867666135858,-0.198076386122,+0.455983795705);
  # 
  # Each column of this matrix is a direction vector in the (L2,B2)
  # *galactic* system, expressed in *supergalactic* coordinates.
  # Equivalently, each row is a supergalactic direction vector
  # expressed in (L2,B2) galactic coordinates.
  #
  @Tsg = (
	-0.735742574804,+0.677261296414,+0.000000000000,
	-0.074553778365,-0.080991471307,+0.993922590400,
	+0.673145302109,+0.731271165817,+0.110081262225);

  @Tse = &m3mmul(@Tsg,@Tge);
  @Tgs = &transpose(@Tsg);
  @Tes = &transpose(@Tse);
  @Teg = &transpose(@Tge);

  # C-galaxy coordinates = galactic(L2,B2) coordinates, negating X and Y.
  @Tcg = @Tgc = (-1,0,0, 0,-1,0, 0,0,1);
  @Tcs = &m3mmul( @Tcg, @Tgs );
  @Tsc = &m3mmul( @Tsg, @Tgc );
  @Tce = &m3mmul( @Tcg, @Tge );
  @Tec = &transpose( @Tce );

  @Tze = &m3( &tfm('x', 23.4393) );	# Ecliptic ("zodiac") to J2000 (3x3 matrix)
  @Tez = &transpose(@Tze);

  @Tzc = &m3mmul( @Tze, @Tec );
  @Tcz = &transpose(@Tzc);
  @Tzg = &m3mmul( @Tze, @Teg );
  @Tgz = transpose(@Tzg);
  @Tzs = &m3mmul( @Tze, @Tes );
  @Tsz = transpose(@Tzs);

}

# &Tprecess(fromyear, toyear)
# precession of equatorial coordinates from one epoch to another.
# returns 3x3 matrix: vm3mul( p_fromyear, &Tprecess(fromyear,toyear) ) = p_toyear
# Adapted from P.T. Wallace's SLALIB routine PREC.
# Applies to limited range of years.  From the notes:
#     1)  The epochs are TDB (loosely ET) Julian epochs.
#
#     2)  The matrix is in the sense   V(EP1)  =  RMATP * V(EP0)
#
#     3)  Though the matrix method itself is rigorous, the precession
#         angles are expressed through canonical polynomials which are
#         valid only for a limited time span.  There are also known
#         errors in the IAU precession rate.  The absolute accuracy
#         of the present formulation is better than 0.1 arcsec from
#         1960AD to 2040AD, better than 1 arcsec from 1640AD to 2360AD,
#         and remains below 3 arcsec for the whole of the period
#         500BC to 3000AD.  The errors exceed 10 arcsec outside the
#         range 1200BC to 3900AD, exceed 100 arcsec outside 4200BC to
#         5600AD and exceed 1000 arcsec outside 6800BC to 8200AD.
#         The SLALIB routine sla_PRECL implements a more elaborate
#         model which is suitable for problems spanning several
#         thousand years.
#  References:
#     Lieske,J.H., 1979. Astron.Astrophys.,73,282. equations (6) & (7), p283.
#     Kaplan,G.H., 1981. USNO circular no. 163, pA2.

sub Tprecess {
  local($from, $to) = @_;

  $to = 2000 if $to == 0 ;
  $from = 2000 if $from == 0;

  local($t0) = ($from - 2000) / 100; # "from" in centuries-from-2000
  local($dt) = ($to - $from) / 100; # delta time in centuries
  # delta-time, scaled so $dtdeg * arc-sec-per-century = degrees
  local($dtdeg) = $dt / 3600;

  local($w) = 2306.2181+(1.39656-0.000139*$t0)*$t0;

  local($zetadeg) = ($w+((0.30188-0.000344*$t0)+0.017998*$dt)*$dt)*$dtdeg;
  local($zdeg) = ($w+((1.09468+0.000066*$t0)+0.018203*$dt)*$dt)*$dtdeg;
  local($thetadeg) = ((2004.3109+(-0.85330-0.000217*$t0)*$t0)
          +((-0.42665-0.000217*$t0)-0.041833*$dt)*$dt)*$dtdeg;

  local(@q) = &quatmul( &quatmul(
		&ax2quat( 'z', $zetadeg ),
		  &ax2quat( 'y', -$thetadeg ) ),
		    &ax2quat( 'z', $zdeg ) );
  &m3( &quat2t( @q ) );
}


sub t2quat {
  local(@t) = (0+@_ == 9) ? &m4(@_) : (0+@_ == 16) ? @_ : &tfm(@_);

  local($s) = &mag( @t[0..2] );

# A rotation matrix is
#  ww+xx-yy-zz    2(xy-wz)  2(xz+wy)
#  2(xy+wz)    ww-xx+yy-zz  2(yz-wx)
#  2(xz-wy)       2(yz+wx)  ww-xx-yy+zz
  
  # ww+xx+yy+zz = ss
  local($ww,$xx,$yy,$zz);
  local($x,$y,$z,$w);
  $ww = ($s + $t[0*4+0] + $t[1*4+1] + $t[2*4+2]);	# 4 * w^2
  $xx = ($s + $t[0*4+0] - $t[1*4+1] - $t[2*4+2]);
  $yy = ($s - $t[0*4+0] + $t[1*4+1] - $t[2*4+2]);
  $zz = ($s - $t[0*4+0] - $t[1*4+1] + $t[2*4+2]);

  local($max) = $ww;
  $max = $xx if $max < $xx;
  $max = $yy if $max < $yy;
  $max = $zz if $max < $zz;

  if($ww == $max) {
    $w = sqrt($ww) * 2;			# 4w
    $x = ($t[2*4+1] - $t[1*4+2]) / $w;	# 4wx/4w
    $y = ($t[0*4+2] - $t[2*4+0]) / $w;	# 4wy/4w
    $z = ($t[1*4+0] - $t[0*4+1]) / $w;	# 4wz/4w
    $w *= .25;			# w

  } elsif($xx == $max) {
    $x = sqrt($xx) * 2;			# 4x
    $w = ($t[2*4+1] - $t[1*4+2]) / $x;	# 4wx/4x
    $y = ($t[1*4+0] + $t[0*4+1]) / $x;	# 4xy/4x
    $z = ($t[0*4+2] + $t[2*4+0]) / $x;	# 4xz/4x
    $x *= .25;			# x

  } elsif($yy == $max) {
    $y = sqrt($yy) * 2;			# 4y
    $w = ($t[0*4+2] - $t[2*4+0]) / $y;	# 4wy/4y
    $x = ($t[1*4+0] + $t[0*4+1]) / $y;	# 4xy/4y
    $z = ($t[2*4+1] + $t[1*4+2]) / $y;	# 4yz/4y
    $y *= .25;			# y

  } else {
    $z = sqrt($zz) * 2;			# 4z
    $w = ($t[1*4+0] - $t[0*4+1]) / $z;	# 4wz/4z
    $x = ($t[0*4+2] + $t[2*4+0]) / $z;	# 4xz/4z
    $y = ($t[2*4+1] + $t[1*4+2]) / $z;	# 4yz/4z
    $z *= .25;
  }

  $s = sqrt($s);
  (-$w/$s, $x/$s,$y/$s,$z/$s);
}

# @quat = &t2quat( transform )
# Convert 4x4 matrix into unit quaternion
sub old_t2quat {
    local(@t) = (0+@_ == 16) ? @_ : &tfm(@_);
    local(@v) = ($t[9]-$t[6], $t[2]-$t[8], $t[4]-$t[1]);
    local($scl) = &mag(@t[0..2]);
    local($trace) = $scl ? ($t[0]+$t[5]+$t[10])/$scl : 3; # 1 + 2 cos(angle)
    $trace = -1 if $trace < -1;
    $trace = 3 if $trace > 3;
    local($s) = sqrt(3 - $trace) / 2;			  # sin(angle/2)
    if($trace < -.25) {
	# Angle near pi; sin(angle) is small, so use cos-related mat elements
	local($c) = ($trace-1)/2;	# cos(angle)
	local($v) = 1-$c;		# versine(angle)
	local($i, $t);
	if($t[0] > -.5) {
	    $v[0] = sqrt(($t[0]-$c)/$v) * ($v[0]<0 ? -1 : 1);
	    $v[1] = ($t[1]+$t[4])/(2*$v*$v[0]);
	    $v[2] = ($t[2]+$t[8])/(2*$v*$v[0]);
	} elsif($t[5] > -.5) {
	    $v[1] = sqrt(($t[5]-$c)/$v) * ($v[1]<0 ? -1 : 1);
	    $v[0] = ($t[1]+$t[4])/(2*$v*$v[1]);
	    $v[2] = ($t[6]+$t[9])/(2*$v*$v[1]);
	} elsif($t[10] > $c) { # it should be > -.5 too, but just in case...
	    $v[2] = sqrt(($t[10]-$c)/$v) * ($v[2]<0 ? -1 : 1);
	    $v[0] = ($t[2]+$t[8])/(2*$v*$v[2]);
	    $v[1] = ($t[6]+$t[9])/(2*$v*$v[2]);
	}
    }
    local($v) = &mag(@v);
    $s /= -$v if $v>0;
    return ( sqrt(1 + $trace)/2, $v[0]*$s, $v[1]*$s, $v[2]*$s );
}

# @transform = &quat2t( quaternion )
# Turn quaternion into 4x4 matrix
sub quat2t {
    local(@q) = @_;
    local($u) = &mag;
    if(@_ == 3) {
	@q = ($u >= 1) ? ( 0, &svmul(1/$u, @_) ) : ( sqrt(1-$u*$u), @_ );
    } elsif($u != 1) {
	@q = &svmul(1/$u, @_);
    }
    local($x2, $xy, $xz, $xw, $y2, $yz, $yw, $z2, $zw);
    $x2 = $q[1]*$q[1]; $xy = $q[1]*$q[2]; $xz = $q[1]*$q[3]; $xw = $q[1]*$q[0];
    $y2 = $q[2]*$q[2]; $yz = $q[2]*$q[3]; $yw = $q[2]*$q[0];
    $z2 = $q[3]*$q[3]; $zw = $q[3]*$q[0];
    
    (
	1-2*($y2+$z2),	2*($xy+$zw),	2*($xz-$yw),	0,
	2*($xy-$zw),	1-2*($x2+$z2),	2*($yz+$xw),	0,
	2*($xz+$yw),	2*($yz-$xw),	1-2*($x2+$y2),	0,
	0,		0,		0,		1
    );
}

sub quat2t_junk {
    if(@_ == 3) {
	local($sinhalf) = &mag(@_);		# sin(angle/2)
	local($coshalf) = ($sinhalf>-1&&$sinhalf<1) ? sqrt(1 - $sinhalf*$sinhalf) : 0;
	return &tfm(&normalize(@_), 2 * atan2($sinhalf, $coshalf) * 180/$pi);
    }
    local(@v) = &normalize(@_[1..3]);  # ijk components
    local($angle) = 2 * atan2(sqrt(1-$_[0]*$_[0]), $_[0]); # 2 acos q.re
    return &tfm(@v, $angle*180/$pi);
}

# Quaternion to axis and angle(degrees), as taken by tfm: x,y,z, angle
sub quat2a {
    local(@q) = @_;
    local($u);
    if(@_ == 3) {
	$u = &mag;
	@q = ($u >= 1) ? ( 0, &svmul(1/$u, @_) ) : ( sqrt(1-$u*$u), @_ );
    }
    ( &normalize( @q[1..3] ), atan2( &mag(@q[1..3]), $q[0] ) * 360/$pi );
}


# Convert Euler angles -- in the order used by the CAVE,
#  Y(azim) then X(elev) then Z(roll), with Z closest to object coords --
# into a quaternion.
# Given our order convention, we multiply quat(roll) * quat(elev) * quat(azim).

sub aer2quat {
  local($az,$el,$ro) = @_;
  # sines and cosines of half-angles
  local($ca, $sa) = (cos($az*$pi/360), sin($az*$pi/360));  # azim: Y rot
  local($ce, $se) = (cos($el*$pi/360), sin($el*$pi/360));  # elev: X rot
  local($cr, $sr) = (cos($ro*$pi/360), sin($ro*$pi/360));  # roll: Z rot
  # quat(elev) * quat(azim)
  (@qelaz) = ( $ca*$ce, $ca*$se, $sa*$ce, -$sa*$se );

  #X# debug
  local(@result) = &quatmul( $cr,0,0,$sr, &quatmul( $ce,$se,0,0, $ca,0,$sa,0 ));
  local(@result2) = &quatmul( $cr,0,0,$sr, @qelaz );
  @debug = &vsub(@result, @result2);
  @result2;
}

sub m4 {
  return @_ if @_ == 16;
  ( @_[0..2], 0, @_[3..5], 0, @_[6..8], 0,  0,0,0,1 );
}

sub m3 {
  return @_ if @_ == 9;
  @_[0..2, 4..6, 8..10];
}

sub aer2t {
  &quat2t( &aer2quat( @_ ) );
}

sub t2aer {
  my(@M) = &m3(@_);
  my($rx,$ry,$rz);
  my($mag) = &mag( @M[6..8] );
  if($mag == 0) {
    print STDERR "t2aer: expected 4x4 matrix\n";
    return (0,0,0);
  }
  @M = &svmul( 1/$mag, @M );
  local($sx) = -$M[2*3+1];
  local($cx) = ($sx<-1 || $sx>1) ? 0 : sqrt(1 - $sx*$sx);
  $rx = atan2( $sx, $cx ) * 180/$pi;
  if($cx < .001) {
    $ry = atan2( $M[1*3+0], $M[0*3+0] ) * 180/$pi;
    $ry = -$ry if $rx < 0;
    $rz = 0;
  } else {
    $ry = atan2( $M[2*3+0], $M[2*3+2] ) * 180/$pi;
    $rz = atan2( $M[0*0+1], $M[1*3+1] ) * 180/$pi;
  }
  ($ry, $rx, $rz);
}

sub xyzmrep {  # xyzmrep("yxz", Y,X,Z) => X,Y,Z
  my($p) = shift(@_);
  $p =~ tr/, \t//d;
  $p =~ tr/xXyYzZ/001122/;
  my(@xyz);
  @xyz[ split(//, $p) ] = @_;
  @xyz;
}
sub zyxperm {	# zyxperm("yxz", X,Y,Z) => "zxy", Z,X,Y
  my($p) = scalar reverse( shift(@_) );
  $p =~ tr/, \t//d;
  my($q);
  ($q = $p) =~ tr/xXyYzZ/001122/;
  ( $p, @_[ split(//, $q) ] );
}

sub meuler2quat { # meuler2quat("yxz", X,Y,Z) = euler2quat("zxy", Z,X,Y)
  &euler2quat( &zyxperm( @_ ) );
}
sub quat2meuler { # quat2meuler("yxz", q) = quat2euler("zxy", q) returned in X,Y,Z order
  my($revp) = scalar reverse( shift(@_) );
  &xyzmrep( $revp, &quat2euler( $revp, @_ ) );
}

sub meuler2t {
  &euler2t( &zyxperm( @_ ) );
}
sub t2meuler {
  my($revp) = scalar reverse( shift(@_) );
  &xyzmrep( $revp, &t2euler( $revp, @_ ) );
}

sub euler2quat {	# euler2quat( "yxz", Ydeg,Xdeg,Zdeg ) => quaternion zrot(Z)*xrot(X)*yrot(Y)
  my($abc, $A,$B,$C) = @_;
  $abc =~ s/[\s,]//g;
  $abc =~ tr/xXyYzZ/001122/;
  my($a,$b,$c) = split(//, $abc);
  # sines and cosines of half-angles
  $A *= $pi/360;
  $B *= $pi/360;
  $C *= $pi/360;

  my(@qA) = (cos($A), 0,0,0);  $qA[$a+1] = sin($A);
  my(@qB) = (cos($B), 0,0,0);  $qB[$b+1] = sin($B);
  my(@qC) = (cos($C), 0,0,0);  $qC[$c+1] = sin($C);
  
  &quatmul( @qC, &quatmul( @qB, @qA ) );
}

sub euler2t {
  &quat2t( &euler2quat(@_) );
}

sub quat2euler {	# ( "xzy", u,i,j,k ) => ( angleX, Z, Y )
			# such that rotY(Y)*rotZ(Z)*rotX(X) = quat2t(quat)
  my($abc) = shift(@_);
  my(@q) = @_;
  &t2euler( $abc, &quat2t(@q) );
}

sub t2euler {	# t2euler( axA,axB,axC, T ) => angleA,angleB,angleC
		# such that p * rotC(angleC) * rotB(angleB) * rotA(angleA) = p * T
  my($abc) = shift(@_);
  $abc =~ s/[\s,]//g;
  $abc =~ tr/xXyYzZ/001122/;
  my($a,$b,$c) = split(//, $abc);
  my(@T) = @_;

  unless($a+$b+$c == 0+1+2 && length($abc) == 3) {
    print STDERR "t2euler(\"xzy\",T) -> angleX,Z,Y(degrees) s.t. rotx(X)*rotz(Z)*roty(Y) = T.\n\t(So t2euler(\"yxz\",T) == t2aer(T).)\n\tFirst parameter must be some 3-character permutation of \"xyz\" or \"012\".\n";
    return ();
  }

  @T = &m3(@T) if @T == 16;
  my($mag) = &mag( @T[6..8] );

  unless(@T == 9 && $mag>0) {
    print STDERR "t2euler(\"yzx\",T) -> angleY,Z,X s.t. rotY*rotZ*rotX = T; T must be a 3x3 or 4x4 matrix.\n";
    return ();
  }
  @T = &svmul( 1/$mag, @T );

  my($perm) = ( (($a>$b) + ($b>$c) + ($a>$c)) % 2 ) ? -1 : 1;

  my($A,$B,$C);

  my(@Ta) = @T[3*$a..3*$a+2];
  my(@Tb) = @T[3*$b..3*$b+2];
  my(@Tc) = @T[3*$c..3*$c+2];
  my($Tca) = $Tc[$a];		# Tca * -perm = sin(B)
  my($cosB) = sqrt( $Ta[$a]*$Ta[$a] + $Tb[$a]*$Tb[$a] );  # hypot( Taa, Tba ) = hypot( Tcb, Tcc ) = cos(B)
  $B = atan2( $perm * $Tca, $cosB );

  # Use Tba/Taa, Tcb/Tcc terms if cos(B) isn't too small
  $A = atan2( -$perm*$Tc[$b], $Tc[$c] );	# A = atan2( -P*Tcb, Tcc )
  $C = atan2( -$perm*$Tb[$a], $Ta[$a] );	# C = atan2( -P*Tba, Taa )
  if($Tca > 0.9) {
	# perm*sin(B) is near +1, cos(B) near zero, so Taa/Tba/Tcb/Tcc terms inaccurate.
	# $Tca is near +1, use A+C terms, which should be quite accurate
	# -perm*(Tab+Tbc) = sin(A+C) * (1 + Tca)
	#       (Tbb-Tac) = cos(A+C) * (1 + Tca)
	my($ApC) = atan2( $perm * ($Ta[$b] + $Tb[$c]), $Tb[$b] - $Ta[$c] );
	# Tweak A and C so that they sum to this.
	my($delta) = 0.5 * ($ApC - ($A + $C));
	if($delta > 2) { $delta -= $pi; }
	elsif($delta < -2) { $delta += $pi; }
	print STDERR "A $A C $C A+C $ApC => A+$delta C+$delta\n" if $debug;
	$A += $delta;
	$C += $delta;
  } elsif($Tca < -0.9) {
	# perm*sin(B) is near -1, cos(B) near zero, so Taa/Tba/Tcb/Tcc terms inaccurate
	# Tca is near -1, use A-C terms, which should be quite accurate
	# perm*(Tab-Tbc) = sin(A-C) * (1 - Tca)
	#      (Tac+Tbb) = cos(A-C) * (1 - Tca)
	my($AmC) = atan2( -$perm * ($Ta[$b] - $Tb[$c]), $Ta[$c] + $Tb[$b] );
	# Tweak A and C so that their difference is this.
	my($delta) = 0.5 * ($AmC - ($A - $C));
	if($delta > 2) { $delta -= $pi; }
	elsif($delta < -2) { $delta += $pi; }
	print STDERR "A $A C $C A-C $AmC => A+$delta C-$delta\n" if $debug;
	$A += $delta;
	$C -= $delta;
  }
  ( 180/$pi*$A, 180/$pi*$B, 180/$pi*$C );
}


  
# Convert quaternion to Euler angles 
sub quat2aer {
  local($w,$x,$y,$z);
  if(@_ == 3) {
    local($u) = &mag;
    ($w,$x,$y,$z) = $u>1 ? (0, &svmul(1/$u, @_)) : (sqrt(1-$u*$u), @_);
  } else {
    ($w,$x,$y,$z) = &normalize(@_);
  }
  local($srx) = 2*($x*$w - $y*$z);
  local($rx) = atan2( $srx, ($srx<-1||$srx>1) ? 0 : sqrt(1-$srx*$srx) );
  local($ry) = atan2( 2*($x*$z + $y*$w), 1 - 2*($x*$x + $y*$y) );
}

# @quataxb = &quatmul( @quata, @quatb )
# Quaternion multiplication
sub quatmul {
   ($_[0]*$_[4] - $_[1]*$_[5] - $_[2]*$_[6] - $_[3]*$_[7], # rr-ii-jj-kk
    $_[0]*$_[5] + $_[1]*$_[4] - $_[2]*$_[7] + $_[3]*$_[6], # ri+ir-jk+kj
    $_[0]*$_[6] + $_[2]*$_[4] - $_[3]*$_[5] + $_[1]*$_[7], # rj+jr-ki+ik
    $_[0]*$_[7] + $_[3]*$_[4] - $_[1]*$_[6] + $_[2]*$_[5]);# rk+kr-ij+ji
}

sub quatdiv {
   ($_[0]*$_[4] + $_[1]*$_[5] + $_[2]*$_[6] + $_[3]*$_[7], #  rr-ii-jj-kk
  - $_[0]*$_[5] + $_[1]*$_[4] + $_[2]*$_[7] - $_[3]*$_[6], # -ri+ir+jk-kj
  - $_[0]*$_[6] + $_[2]*$_[4] + $_[3]*$_[5] - $_[1]*$_[7], # -rj+jr+ki-ik
  - $_[0]*$_[7] + $_[3]*$_[4] + $_[1]*$_[6] - $_[2]*$_[5]);# -rk+kr+ij-ji
}

sub quatinv {
   (-$_[0], @_[1..3]);
}

# x y z rx ry rz (multiplied in the virdir order, rz*rx*ry*transl(x,y,z)) => T
sub vd2tfm {
  local(@vdwf) = @_;
  @vdwf = split(' ', $vdwf[0]) if(@vdwf == 1);
  if(@vdwf == 16) {
    return @vdwf;
  } elsif(@vdwf == 6 || @vdwf == 7) {
    return &tmul( &tfm('z', $vdwf[5]),
		  &tfm('x', $vdwf[3]),
		  &tfm('y', $vdwf[4]),
		  &tfm( @vdwf[0..2] ) );
  } else {
    printf STDERR "$0: vd2tfm: expected either 6 numbers (x y z rx ry rz) or 16, not these %d: ``%s''\n",
	0+@vdwf, join(" ", @vdwf);
    return &tfm(1);
  }
}

sub tfm2vd {
  local(@yxz) = &t2aer;
  (@_[12..14], @yxz[1,0,2]);
}

# &ax2quat( 'x'|'y'|'z', degrees )
%__ax2quat = ('x',0, 'y',1, 'z',2, 'X',0, 'Y',1, 'Z',2);
sub ax2quat {
  local($axis) = $_[0];
  $axis = $__ax2quat{$axis} if defined $__ax2quat{$axis};
  local($halfang) = $_[1] * $pi/360;
  local(@q) = (cos($halfang), 0,0,0);
  $q[$axis+1] = sin($halfang);
  @q;
}

sub vd2quat {
  local(@vdwf) = @_;
  @vdwf = split(' ', $vdwf[0]) if(@vdwf == 1);
  if(@vdwf == 16) {
    return &t2quat(@vdwf);
  }
  unshift(@vdwf, 0,0,0) if(@vdwf == 3);
  if(@vdwf == 6 || @vdwf == 7) {
    &quatmul( &ax2quat('z', $vdwf[5]),
	&quatmul( &ax2quat('x', $vdwf[3]),
		  &ax2quat('y', $vdwf[4]) ) );
  } else {
    print STDERR "$0: vd2quat: expected either 6 numbers (x y z rx ry rz) or 16, not ``", join(" ",@_), "''\n";
    return (1,0,0,0);
  }
}


# &qrotbtwn(x1,y1,z1, x2,y2,z2) constructs the quaternion which rotates
# vector x1,y1,z1 into x2,y2,z2
sub qrotbtwn {
    # Direction is (x2,y2,z2) cross (x1,y1,z1) 
    local(@ijk) = &normalize(&cross(@_));
    local($cost) = &dot(@_) / sqrt(&dot(@_[0..2,0..2]) * &dot(@_[3..5,3..5]));
    local($sinhalf) = sqrt((1 - $cost) / 2);
    # magnitude of ijk is sin(angle/2)
    return ( sqrt((1+$cost)/2), $ijk[0]*$sinhalf, $ijk[1]*$sinhalf,
				$ijk[2]*$sinhalf );
}

# &puttfm( numbers ) prints an NxN transformation, N numbers per line
sub puttfm {
    local($n) = int(sqrt(@_));
    local($fmt) = join(" ", ("%10.7g") x $n) . "\n";
    while(@_) {
	printf $fmt, splice(@_, 0, $n);
    }
}

sub put {
    if(grep(/[^-+eE.\d]/, @_)) {
	print join(" ", @_), "\n";
	return;
    }
    local($n) = (0+@_);
    local(@data) = @_;
    grep(/\d/ && ($_ = ($_ < -$choplimit || $_ > $choplimit) ? $_ : 0), @data);

    if($n <= 2) {
	print join(" ", @_), "\n";
    } elsif($n <= 8) {
	printf "%10.7g " x $n . "\n", @_;
    } else {
	&puttfm(@data);
    }
}

sub pt {
    if(grep(/[^-+eE.\d]/, @_)) {
	print join(" ", @_), "\n";
    } else {
	while(@_) {
	    printf "%.11g%s", shift(@_), (@_>0?" ":"\n");
	}
    }
    "";
}


sub deg {
    $_[0] * 180/$pi;
}

sub rad {
    $_[0] * $pi/180;
}

sub tandeg {
    sin(&rad) / cos(&rad);
}

sub log10 {
    0.434294481903252 * log($_[0]);
}

# Minimum of a bunch of numbers
sub min {
    local($min) = $_[0];
    while($#_ >= 0) {
	shift;
	$min = $_[0] if $min > $_[0];
    }
    return $min;
}

# Maximum of a bunch of numbers
sub max {
    local($max) = $_[0];
    while($#_ >= 0) {
	shift;
	$max = $_[0] if $max < $_[0];
    }
    return $max;
}

sub dot {
    local($dot) = 0;
    local($len) = int(@_/2);
    local($i);
    for($i=0;$i<$len;$i++) {
	$dot += $_[$i] * $_[$i+$len];
    }
    $dot;
}

sub cross {
    return ($_[1]*$_[5] - $_[2]*$_[4],
	    $_[2]*$_[3] - $_[0]*$_[5],
	    $_[0]*$_[4] - $_[1]*$_[3]);
}

# Round to nearest multiple of 
sub round {
    local($_,$mod,$zero) = @_;
    $mod = 1 unless $mod;
    return $mod * int( ($_-$zero)/$mod + ($_ < $zero ? -.5 : .5) ) + $zero;
}

# Produce an orthonormal basis, given partial information.
# Input is the dimension and a list of row vectors:
#  d,
#   i, ai0,ai1,...,ai<d-1>,
#   j, aj0,aj1,...,aj<d-1>,
# ...
# Returns a d by d orthonormal matrix, with i'th row equal to ai0...ai<d-1>,
# etc.  Indices run from 0 to d-1 (not 1 to d).

sub basis {
    local($d) = shift;
    local(@done) = (0) x $d;
    local(@M, @T, @v, $i, $j, $row);
    # @M is the list of vectors assigned so far.
    # @T is the final output array, with members of @M
    # arranged in appropriate rows.
    # @done is an array with zeros for unassigned rows, ones elsewhere.
    for($done = 0; $done < $d; $done++) {
	if(@_ >= $d+1) {
	    $row = shift(@_);
	    @v = &normalize( splice(@_, 0, $d) );
	} else {
	    if(@_) {
		print STDERR "basis($d, ...): didn't get a whole number of <row>,<vector> tuples!\n";
		@_ = ();
	    }
	    for($row = 0; $row < $d && $done[$row]; $row++) {
	    }
	    @v = (0) x $d;
	    $v[$row] = 1;
	}

	for($j = 0; $j < $d; $j++) {
	    # Orthogonalize against all preceding rows.
	    for($i = 0; $i < $#M; $i += $d) {
		$dot = &dot(@M[$i..$i+$d-1], @v);
		@v = &vsadd(-$dot, @M[$i..$i+$d-1], @v);
	    }
	    # Normalize
	    $dot = &dot(@v, @v);
	    last if $dot > .000001;
	    # Recover from nearly-degenerate case.
	    # We perturb one coordinate and try again.
	    $v[($j+$row) % $d] += 1;
	} 
	@v = &svmul(1/sqrt($dot), @v);
	push(@M, @v);
	@T[$row*$d .. $row*$d+$d-1] = @v;
	$done[$row] = 1;
    }
    local($det) = &det(@T);
    if($det < 0) {
	@T[$row*$d..$row*$d+$d-1] = &svmul( -1, @T[$row*$d..$row*$d+$d-1] );
    }
    return @T;
}

sub det {
    if(@_ == 1) {
	return $_[0];
    } elsif(@_ == 4) {
	return $_[0]*$_[3] - $_[1]*$_[2];
    } elsif(@_ == 9) {
	return &dot( &cross( @_[0..2, 3..5] ), @_[6..8] );
    } else {
	# Oh well, maybe later.
	print STDERR "Oops, ignoring determinant of ", sqrt(0+@_), "-rank matrix.\n";
	return 1;
    }
}

# Transpose a square matrix.
sub transpose {
    local($d) = int(sqrt(@_));
    local($i, $j);
    local(@T);
    for($i = 0; $i < $d; $i++) {
	for($j = 0; $j < $d; $j++) {
	    push(@T, @_[$i + $j*$d]);
	}
    }
    return @T;
}

# Print a square matrix tidily.
sub putmatrix {
    local($d) = int(sqrt(@_));
    local($i);
    while($@ > 0) {
	printf " %9.6g", shift;
	print "\n" if ++$i % $d == 0;
    }
    print "\n" if $i % $d != 0;
}

sub list {
    local($_) = join(" ", @_);
    $_ =~ tr/,(){}[]/       /s;
    return split(' ', $_);
}

# Color conversion: out of place here, but useful
sub hls2rgb {
  local($h,$l,$s) = @_;
  local($max) = $l;
  local($delta) = $max*$s;
  local(@rgb) = ($max-$delta) x 3;
  $h -= int($h);
  $h += 1 if $h < 0;
  $h *= 6;
  local($t) = &abs($h-2)-1;
  if($t<0) { $rgb[0] = $max; }
  elsif($t<1) { $rgb[0] = $max-$delta*$t; }
  $t = &abs($h-4)-1;
  if($t<0) { $rgb[1] = $max; }
  elsif($t<1) { $rgb[1] = $max-$delta*$t; }
  $t = 2 - &abs(3-$h);
  if($t<0) { $rgb[2] = $max; }
  elsif($t<1) { $rgb[2] = $max-$delta*$t; }
  return @rgb;
}

sub stats {
    local($stat) = (@_ > 0 && ref($_[0])) ? shift(@_) : [0,0,0];
    local($N, $mean, $ssq) = @{$stat};
    local($v, $delta);
    foreach $v ( @_ ) {
	$N++;
	$delta = $v - $mean;
	$mean += $delta / $N;
	$ssq += $delta*$delta * (1 - 1/$N);
	# or: variance = (variance*(N-1) + delta*delta * (N-1)/N) / N
	#		= (variance + delta*delta/N) * (N-1)/N
    }
    @{$stat} = ($N, $mean, $ssq);
    return ( $stat, $N, $mean, sqrt($N ? $ssq/$N : 0) );
}

# a [-1..1, -1..1] square onto a torus or Moebius strip.

# Uses global parameter $torus:
#       $torus = -1   rectangle
#       $torus = 0    cylinder
#       $torus = 1    torus
# and $r for the hole in the center of the torus.  Torus' major radius = 1.

$surfmap = "tormap" unless $surfmap;
sub tormap {
    local($v,$u) = @_;
    if($torus > 0) {
	local($rp) = $r + (1/$torus + cos($pi*$v)) / $pi;
	return ($rp * sin($pi*$u*$torus), sin($pi*$v)/$pi,
		$rp * cos($pi*$u*$torus) - $r - (1 + 1/$torus)/$pi );
    } elsif($torus > -1) {
	local($cyl) = $torus + 1;    # $cyl = 0 for square, 1 for cylinder
	if ($cylvert) { # vertical cylinder
	  return (sin($pi*$u*$cyl)/$pi/$cyl, $v,
		  (cos($pi*$u*$cyl) - 1)/$pi/$cyl);
	} else { # horizontal cylinder
	  return ($u, sin($pi*$v*$cyl)/$pi/$cyl,
		  (cos($pi*$v*$cyl) - 1)/$pi/$cyl );
	}
    } else {
	return ($u, $v, 0);
    }
}

sub imgfit {
    local($cen, $min, $max, $scale) = @_;
    if($max eq "") {
	print STDERR "Usage: imgfit(center, min, max [, scale])
returns min', max', (max'-min') -- range in which \"cen\" is centered, scaled up by \"scale\"\n";
	return;
    }
    $scale = 1 unless $scale;
    local($r) = $cen-$min;
    $r = $max-$cen if $r < $max-$cen;
    (($cen-$r)*$scale, ($cen+$r)*$scale, 2*$r*$scale);
}

sub history {
    local($howmany) = $_[$#_];
    $howmany = 0+@HIST unless $howmany>0;
    local($numbered) = (join("",@_) =~ /n/);
    local($i);
    for($i = @HIST-$howmany; $i < @HIST; $i++) {
	if($numbered) {
	    printf "%-3d %s\n", $i, $HIST[$i];
	} else {
	    printf " %s\n", $HIST[$i];
	}
    }
}

sub h {
    &history;
}

sub tfm_interact {
  # If we were invoked as a shell command, act as a perl calculator.
  local($tty) = (-t STDIN);
  print STDERR "Type \"help\" for help\n> " if $tty;
  @prefix = ();
  while(defined(($lastinput = <>)) && $lastinput !~ /^(q|quit|exit|bye)$/i) {
    $lastinput =~ s/^[\s>]*//;
    $lastinput = "&help ", @prefix = () if $lastinput =~ /^\s*\?\s*$/;
    chomp $lastinput;
    if($lastinput =~ /(.*)\\$/) {
	push(@prefix, $1);
	print STDERR "\t" if $tty;
	next;
    }
    $wholeinput = join("\n\t", @prefix, $lastinput);
    if(($wholeinput =~ tr/{/{/) > ($wholeinput =~ tr/}/}/)) {
	push(@prefix, $lastinput);
	print STDERR "\t" if $tty;
	next;
    }
    push(@HIST, $wholeinput);
    @_ = eval($wholeinput);
    if($@ ne "") {
	print $@, "\n";
    } elsif($wholeinput !~ /;$/ && $wholeinput ne "" && $wholeinput !~ /^&*help$/) {
	&put(@_);
	$_ = $_[0];
    }
    @prefix = ();
    print STDERR "> " if $tty;
  }
}

if ($0 =~ /tfm\.pl$/ || $tfm_interactive) { 
  &tfm_interact;
}

1;

# $Log: tfm.pl,v $
# Revision 1.9  2007/02/06 05:22:09  slevy
# Ident -> Id.
#
# Revision 1.8  2007/02/06 05:19:28  slevy
# &lookat() returns a c2w matrix, not w2c.
# Add Ident and Log entries.
#
