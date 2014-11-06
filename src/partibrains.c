#ifdef __cplusplus
extern "C" {	// in case someone feeds this to the C++ compiler,...
#endif

static char local_id[] = "$Id: partibrains.c,v 1.131 2013/11/30 21:33:08 slevy Exp $";
static char copyright[] = "Copyright (c) 2002 NCSA, University of Illinois Urbana-Champaign";

/*
 * Brains of partiview: carrying and displaying data, parsing commands.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

/*
 * $Log: partibrains.c,v $
 * Revision 1.131  2013/11/30 21:33:08  slevy
 * Tidy.
 *
 * Revision 1.130  2013/11/30 20:58:01  slevy
 * Add new magic encoded-color name - "rgb888", by analogy with rgb565.
 * If a color-index variable is named rgb888, it's interpreted as a full 24-bit color value:
 *   R*65536 + G*256 + B
 * with 0 <= R,G,B <= 255.
 *
 * Revision 1.129  2013/07/12 14:19:21  slevy
 * Allow picking vectors (as in "vec on", "vecscale", etc.).
 * Make use of vecalpha value.
 *
 * Revision 1.128  2013/03/13 16:31:56  slevy
 * Add vecalpha command.
 *
 * Revision 1.127  2011/02/18 01:42:43  slevy
 * Fix lint -- check result from pipe() for "add" command.
 *
 * Revision 1.126  2011/02/18 01:27:05  slevy
 * Add arrows on vectors.  "vec"/"vector" command now accepts any of
 * "vec off", "vec on", or "vec arrow".  With "arrow",
 * draws an arrowhead on each vector, with 3-D size equal to
 * the vector's length times the 'arrowscale'.  The arrowhead always
 * lies in the screen plane, so its size gives a cue to the vector's
 * true 3-D length (e.g. when the vector is viewed nearly end-on,
 * even a small arrowhead can look longer than the vector does).
 * This also means that arrowheads flip orientation when a vector
 * passes through being seen nearly end-on.
 *
 * Arrowhead size is set by a new second parameter to "vecscale".
 * Default is 0.25, meaning that arrowheads are a quarter the main vector's length.
 *
 * Revision 1.125  2011/01/21 18:38:28  slevy
 * mesh_prepare_elements(): use the right face count.
 *
 * Revision 1.124  2011/01/19 21:59:55  slevy
 * Add partially-baked stuff for glDrawElements()/glDrawArrays() prep for POLYMESH objects.
 * Not in use yet.
 *
 * Add "vec/vectors" and "vecscale" control commands and "vecvar" data command.
 * When enabled, draws a vector at each point, as specified by some
 * triple of attributes given by "vecvar".  Length is [vecvar .. vecvar+2]
 * scaled by vecscale (default 1).
 *
 * Revision 1.123  2009/06/26 08:38:46  slevy
 * Add -w option on mesh object -- set linewidth.
 *
 * Revision 1.122  2008/07/25 16:04:19  slevy
 * Rename getline() to get_line(), avoiding collision with new C lib function.
 *
 * Revision 1.121  2008/07/22 18:48:03  slevy
 * CONSTify char *'s in parse_selexpr() and a few other places.
 * Explicitly (int) when we've parsed into a float.
 *
 * Revision 1.120  2006/05/03 01:48:26  slevy
 * Add Usage messages for "hist" and "waveobj".
 * Interpret threshholding correctly for "hist".
 * Make "thresh off" finally work again, too.  Good grief.
 *
 * Revision 1.119  2006/03/29 18:16:42  slevy
 * Make "where" command report cam-to-obj as well as cam-to-world transforms.
 *
 * Revision 1.118  2006/02/09 07:34:12  slevy
 * Need conste.h if we're doing USE_CONSTE.
 *
 * Revision 1.117  2006/01/17 07:37:11  slevy
 * Move some specklist-maintenance functions to specks.c.
 * Work in a way compatible with locking.
 *
 * Revision 1.116  2005/12/14 20:47:31  slevy
 * Think we have a workaround for Apple/GLX glDrawInterleavedArrays()
 * bug, so remove the Apple-specific workaround.
 *
 * Revision 1.115  2005/11/30 00:09:29  slevy
 * Use separate color and vertex arrays instead of glInterleavedArrays(),
 * which seems to have bugs on some implementations (?!).
 * Let's see if this helps.
 *
 * Revision 1.114  2005/10/18 21:22:53  slevy
 * Discard excess glBegin()/glEnd()'s from drawspecks()'s oldopengl path.
 * Add env var PARTIOLDOPENGL to force using non-glDrawElements path.
 *
 * Revision 1.113  2005/06/13 18:12:07  slevy
 * Fix long-standing bug where polygons would get the wrong orientation
 * if some were fixed-oriented and others screen-facing.
 * Portability tweak from Toshi -- be even more pessimistic about stack size.
 *
 * Revision 1.112  2005/02/08 20:10:59  slevy
 * Force using oldopengl (no glInterleavedArrays()/glDrawElements()) on MacOS X --
 * it seems to get the data completely wrong (most X's = 0, etc.).
 * If someone ever figures out why, I'd be curious to know.
 *
 * Revision 1.111  2004/10/19 01:52:27  slevy
 * Give specks_ieee_server() the right type to be an sproc function.
 * Need config.h early.
 * Initialize "oldopengl".
 * More digits for "where".
 *
 * Revision 1.110  2004/10/06 05:35:19  slevy
 * Add 3-D texturing.
 *
 * Revision 1.109  2004/07/26 16:44:50  slevy
 * specks_set_byvariable() now correctly fails on 0-length names.
 *
 * Revision 1.108  2004/07/26 04:33:01  slevy
 * Use vertex arrays for drawing points, if available.
 * Allow dyndata processors to switch themselves off with st->dyn.enabled = -1.
 *
 * Revision 1.107  2004/07/23 20:57:12  slevy
 * Move alloca boilerplate (ugh) earlier.
 * Use HAVE_SQRTF.
 *
 * Revision 1.106  2004/07/06 23:06:22  slevy
 * Make use of multiple timesteps even if we have a dyndata handler.
 *
 * Revision 1.105  2004/04/19 21:04:12  slevy
 * Un-CONST-ify most of what had been constified.  It's just too messy.
 *
 * Revision 1.104  2004/04/19 17:42:42  slevy
 * Use AC_FUNC_ALLOCA and the autoconf-standard alloca boilerplate
 * in each of the files that uses alloca.
 *
 * Revision 1.103  2004/04/12 19:09:54  slevy
 * Crank up default specular highlights.
 * CONSTify getfloats().
 *
 * Revision 1.102  2004/01/14 02:03:41  slevy
 * Add Chromatek color-coding of depth, contributed by Carl Hultquist
 * (chultquist@smuts.uct.ac.za, student of Anthony Fairall), as part of his
 * Computer Science Honours project.  Works with Chromatek glasses,
 * which make red things look nearby, blue things far away.
 * New commands:
 *    chromadepth {on|off}
 * 	replaces colors of points and polygons with depth-coded colors
 *    chromaparams ZSTART ZRANGE
 * 	sets Z-range (distance from camera plane) over which
 * 	colormapping applies.  Z=ZSTART maps to first entry,
 * 	Z=ZSTART+ZRANGE maps to last entry in colormap.
 *    chromacmap  FILE.CMAP
 * 	Specifies colormap.
 * See example files data/chromadepth.cmap and data/hipchroma.
 *
 * Revision 1.101  2004/01/09 13:32:06  slevy
 * Parse lum command correctly again...
 *
 * Revision 1.100  2004/01/09 04:54:46  slevy
 * Add lum options:
 *    "all"  - revert to autoranging
 *    "abs [base]"  - lum = scalefactor * abs(value - base)
 *    "lin"  - default behavior (unsets any "abs")
 *
 * Revision 1.99  2003/12/10 19:38:05  slevy
 * Add "pb" datacommand to read partadv-style .pb files.  Finally, a general-purpose
 * binary particle format!
 *
 * Revision 1.98  2003/11/03 17:06:55  smarx
 * small additions/changes to make os x and windows buildable and functional. open issues on windows time motion widgets and terminating cleanly.
 *
 * Revision 1.97  2003/10/26 16:20:22  slevy
 * Merge Steve Marx's many improvements -- generic slider,
 * MacOS X port, "home" command/button, lots of bug fixes -- into main branch!
 *
 * Revision 1.96  2003/10/25 00:42:52  slevy
 * Add "echo" data+control command.
 * Try to add pthread-based USE_IEEEIO.  Don't know if this works yet,
 * but it compiles.
 *
 * Revision 1.95  2003/09/30 08:44:37  slevy
 * Make waveobj accept objects made of line segments.
 * Revision 1.94.2.1  2003/05/15 14:58:57  smarx
 * rel_0_7_04 upgrades to fltk-1.1.3, build support for os x, os x middle mouse button keyboard simulation, os x workaround for partiview/fltk issues with proper redraw. this release is using a recent fltk-1.1.3 cvs upgrade - not the standard release - to resolve OpenGL bugs that will be in fltk-1.1.4. Note that this release of partiview requires fltk-1.1.x support which precludes use of the old file chooser
 *
 * Revision 1.94  2003/02/15 06:55:57  slevy
 * Add "slvalid" flag to struct dyndata.  Don't just use currealtime as
 * the validity test.
 *
 * Revision 1.93  2002/10/04 00:50:21  slevy
 * Fix CAVE-text-alignment hack for non-CAVE case.
 *
 * Revision 1.92  2002/10/02 07:00:04  slevy
 * Change text orientation in THIEBAUX_VIRDIR (often multipipe CAVE)
 * mode.  Rather than having text be oriented per-graphics-window,
 * we choose the orientation of the master pipe.
 *
 * Revision 1.91  2002/07/20 15:53:03  slevy
 * Make wavefront .obj models work with (or without) texture coordinates
 * and/or vertex normals.
 *
 * Revision 1.90  2002/07/20 05:03:39  slevy
 * Support textures and per-vertex normals from "waveobj" files.
 *
 * Revision 1.89  2002/07/10 22:51:59  slevy
 * Allow ``only= field > value'' etc.
 *
 * Revision 1.88  2002/06/18 21:43:52  slevy
 * Report copyright when "version" requested.
 *
 * Revision 1.87  2002/06/18 21:24:36  slevy
 * Add copyright string.
 * Add Illinois-open-source-license reference to LICENSE.partiview file.
 *
 * Revision 1.86  2002/06/05 01:13:31  slevy
 * Ha -- when reading tfm's, remember to initialize has_scl!
 *
 * Revision 1.85  2002/06/04 23:06:07  slevy
 * Use parti_idof(), not parti_object( NULL, &st, 0 ), to turn object into id.
 *
 * Revision 1.84  2002/04/22 17:58:49  slevy
 * Allow "w" option to "bound" command: compute bounding box in world
 * instead of object coordinates.
 *
 * Revision 1.83  2002/04/17 20:47:58  slevy
 * Add a not-quite-proprietary notice to all source files.
 * Once we pick a license this might change, but
 * in the mean time, at least the NCSA UIUC origin is noted.
 *
 * Revision 1.82  2002/04/16 18:40:26  slevy
 * Move tokenize() and rejoinargs() into findfile.c,
 * and out of this overstuffed piece of junk.
 *
 * Revision 1.81  2002/04/12 05:50:20  slevy
 * Fix (?) "only=" etc. commands, broken since I added the selection stuff last July!
 *
 * Revision 1.80  2002/03/31 22:40:32  slevy
 * "tfm" command -- only print full matrices if "tfm -v" given,
 * otherwise just x y z rx ry rz scale; print matrices in %g format for
 * full precision; accept 7th parameter as scale factor on input
 * just as we print it on output.
 *
 * Revision 1.79  2002/03/14 18:14:58  slevy
 * Peel out a bit more CAVE code,
 * and refer specifically to Marcus Thiebaux's virdir
 * since there's now also a Matt Hall version.
 *
 * Revision 1.78  2002/03/14 04:54:09  slevy
 * Strip out a bunch of virdir-specific CAVE menu clutter,
 * now moved into partimenu.c.
 *
 * Revision 1.77  2002/03/11 22:31:23  slevy
 * Move virdir-specific parti_seto2w() and specks_display() to partiview.cc.
 * "rawdata -a" particle-dump includes position, color, brightness too.
 *
 * Revision 1.76  2002/02/02 04:13:07  slevy
 * Unless PARTIMENU explicitly set, just get rid of the blasted menus.
 *
 * Revision 1.75  2002/01/27 00:38:19  slevy
 * Add "setenv name value" data command.
 *
 * Revision 1.74  2002/01/23 03:56:34  slevy
 * Add "timealign", "skipblanktimes" commands.
 * Default change: no longer skip blank times!
 * Per-data time ranges now extend overall clock range,
 * don't replace it.
 * Reset material properties (shininess, specular, ambient)
 * when drawing a lighted mesh.
 *
 * Revision 1.73  2001/12/28 07:23:57  slevy
 * Move plugins (kira, warp, ieeeio) off to separate source files.
 * "where" command gives full-precision values, and reports jump-pos with scale.
 * Allow for model-specific render function (initially for Maya models).
 * env TXDEBUG.
 *
 * Revision 1.72  2001/11/09 05:27:53  slevy
 * In specks_set_timestep(), don't set clock's range if we only have
 * a single timestep ourselves!
 *
 * Revision 1.71  2001/11/08 06:32:52  slevy
 * Extract async code into async.c.
 * Add pointsize metering, normally #ifdef USE_PTRACK'ed out.
 *
 * Revision 1.70  2001/11/06 06:20:30  slevy
 * Centralize test for nonempty timestep slot: specks_nonempty_timestep().
 * Fix tokenize() for case of "quoted" or 'quoted' arg strings.
 *
 * Revision 1.69  2001/10/19 17:48:51  slevy
 * Don't leak FILE in specks_read_waveobj().
 *
 * Revision 1.68  2001/08/29 17:54:00  slevy
 * Make rejoinargs() work robustly -- use a private static malloced area,
 * don't overwrite args in memory.
 *
 * Revision 1.67  2001/08/28 18:27:34  slevy
 * Remove lots of unused variables.  Move some used only in CAVE code
 * inside #ifdef brackets.
 *
 * Revision 1.66  2001/08/28 02:05:22  slevy
 * Fix format string usage.
 *
 * Revision 1.65  2001/08/26 17:40:04  slevy
 * Bounds-check all references to st->meshes[][], st->annot[][], etc. --
 * all those maintained by specks_timespecksptr.  We no longer guarantee
 * that st->curtime is always in range 0..st->ntimes-1, so just yield NULL
 * for out-of-range references.  New CURDATATIME(fieldname) does it.
 *
 * Revision 1.64  2001/08/26 02:10:40  slevy
 * "color rgb ..." sets colors of otherwise-uncolored mesh objects.
 * Only call parti_parse_args() if non-CAVE.
 *
 * Revision 1.63  2001/08/24 01:02:38  slevy
 * Add polygonal meshes -- wavefront .obj models.
 *
 * Revision 1.62  2001/08/16 19:58:56  slevy
 * Pull non-specks-specific code out of partibrains.c
 * and into partiview.cc, using new parti_add_commands hook.
 *
 * Revision 1.61  2001/08/16 17:23:41  slevy
 * New parti_parse_args() for generic add-on commands.
 * "where" prints more information.
 *
 * Revision 1.60  2001/07/19 20:10:23  slevy
 * st->threshsel is now kept as a SEL_DEST.
 * Fix seldest2src(), not even wrong.
 * Use specks_reupdate() after changing thresh, so the reported
 * selcount should be right.
 * specks_rethresh(): keep track of whether anything changed,
 * and increment st->selseq if so.
 *
 * Revision 1.59  2001/07/18 19:24:57  slevy
 * parse_selexpr() returns bit-encoded value saying whether src or dest or
 * both were filled in.
 *
 * Revision 1.58  2001/07/17 17:28:08  slevy
 * Increase MAXPTSIZE to allow bigger dots!
 *
 * Revision 1.57  2001/07/16 17:58:37  slevy
 * Don't take spurious snapshot if "snapset" command gets -... args.
 * Revert to compatible texture behavior: texture -A is again the default,
 * use texture -O for "over"-style blending.
 *
 * Revision 1.56  2001/07/15 23:09:46  slevy
 * Um, let "wanted == 0" and SEL_USE indicate that we want to match all pcles.
 *
 * Revision 1.55  2001/07/15 22:55:14  slevy
 * Er, those 'leadc' parameters to seldest()/selsrc() are ints, not SelTokens.
 *
 * Revision 1.54  2001/07/12 21:42:25  slevy
 * Use new selcounts() function to report stats whenever we mention a SelOp.
 * ``sel <src>'' alone now works too, to just report match count.
 *
 * Revision 1.53  2001/07/10 17:18:05  slevy
 * threshsel is a src, not a dest-type SelOp.
 *
 * Revision 1.52  2001/07/09 23:46:56  slevy
 * A bit more ripening of selection system...
 * And, report match counts from "sel", "thresh", "emph".
 *
 * Revision 1.51  2001/07/07 15:35:49  slevy
 * Implement more (most?) of selection framework:
 *     selsrc(), seldest(), selname(), seltoken().
 * "thresh" now accepts "-s 'destexpr'".
 * Add new selthresh SelOp.
 *
 * Revision 1.50  2001/07/04 03:44:12  slevy
 * Add framework of set-selection stuff; not really usable yet.
 * Next: implement "see" in drawspecks().
 * Add "picked" callback which gets notified of all pick results.
 *
 * Revision 1.49  2001/06/30 18:14:22  slevy
 * Provide for storing another bit in rgba alpha field: EMPHBIT, for
 * emphasis tag.
 * Use new sfStrDrawTJ (with transform & justification) for text.
 * Yeow, handle polymax properly -- default == infinity!
 * Add "vcmap" for per-datafield colormaps.
 * Enable GL_SMOOTH mode to get texture colors to work right (huh? why do we need to?).
 * "tfm c"/"tfm w" choose cam vs. world (default) location for object tfms.
 *
 * Revision 1.48  2001/05/30 14:32:55  slevy
 * Add subcameras ("subcam" command).
 * popen() needs "r" not "rb" -- no such thing as binary mode.
 * kira_open() gets original filename if we can't find it, for error messaging.
 *
 * Revision 1.47  2001/05/15 12:18:57  slevy
 * New interface to dynamic-data routines.
 * Now, the only #ifdef KIRA/WARP needed in partibrains.c are
 * the data-command initialization routines.  All others,
 * including control-command parsing and specialized drawing,
 * is now via a function table.
 *
 * Revision 1.46  2001/05/14 15:51:34  slevy
 * Initialize new speckseq value whenever we make a new specklist.
 * Put colorseq/sizeseq/threshseq invalidation in kira_parti, not here,
 * since we needn't do it when warping.
 * By default, clock range is whatever it had been set to before.
 *
 * Revision 1.45  2001/05/12 07:22:24  slevy
 * Add get-time-range func for dynamic data.
 * Initialize all dynamic-data stuff explicitly.
 *
 * Revision 1.44  2001/05/11 10:05:38  slevy
 * Add "warp" command if -DUSE_WARP.
 * Ellipsoids allow 3-component (Rx Ry Rz) orientation numbers too.
 * st->dyndata doesn't mean we should trash the anima[][] specklist.
 * Need to keep it intact for warping.
 *
 * Revision 1.43  2001/05/02 09:51:01  slevy
 * Add "ellipsoids" and "meshes" commands to toggle their display.
 * Finally parse snapshot arguments properly...?
 *
 * Revision 1.42  2001/04/26 08:52:46  slevy
 * Add hook for off-screen rendering: "-w width[xheight]" option for
 * snapshot/snapset.  Not actually implemented yet.
 * Add "clipbox hide".  This (a) doesn't display the big yellow box
 * and (b) doesn't enable OpenGL clipping -- just whatever object culling
 * the draw routine does anyway.
 * specks_draw_mesh() now pays attention to drawing style (solid/line/plane/point).
 * When drawing ellipsoids, disable blending altogether if bgcolor != 0.
 * Small, distant labels now disappear by default.  To get the old
 * draw-tiny-line behavior, use a negative "labelmin" value.
 *
 * Revision 1.41  2001/04/20 13:47:39  slevy
 * Allow *cment commands to alter multiple colormap entries: new editcmap().
 *
 * Revision 1.40  2001/04/10 19:18:33  slevy
 * Open sdb files (and while we're at it, other files too) in binary mode.
 *
 * Revision 1.39  2001/04/04 20:33:02  slevy
 * Use findfile() for "read" ctrl-command.
 *
 * Revision 1.38  2001/03/30 16:49:00  slevy
 * Change enum SurfStyle to avoid mentioning POINT, which Windows uses too.
 *
 * Revision 1.37  2001/03/30 14:00:42  slevy
 * Allow cment/boxcment/textcment as data commands too.
 * Add "ghosts" ctl command (not really implemented).
 * Add "winsize" ctl command.
 * Ellipsoids now accept -l levelno option, in which case
 * "hide levelno"/"show levelno" applies to them.  Default is -1,
 * which means "show if any level is shown", but hidden by "hide all".
 * Scrap ghost maintenance.  Do that elsewhere instead.
 * Make "maxcomment" work properly.
 *
 * Revision 1.36  2001/03/19 11:55:08  slevy
 * New version.c (derived from ../VERSION) contains current version string.
 * New "version" command in partibrains reports that and the partibrains.c CVS ver no.
 *
 * Revision 1.35  2001/03/19 10:39:07  slevy
 * Handle speck comments properly.
 *
 * Revision 1.34  2001/03/15 18:19:06  slevy
 * Don't include comments in argc/argv -- it complicates things.
 * Have a separate "comment" pointer.  Pass it to specks_read_ellipsoid too.
 *
 * Revision 1.33  2001/03/15 15:37:06  slevy
 * Yeow -- handle VIRDIR prefix properly!
 * Complain of unrecognized data commands.
 *
 * Revision 1.32  2001/03/14 17:27:05  slevy
 * Make rejoinargs() work -- don't omit last arg.
 * "object" command alone reports our currently selected object.
 *
 * Revision 1.31  2001/03/13 22:45:08  slevy
 * Add "verbose" flag to parti_allobjs, so "gall -v ..."
 * reports object name before invoking each command.
 *
 * Revision 1.30  2001/03/13 08:23:39  slevy
 * Use parti_object's new "create-if-not-present" flag.
 * Data-language references can create, command-language ones can't.
 * Switch specks_read() to argc/argv style, using new "tokenize()" function.
 * Some commands still want the original string, so we also have rejoinargs().
 * Allow for adjustable comment length with "maxcomment" data command.
 * New "ellipsoid" and "mesh" objects.  Only quadmeshes implemented right now.
 * Data-language "tfm" command is now quiet.  Command-language tfm still verbose.
 * Check at run time for endian-config errors and refuse to run if wrong!
 *
 * Revision 1.29  2001/03/08 22:02:29  slevy
 * Disable the parti menu unless PARTIMENU envar set.
 * Allow alpha to adjust brightness of mesh objects.
 *
 * Revision 1.28  2001/03/05 03:04:41  slevy
 * Add mesh objects.  Or, quad meshes, anyway.
 *
 * Revision 1.27  2001/03/04 16:47:35  slevy
 * Make "add" and "eval" behave consistently.
 * polyorivar and texturevar now accept field names as well as numbers.
 *
 * Revision 1.26  2001/02/22 20:04:34  slevy
 * CONSTify.
 *
 * Revision 1.25  2001/02/19 22:01:16  slevy
 * Satisfy windows C compiler: pull enum FadeModel outside struct speck, etc.
 *
 * Revision 1.24  2001/02/19 20:50:33  slevy
 * Oops, "object" should always invoke parti_object() whether there's an
 * alias or not!
 *
 * Revision 1.23  2001/02/17 22:02:54  slevy
 * Enlarge polygons so that unit circle is inscribed, not circumscribed.
 * Then "txscale .5" always shows entire texture regardless of polysides.
 * Add new "ptsize" command -- makes more sense than "fast".  "fast" still works.
 *
 * Revision 1.22  2001/02/17 17:44:05  slevy
 * For polygons, rotate circle of vertices by 1/2 step.
 * Then, for "polysides 4" and "txscale .707",
 * the vertices coincide with the corners of the 0..1 texture.
 *
 * Revision 1.21  2001/02/17 05:39:45  slevy
 * Allow (in data language) "object gN=NAME".
 *
 * Revision 1.20  2001/02/15 05:41:12  slevy
 * new textcmap, textcment commands.  Regularize color-gamma-mapping.
 * Object aliases: "object gN=ALIAS", or in command mode, "gN=ALIAS".
 * Accept "ellipsoid" data tag; not yet implemented.
 *
 * Revision 1.19  2001/02/05 00:41:52  slevy
 * Accept "time" as synonym for "step".
 * Add "pickrange".
 * Mention jump, center in help msg.
 *
 * Revision 1.18  2001/02/03 23:43:10  slevy
 * Don't let "kira tree" hide "kira track" -- demand 3 chars!
 *
 * Revision 1.17  2001/02/03 16:49:42  slevy
 * Update cookedcmap when "cment" changes cmap.
 *
 * Revision 1.16  2001/02/03 15:29:03  slevy
 * Toss unused variable.
 *
 * Revision 1.15  2001/02/03 14:50:44  slevy
 * Add "setgamma" (abbr. "setgam" or "cgam") command to adjust colors.
 * Add "kira tree {off|on|cross|tick} [tickscale]" subcommand
 * for showing tree structure of interacting groups.
 *
 * Revision 1.14  2001/01/31 17:11:54  slevy
 * Ensure that, for starlab, clock is always in "continuous" mode.
 *
 * Revision 1.13  2001/01/31 17:07:12  slevy
 * Add RCS Id and Log strings.
 *
 */

#define __USE_MISC	/* makes <math.h> define sqrtf() on GNU libc */

#include "config.h"

#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
  #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
extern void *alloca(int);
#   endif
#  endif
# endif
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>	/* for mallinfo(), amallinfo(), maybe alloca() too */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#if !defined(HAVE_SQRTF)
# define sqrtf(x)  sqrt(x)	/* if no sqrtf() */
#endif

#undef isspace		/* hack for irix 6.5 back-compat */
#undef isdigit
#undef isalnum

#if unix
# include <unistd.h>
# include <sys/types.h>
# include <netinet/in.h>  /* for htonl */
# include <time.h>

# ifndef WORDS_BIGENDIAN
#  include "config.h"	/* for WORDS_BIGENDIAN */
# endif
# if USE_IEEEIO
#  include <sys/prctl.h>
#  ifndef PR_SADDR		/* if SGI, use sproc() for bg processing */
#    define USE_PTHREAD  1	/* linux or other UNIX; use pthreads for bg */
#    include <pthread.h>
#  endif  /* end non-SGI */
# endif   /* end USE_IEEEIO */

#else /*WIN32*/
# include "winjunk.h"
# define WORDS_BIGENDIAN 0
#endif

#include <string.h>
#include <errno.h>


#if WORDS_BIGENDIAN
#define RGBALPHA(rgb, alpha)	((rgb) | (alpha))
#define RGBWHITE		0xFFFFFF00
#define	PACKRGBA(r,g,b,a)	((r)<<24 | (g)<<16 | (b)<<8 | (a))
#define RGBA_R(rgba)		(((rgba)>>24) & 0xFF)
#define RGBA_G(rgba)		(((rgba)>>16) & 0xFF)
#define RGBA_B(rgba)		(((rgba)>>8) & 0xFF)
#define RGBA_A(rgba)		(((rgba)) & 0xFF)
#else
#define RGBALPHA(rgb, alpha)	((rgb) | ((alpha)<<24))
#define RGBWHITE		0x00FFFFFF
#define	PACKRGBA(r,g,b,a)	((a)<<24 | (b)<<16 | (g)<<8 | (r))
#define RGBA_R(rgba)		(((rgba)) & 0xFF)
#define RGBA_G(rgba)		(((rgba)>>8) & 0xFF)
#define RGBA_B(rgba)		(((rgba)>>16) & 0xFF)
#define RGBA_A(rgba)		(((rgba)>>24) & 0xFF)
#endif

#define	THRESHBIT		PACKRGBA(0,0,0,1)
#define	EMPHBIT			PACKRGBA(0,0,0,2)
#define	EXTRABITS		PACKRGBA(0,0,0,0xff)
#define	RGBBITS			PACKRGBA(0xff,0xff,0xff,0)


#include "geometry.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>    /* for GLuint */
#endif

#include "shmem.h"	/* NewN(), etc. */
#include "futil.h"

#include "specks.h"

#include "textures.h"
#include "findfile.h"
#include "partiviewc.h"
#include "sfont.h"

#include <sys/types.h>
#include <signal.h>

#ifdef USE_KIRA
#include "kira_parti.h"
#endif

#ifdef USE_CONSTE
#include "conste.h"
#endif

#if CAVEMENU
# include "cavemenu.h"
# include "partimenu.h"
# define IFCAVEMENU(x)  (x)
#else
# define IFCAVEMENU(x)  /* nothing */
#endif

#if THIEBAUX_VIRDIR
# include "vd_util.h"
#endif

  /* Star Renderer (.sdb) structure -- from stardef.h */
typedef enum {ST_POINT, ST_BRIGHT_CLOUD ,ST_DARK_CLOUD, ST_BOTH_CLOUD, ST_OFF} stype;

typedef struct {
        float  x, y, z;
        float  dx, dy, dz;
        float  magnitude, radius;
        float  opacity;
        int  num;
        unsigned short  color;
        unsigned char   group;
        unsigned char   type;
} db_star;

typedef  struct  hrec { float  t;  int  num;}  hrec_t;
typedef  struct  mrec { float  mass, x, y, z, vx, vy, vz, rho, temp, sfr, gasmass;
                 int  id, token;}  mrec_t;
  /* end Star Renderer */


#define VDOT( v1, v2 )  ( (v1)->x[0]*(v2)->x[0] + (v1)->x[1]*(v2)->x[1] + (v1)->x[2]*(v2)->x[2] )


static int defcmap[] = {
  0x11eeee00,
  0x1106ee00, 0x120ea900, 0x1316ce00, 0x1520d500, 0x172ece00, 0x193fcb00,
  0x1c54b400, 0x206da400, 0x24889200, 0x2aa58400, 0x33c07600, 0x3ed96a00,
  0x4eed6100, 0x63fa5a00, 0x7efe5100, 0x99fb4c00, 0xafef4700, 0xc0dc4000,
  0xcbc33900, 0xd4a83000, 0xda8b2900, 0xdf702700, 0xe2572500, 0xe5411700,
  0xe82f1000, 0xea211500, 0xeb171400, 0xed0f1200, 0xee0aee00,
  0xffffff00,
};

int orientboxcolor = PACKRGBA( 0xff, 0xff, 0, 0xff );

void specks_read_boxes( struct stuff *st, char *fname, int timestep );
int  specks_add_box( struct stuff *st, struct AMRbox *box, int timestep );
int  specks_purge( void *vstuff, int nbytes, void *aarena );
int  specks_count( struct specklist *head );
int  specks_gobox( struct stuff *st, int boxno, int argcrest, char *argvrest[] );
int specks_cookcment( struct stuff *st, int cment );
void specks_rgbremap( struct stuff *st );

void specks_reupdate( struct stuff *st, struct specklist *sl );
struct specklist **specks_find_annotation( struct stuff *, struct specklist **);
void specks_set_current_annotation( struct stuff *st, char *annotation );
void specks_add_annotation( struct stuff *st, char *annotation, int timestep );
void specks_timerange( struct stuff *st, double *tminp, double *tmaxp );

void strncpyt( char *dst, char *src, int dstsize ) {
  int len = strlen(src);
  if(len >= dstsize) len = dstsize-1;
  memcpy(dst, src, len);
  dst[len] = '\0';
} 

#ifdef sgi
static float defgamma = 1.0;
#else
static float defgamma = 2.5;
#endif

struct stuff *
specks_init( int argc, char *argv[] )
{
  int i;
  struct stuff *st = NewN( struct stuff, 1 );

  i = PACKRGBA(1, 0, 0, 0);
  if(*(char *)&i != 1) {
    msg("specks_init(): trouble: is WORDS_BIGENDIAN mis-set?  Giving up.");
    exit(1);
  }

  memset(st, 0, sizeof(*st));
  st->spacescale = 1.0;
  st->fog = 0;
  st->psize = 1;
  st->alpha = .5;
  st->gamma = defgamma;
  st->rgbgamma[0] = st->rgbgamma[1] = st->rgbgamma[2] = 1.0;
  st->rgbright[0] = st->rgbright[1] = st->rgbright[2] = 1.0;
  specks_rgbremap( st );
  st->alias = NULL;
  st->useme = 1;
  st->usepoly = 0;
  st->usepoint = 1;
  st->usevec = 0;
  st->vecvar0 = 0;
  st->vecscale = 1.0f;
  st->vecalpha = 1.0f;
  st->vecarrowscale = 0.25f;
  st->usetext = 1;
  st->usetextaxes = 1;
  st->usetextures = 1;
  st->useboxes = 1;
  st->usemeshes = 1;
  st->staticmeshes = NULL;
  st->mullions = 0;
  st->useellipsoids = 1;
  st->polysizevar = -1;
  st->polyarea = 0;
  st->polyorivar0 = -1;
  st->use_chromadepth = 0;
  st->chromaslidestart = 2.0;
  st->chromaslidelength = 20;
  st->texturevar = -1;
  st->txscale = .5;
  st->boxlabels = 0;
  st->boxlabelscale = 1.0;
  st->boxlevelmask = ~0;	/* all levels on */
  st->boxaxes = 0;		/* boxes don't show orientation markers */
  st->goboxscale = 1.0;
  st->textsize = .05;
  st->npolygon = 11;
  st->subsample = 1;
  st->everycomp = 1;
  st->maxcomment = sizeof(st->sl->specks->title) - 1;
  st->dyn.enabled = 0;
  st->dyn.data = NULL;
  st->dyn.getspecks = NULL;
  st->dyn.draw = NULL;
  st->dyn.trange = NULL;
  st->dyn.help = NULL;
  st->dyn.ctlcmd = NULL;
  st->dyn.free = NULL;
  st->speckseq = 0;

  st->menudemandfps = 4.0;

  st->pfaint = .05;	/* params for "fast" point-drawing */
  st->plarge = 10;
  st->polymin = .5;	/* don't draw polygons if smaller (pixels) */
  st->polymax = 1e8;	/* don't allow polygons to get bigger than this (pixels) */
  st->polyfademax = 0;
  st->textmin = 2;	/* replace labels with line-segments if smaller (pixels) */
  st->ntextures = 0;
  st->textures = NULL;

  st->fade = F_SPHERICAL;
  st->fadeknee1 = 10.0;
  st->fadeknee2 = 1.0;
  st->knee2steep = 1.0;

  st->gscale = 1.;
  st->gtrans.x[0] = st->gtrans.x[1] = st->gtrans.x[2] = 0;
  st->objTo2w = Tidentity;

  st->ncmap = st->boxncmap = st->textncmap = COUNT(defcmap);
  st->cmap = NewN(struct cment, COUNT(defcmap));
  st->boxcmap = NewN(struct cment, COUNT(defcmap));
  st->textcmap = NewN(struct cment, COUNT(defcmap));
  for(i = 0; i < COUNT(defcmap); i++) {
    st->cmap[i].raw = st->boxcmap[i].raw = st->textcmap[i].raw
	= htonl(defcmap[i]);
    st->cmap[i].cooked = st->boxcmap[i].cooked = st->textcmap[i].cooked
	= specks_cookcment( st, st->cmap[i].raw );
  }
  /* Ensure that textcmap[0] is white by default */
  st->textcmap[0].raw = PACKRGBA( 170, 170, 170, 0 );
  st->textcmap[0].cooked = specks_cookcment( st, st->textcmap[0].raw );

  st->sizedby = 0;
  st->coloredby = 1;
  st->sizeseq = st->colorseq = st->threshseq = 0;
  st->trueradius = 0;
  st->sdbvars = shmstrdup( "mcr" );

  st->useemph = 0;
  selinit( &st->emphsel );
  st->emphfactor = 10;

  selinit( &st->threshsel );
  selinit( &st->seesel );

  st->ntimes = 0;
  st->ndata = 0;
  st->curtime = 0;
  st->curdata = 0;
  st->datatime = 0;
  st->usertrange = 0;
  st->utmin = -HUGE;
  st->utmax = HUGE;
  st->utwrap = 0.1;
  st->sl = NULL;

  st->boxes = NULL;
  st->boxlevels = 0;
  st->boxlinewidth = 0.75;

  st->depthsort = 0;

  st->clk = NewN(SClock, 1);
  clock_init(st->clk);
  clock_set_running(st->clk, 1);

  memset(st->anima, 0, sizeof(st->anima));
  memset(st->annot, 0, sizeof(st->annot));
  memset(st->datafile, 0, sizeof(st->datafile));
  memset(st->fname, 0, sizeof(st->fname));
  memset(st->meshes, 0, sizeof(st->meshes));

#if CAVE
  shmrecycler( specks_purge, st );
#endif

  for(i = 1; i < argc; i++)
    specks_read( &st, argv[i] );

  IFCAVEMENU( partimenu_init( st ) );

  return st;
}

void specks_rethresh( struct stuff *st, struct specklist *sl, int by )
{
  int i;
  int curdata = st->curdata;
  int nel = sl->nspecks;
  float threshmin = st->thresh[0], threshmax = st->thresh[1];
  struct speck *p = sl->specks;
  SelMask *sel = sl->sel;
  SelOp threshseldest;
  int changed = 0;
  int min, max;

  sl->threshseq = st->threshseq;

  if(sl->text != NULL)	/* specklists with labels shouldn't be thresholded */
    return;

  if(curdata >= st->ndata)
    curdata = 0;

  min = (SMALLSPECKSIZE(by)>=sl->bytesperspeck) ? 0 : st->usethresh&P_THRESHMIN;
  max = (SMALLSPECKSIZE(by)>=sl->bytesperspeck) ? 0 : st->usethresh&P_THRESHMAX;
 

  threshseldest = st->threshsel;

  for(i = 0, p = sl->specks; i < nel; i++, p = NextSpeck( p, sl, 1 )) {
    SelMask was = sel[i];
    if((min&&p->val[by]<threshmin) || (max&&p->val[by]>threshmax)) {
	/* p->rgba |= THRESHBIT; */
	SELUNSET( sel[i], &threshseldest );
    } else {
	/* p->rgba &= ~THRESHBIT; */
	SELSET( sel[i], &threshseldest );
    }
    changed |= was ^ sel[i];
  }
  if(changed) {
    sl->selseq++;
    if(st->selseq < sl->selseq) st->selseq = sl->selseq;
  }
}

int specks_cookcment( struct stuff *st, int cment )
{
    unsigned char crgba[4];
    memcpy(crgba, &cment, 4);		/* XXX 64-bit bug? */
    return (cment & ~RGBWHITE) |
	   PACKRGBA(
		st->rgbmap[0][crgba[0]],
		st->rgbmap[1][crgba[1]],
		st->rgbmap[2][crgba[2]],
		0 );
}

void specks_rgbremap( struct stuff *st )
{
  int i, k;
  for(i = 0; i < 256; i++) {
    float t = (float)i / 256;
    float v = 255.99f * pow(t, 1/st->rgbgamma[0]);
    k = (int) (st->rgbright[0] * v);


    st->rgbmap[0][i] = (k <= 0) ? 0 : (k > 255) ? 255 : k;
    if(st->rgbgamma[0] != st->rgbgamma[1])
        v = 255.99f * pow(t, 1/st->rgbgamma[1]);
    k = (int) (st->rgbright[1] * v);
    st->rgbmap[1][i] = (k <= 0) ? 0 : (k > 255) ? 255 : k;
    if(st->rgbgamma[1] != st->rgbgamma[2])
        v = 255.99f * pow(t, 1/st->rgbgamma[2]);
    k = (int) (st->rgbright[2] * v);
    st->rgbmap[2][i] = (k <= 0) ? 0 : (k > 255) ? 255 : k;
  }
  /* remake colormap too */
  for(i = 0; i < st->ncmap; i++)
    st->cmap[i].cooked = specks_cookcment( st, st->cmap[i].raw );

  /* and chromadepth colormaps */
  if (st->chromacm != NULL) {
    for (i = 0; i < st->nchromacm; i++)
	st->chromacm[i].cooked = specks_cookcment(st, st->chromacm[i].raw);
  }
  st->colorseq++;
}


Point e;

void specks_recolor( struct stuff *st, struct specklist *sl, int by )
{
  struct valdesc *vd;
  int i;
  int curdata = st->curdata;
  int nel = sl->nspecks;
  struct speck *sp = sl->specks;
  float cmin, cmax, normal;
  int ncmap = st->ncmap;
  struct cment *cmap = st->cmap;
  int index;
  enum RGBCode { NONE, RGB565, RGB888 } rgbcode;
  unsigned char (*rgbmap)[256];

  sl->coloredby = by;
  sl->colorseq = st->colorseq;

  if(sl->text != NULL)	/* specklists with labels shouldn't be recolored */
    return;

  if(curdata >= st->ndata)
    curdata = 0;

  if(by == CONSTVAL) {
    /* Hack -- color by given RGB value */
    char crgba[4];
    int r, g, b, rgba;
    vd = &st->vdesc[curdata][CONSTVAL];
    r = (vd->cmin<=0) ? 0 : vd->cmin>=1 ? 255 : (int)(255.99f * vd->cmin);
    g = (vd->cmax<=0) ? 0 : vd->cmax>=1 ? 255 : (int)(255.99f * vd->cmax);
    b = (vd->mean<=0) ? 0 : vd->mean>=1 ? 255 : (int)(255.99f * vd->mean);
    crgba[0] = st->rgbmap[0][r];
    crgba[1] = st->rgbmap[1][g];
    crgba[2] = st->rgbmap[2][b];
    crgba[3] = 0;
    rgba = *(int *)&crgba[0];	/* XXX 64-bit bug? */
    for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1)) {
	sp->rgba = rgba | (sp->rgba & EXTRABITS);
    }
    return; 
  }

  if(by >= MAXVAL || by < 0 || SMALLSPECKSIZE(by) > sl->bytesperspeck)
	by = 0;
	
  vd = &st->vdesc[curdata][by];

  if(vd->vncmap > 0) {
    ncmap = vd->vncmap;
    cmap = vd->vcmap;
  }


  cmin = vd->cmin, cmax = vd->cmax;
  if(cmin == cmax && cmin == 0 || (vd->call && !vd->cexact)) {
    vd->cmin = cmin = vd->min;
    vd->cmax = cmax = vd->max;
  }

  normal = (cmax != cmin && ncmap>2) ? (ncmap-2)/(cmax - cmin) : 0;
  rgbcode = NONE;
  if(0==strcmp(vd->name, "rgb565") || 0==strcmp(vd->name, "colors565")) rgbcode = RGB565;
  else if(0==strcmp(vd->name, "rgb888") || 0==strcmp(vd->name, "colors888")) rgbcode = RGB888;

  /* cexact field means:
   *   0 (default): scale data range to cmap index 1..ncmap-2;
   *		    use 0 and ncmap-1 for low- and high- out-of-range values.
   *
   *   1 ("exact"): use data value as literal colormap index, 0..ncmap-1.
   */
  rgbmap = &st->rgbmap[0];
  for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1)) {
    switch(rgbcode) {
    case RGB565:
	index = sp->val[by];
	sp->rgba = PACKRGBA(
			rgbmap[0][(index&0xF800)>>(11-(8-5))],
			rgbmap[1][(index&0x07E0)>>(5-(8-6))],
			rgbmap[2][(index&0x1F)<<(8-5)],
			0 )
		  | (sp->rgba & EXTRABITS);
	break;
    case RGB888:
	index = sp->val[by];
	sp->rgba = PACKRGBA(
			rgbmap[0][(index>>16)&0xFF],
			rgbmap[1][(index>>8)&0xFF],
			rgbmap[2][index&0xFF],
			0 )
		 | (sp->rgba & EXTRABITS);
    	break;

    default:
	index = vd->cexact  ?   sp->val[by] + cmin
			    :  (sp->val[by] - cmin) * normal + 1;

	if(index < 0) index = 0;
	else if(index >= ncmap) index = ncmap-1;

	sp->rgba = cmap[index].cooked | (sp->rgba & EXTRABITS);
    }
  }
}

void specks_resize( struct stuff *st, struct specklist *sl, int by )
{
  int i;
  int nel = sl->nspecks;
  struct speck *sp = sl->specks;
  int curdata = st->curdata;
  struct valdesc *vd;
  float lmin, lmax, normal, lbase;
  int labs;

  sl->sizedby = by;
  sl->sizeseq = st->sizeseq;

  if(sl->text != NULL) /* specklists with labels shouldn't be resized */
    return;

  if(curdata < 0 || curdata >= st->ndata)
    curdata = 0;

  if(by == CONSTVAL) {
    vd = &st->vdesc[curdata][CONSTVAL];
    for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1)) {
	sp->size = vd->lmin;
	if(SELECTED(sl->sel[i], &st->emphsel)) {
	    sp->size *= st->emphfactor;
	}
    }
    return;
  }

  if(by < 0 || by >= MAXVAL)
    by = 0;

  vd = &st->vdesc[curdata][by];

  lbase = vd->lbase;
  labs = (vd->lop == L_ABS);
  lmin = vd->lmin, lmax = vd->lmax;
  if(lmin == lmax && lmin == 0 || vd->lall) {
	vd->lmin = lmin = vd->min;
	vd->lmax = lmax = vd->max;
  }

  if(lmax == lmin)
    normal = 1;
  else
    normal = 1 / (lmax - lmin);


  if(st->useemph && st->emphsel.use != SEL_NONE) {
    for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1)) {
	sp->size = (labs ? fabsf(sp->val[by] - lmin) : (sp->val[by] - lmin)) * normal + lbase;
	if(SELECTED(sl->sel[i], &st->emphsel)) {
	    sp->size *= st->emphfactor;
	}
    }
  } else {
    /* normal case */
      if(labs) {
	for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1))
	    sp->size = fabsf(sp->val[by] - lmin) * normal + lbase;
      } else {
	for(i = 0; i < nel; i++, sp = NextSpeck(sp, sl, 1))
	    sp->size = (sp->val[by] - lmin) * normal + lbase;
      }
  }
}

void specks_datawait(struct stuff *st) {
#if USE_IEEEIO
    while(st->fetching && st->fetchpid > 0
	    && st->fetchtime == st->curtime
	    && st->fetchdata == st->curdata)
	usleep(50000);
#endif
}

#ifdef USE_IEEEIO

#if USE_PTHREAD
 pthread_t ieeethread;
#endif

void specks_ieee_server( void *vst ) {
  struct stuff *st = (struct stuff *)vst;
  struct specklist *sl;

#ifdef PR_SET_PDEATHSIG	/* linux variant of prctl */
  prctl(PR_SET_PDEATHSIG, SIGTERM);
#else
  prctl(PR_TERMCHILD);	/* Die when parent dies */
#endif

  /* Await a request */
  for(;;) {
    time_t then;
    while(st->fetching <= 0)
	usleep(5*10000);
    then = time(NULL);
    sl = specks_ieee_read_timestep( st, st->subsample,
				st->fetchdata, st->fetchtime );
    if(then + 5 < time(NULL))
	msg("... got %x (%d particles) from %s", sl, specks_count(sl),
		st->fname[st->fetchdata][st->fetchtime]);
    if(st->curtime == st->fetchtime && st->curdata == st->fetchdata
			&& sl != NULL) {
	st->sl = sl;
	parti_redraw();
    }
    st->fetching = 0;
  }
}

#endif /*USE_IEEEIO*/

void specks_set_timebase( struct stuff *st, double timebase ) {
  clock_set_timebase( st->clk, timebase );
  /* parti_set_timebase( st, timebase ); */
}

void specks_set_speed( struct stuff *st, double newspeed ) {
  clock_set_speed( st->clk, newspeed );
  parti_set_speed( st, newspeed );
}

void specks_set_fspeed( struct stuff *st, double newspeed ) {
  /* ignore it! */
  /* or maybe use fspeed to specify finite clock resolution */
}

void specks_set_timestep( struct stuff *st )
{
  struct specklist *sl = st->sl;
  struct specklist **slp;
  double realtime, tmin, tmax;
  int timestep;

  st->used++;

  tmin = st->clk->tmin;
  tmax = st->clk->tmax;
  specks_timerange( st, &tmin, &tmax );
  if(st->usertrange) {
    /*if(tmin < st->utmin)*/ tmin = st->utmin;
    /*if(tmax > st->utmax)*/ tmax = st->utmax;
  }
  /* Only expand clock's existing time range */
  if(tmin > st->clk->tmin) tmin = st->clk->tmin;
  if(tmax < st->clk->tmax) tmax = st->clk->tmax;
  clock_set_range( st->clk, tmin, tmax, st->utwrap );
  realtime = clock_time( st->clk );
  timestep = realtime;

  if(st->dyn.enabled > 0 && st->dyn.getspecks) {
    struct specklist *sl;

    if(realtime == st->currealtime && st->sl != NULL && st->dyn.slvalid && st->sl->used >= 0)
	return;
    sl = (*st->dyn.getspecks)(&st->dyn, st, realtime);
    if(sl == NULL)
	return;
    st->dyn.slvalid = 1;
    sl->used = 1;
    st->currealtime = realtime;
    st->curtime = timestep;

#if 0 /*WHATSTHIS?*/
    /* If user provided multiple timesteps, use them. */
    /* Maybe we're using dyndata to interpolate or something. */
    slp = specks_timespecksptr( st, st->curdata,
		(unsigned int)st->curtime < st->ntimes ?
		st->curtime : st->ntimes-1 );
#endif /*is this good for anything?*/

    st->sl = sl;
    /* Do we need this? */ /* *slp = sl; */ /* Guess not... */
    parti_set_timestep( st, realtime );
    return;
  }

  if(timestep >= st->ntimes) timestep = st->ntimes - 1;
  if(timestep < 0) timestep = 0;

#if 0 /*WHATSTHIS?*/
  /* The idea here was that, if we'd read subsampled data for some timestep
   * and later reduced the sampling factor so that we needed more,
   * this would discard the data so that we'd decide to reload it.
   * But just erasing the data isn't how we should do this...
   */
  slp = specks_timespecksptr( st, st->curdata, timestep );
  if(*slp != NULL && (*slp)->subsampled > st->subsample) {
    specks_discard( st, slp );
  }
#endif

  sl = specks_timespecks( st, st->curdata, timestep );

#ifdef USE_IEEEIO

  if(sl == NULL && (st->fetching == 0 || st->datasync)
		&& st->curdata>=0 && st->curdata<st->ndata
		&& st->datafile[st->curdata] != NULL
		&& timestep>=0 && timestep<st->ntimes
		&& st->datafile[st->curdata][timestep] != NULL) {

    st->fetchdata = st->curdata;
    st->fetchtime = timestep;

    if(getenv("NO_SPROC") || st->datasync) {
	sl = specks_ieee_read_timestep( st, st->subsample,
                                st->fetchdata, st->fetchtime );
        msg("Got %x (%d particles) from %s  (d%d t%d)\n",
		sl, specks_count(sl), st->fname[st->fetchdata][st->fetchtime],
		st->fetchdata, st->fetchtime);

    } else {
	st->fetching = 1;

	if(st->fetchpid <= 0) {
#if USE_PTHREAD

	    st->fetchpid = pthread_create( &ieeethread, NULL, specks_ieee_server, st );
	    if(st->fetchpid < 0)
		perror("ieee pthread_create");

#else	    /* sgi -- has sproc() */

	    st->fetchpid = sproc( specks_ieee_server, PR_SADDR|PR_SFDS, st );
	    if(st->fetchpid < 0)
		perror("sproc");
#endif

	}
    }
  }

  if(st->fetchpid > 0) {
    if(kill(st->fetchpid, 0) < 0) {
	perror("specks server vanished: kill -0");
	st->fetching = 0;
	st->fetchpid = 0;
    }
  }
#endif /*USE_IEEEIO*/

  if(st->skipblanktimes
	&& sl == NULL
	&& !(st->boxtimes>timestep && st->boxes[timestep]!=NULL)
	&& st->meshes[st->curdata][timestep] == NULL)
    return;

  st->sl = sl;   /* st->sl <= anima[][] */
  st->curtime = timestep;
  st->currealtime = timestep;
#if !THIEBAUX_VIRDIR
  st->frame_time = st->curtime;	/* if non-VD, we have no frame-function */
#endif

  { struct specklist *asl = CURDATATIME(annot);
    specks_set_current_annotation( st, asl ? asl->text : NULL );
  }
  parti_set_timestep( st, timestep );

}

void specks_set_current_annotation( struct stuff *st, char *annotation )
{
  st->annotation = annotation;
  IFCAVEMENU( partimenu_annot( st->annotation ) );
}

void specks_add_annotation( struct stuff *st, char *annotation, int timestep )
{
  struct specklist *sl, **slp;
  int curtime = (timestep < 0) ? st->curtime : timestep;

  if(annotation == NULL) annotation = "";

  specks_ensuretime( st, st->curdata, curtime );
  if((unsigned int)curtime >= st->ntimes) curtime = 0;
  slp = &st->annot[st->curdata][curtime];	/* safe -- no need for CURDATATIME() check */
  if((sl = *slp) == NULL) {
    sl = *slp = NewN( struct specklist, 1 );
    memset(*slp, 0, sizeof(**slp));
    sl->speckseq = ++st->speckseq;
  } else if(sl->text) {
    Free(sl->text);
  }
  sl->text = NewN( char, strlen(annotation)+1 );
  strcpy( sl->text, annotation );
}

void swab32( int n, int *ovp, int *ivp ) {
    while(n-- > 0) {
	register unsigned int v = *ivp++;
	*ovp++ = (v>>24)&0xFF | (v>>8)&0xFF00 | (v&0xFF00)<<8 | (v&0xFF)<<24;
    }
}

#define PBH_MAGIC       0xffffff98

/*
 * .pb files (as from David Wojtowicz's partadv, adapted by Stuart Levy) have form:
 * <PBH_MAGIC (int32)> <offset-to-data (int32)> <number-of-attributes (int32)>
 * attrname0 \0 attrname1 \0 ...
 * and then at byte address offset-to-data, is some unspecified number of:
 * <id (int32)> <x> <y> <z>  <attrvalue0> <attrvalue1> ... <attrvalue<nattr-1>>
 * ending at end-of-file.  X, Y, Z and attribute values are 32-bit IEEE floats.
 * File may be in either big- or little-endian format; readers should
 * try interpreting the PBH_MAGIC value either way, and assume that all other
 * fields have the same endian-ness that it has.
 */

void specks_read_pb( struct stuff *st, char *pbfname, int timestep )
{
    float vmin[MAXVAL], vmax[MAXVAL], vsum[MAXVAL];
    struct specklist *sl, **slp;
    register struct speck *sp;
    struct hdr {
	int magic;
	int dataoff;
	int attrin;
    } header;
    int nspecks, nattr, i, k, nnow;
    int swappedmagic;
    int inswap;
    int readunit, readwords;
    long filelen;
    struct valdesc *vd;
    float *unit;
    FILE *inf = fopen(pbfname, "rb");

    if(inf == NULL) {
	msg("pb: %s: cannot open: %s", pbfname, strerror(errno));
	return;
    }

    if(fread(&header, 4, 3, inf) != 3) {
	msg("pb: %s: dud .pb file", pbfname);
	fclose(inf);
	return;
    }
    swappedmagic = header.magic;
    swab32( 1, &swappedmagic, &swappedmagic );
    if(header.magic == PBH_MAGIC) {
	inswap = 0;
    } else if(swappedmagic == PBH_MAGIC) {
	inswap = 1;
	swab32( 3, (int *)&header, (int *)&header );
    } else {
	msg("pb: %s lacks PBH_MAGIC header", pbfname);
	fclose(inf);
	return;
    }

    vd = &st->vdesc[st->curdata][0];
    if(vd->name[0] == '\0' || vd->name[0] == '-')
	strcpy(vd->name, "id");

    for(nattr = 1; nattr <= header.attrin && nattr < MAXVAL; nattr++) {
	int c, k = 0;
	vd = &st->vdesc[st->curdata][nattr];
	if(vd->name[0] != '\0' && vd->name[0] != '-')
	    vd = NULL;
	k = 0;
	while((c = getc(inf)) != EOF && c != '\0') {
	    if(vd && k < sizeof(vd->name)-1)
		vd->name[k++] = c;
	}
	if(vd)
	    vd->name[k] = '\0';
    }

    errno = 0;
    fseek(inf, 0, SEEK_END);

    readwords = (4 + header.attrin);
    readunit = readwords * 4;

    unit = NewA( float, readwords );
    memset( vsum, 0, nattr*sizeof(float) );

    filelen = ftell(inf);
    if(filelen < 0) {
	msg("pb: %s: not a real file (can't determine length)", pbfname);
	fclose(inf);
	return;
    }

    if((filelen - header.dataoff) % readunit != 0) {
	msg("pb %s: file is %ld bytes long, but %ld-%d not a multiple of %d!?",
		pbfname, filelen, filelen, header.dataoff, readunit);
	fclose(inf);
	return;
    }
    nspecks = (filelen - header.dataoff) / readunit;

    sl = NewN(struct specklist, 1);
    memset(sl, 0, sizeof(*sl));
    sl->speckseq = ++st->speckseq;

    sl->bytesperspeck = SMALLSPECKSIZE( nattr );

    sl->scaledby = st->spacescale;
    sp = NewNSpeck(sl, nspecks);
    sl->specks = sp;
    sl->sel = NewN( SelMask, nspecks );
    sl->nsel = nspecks;
    memset(sl->sel, 0, nspecks*sizeof(SelMask));

    fseek(inf, header.dataoff, SEEK_SET);
    for(i = 0; i < nspecks; i++) {
	int a;
	struct speck *sp = NextSpeck( sl->specks, sl, i );
	if(fread(unit, readunit, 1, inf) <= 0) {
	    msg("pb %s: got only %d of %d entries", pbfname, i, nspecks);
	    sl->nspecks = sl->nsel = nspecks = i;
	    break;
	}
	if(inswap) {
	    int id;
	    swab32( 1, &id, (int *)&unit[0] );
	    sp->val[0] = (float)id;
	    swab32( 3, (int *)&sp->p.x[0], (int *)&unit[1] );
	    swab32( nattr-1, (int *)&sp->val[1], (int *)&unit[4] );
	} else {
	    sp->val[0] = *(int *)&unit[0];
	    memcpy( &sp->p.x[0], &unit[1], 3*sizeof(float) );
	    memcpy( &sp->val[1], &unit[4], (nattr-1)*sizeof(float) );
	}
	if(i == 0) {
	    for(a = 0; a < nattr; a++)
		vmin[a] = vmax[a] = vsum[a] = sp->val[a];
	} else {
	    for(a = 0; a < nattr; a++) {
		float v = sp->val[a];
		if(vmin[a] > v) vmin[a] = v;
		else if(vmax[a] < v) vmax[a] = v;
		vsum[a] += v;
	    }
	}
    }
    sl->nspecks = sl->nsel = nspecks;

    /* update statistics */
    if(nspecks > 0) {
	int a;
	for(a = 0; a < nattr; a++) {
	    struct valdesc *vdp = &st->vdesc[st->curdata][a];
	    if(vdp->min > vmin[a]) vdp->min = vmin[a];
	    if(vdp->max < vmax[a]) vdp->max = vmax[a];
	    vdp->nsamples += sl->nspecks;
	    vdp->sum += vsum[a];
	    vdp->mean = vdp->sum / vdp->nsamples;
	}
    }

    /* Add to running list */
    specks_insertspecks( st, st->curdata, timestep, sl );

    fclose(inf);
}

	    

#if !WORDS_BIGENDIAN
void starswap(db_star *st) {
  int i, *wp;
  /* byte-swap x,y,z, dx,dy,dz, magnitude,radius,opacity fields (32-bit float),
   * 		num (32-bit int),
   *		color (16-bit short).
   * group and type fields shouldn't need swapping,
   * assuming the compiler packs bytes into a word in increasing
   * address order.  Seems safe.
   */
  for(i = 0, wp = (int *)st; i < 10; i++)
    wp[i] = htonl(wp[i]);
  st->color = htons(st->color);
}
#endif /*!WORDS_BIGENDIAN*/

void specks_read_sdb( struct stuff *st, char *sdbfname, int timestep )
{
  FILE *inf = fopen(sdbfname, "rb");
  long flen;
  int nspecks, i;
  float min[MAXVAL], max[MAXVAL], sum[MAXVAL];
  struct specklist *sl, **slp;
  register struct speck *sp;
  int dfltvars = (strcmp(st->sdbvars, "mcr") == 0);
  int nvars = strlen(st->sdbvars);

  if(inf == NULL) {
    msg("sdb: %s: cannot open: %s", sdbfname, strerror(errno));
    return;
  }
  /* Just measure file size */
  errno = 0;
  fseek(inf, 0, SEEK_END);
  flen = ftell(inf);
  if(flen == -1 || flen == 0) {
    msg("sdb: %s: can't measure length of file: %s", sdbfname, strerror(errno));

    return;
  }
  nspecks = (flen / sizeof(db_star));

  if(nspecks <= 0) {
    msg("sdb: %s: ignoring empty sdb file", sdbfname);
    return;
  }
  
  sl = NewN(struct specklist, 1);
  memset(sl, 0, sizeof(*sl));
  sl->speckseq = ++st->speckseq;
  
  sl->bytesperspeck = SMALLSPECKSIZE( nvars );
  if(nvars > MAXVAL) nvars = MAXVAL;

  sl->scaledby = st->spacescale;
  sp = NewNSpeck(sl, nspecks);
  sl->specks = sp;
  sl->sel = NewN( SelMask, nspecks );
  sl->nsel = nspecks;
  memset(sl->sel, 0, nspecks*sizeof(SelMask));

  fseek(inf, 0, SEEK_SET);
  for(i = 0; i < nspecks; i++, sp = NextSpeck(sp, sl, 1)) {
    db_star star;
    char *cp;
    int k;
    float *vp;
    if(fread(&star, sizeof(star), 1, inf) <= 0)
	break;
#if !WORDS_BIGENDIAN
    starswap(&star);
#endif
    sp->p.x[0] = star.x * sl->scaledby;
    sp->p.x[1] = star.y * sl->scaledby;
    sp->p.x[2] = star.z * sl->scaledby;
    if(dfltvars) {
	sp->val[0] = exp((-18-star.magnitude)*.921/*log(100)/5*/);
	sp->val[1] = star.color;
	sp->val[2] = star.radius;
    } else {
	for(vp = &sp->val[0], cp = st->sdbvars; *cp; cp++, vp++) {
	    switch(*cp) {
	    case 'm': *vp = exp((-18-star.magnitude)*.921/*log(100)/5*/); break;
	    case 'M': *vp = star.magnitude; break;
	    case 'c': *vp = star.color; break;
	    case 'r': *vp = star.radius; break;
	    case 'o': *vp = star.opacity; break;
	    case 'g': *vp = star.group; break;
	    case 't': *vp = star.type; break;
	    case 'x': *vp = star.dx; break;
	    case 'y': *vp = star.dy; break;
	    case 'z': *vp = star.dz; break;
	    case 'S': *vp = sqrt(star.dx*star.dx + star.dy*star.dy + star.dz*star.dz); break;
	    case 'n': *vp = star.num; break;
	    default: *vp = 1; break;
	    }
	}
    }

    if(i == 0) {
	for(k = 0; k < nvars; k++)
	    sum[k] = min[k] = max[k] = sp->val[k];
    } else {
	for(k = 0; k < nvars; k++) {
	    if(min[k] > sp->val[k]) min[k] = sp->val[k];
	    else if(max[k] < sp->val[k]) max[k] = sp->val[k];
	    sum[k] += sp->val[k];
	}
    }
  }
  sl->nspecks = i;
  sl->sizedby = 0;
  sl->coloredby = 1;

  /* Update statistics */
  if(sl->nspecks > 0) {
    struct valdesc *vdp = &st->vdesc[st->curdata][0];
    for(i = 0; i < nvars; i++, vdp++) {
	if(vdp->min > min[i]) vdp->min = min[i];
	if(vdp->max < max[i]) vdp->max = max[i];
	vdp->nsamples += sl->nspecks;
	vdp->sum += sum[i];
	vdp->mean = vdp->sum / vdp->nsamples;

	if(vdp->name[0] == '\0') {
	    CONST char *name = "unk";
	    switch(st->sdbvars[i]) {
	    case 'm': name = "lumsdb"; break;
	    case 'M': name = "magsdb"; break;
	    case 'c': name = vdp->max > 16384 ? "rgb565" : "colorsdb";
		      vdp->cexact = 1;
		      break;
	    case 'r': name = "radius"; break;
	    case 'o': name = "opacity"; break;
	    case 'g': name = "group"; break;
	    case 't': name = "type"; break;
	    case 'x': name = "dx"; break;
	    case 'y': name = "dy"; break;
	    case 'z': name = "dz"; break;
	    case 'S': name = "speed"; break;
	    case 'n': name = "number"; break;
	    }
	    strcpy(vdp->name, name);
	}
    }
    specks_recolor( st, sl, st->coloredby );
    specks_resize( st, sl, st->sizedby );
  }

  /* Add to running list */
  specks_insertspecks( st, st->curdata, timestep, sl );

  fclose(inf);
}

void specks_timerange( struct stuff *st, double *tminp, double *tmaxp )
{
  if(st->dyn.enabled && st->dyn.trange &&
	(*st->dyn.trange)(&st->dyn, st, tminp, tmaxp, 0/*ask for full range*/)) {
    /* OK */
  } else {
    *tminp = 0;
    *tmaxp = (st->ntimes == 0) ? 0 : st->ntimes - 1;
  }
}

int specks_get_datastep( struct stuff *st )
{
  return st->curtime;
}

double specks_get_realtime( struct stuff *st )
{
  return st->currealtime;
}

void specks_reupdate( struct stuff *st, struct specklist *sl )
{
  struct specklist *tsl;

  if(sl != NULL && sl->threshseq != st->threshseq) {
    for(tsl = sl; tsl != NULL; tsl = tsl->next)
	specks_rethresh( st, tsl, st->threshvar );
  }

  if(sl != NULL && sl->colorseq != st->colorseq) {
    for(tsl = sl; tsl != NULL; tsl = tsl->next)
	specks_recolor( st, tsl, st->coloredby );
  }

  if(sl != NULL && sl->sizeseq != st->sizeseq) {
    for(tsl = sl; tsl != NULL; tsl = tsl->next)
	specks_resize( st, tsl, st->sizedby );
  }
}

static int specks_nonempty_timestep( struct stuff *st, int timestep ) {
  
  if(specks_timespecks( st, st->curdata, timestep ) != NULL)
    return 1;

  if((st->datafile[st->curdata] != NULL &&
		st->datafile[st->curdata][timestep] != NULL))
    return 1;

  if(timestep >= 0 && timestep < st->ntimes &&
		st->meshes[st->curdata][timestep] != NULL)
    return 1;

  if(timestep >= 0 && timestep < st->boxtimes &&
		st->boxes[timestep] != NULL)
    return 1;

  return 0;
}


void specks_set_time( struct stuff *st, double newtime )
{

  clock_set_time( st->clk, newtime );
  specks_set_timestep( st );
  if(st->sl == NULL && st->ntimes > 1 && st->clk->parent == NULL) {
    /* Skip blank time-slots -- keep incrementing until either:
     *  - we find a time-slot that has (or could have) some data, or
     *  - we've run through all time-steps (avoid infinite loops!).
     */
    int nudges, ts = 0;
    if(st->skipblanktimes) {
	for(nudges = 0; nudges < st->ntimes; nudges++) {
	    ts = (st->curtime + nudges) % st->ntimes;
	    if( specks_nonempty_timestep( st, ts ) )
		break;
	}
    }
    clock_set_time( st->clk, ts );
    specks_set_timestep( st );
  }
  /* st->playnext = now + (st->fspeed != 0 ? 1/st->fspeed : 0); */

  IFCAVEMENU( partimenu_setpeak( st ) );

  specks_reupdate( st, st->sl );

  IFCAVEMENU( partimenu_refresh( st ) );
}

/*
 * Just stash these values in our frame function so they won't change visibly
 * during a frame.  Each frame function will do this; we'll just hope that
 * they don't change as the various cave-wall processes start.
 */
void specks_current_frame( struct stuff *st, struct specklist *sl )
{
  st->frame_sl = sl;
  st->frame_time = st->curtime;
  st->frame_data = st->curdata;
  st->frame_annotation = st->annotation;
  specks_reupdate( st, sl );
}

#define	MAXXYFAN 16

extern void specks_draw_boxes( struct stuff *st, struct AMRbox *boxes, int levelmask, Matrix Ttext, int oriented );

extern void specks_draw_mesh( struct stuff *st, struct mesh *m, int *texturing );
extern void specks_draw_ellipsoid( struct stuff *st, struct ellipsoid *e );

struct cpoint {
    int rgba;
    Point p;
};

static int oldopengl = -1;

static void init_opengl(void)
{
    if(oldopengl < 0) {
	const GLubyte *s = glGetString( GL_VERSION );
	char *oog = getenv("PARTIOLDOPENGL");
	oldopengl = (oog!=NULL) ? atoi(oog)
		    : (s==NULL || 0==strncmp((const char *)s, "1.0", 3))
			? 1 : 0;
    }
}

#ifdef DEBUG
static void glcheck(char *where) {
    int e;
    while((e = glGetError()) != 0) {
	fprintf(stderr, "glerr 0x%x: %s\n", e, where);
    }
}
#else
# define glcheck(where)  /* nothing */
#endif

void dumpcpoints( struct cpoint *cp, int n )
{
    glcheck("pre-dumpcpoints");
    glBegin( GL_POINTS );
    while(--n >= 0) {
	glColor4ubv( (GLubyte *)&cp->rgba );
	glVertex3fv( &cp->p.x[0] );
	cp++;
    }
    glEnd();
    glcheck("post-dumpcpoints");
}

void dumpcpointsarray( struct cpoint *cp, int n )
{
   glcheck("pre-dumpcpointsarray");
    /* it looks like there are bugs in glInterleavedArrays( GL_C4UB_V3F ... )
     * at least in some common implementations, like GLX
     * (NVidia's GLX??  Mesa's GLX??), and on MacOS X.
     * Use separate Vertex and Color arrays instead and hope for the best.
     * Note that this requires we do glEnable/DisableClientState too.  Oh well.
     */
   
    // glInterleavedArrays( GL_C4UB_V3F, sizeof(struct cpoint), cp );
    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(struct cpoint), &cp[0].rgba );
    glVertexPointer( 3, GL_FLOAT, sizeof(struct cpoint), &cp[0].p.x[0] );
    glDrawArrays( GL_POINTS, 0, n );
   glcheck("post-dumpcpointsarray");
}

static Point depth_fwd;
static float depth_d;

struct order {
  float z;
  struct speck *sp;
  struct specklist *sl;
};

static int depthcmp( const void *a, const void *b )
{
  return ((struct order *)a)->z < ((struct order *)b)->z ?    1
	: ((struct order *)a)->z > ((struct order *)b)->z ? -1 : 0;
}

static int additive_blend;

void sortedpolys( struct stuff *st, struct specklist *slhead, Matrix *Tc2wp, float radperpix, float polysize )
{
  struct speck *sp;
  struct order *op, *obase;
  int i, k, total, skip;
  struct specklist *sl;
  int usethresh = /* st->usethresh&P_USETHRESH ? THRESHBIT : */ 0;
  int bps = 0;
  int prevrgba = -1;
  int usearea = st->polyarea;
  int sizevar = st->polysizevar;
  int polyorivar = st->polyorivar0;
  int texturevar = st->texturevar;
  int txno;
  int texturing = -1;
  float polyminrad = st->polymin * radperpix;
  float polymaxrad = st->polymax * radperpix;
  float mins2d = polyminrad * polyminrad;
  int rgba;
  int alpha = st->alpha * 255;
  int nfan = st->npolygon<MAXXYFAN ? st->npolygon : MAXXYFAN;
  float xyfan[MAXXYFAN][2];
  Point sfan[MAXXYFAN];
  Matrix Tc2w = *Tc2wp;
  float scl = vlength( (Point *)&Tc2w.m[0*4+0] );
  float fanscale;
  Texture *wanttx;
  int additive = additive_blend;
  int wantblend = additive;

  int useclip = (st->clipbox.level != 0);
  Point clipp0 = st->clipbox.p0;
  Point clipp1 = st->clipbox.p1;


  for(total = 0, sl = slhead; sl != NULL; sl = sl->next) {
    if(sl->text != NULL || sl->nspecks == 0 || sl->special != SPECKS)
	continue;
    skip = st->subsample;
    if(sl->subsampled != 0)	/* if already subsampled */
	skip /= sl->subsampled;
    if(skip <= 0) skip = 1;


    if(bps < sl->bytesperspeck) bps = sl->bytesperspeck;
    if(usethresh) {
	for(i=sl->nspecks, sp=sl->specks; i>0; i-=skip, sp=NextSpeck(sp,sl,skip)) {
	    if(SELECTED(sl->sel[i], &st->seesel))
		total++;
	}
    } else {
	total += sl->nspecks / skip;
    }
  }

  obase = op = (struct order *)malloc( (total+1) * sizeof(struct order) );
  for(sl = slhead; sl != NULL; sl = sl->next) {
    if(sl->text != NULL || sl->nspecks == 0 || sl->special != SPECKS)
	continue;
    skip = st->subsample;
    if(sl->subsampled != 0)	/* if already subsampled */
	skip /= sl->subsampled;
    if(skip <= 0) skip = 1;
    for(i=sl->nspecks, sp=sl->specks; i > 0; i-=skip, sp=NextSpeck(sp,sl,skip)) {
	float dist;
	if(!SELECTED(sl->sel[i], &st->seesel))
	    continue;
	dist = VDOT( &sp->p, &depth_fwd ) + depth_d;
	if(dist < 0)
	    continue;
	if(useclip &&
	  (sp->p.x[0] < clipp0.x[0] ||
	   sp->p.x[0] > clipp1.x[0] ||
	   sp->p.x[1] < clipp0.x[1] ||
	   sp->p.x[1] > clipp1.x[1] ||
	   sp->p.x[2] < clipp0.x[2] ||
	   sp->p.x[2] > clipp1.x[2]))
	    continue;
	op->z = dist;
	op->sp = sp;
	op->sl = sl;
	op++;
    }
  }

  total = op - obase;
  qsort( obase, total, sizeof(*obase), depthcmp );

  prevrgba = 0;

  /* Build prototype fan -- unit disk in screen plane */
  /* Scale the fan big enough that the unit disk is inscribed in our polygon */
  fanscale = 1 / (scl * cos(M_PI/nfan));
  for(i = 0; i < nfan; i++) {
    float theta = 2*M_PI*(i+0.5f)/nfan;
    xyfan[i][0] = cos(theta);
    xyfan[i][1] = sin(theta);
    vcomb( &sfan[i], xyfan[i][0] * fanscale, (Point *)&Tc2w.m[0*4+0],
		     xyfan[i][1] * fanscale, (Point *)&Tc2w.m[1*4+0] );
  }

  if(st->usetextures == 0 || SMALLSPECKSIZE(texturevar) > bps)
    texturevar = -1;
  if(SMALLSPECKSIZE(polyorivar) > bps)
    polyorivar = -1;

  for(i = 0, op = obase; i < total; i++, op++) {
    float dist, size;

    sp = op->sp;
    dist = op->z;
    size = sp->val[sizevar] * polysize;
    if(usearea) {
	if(size < dist * dist * mins2d)
	    continue;
	size = sqrtf(size);
    } else {
	if(size < dist * polyminrad)
	    continue;
    } 
    if(size > dist * polymaxrad)
	size = dist * polymaxrad;

    rgba = sp->rgba & RGBBITS;
    if(rgba != prevrgba) {
	prevrgba = rgba;
	rgba = RGBALPHA( prevrgba, alpha );
	glColor4ubv( (GLubyte *)&rgba );
    }

    if(texturevar >= 0 &&
	    (txno = sp->val[texturevar]) >= 0 &&
	    txno < st->ntextures &&
	    (wanttx = st->textures[txno]) != NULL) {

	txbind( wanttx, &texturing );
	wantblend = (wanttx->flags & TXF_ADD) ? 1 : additive_blend;
    } else if(texturing) {
	glDisable( GL_TEXTURE_2D );
	texturing = 0;
    }

    if(wantblend != additive) {
	additive = wantblend;
	glBlendFunc( GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA );
    }

    if(polyorivar >= 0 && sp->val[polyorivar] < 9) {
	float *xv = &sp->val[polyorivar];
	float *yv = &sp->val[polyorivar+3];

	glBegin( GL_TRIANGLE_FAN );
	if(texturing) {
	    for(k = 0; k < nfan; k++) {
		glTexCoord2fv( &xyfan[k][0] );
		glVertex3f(
		    sp->p.x[0] + size*(xyfan[k][0]*xv[0] + xyfan[k][1]*yv[0]),
		    sp->p.x[1] + size*(xyfan[k][0]*xv[1] + xyfan[k][1]*yv[1]),
		    sp->p.x[2] + size*(xyfan[k][0]*xv[2] + xyfan[k][1]*yv[2]));
	    }
	} else {
	    for(k = 0; k < nfan; k++) {
		glVertex3f(
		    sp->p.x[0] + size*(xyfan[k][0]*xv[0] + xyfan[k][1]*yv[0]),
		    sp->p.x[1] + size*(xyfan[k][0]*xv[1] + xyfan[k][1]*yv[1]),
		    sp->p.x[2] + size*(xyfan[k][0]*xv[2] + xyfan[k][1]*yv[2]));
	    }
	}
	glEnd();

    } else {
	glBegin( GL_TRIANGLE_FAN );
	if(texturing) {
	    for(k = 0; k < nfan; k++) {
		glTexCoord2fv( &xyfan[k][0] );
		glVertex3f(
		    sp->p.x[0] + size*sfan[k].x[0],
		    sp->p.x[1] + size*sfan[k].x[1],
		    sp->p.x[2] + size*sfan[k].x[2] );
	    }
	} else {
	    for(k = 0; k < nfan; k++) {
		glVertex3f(
		    sp->p.x[0] + size*sfan[k].x[0],
		    sp->p.x[1] + size*sfan[k].x[1],
		    sp->p.x[2] + size*sfan[k].x[2] );
	    }
	}
	glEnd();
    }
  }
  free(obase);
  if(texturing > 0)
    txbind( NULL, NULL );
}
  
  

void drawspecks( struct stuff *st )
{
  int i, slno, k;
  int rgba, alpha, prevrgba = 0;
  float prevsize = 0;
  struct specklist *sl, *slhead;
  register struct speck *p;
  Matrix Tw2c, Tc2w, Ttext, Tproj, Ttemp;
  static Point zero = {0,0,0};
  GLint xywh[4];
  float radperpix;
  Point tp, fan[MAXXYFAN];
  Point eyepoint;
  Point fwd;
  float fwdd;
  float tscale, scl, fanscale;
  int skip;
  static int nxyfan = 0;
  static float xyfan[MAXXYFAN][2];
  static unsigned char randskip[256];
  int fast = st->fast;
  int inpick = st->inpick;
  /*int usethresh = st->usethresh&P_USETHRESH ? THRESHBIT : 0;*/
#if USE_PTRACK
  int useptrack = (getenv("PTRACK") != NULL);
#endif
  SelOp seesel = st->seesel;
  float polyminrad, polymaxrad;
  int useclip = (st->clipbox.level != 0);
  Point clipp0 = st->clipbox.p0;
  Point clipp1 = st->clipbox.p1;

  float plum = st->psize;

  float knee1dist2 = st->fadeknee1 * st->fadeknee1;
  float knee2dist2 = st->fadeknee2 * st->fadeknee2;
  float orthodist2 = st->fadeknee2 * st->fadeknee2;
  float steep2knee2 = st->knee2steep * st->knee2steep / knee2dist2;
  float faderball2 = 1 / (st->fadeknee2 * st->fadeknee2);
  Point fadecen = st->fadecen;
  enum FadeType fademodel = st->fade;

  /* chromadepth */
  int lastchroma = st->nchromacm - 1;
  float chromaslidestart = st->chromaslidestart;
  float chromadistscale = (lastchroma > 0 && st->chromaslidelength > 0) ? st->nchromacm / st->chromaslidelength : 0;
  int use_chromadepth = st->use_chromadepth && (chromadistscale != 0);
  struct cment *chromacm = st->chromacm;

  glcheck("pre-drawspecks");

  if(!st->useme)
    return;

  if(oldopengl < 0) init_opengl();

  switch(fademodel) {
  case F_CONSTANT:
	if(orthodist2 <= 0)
	    orthodist2 = 1;
	break;
  case F_KNEE12:
	if(st->fadeknee1 >= st->fadeknee2 || st->fadeknee1 <= 0)
	    fademodel = F_KNEE2;	/* and fall into... */
  case F_KNEE2:
	if(st->fadeknee2 <= 0)
	    fademodel = F_SPHERICAL;
	break;
  case F_LREGION:
	if(st->fadeknee2 <= 0) faderball2 = 1;
	break;
  }


  { float r=0,g=0,b=0;	/* Ugh. Allow background to be non-black */
    sscanf(parti_bgcolor(NULL), "%f%f%f", &r,&g,&b);
    additive_blend = (r+g+b == 0);
  }

  if(st->clipbox.level > 0) {
    GLdouble plane[4];

    if(st->clipbox.level == 1) {
	struct AMRbox b[2];
	b[0] = st->clipbox;
	b[0].level = 0;
	b[1].level = -1;
	specks_draw_boxes( st, b, ~0, Tidentity, 1 );
    }
    for(i = 0; i < 3; i++) {
	plane[0] = plane[1] = plane[2] = 0;
	plane[i] = 1;
	plane[3] = -st->clipbox.p0.x[i];
	glClipPlane( GL_CLIP_PLANE0 + i, plane );
	glEnable( GL_CLIP_PLANE0 + i );
	plane[i] = -1;
	plane[3] = st->clipbox.p1.x[i];
	glClipPlane( GL_CLIP_PLANE0 + 3 + i, plane );
	glEnable( GL_CLIP_PLANE0 + 3 + i );
    }
  }


  if(nxyfan != st->npolygon && st->npolygon > 0) {
    if(st->npolygon > MAXXYFAN) st->npolygon = MAXXYFAN;
    nxyfan = st->npolygon;
    fanscale = 1 / cos(M_PI/nxyfan);
    for(i = 0; i < nxyfan; i++) {
	float th = (i+.5f)*2*M_PI / nxyfan;
	xyfan[i][0] = cos(th) * fanscale;
	xyfan[i][1] = sin(th) * fanscale;
    }
    srandom(11);
    for(i = 0; i < 256; i++)
	randskip[i] = random() & 0xFF;
  }


  alpha = st->alpha * 255;
  rgba = RGBALPHA( RGBWHITE, alpha );	/* BIG-ENDIAN (1,1,1,alpha) */

  /* Find displacements which lie in the screen plane, for making
   * billboard-style polygonal patches, and for text.
   */
  glGetFloatv( GL_MODELVIEW_MATRIX, Tw2c.m );
  eucinv( &Tc2w, &Tw2c );
  scl = vlength( (Point *)&Tw2c.m[0] );
  for(i = 0; i < nxyfan; i++) {
    tp.x[0] = scl*st->polysize*xyfan[i][0];
    tp.x[1] = scl*st->polysize*xyfan[i][1];
    tp.x[2] = 0;
    vtfmvector( &fan[i], &tp, &Tc2w );
  }

  /* Find projection matrix and screen (well, viewport) size,
   * so we can convert angular sizes to screen (pixel) sizes,
   * in radians per pixel.
   */
  glGetFloatv( GL_PROJECTION_MATRIX, Tproj.m );
  glGetIntegerv( GL_VIEWPORT, xywh );
  radperpix = 1 / (.5*xywh[2] * Tproj.m[0*4+0]);

  /* Construct a "forward" vector in object coords too, for measuring
   * distance from camera plane.  Note camera looks toward its -Z axis not +Z!
   */
  tp.x[0] = 0, tp.x[1] = 0, tp.x[2] = -1;
  vtfmvector( &fwd, &tp, &Tc2w );
  vunit( &fwd, &fwd );
  /*
   * Actually we want a plane equation, whose value is zero in the
   * eye plane.  Camera-space distance from camera plane = 
   *		vdot( &objectpoint, &fwd ) + fwdd.
   */
  vtfmpoint( &eyepoint, &zero, &Tc2w );
  fwdd = -vdot( &eyepoint, &fwd );

  {
    Matrix Tscreen2obj, Tscreen2global, Tobj2global, Tglobal2obj;
    float tscl;
#ifdef THIEBAUX_VIRDIR
    VD_get_cam2world_matrix( Tscreen2global.m );
    parti_geto2w( st, parti_idof(st), &Tobj2global );
    eucinv( &Tglobal2obj, &Tobj2global );
    mmmul( &Tscreen2obj, &Tscreen2global,&Tglobal2obj );
#else
    Tscreen2obj = Tc2w;
#endif
    tscl = 1 / vlength( (Point *)&Tscreen2obj.m[0] );
    tscale = tscl * st->textsize;
    mcopy( &Ttemp, &Tidentity );
    Ttemp.m[0*4+0] = Ttemp.m[1*4+1] = Ttemp.m[2*4+2] = tscale;
    mmmul( &Ttext, &Ttemp, &Tscreen2obj );
    vsettranslation( &Ttext, &zero );
  }

  glDisable( GL_LIGHTING );

  /* Draw any boxes (even if we have no specks) for this timestep */
  if(st->useboxes && st->boxlevelmask != 0
	&& st->frame_time >= 0 && st->frame_time < st->boxtimes
	&& st->boxes[st->frame_time] != NULL) {
    specks_draw_boxes( st, st->boxes[st->frame_time], st->boxlevelmask, Ttext, 0 );
  }
  if(st->useboxes && st->staticboxes != NULL) {
    specks_draw_boxes( st, st->staticboxes, st->boxlevelmask, Ttext, st->boxaxes );
  }

  slhead = st->frame_sl;	/* st->sl as snapped by specks_ffn() */
  if(slhead == NULL)
    slhead = st->sl;		/* maybe there is no specks_ffn() */

  if(st->dyn.enabled && st->dyn.draw)
    (*st->dyn.draw)( &st->dyn, st, slhead, &Tc2w, radperpix );

#ifdef USE_CONSTE
  if (st->conste)
    conste_draw( st );
#endif

  skip = st->subsample;
  if(slhead && slhead->subsampled != 0)	/* if already subsampled */
    skip /= slhead->subsampled;
  if(skip == 0) skip = 1;

  for(sl = slhead; sl != NULL; sl = sl->next)
    sl->used = st->used;

  if((unsigned int)st->sizedby <= MAXVAL
			&& (unsigned int)st->curdata < MAXFILES
			&& st->vdesc[st->curdata][st->sizedby].lum != 0) {
	plum *= st->vdesc[st->curdata][st->sizedby].lum;
  }
  if(st->subsample > 0 && st->everycomp)
      plum *= st->subsample;	/* Compensate for "every" subsampling */


  if(st->alpha >= 1) {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask( GL_TRUE );
  } else {
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, additive_blend ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA );
    glEnable(GL_DEPTH_TEST);
    glDepthMask( GL_FALSE );
  }

  if(inpick) glLoadName(0);
  
  if(st->usevec != VEC_OFF && st->vecscale != 0) {
      int vecvar0 = st->vecvar0;
      float vscl = st->vecscale;
      float varrow = st->vecarrowscale * vscl;
      int doarrow = (st->usevec == VEC_ARROW && varrow != 0); 
      int ialpha = (st->vecalpha>1 ? 0xFF : st->vecalpha<0 ? 0 : (int)(255*st->vecalpha));
      int alphabits = RGBALPHA(0, ialpha);

      glBegin( GL_LINES );

      for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
	if(sl->bytesperspeck < SMALLSPECKSIZE(st->vecvar0+3))
	    continue;

	if(inpick) {
	    glLoadName(slno);
	    glPushName(0);
	}

	for(i = 0, p = sl->specks; i < sl->nspecks; i+=skip, p = NextSpeck( p, sl, skip )) {
	    float dist, size;
	    int rgba;

	    if(!SELECTED(sl->sel[i], &seesel))
		continue;

	    if(inpick)
		glLoadName( i );

	    rgba = (p->rgba & RGBWHITE) | alphabits;
	    glColor4ubv( (GLubyte *)&rgba );

	    if(doarrow) {
		Point head, heye, uvec, vvec;
		Point hleft, hright;

		float larrow = varrow * vlength( (Point*)&p->val[vecvar0] );

		vsadd( &head, &p->p,  vscl,(Point*)&p->val[vecvar0] );
		vsub( &heye, &head, &eyepoint );
		vcross( &vvec, &heye, (Point*)&p->val[vecvar0] );
		vunit( &vvec, &vvec );
		vcross( &uvec, &heye, &vvec );
		vunit( &uvec, &uvec );

		glVertex3fv( &p->p.x[0] );
		glVertex3fv( &head.x[0] );

		vsadd( &hleft, &head,  larrow, &uvec );
		vsadd( &hleft, &hleft, -0.6f*larrow, &vvec );

		vsadd( &hright, &head,   larrow, &uvec );
		vsadd( &hright, &hright, 0.6f*larrow, &vvec );

		glVertex3fv( &hleft.x[0] );
		glVertex3fv( &head.x[0] );

		glVertex3fv( &hright.x[0] );
		glVertex3fv( &head.x[0] );

	    } else {
		glVertex3fv( &p->p.x[0] );
		glVertex3f( p->p.x[0] + vscl*p->val[vecvar0], p->p.x[1] + vscl*p->val[vecvar0+1], p->p.x[2] + vscl*p->val[vecvar0+2] );
	    }
	    
	}
	if(inpick)
	    glPopName();
      }
      glEnd();
  }
	

  if(st->usepoly && st->polysize > 0) {
    int texturing = 0;
    int texturevar = st->usetextures && st->texturevar >= 0
			&& st->texturevar < MAXVAL
		   ? st->texturevar : -1;
    int usearea = st->polyarea;
    int sizevar = st->polysizevar;
    float polysize = st->polysize;
    float mins2d;

    int txno;

    static int txdebug = -1;

    if(txdebug == -1) {
	if(getenv("TXDEBUG"))
	    txdebug = atoi(getenv("TXDEBUG"));
	else
	    txdebug = 0;
    }

   if(!(txdebug&1)) {
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    glTranslatef( .5, .5, 0 );
    glScalef( st->txscale, st->txscale, st->txscale );
   }
    glMatrixMode( GL_MODELVIEW );

    /* Textures seem to be multiplied by some mysterious factor,
     * unless we're in GL_SMOOTH mode.  What gives?
     */
   if(!(txdebug&2))
    glShadeModel( texturevar >= 0 ? GL_SMOOTH : GL_FLAT );


    polyminrad = st->polymin * radperpix;
    polymaxrad = st->polymax * radperpix;
    mins2d = polyminrad*polyminrad;

    if(sizevar == -1) {
	/* If polygon size is tied to point size,
	 * then include pointsize scale factors in polygon scaling.
	 */
      if((unsigned int)st->sizedby <= MAXVAL
		&& (unsigned int)st->curdata < MAXFILES
		&& st->vdesc[st->curdata][st->sizedby].lum != 0)
	polysize *= st->vdesc[st->curdata][st->sizedby].lum;
      if(st->subsample > 0 && st->everycomp)
        polysize *= st->subsample; /* Compensate for "every" subsampling */
    }

    if(st->depthsort && !inpick) {
	depth_fwd = fwd;
	depth_d = fwdd;
	sortedpolys( st, slhead, &Tc2w, radperpix, polysize );

    } else {
      int usepolymax = (st->polymax < 1e8);

#ifdef POLYFADE
      int polyfade = 0;
      if(st->polyfademax > st->polymin) {
	polyfade = 1;
	polyfadesize = 
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, additive_blend ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA );
      }
#endif

      for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
	if(sl->text != NULL || sl->special != SPECKS) continue;
	if(inpick) {
	    glLoadName(slno);
	    glPushName(0);
	}
	for(i = 0, p = sl->specks; i < sl->nspecks; i+=skip, p = NextSpeck( p, sl, skip )) {
	    float dist = VDOT( &p->p, &fwd ) + fwdd;
	    float size;

	    if(!SELECTED(sl->sel[i], &seesel))
		continue;

	    size = p->val[sizevar] * polysize;

	    if(dist + size <= 0) continue;
	    if(usearea) {
		if(size < dist * dist * mins2d)
		    continue;
		size = sqrtf(size);
	    } else {
		if(size < dist * polyminrad)
		    continue;
	    } 
	    if(usepolymax && size > dist * polymaxrad && dist > 0)
		size = dist * polymaxrad;

	    if (use_chromadepth) {
		int cindex;
		cindex = (dist - chromaslidestart) * chromadistscale;
		if (cindex < 0)
		    cindex = 0;
		else if (cindex > lastchroma)
		    cindex = lastchroma;
		rgba = chromacm[cindex].cooked & RGBBITS;
	    } else
		rgba = p->rgba & RGBBITS;

	    if(rgba != prevrgba) {
		prevrgba = rgba;
		rgba = RGBALPHA( prevrgba, alpha );
		glColor4ubv( (GLubyte *)&rgba );
	    }
	    if(st->polyorivar0 >= 0 && p->val[st->polyorivar0] < 9) {
		for(k = 0; k < nxyfan; k++) {
		    vcomb( &fan[k],
			size*xyfan[k][0], (Point *)&p->val[st->polyorivar0],
			size*xyfan[k][1], (Point *)&p->val[st->polyorivar0+3] );
		}
		prevsize = 0;
	    } else if(p->size != prevsize) {
		float s = scl*size;
		for(k = 0; k < nxyfan; k++) {
		    vcomb( &fan[k], s*xyfan[k][0], (Point *)&Tc2w.m[0*4+0],
				    s*xyfan[k][1], (Point *)&Tc2w.m[1*4+0] );
		}
		prevsize = size;
	    }

#define PFAN(vno, comp)  p->p.x[comp] + fan[vno].x[comp]

	    if(inpick) {
		glLoadName( i );
		glBegin( GL_TRIANGLE_FAN );
		for(k = 0; k < nxyfan; k++) {
		    glVertex3f( PFAN(k,0), PFAN(k,1), PFAN(k,2) );
		}
		glEnd();

	    } else if(texturevar >= 0
		    && (txno = p->val[texturevar]) >= 0
		    && txno < st->ntextures &&
		    st->textures[txno] != NULL) {

		txbind( st->textures[txno], &texturing );

		glBegin( GL_TRIANGLE_FAN );
		for(k = 0; k < nxyfan; k++) {
		    glTexCoord2fv( &xyfan[k][0] );
		    glVertex3f( PFAN(k,0), PFAN(k,1), PFAN(k,2) );
		}
		glEnd();

	    } else {
		if(texturing) {
		    texturing = 0;
		    glDisable( GL_TEXTURE_2D );
		}
		glBegin(GL_TRIANGLE_FAN);
		for(k = 0; k < nxyfan; k++)
		    glVertex3f( PFAN(k,0), PFAN(k,1), PFAN(k,2) );
		glEnd();
	    }
#undef PFAN

	}
	if(inpick) glPopName();
      }
    }
    if(texturing) {
	txbind( NULL, NULL );
	glDisable( GL_TEXTURE_2D );
    }
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
  }

  if(st->usepoint && !(st->useboxes == 2)) {

#define MAXPTSIZE 32	/* in half-point units */
#define PERBUCKET 320	/* max points per bucket */

    struct cpoint sized[MAXPTSIZE*2][PERBUCKET];
    int nsized[MAXPTSIZE*2];
    unsigned char invgamma[256];
    float invgam = (st->gamma <= 0) ? 0 : 1/st->gamma;

    for(i = 0; i < 256; i++)
	invgamma[i] = (int) (255.99 * pow( i/255., invgam ));

    if(inpick) {
	for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
	    if(sl->text != NULL || sl->special != SPECKS) continue;
	    glLoadName(slno);
	    glPushName(0);
	    for(i = 0, p = sl->specks; i < sl->nspecks; i+=skip, p = NextSpeck(p, sl, skip)) {
		if(!SELECTED(sl->sel[i], &seesel))
		    continue;

		glLoadName(i);
		glBegin(GL_POINTS);
		glVertex3fv( &p->p.x[0] );
		glEnd();
	    }
	    glPopName();
	}

    } else if(fast) {
	static unsigned char apxsize[MAXPTSIZE*MAXPTSIZE];
	unsigned char faintrand[256];
	int pxsize, oldpxsize;
	int pxmin, pxmax;

	pxmin = 256 * st->pfaint;
	if(st->plarge > MAXPTSIZE) st->plarge = MAXPTSIZE;
	pxmax = 256 * st->plarge * st->plarge;

	if(apxsize[1] == 0) {
	    for(i=0; i<COUNT(apxsize); i++)
		apxsize[i] = (int)ceil(sqrtf(i+1));
	}
	for(i = 0; i < 256; i++)
	    faintrand[i] = randskip[i] * st->pfaint;

	/* Render using fast (non-antialiased) points */
	glDisable( GL_POINT_SMOOTH );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, additive_blend ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA );

	prevrgba = 0;
	prevsize = 0;

	sl = slhead;

	pxsize = oldpxsize = 1;
	glPointSize( pxsize );
	glColor4ubv( (GLubyte *)&rgba );

	for(i = 0; i < MAXPTSIZE*2; i++)
	    nsized[i] = 0;

	if(oldopengl) {
	    glBegin( GL_POINTS );
	} else {
	    glEnableClientState( GL_COLOR_ARRAY );
	    glEnableClientState( GL_VERTEX_ARRAY );
	}
	for(sl = slhead; sl != NULL; sl = sl->next) {
	    if(sl->text != NULL || sl->special != SPECKS) continue;
	    for(i = 0, p = sl->specks; i < sl->nspecks; i+=skip, p= NextSpeck(p, sl, skip)) {
		int lum, myalpha;
		float dist = VDOT( &p->p, &fwd ) + fwdd;
		if(dist <= 0)	/* Behind eye plane */
		    continue;

		if(!SELECTED(sl->sel[i], &seesel))
		    continue;

		if(useclip &&
		  (p->p.x[0] < clipp0.x[0] ||
		   p->p.x[0] > clipp1.x[0] ||
		   p->p.x[1] < clipp0.x[1] ||
		   p->p.x[1] > clipp1.x[1] ||
		   p->p.x[2] < clipp0.x[2] ||
		   p->p.x[2] > clipp1.x[2]))
		    continue;


		lum = 256 * plum * p->size / (dist*dist);

		if(lum < pxmin) {
		    if(lum <= faintrand[(i*i+i) /*randix++*/ & 0xFF])
			continue;
		    pxsize = 1;
		    myalpha = pxmin;
		} else if(lum < 256) {
		    pxsize = 1;
		    myalpha = lum;
		} else if(lum < pxmax) {
		    pxsize = apxsize[lum>>8];
		    myalpha = lum / (pxsize*pxsize);
		} else {
		    /* Could use a polygon here, instead. */
		    pxsize = st->plarge;
		    myalpha = 255;
		}
#ifdef USE_PTRACK
		if(useptrack) printf("pfast %d %d %d %d\n", lum, pxsize, myalpha, invgamma[myalpha] & 0xFC);
#endif

#ifdef DEBUG
		if(pxsize < 1 || pxsize > 6 || myalpha <= 0 || myalpha > 255) {
		    static int oops;
		    oops++;
		}
		if((unsigned int)myalpha > 255 || (unsigned int)invgamma[myalpha]  > 255) {
		    static int oops2;
		    oops2++;
		}
#endif

		if (use_chromadepth) {
		  int cindex;
		  cindex = (dist - chromaslidestart) * chromadistscale;
		  if (cindex < 0)
			cindex = 0;
		  else if (cindex > lastchroma)
			cindex = lastchroma;
		  rgba = RGBALPHA(chromacm[cindex].cooked, invgamma[myalpha] & 0xFC);
		}
		else
		  rgba = RGBALPHA( p->rgba&RGBBITS, invgamma[myalpha] & 0xFC );

		if(oldopengl) {
		    /* we're in a glBegin(GL_POINTS) */
		    if(pxsize != oldpxsize) {
			if(nsized[pxsize] >= PERBUCKET) {
			    glEnd();
			    glPointSize(pxsize);
			    dumpcpoints( &sized[pxsize][0], PERBUCKET );
			    nsized[pxsize] = 0;
			    oldpxsize = pxsize;
			    glBegin( GL_POINTS );
			    glColor4ubv( (GLubyte *)&rgba );
			    prevrgba = rgba;
			    glVertex3fv( &p->p.x[0] );

			} else {
			    struct cpoint *cp = &sized[pxsize][nsized[pxsize]];
			    cp->rgba = rgba;
			    cp->p = p->p;
			    nsized[pxsize]++;
			}
		    } else {
			/* same as current pointsize */
			if(rgba != prevrgba) {
			    glColor4ubv( (GLubyte *)&rgba );
			    prevrgba = rgba;
			}

			glVertex3fv( &p->p.x[0] );
		    }
		} else {
		    /* !oldopengl => OpenGL >=1.1 */
		    struct cpoint *cp;
		    if(nsized[pxsize] >= PERBUCKET) {
			glPointSize( pxsize );
			dumpcpointsarray( &sized[pxsize][0], PERBUCKET );
			cp = &sized[pxsize][0];
			nsized[pxsize] = 1;
		    } else {
			cp = &sized[pxsize][nsized[pxsize]];
			nsized[pxsize]++;
		    }
		    cp->rgba = rgba;
		    cp->p = p->p;
		}
	    }
	}
	if(oldopengl) {
	    glEnd();
	    for(i = 0; i < MAXPTSIZE; i++) {
		if(nsized[i] > 0) {
		    glPointSize( i );
		    dumpcpoints( &sized[i][0], nsized[i] );
		}
	    }
	} else {
	    for(i = 0; i < MAXPTSIZE; i++) {
		if(nsized[i] > 0) {
		    glPointSize( i );
		    dumpcpointsarray( &sized[i][0], nsized[i] );
		}
	    }
	    glDisableClientState( GL_COLOR_ARRAY );
	    glDisableClientState( GL_VERTEX_ARRAY );
	}

    } else {

	/* 
	 * Render using anti-aliased points
	 */
	static unsigned char apxsize[(MAXPTSIZE*2)*(MAXPTSIZE*2)];
	unsigned char faintrand[256];
	static float percoverage[MAXPTSIZE*2];
	static int needMesaHack = 0;
	int pxsize, oldpxsize;
	int pxmin, pxmax;
	int minsize, minalpha;

	pxmin = (256 * (2*2)) * st->pfaint;
	if(st->plarge > MAXPTSIZE*2) st->plarge = MAXPTSIZE*2;
	pxmax = (256 * (2*2)) * st->plarge * st->plarge;

	if(apxsize[1] == 0) {
	    char *version = (char *)glGetString( GL_VERSION );

	    /* Hack: MESA doesn't handle small anti-aliased points well;
	     * force them all to be of size 1.0 or 2.0 (i.e. pxsize == 2 or 4),
	     * and non-antialiased.
	     */
	    if(getenv("MESAHACK") != NULL)
		needMesaHack = atoi(getenv("MESAHACK"));
	    else if(version &&
		  (!strncmp(version, "CRIME", 5) || !strncmp(version, "IR", 2)))
		needMesaHack = 0;	/* O2 & IR: nice anti-aliased points */
	    else
		needMesaHack = -1;	/* IMPACT (& others?): ugly AA points */

	    for(i=0; i<COUNT(apxsize); i++) {
		apxsize[i] = (int)ceil(sqrtf(i+1));
		if(needMesaHack>0) {
		    if(apxsize[i] <= 2) apxsize[i] = 2;
		    else if(needMesaHack<2 && apxsize[i] <= 4) apxsize[i] = 4;
		}
	    }

	    for(i = 1; i < MAXPTSIZE*2; i++) {
		percoverage[i] = 1.0 / (i*i);
		if(needMesaHack>0) {
		    if(i <= 2) percoverage[i] = .25;
		    else if(needMesaHack<2 && i <= 4) percoverage[i] = .0625;
		}
	    }
	    percoverage[0] = 1.0;

	    if(needMesaHack < 0)
		percoverage[1] = 0.25;	/* if gfx treats psize 0.5 == 1.0 */

	}

	minsize = apxsize[ pxmin >> 8 ];
	minalpha = pxmin * percoverage[ minsize ];

	for(i = 0; i < 256; i++)
	    faintrand[i] = randskip[i] * st->pfaint;

	/* Render using antialiased points */
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, additive_blend ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA );

	prevrgba = 0;
	prevsize = 0;

	sl = slhead;

	pxsize = 2;
	oldpxsize = 99;
	glPointSize( pxsize );
	glColor4ubv( (GLubyte *)&rgba );

	for(i = 0; i < MAXPTSIZE*2; i++)
	    nsized[i] = 0;

	if(oldopengl) {
	    glBegin( GL_POINTS );
	} else {
	    glEnableClientState( GL_COLOR_ARRAY );
	    glEnableClientState( GL_VERTEX_ARRAY );
	}

	for(sl = slhead; sl != NULL; sl = sl->next) {
	    if(sl->text != NULL || sl->special != SPECKS) continue;
	    for(i = 0, p = sl->specks; i < sl->nspecks; i+=skip, p=NextSpeck(p,sl,skip)) {
		int lum, myalpha;
		float dist, dist2, dx, dy, dz;

		if(!SELECTED(sl->sel[i], &seesel))
		    continue;
		if(useclip &&
		  (p->p.x[0] < clipp0.x[0] ||
		   p->p.x[0] > clipp1.x[0] ||
		   p->p.x[1] < clipp0.x[1] ||
		   p->p.x[1] > clipp1.x[1] ||
		   p->p.x[2] < clipp0.x[2] ||
		   p->p.x[2] > clipp1.x[2]))
		    continue;


		switch(fademodel) {
		case F_PLANAR:
		    dist = VDOT( &p->p, &fwd ) + fwdd;
		    if(dist <= 0)	/* Behind eye plane */
			continue;
		    dist2 = dist*dist;
		    break;
		case F_CONSTANT:
		    dist2 = orthodist2;
		    break;
		case F_SPHERICAL:
		    dx = p->p.x[0]-eyepoint.x[0];
		    dy = p->p.x[1]-eyepoint.x[1];
		    dz = p->p.x[2]-eyepoint.x[2];
		    dist2 = dx*dx + dy*dy + dz*dz;
		    break;

		case F_LREGION:	/* not impl yet */
		    dist = VDOT( &p->p, &fwd ) + fwdd;
		    if(dist <= 0)	/* Behind eye plane */
			continue;
		    dx = p->p.x[0] - fadecen.x[0];
		    dy = p->p.x[1] - fadecen.x[1];
		    dz = p->p.x[2] - fadecen.x[2];
		    dist2 = dist * st->fadeknee2
				/ (1 + (dx*dx + dy*dy + dz*dz) * faderball2);
		    break;
		case F_LINEAR:
		    dist = VDOT( &p->p, &fwd ) + fwdd;
		    if(dist <= 0)	/* Behind eye plane */
			continue;
		    dist2 = dist * st->fadeknee2;
		    break;
			
		case F_KNEE2:
		    dx = p->p.x[0]-eyepoint.x[0];
		    dy = p->p.x[1]-eyepoint.x[1];
		    dz = p->p.x[2]-eyepoint.x[2];
		    dist2 = dx*dx + dy*dy + dz*dz;
		    if(dist2 > knee2dist2)
			dist2 *= 1 + steep2knee2 * (dist2 - knee2dist2);
		    break;
		case F_KNEE12:
		    dx = p->p.x[0]-eyepoint.x[0];
		    dy = p->p.x[1]-eyepoint.x[1];
		    dz = p->p.x[2]-eyepoint.x[2];
		    dist2 = dx*dx + dy*dy + dz*dz;
		    if(dist2 < knee1dist2)
			dist2 = knee1dist2;
		    else if(dist2 > knee2dist2)
			dist2 *= 1 + steep2knee2 * (dist2 - knee2dist2);
		    break;
		}

		lum = (256 * (2*2)) * plum * p->size / dist2;

		if(lum <= pxmin) {
		    if(lum <= faintrand[(i*i+i) /*randix++*/ & 0xFF])
			continue;
		    pxsize = minsize;
		    myalpha = minalpha;
		} else if(lum < pxmax) {
		    pxsize = apxsize[lum>>8];
		    myalpha = lum * percoverage[pxsize];
		} else {
		    /* Could use a polygon here, instead. */
		    pxsize = 2 * st->plarge;
		    myalpha = 255;
		}
#ifdef USE_PTRACK
		if(useptrack) printf("paa %d %d/2 %d %d\n", lum, pxsize, myalpha, invgamma[myalpha] & 0xFC);
#endif

		if(pxsize < 1 || pxsize >= MAXPTSIZE*2 || myalpha <= 0 || myalpha > 255) {
		    static int oops;
		    oops++;
		    pxsize = MAXPTSIZE*2 - 1;
		    myalpha = 255;
		}

		if (use_chromadepth) {
		  int cindex;
		  dist = VDOT( &p->p, &fwd ) + fwdd;
		  if (dist <= 0)
		    continue;
		  cindex = (dist - chromaslidestart) * chromadistscale;
		  if (cindex < 0)
			cindex = 0;
		  else if (cindex > lastchroma)
		    cindex = lastchroma;
		  rgba = RGBALPHA(chromacm[cindex].cooked, invgamma[myalpha] & 0xFC);
		}
		else
		  rgba = RGBALPHA( p->rgba&RGBBITS, invgamma[myalpha] & 0xFC );

		if(oldopengl) {
		    if(pxsize != oldpxsize) {
			if(nsized[pxsize] >= PERBUCKET) {
			    if(needMesaHack>0 && (pxsize <= 4) != (oldpxsize <= 4)) {
				if(pxsize <= 4) glDisable( GL_POINT_SMOOTH );
				else glEnable( GL_POINT_SMOOTH );
			    }
			    glEnd();
			    glPointSize(0.5f*pxsize);
			    dumpcpoints( &sized[pxsize][0], PERBUCKET );
			    nsized[pxsize] = 0;
			    oldpxsize = pxsize;
			    glBegin( GL_POINTS );
			    glColor4ubv( (GLubyte *)&rgba );
			    prevrgba = rgba;
			    glVertex3fv( &p->p.x[0] );

			} else {
			    struct cpoint *cp = &sized[pxsize][nsized[pxsize]];
			    cp->rgba = rgba;
			    cp->p = p->p;
			    nsized[pxsize]++;
			}
		    } else {
			/* same as current pointsize */
			if(rgba != prevrgba) {
			    glColor4ubv( (GLubyte *)&rgba );
			    prevrgba = rgba;
			}

			glVertex3fv( &p->p.x[0] );
		    }
		} else {
		    /* !oldopengl => OpenGL >=1.1 */
		    struct cpoint *cp;
		    if(nsized[pxsize] >= PERBUCKET) {
			glPointSize( 0.5f*pxsize );
			dumpcpointsarray( &sized[pxsize][0], PERBUCKET );
			cp = &sized[pxsize][0];
			nsized[pxsize] = 1;
		    } else {
			cp = &sized[pxsize][nsized[pxsize]];
			nsized[pxsize]++;
		    }
		    cp->rgba = rgba;
		    cp->p = p->p;
		}

	    }
	}
	if(oldopengl)
	    glEnd();

	if(needMesaHack>0)
	    glDisable( GL_POINT_SMOOTH );

	for(i = 0; i < MAXPTSIZE*2; i++) {
	    if(needMesaHack>0 && i == 4)
		glEnable( GL_POINT_SMOOTH );
	    if(nsized[i] > 0) {
		glPointSize( 0.5f*i );
		if(oldopengl) {
		    dumpcpoints( &sized[i][0], nsized[i] );
		} else {
		    dumpcpointsarray( &sized[i][0], nsized[i] );
		}
	    }
	}
	if(!oldopengl) {
	    glDisableClientState( GL_COLOR_ARRAY );
	    glDisableClientState( GL_VERTEX_ARRAY );
	}

    }
  }

  if(st->usetext && st->textsize != 0) {
    int cment = -1;
    int textmin = abs(st->textmin);
    int label_stubs = (st->textmin < 0);
    glColor3f(1,1,1);

    for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
	if(sl->text != NULL && (p = sl->specks) != NULL) {
	    float dist = VDOT( &p->p, &fwd ) + fwdd;
	    float tsize;

	    if(dist <= 0)
		continue;

	    if(inpick) glLoadName(slno);

	    tsize = st->textsize * p->size;
	    if(tsize < dist * (radperpix * textmin)) {
		if(label_stubs) {
		    /* Draw a stub: a simple line segment of
		     * about the right length
		     */
		    Point ep;
		    /* extract top row of Ttext -- this is the unit
		     * screen-space X vector, expressed in world coords.
		     * Scale to suit.
		     */
		    vcomb( &ep,
			p->size * sfStrWidth(sl->text), (Point *)&Ttext.m[0],
			1, &p->p );

		    if(p->rgba != cment) {
			if((unsigned int)p->rgba >= st->textncmap)
			    glColor3f( 1, 1, 1 );
			else
			    glColor3ubv( (GLubyte *)&st->textcmap[ p->rgba ].cooked );
			cment = p->rgba;
		    }
		    glBegin( GL_LINES );
		    glVertex3fv( p->p.x );
		    glVertex3fv( ep.x );
		    glEnd();
		}
		continue;
	    }

	    /* Otherwise, label is big enough to see -- draw actual text */

	    if(st->usetextaxes) {
		static unsigned char col[3][3] = {255,0,0, 0,255,0, 0,0,255};
		glBegin(GL_LINES);
		for(k = 0; k < 3; k++) {
		    glColor3ubv( &col[k][0] );
		    glVertex3fv( p->p.x );
		    glVertex3f( p->p.x[0] + (k==0?tsize:0),
				p->p.x[1] + (k==1?tsize:0), 
				p->p.x[2] + (k==2?tsize:0) );
		}
		glEnd();
	    }
	    if(p->rgba != cment || st->usetextaxes) {
		if((unsigned int)p->rgba >= st->textncmap)
		    glColor3f( 1, 1, 1 );
		else
		    glColor3ubv( (GLubyte *)&st->textcmap[ p->rgba ].cooked );
		cment = p->rgba;
	    }
	    sfStrDrawTJ( sl->text, p->size, &p->p, &Ttext, NULL );
	}
    }
  }

  if(inpick) glLoadName(0);

  if(st->usemeshes) {
    struct mesh *m;
    int texturing = 0;

    if(additive_blend && st->alpha < 1) {
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    } else {
	glDisable( GL_BLEND );
    }
    glColor4f(1, 1, 1, st->alpha);
    for(m = st->staticmeshes; m != NULL; m = m->next)
	specks_draw_mesh(st, m, &texturing);
    for(m = CURDATATIME(meshes); m != NULL; m = m->next)
	specks_draw_mesh(st, m, &texturing);


    if(texturing) txbind(NULL, NULL);
  }

  if(st->useellipsoids) {
    struct ellipsoid *e;

    if(additive_blend) {
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    } else {
	glDisable( GL_BLEND );
    }
    glColor4f(1, 1, 1, st->alpha);
    for(e = st->staticellipsoids; e != NULL; e = e->next)
	specks_draw_ellipsoid(st, e);
  }



  for(i = 0; i < 6; i++)	/* in case clipbox was enabled when we began */
    glDisable( GL_CLIP_PLANE0 + i );


  glColor4ub(255,255,255,255);
  glDisable( GL_BLEND );
  glEnable( GL_DEPTH_TEST );	/* was already enabled */
  glDepthMask( GL_TRUE );
  glcheck("post-drawspecks");
}

void specks_draw_ellipsoid( struct stuff *st, struct ellipsoid *e )
{
  int nu = e->nu;
  int nv = e->nv;
  int u, v;
  float *su, *cu, *sv, *cv;

  if(st->boxlevelmask == 0 || e->level >= 0 && ((1 << e->level) & st->boxlevelmask) == 0)
    return;

  if(nu == 0 && nv == 0) {
    nu = 15;
    nv = 10;
  }
  su = NewA( float, nu );
  cu = NewA( float, nu );
  sv = NewA( float, nv );
  cv = NewA( float, nv );
  for(u = 0; u < nu; u++) {
    float tu = 2*M_PI*u/nu;
    su[u] = sin(tu);
    cu[u] = cos(tu);
  }
  for(v = 0; v < nv; v++) {
    float tv = M_PI*v/(nv-1);
    sv[v] = sin(tv);
    cv[v] = cos(tv);
  }
  glPushMatrix();
  glTranslatef( e->pos.x[0],e->pos.x[1],e->pos.x[2] );
  if(e->hasori)
    glMultMatrixf( &e->ori.m[0] );
  glScalef( e->size.x[0], e->size.x[1], e->size.x[2] );

  if(e->cindex >= 0) {
    int rgba =
	RGBALPHA(
	   RGBWHITE & st->cmap[(e->cindex < st->ncmap) ? e->cindex : st->ncmap-1].cooked,
	   (int)(st->alpha * 255));
    glColor4ubv((GLubyte *)&rgba);
  }

  glLineWidth( e->linewidth > 0 ? e->linewidth : 1.0f );

  switch(e->style) {
  case S_SOLID:
    for(v = 1; v < nv; v++) {
	glBegin( GL_QUAD_STRIP );
	for(u = 0; u < nu; u++) {
	    glVertex3f( cu[u]*sv[v-1], su[u]*sv[v-1], cv[v-1] );
	    glVertex3f( cu[u]*sv[v], su[u]*sv[v], cv[v] );
	}
	glVertex3f( cu[0]*sv[v-1], su[0]*sv[v-1], cv[v-1] );
	glVertex3f( cu[0]*sv[v], su[0]*sv[v], cv[v] );
	glEnd();
    }
    break;

  case S_LINE:
    for(v = 1; v < nv-1; v++) {
	glBegin( GL_LINE_LOOP );
	for(u = 0; u < nu; u++)
	    glVertex3f( cu[u]*sv[v], su[u]*sv[v], cv[v] );
	glEnd();
    }
    for(u = 0; u < nu; u++) {
	glBegin( GL_LINE_STRIP );
	for(v = 0; v < nv; v++)
	    glVertex3f( cu[u]*sv[v], su[u]*sv[v], cv[v] );
	glEnd();
    }
    break;
    
  case S_PLANE:
    glBegin( GL_LINE_LOOP );
    for(u = 0; u < nu; u++)
	glVertex3f( cu[u], su[u], 0 );
    glEnd();
    glBegin( GL_LINE_LOOP );
    for(u = 0; u < nu; u++)
	glVertex3f( cu[u], 0, su[u] );
    glEnd();
    glBegin( GL_LINE_LOOP );
    for(u = 0; u < nu; u++)
	glVertex3f( 0, cu[u], su[u] );
    glEnd();
    break;

  case S_POINT:
    glPointSize( 1.0 );
    glBegin( GL_POINTS );
    glVertex3f( 0,0,1 );
    glVertex3f( 0,0,-1 );
    for(v = 1; v < nv-1; v++) {
	for(u = 0; u < nu; u++)
	    glVertex3f( cu[u]*sv[v], su[u]*sv[v], cv[v] );
    }
    glEnd();
    break;
  }
  glPopMatrix();
}

void specks_draw_mesh( struct stuff *st, register struct mesh *m, int *texturing )
{
  int u, v, prev, cur;
  int usetx, usetx3d;
  float gap, ungap;
  int f, was;
  int unlit;
  int usemullions = (st->mullions > 0);

  usetx = (st->usetextures && m->tx != NULL
		&& m->txno >= 0 && m->txno < st->ntextures
		&& st->textures[m->txno] != NULL);
  usetx3d = usetx && (st->textures[m->txno]->flags & TXF_3D);
  glLineWidth( m->linewidth > 0 ? m->linewidth : 1.0f );
  txbind( usetx ? st->textures[m->txno] : NULL, texturing );
  if(m->fnorms != NULL && st->usemeshes != 2) {
    static float amb[4] = { 0,0,0, 1 };
    static float spec[4] = { .8,.8,.8, 1 };
    glEnable( GL_LIGHTING );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glEnable( GL_COLOR_MATERIAL );
    glShadeModel( GL_SMOOTH );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 20. );
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, amb );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, spec );
    unlit = 0;

  } else {
    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
    unlit = 1;
  }

  if(m->cindex >= 0) {
    int rgba =
	RGBALPHA(
	   RGBWHITE & st->cmap[(m->cindex < st->ncmap) ? m->cindex : st->ncmap-1].cooked,
	   (int)(st->alpha * 255));
    glColor4ubv((GLubyte *)&rgba);
  } else if(st->coloredby == CONSTVAL) {
    struct valdesc *vd = &st->vdesc[st->curdata][CONSTVAL];
    glColor4f( vd->cmin, vd->cmax, vd->mean, st->alpha );
  }

  switch(m->type) {
  case MODEL:
    if(m->objrender)
	(*m->objrender)( st, m );
    break;
  case QUADMESH:
    switch(m->style) {
	case S_SOLID:
	for(v = 1; v < m->nv; v++) {
	    prev = (v-1) * m->nu;
	    cur = v * m->nu;
	    glBegin( GL_QUAD_STRIP );
	    if(usetx) {
		if(usetx3d) {
		    for(u = 0; u < m->nu; u++) {
			glTexCoord3fv( &m->tx[prev+u].x[0] );
			glVertex3fv( &m->pts[prev+u].x[0] );
			glTexCoord3fv( &m->tx[cur+u].x[0] );
			glVertex3fv( &m->pts[cur+u].x[0] );
		    }
		} else {
		    for(u = 0; u < m->nu; u++) {
			glTexCoord2fv( &m->tx[prev+u].x[0] );
			glVertex3fv( &m->pts[prev+u].x[0] );
			glTexCoord2fv( &m->tx[cur+u].x[0] );
			glVertex3fv( &m->pts[cur+u].x[0] );
		    }
		}
	    } else {
		for(u = 0; u < m->nu; u++) {
		    glVertex3fv( &m->pts[prev+u].x[0] );
		    glVertex3fv( &m->pts[cur+u].x[0] );
		}
	    }
	    glEnd();
	}
	break;

	case S_LINE:
	    for(v = 0; v < m->nv; v++) {
		glBegin( GL_LINE_STRIP );
		cur = v * m->nu;
		for(u = 0; u < m->nu; u++)
		    glVertex3fv( &m->pts[cur+u].x[0] );
		glEnd();
	    }
	    for(u = 0; u < m->nu; u++) {
		glBegin( GL_LINE_STRIP );
		for(v = 0; v < m->nv; v++)
		    glVertex3fv( &m->pts[v*m->nu + u].x[0] );
		glEnd();
	    }
	    break;

	case S_POINT:
	    cur = m->nu * m->nv;
	    glBegin( GL_POINTS );
	    for(u = 0; u < cur; u++)
		glVertex3fv( &m->pts[u].x[0] );
	    glEnd();
	    break;

	case S_PLANE:
	    glBegin( GL_LINE_LOOP );
	    for(u = 0; u < m->nu; u++)
		glVertex3fv( &m->pts[u].x[0] );
	    for(v = 1; v < m->nv; v++)
		glVertex3fv( &m->pts[v*m->nu + m->nu-1].x[0] );
	    cur = (m->nv-1) * m->nu;
	    for(u = m->nu; --u >= 0; )
		glVertex3fv( &m->pts[cur+u].x[0] );
	    for(v = m->nv-1; --v > 0; )
		glVertex3fv( &m->pts[v*m->nu].x[0] );
	    glEnd();
	    break;
    }
    break;

  case POLYMESH:
    gap = st->mullions;
    ungap = 1 - gap;
   
    was = 0;
    for(f = 0; f < m->nfaces; f++) {
	int fv0 = m->fv0[f];
	int fvn = m->fvn[f];
	int *fvs0 = &m->fvs[ fv0 ];
	Point *pts = m->pts;
	Point *txs = m->tx;
	Point *vnorms = m->vnorms;

#define FVERT(i)   fvs0[(i)*FVSTEP + FVS_VERT]
#define FTX(i)     fvs0[(i)*FVSTEP + FVS_TX]
#define FVNORM(i)  fvs0[(i)*FVSTEP + FVS_VNORM]

	int usevn = unlit ? -1 : (FVNORM(0) >= 0) ? 1 : 0;
	int usetex = usetx && (FTX(0) >= 0);

	if(usevn == 0)
	    glNormal3fv( m->fnorms[f].x );

	if(fvn == 2) {
	    if(was != GL_LINES) {
		if(was) glEnd();
		glBegin( was=GL_LINES );
	    }
	    glVertex3fv( pts[ FVERT(0) ].x );
	    glVertex3fv( pts[ FVERT(1) ].x );

	} else if(fvn == 3) {
	    Point bary, *p0, *p1, *p2;
	    if(usemullions) {
		p0 = &pts[ FVERT(0) ];
		p1 = &pts[ FVERT(1) ];
		p2 = &pts[ FVERT(2) ];
		bary.x[0] = (p0->x[0] + p1->x[0] + p2->x[0]) * 0.3333333333f;
		bary.x[1] = (p0->x[1] + p1->x[1] + p2->x[1]) * 0.3333333333f;
		bary.x[2] = (p0->x[2] + p1->x[2] + p2->x[2]) * 0.3333333333f;

#define INTERP(p) p->x[0]*gap + bary.x[0]*ungap, p->x[1]*gap + bary.x[1]*ungap, p->x[2]*gap + bary.x[2]*ungap
#define TXINTERP(tx) tx->x[0]*gap + txbary[0]*ungap, tx->x[1]*gap + txbary[1]*ungap

		if(was) {
		    was = 0;
		    glEnd();
		}
		glBegin( GL_QUAD_STRIP );
		if(usetex) {
		    Point *tx0 = &txs[ FTX(0) ];
		    Point *tx1 = &txs[ FTX(1) ];
		    Point *tx2 = &txs[ FTX(2) ];
		    float txbary[2];

		    txbary[0] = (tx0->x[0] + tx1->x[0] + tx2->x[0]) * 0.33333333f;
		    txbary[1] = (tx0->x[1] + tx1->x[1] + tx2->x[1]) * 0.33333333f;

		    if(usevn>0) glNormal3fv( vnorms[ FVNORM(0) ].x );
		    glTexCoord2fv( tx0->x );
		    glVertex3fv( p0->x );
		    glTexCoord2f( TXINTERP( tx0 ) );
		    glVertex3f( INTERP( p0 ) );

		    if(usevn>0) glNormal3fv( vnorms[ FVNORM(1) ].x );
		    glTexCoord2fv( tx1->x );
		    glVertex3fv( p1->x );
		    glTexCoord2f( TXINTERP( tx1 ) );
		    glVertex3f( INTERP( p1 ) );

		    if(usevn>0) glNormal3fv( vnorms[ FVNORM(2) ].x );
		    glTexCoord2fv( tx2->x );
		    glVertex3fv( p2->x );
		    glTexCoord2f( TXINTERP( tx2 ) );
		    glVertex3f( INTERP( p2 ) );

		    if(usevn>0) glNormal3fv( vnorms[ FVNORM(0) ].x );
		    glTexCoord2fv( tx2->x );
		    glVertex3fv( p0->x );
		    glTexCoord2f( TXINTERP( tx0 ) );
		    glVertex3f( INTERP( p0 ) );

		} else if(usevn>0) {
		    glNormal3fv( vnorms[FVNORM(0)].x );
		    glVertex3fv(p0->x); glVertex3f( INTERP(p0) );

		    glNormal3fv( vnorms[FVNORM(1)].x );
		    glVertex3fv(p1->x); glVertex3f( INTERP(p1) );

		    glNormal3fv( vnorms[FVNORM(2)].x );
		    glVertex3fv(p2->x); glVertex3f( INTERP(p2) );

		    glNormal3fv( vnorms[FVNORM(0)].x );
		    glVertex3fv(p0->x); glVertex3f( INTERP(p0) );

		} else {
		    glVertex3fv(p0->x); glVertex3f( INTERP(p0) );
		    glVertex3fv(p1->x); glVertex3f( INTERP(p1) );
		    glVertex3fv(p2->x); glVertex3f( INTERP(p2) );
		    glVertex3fv(p0->x); glVertex3f( INTERP(p0) );
		}
		glEnd();

	    } else {
		/* No mullions */
		if(was != GL_TRIANGLES) {
		    if(was) glEnd();
		    glBegin( was=GL_TRIANGLES );
		}
		if(usetex) {
		    if(usevn>0) {
			glNormal3fv( vnorms[FVNORM(0)].x );
			glTexCoord2fv( txs[ FTX(0) ].x );
			glVertex3fv( pts[ FVERT(0) ].x );
			glNormal3fv( vnorms[FVNORM(1)].x );
			glTexCoord2fv( txs[ FTX(1) ].x );
			glVertex3fv( pts[ FVERT(1) ].x );
			glNormal3fv( vnorms[FVNORM(2)].x );
			glTexCoord2fv( txs[ FTX(2) ].x );
			glVertex3fv( pts[ FVERT(2) ].x );
		    } else {
			glTexCoord2fv( txs[ FTX(0) ].x );
			glVertex3fv( pts[ FVERT(0) ].x );
			glTexCoord2fv( txs[ FTX(1) ].x );
			glVertex3fv( pts[ FVERT(1) ].x );
			glTexCoord2fv( txs[ FTX(2) ].x );
			glVertex3fv( pts[ FVERT(2) ].x );
		    }
		} else if(usevn>0) {
		    glNormal3fv( vnorms[FVNORM(0)].x );
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glNormal3fv( vnorms[FVNORM(1)].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glNormal3fv( vnorms[FVNORM(2)].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		} else {
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		}
	    }
	} else if(fvn == 4) {
	    if(was != GL_QUADS) {
		if(was) glEnd();
		glBegin( was=GL_QUADS );
	    }
	    if(usetex) {
		if(usevn>0) {
		    glNormal3fv( vnorms[FVNORM(0)].x );
		    glTexCoord2fv( txs[FTX(0)].x );
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glNormal3fv( vnorms[FVNORM(1)].x );
		    glTexCoord2fv( txs[FTX(1)].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glNormal3fv( vnorms[FVNORM(2)].x );
		    glTexCoord2fv( txs[FTX(2)].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		    glNormal3fv( vnorms[FVNORM(3)].x );
		    glTexCoord2fv( txs[FTX(3)].x );
		    glVertex3fv( pts[ FVERT(3) ].x );
		} else {
		    glTexCoord2fv( txs[FTX(0)].x );
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glTexCoord2fv( txs[FTX(1)].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glTexCoord2fv( txs[FTX(2)].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		    glTexCoord2fv( txs[FTX(3)].x );
		    glVertex3fv( pts[ FVERT(3) ].x );
		}
	    } else {
		if(usevn>0) {
		    glNormal3fv( vnorms[FVNORM(0)].x );
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glNormal3fv( vnorms[FVNORM(1)].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glNormal3fv( vnorms[FVNORM(2)].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		    glNormal3fv( vnorms[FVNORM(3)].x );
		    glVertex3fv( pts[ FVERT(3) ].x );
		} else {
		    glVertex3fv( pts[ FVERT(0) ].x );
		    glVertex3fv( pts[ FVERT(1) ].x );
		    glVertex3fv( pts[ FVERT(2) ].x );
		    glVertex3fv( pts[ FVERT(3) ].x );
		}
	    }

	} else if(fvn < 0) {
	    int i;
	    if(was) {
		glEnd();
		was = 0;
	    }
	    glBegin( GL_TRIANGLE_STRIP );
	    i = 0;
	    do {
		if(usevn>0) glNormal3fv( vnorms[ FVNORM(0) ].x );
		if(usetex) glTexCoord2fv( txs[ FTX(0) ].x );
		glVertex3fv( pts[ FVERT(0) ].x );
		fvs0 += FVSTEP;
	    } while(++fvn < 0);
	    glEnd();

	} else {
	    int i;
	    if(was) {
		glEnd();
		was = 0;
	    }
	    glBegin( GL_POLYGON );
	    i = 0;
	    do {
		if(usevn>0) glNormal3fv( vnorms[ FVNORM(0) ].x );
		if(usetex) glTexCoord2fv( txs[ FTX(0) ].x );
		glVertex3fv( pts[ FVERT(0) ].x );
		fvs0 += FVSTEP;
	    } while(--fvn > 0);
	    glEnd();
	}
    }
    if(was) glEnd();
    break;
  }
  if(m->fnorms != NULL) {
    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
  }
}

void specks_draw_boxes( struct stuff *st, struct AMRbox *boxes, int boxlevelmask, Matrix Ttext, int oriented )
{
  static short arcs[] = {
	5, 4, 6, 2, 3, 1, 5, 7, 6,
	-1,
	7, 3,
	0, 1,
	0, 2,
	0, 4
  };
  struct AMRbox *box;
  int i;
  float boxscale, s0, s1;
  int isscaled;

  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glLineWidth( st->boxlinewidth );
  glEnable( GL_BLEND );
  glDisable( GL_LINE_SMOOTH );

  /* Scan through the array, whose end is marked with a box at level -1. */
  for(box = boxes; box->level >= 0; box++) {
    boxscale = (box->level < 0) ? 1.0
	   : st->boxscale[ box->level>=MAXBOXLEV ? MAXBOXLEV-1 : box->level ];
    isscaled = (boxscale != 0 && boxscale != 1);
    if(isscaled) {
	s0 = .5*(1 + boxscale);
	s1 = .5*(1 - boxscale);
    }
    if((1 << box->level) & boxlevelmask) {
	int cval = RGBALPHA(st->boxcmap[
		    box->level<0 ? 0
		    : box->level>=st->boxncmap ?
			st->boxncmap-1 : box->level ].cooked,
		    0xFF);
	int colorme = 0;

	if(oriented)
	    cval = orientboxcolor;
	glColor4ubv( (GLubyte *)&cval );
	glBegin( GL_LINE_STRIP );
	for(i = 0; i < COUNT(arcs); i++) {
	    int vert = arcs[i];
	    if(vert < 0) {
		glEnd();
		glBegin( GL_LINES );
	    } else {

		if(oriented) {
		    if(vert == 0) {
			glColor4ubv( (GLubyte *)&cval );
			colorme = 1;
		    } else if(colorme) {
			glColor3f( vert&1?1:0, vert&2?1:0, vert&4?1:0 );
		    }
		}

		if(isscaled && !oriented && box->level >= 0) {
		    glVertex3f(
			vert&1	? s0*box->p0.x[0] + s1*box->p1.x[0]
				: s1*box->p0.x[0] + s0*box->p1.x[0],
			vert&2	? s0*box->p0.x[1] + s1*box->p1.x[1]
				: s1*box->p0.x[1] + s0*box->p1.x[1],
			vert&4	? s0*box->p0.x[2] + s1*box->p1.x[2]
				: s1*box->p0.x[2] + s0*box->p1.x[2] );
		} else {
		    glVertex3f(
			(vert&1 ? box->p1.x : box->p0.x)[0],
			(vert&2 ? box->p1.x : box->p0.x)[1],
			(vert&4 ? box->p1.x : box->p0.x)[2]
		    );
		}
	    }
	}
	glEnd();
	if(st->boxlabels && st->boxlabelscale != 0) {
	    float sz = .16 * vdist( &box->p0, &box->p1 ) / st->textsize;
	    Point mid;
	    char lbl[16];
	    vlerp( &mid, .5, &box->p0, &box->p1 );
	    sprintf(lbl, "%d", box->boxno);
	    sfStrDrawTJ( lbl, sz, &mid, &Ttext, "c" );
	}
    }
  }
  glLineWidth( 1 );
}

int specks_partial_pick_decode( struct stuff *st, int id,
			int nhits, int nents, GLuint *hitbuf,
			unsigned int *bestzp, struct specklist **slp,
			int *specknop, Point *pos )
{
  int i, hi, ns, slno;
  GLuint z0, bestz = *bestzp;
  int bestslno = 0, bestspeck = -1;
  struct specklist *sl, *slhead;

  if(st == NULL)
    return 0;

  if(st->picked != NULL)
    (*st->picked)(st, NULL, NULL, nhits);	/* "prepare for up to nhits calls" */

  slhead = st->frame_sl;		/* st->sl as snapped by specks_ffn() */
  if(slhead == NULL) slhead = st->sl;	/* maybe there is no specks_ffn() */
  if(slhead == NULL) return 0;

  for(hi = 0, i = 0; hi < nhits && i < nents; hi++, i += ns + 3) {
    ns = hitbuf[i];
    if(ns < 1 || ns > 16)
	break;			/* trouble */
    z0 = hitbuf[i+1];

    if(id != hitbuf[i+3] || ns <= 1) continue;

    if(st->usepoly>1)		/* debug */
	printf(ns>1?"[%x %d/%d]":"[%x %d]", z0, hitbuf[i+3],hitbuf[i+4]);

    if(bestz > z0 && ns > 1 && (slno = hitbuf[i+4]) > 0) {
	bestz = z0;
	bestslno = slno;
	bestspeck = (ns>2) ? hitbuf[i+5] : 0;
    }
    if(st->picked != NULL && ns > 2) {
	for(sl = slhead, slno = 1; sl != NULL && slno < hitbuf[i+4]; sl = sl->next, slno++)
	    ;
	if(sl && hitbuf[i+5] < sl->nspecks)
	    if((*st->picked)(st, &hitbuf[i], sl, hitbuf[i+5]))
		return 0;
    }
  }

  if(st->picked != NULL) {
    if((*st->picked)(st, NULL, NULL, 0))
	return 0;
  }
  if(bestslno <= 0)
    return 0;

  for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
    if(slno == bestslno) {
	if(bestspeck < 0 || bestspeck >= sl->nspecks) {
	    msg("Bogus pick result: sl %x bestspeck %d of 0..%d!\n",
		sl, bestspeck, sl->nspecks-1);
	    return 0;
	}
	if(slp) *slp = sl;
	if(specknop) *specknop = bestspeck;
	if(pos) {
	    if(bestspeck < 0 || bestspeck >= sl->nspecks)
		bestspeck = 0;
	    *pos = NextSpeck( sl->specks, sl, bestspeck )->p;
	}
	*bestzp = bestz;
	return 1;
    }
  }
  return 0;
}

int selname( struct stuff *st, char *name, int create ) {
    int i, avail=0;
    if(name == NULL) return 0;
    if(!strcmp(name, "all") || !strcmp(name, "on")) return SEL_ALL;
    if(!strcmp(name, "none") || !strcmp(name, "off")) return SEL_OFF;
    if(!strcmp(name, "thresh")) return SEL_THRESH;
    if(!strcmp(name, "pick")) return SEL_PICK;
    for(i = COUNT(st->selitems); --i >= 0; ) {
	if(!strcmp(name, st->selitems[i].name))
	    return i+1;
	if(st->selitems[i].name[0] == '\0') avail = i;
    }
    if(create) {
	sprintf(st->selitems[avail].name, "%.15s", name);
	return avail+1;
    }
    return 0;
}

void selinit( SelOp *sp ) {
    sp->wanted = 0;
    sp->wanton = 0;
    sp->use = SEL_NONE;
}

static char *selcat( struct stuff *st, char *from, int selno, int preop ) {
    char *what = "";
    static char s[8];
    switch(selno) {
    case SEL_NONE: what = "off"; break;
    case SEL_ALL: what = "all"; break;
    case SEL_THRESH: what = "thresh"; break;
    case SEL_PICK: what = "pick"; break;
    default:
	if(selno > 0 && selno <= MAXSEL && st->selitems[selno-1].name[0] != '\0')
	    what = &st->selitems[selno-1].name[0];
	else {
	    sprintf(s, "%d", selno<0||selno>MAXSEL?0:selno);
	    what = s;
	}
    }
    if(preop)
	*from++ = preop;
    return from + sprintf(from, "%.15s ", what);
}


char *show_selexpr( struct stuff *st, SelOp *destp, SelOp *srcp )
{
    char str[(MAXSELNAME + 2)*33];
    char *tail = str;
    static char *sstr;
    static int sroom = -1;
    int i;

    if(destp) {
	if(destp->wanted == 0) {
	    tail = selcat(st, tail, SEL_ALL, 0 );
	} else {
	    for(i = SEL_THRESH; i > 0; i--) {
		if(~destp->wanted & SELMASK(i)) {
		    tail = selcat( st, tail, i,
				destp->wanton & SELMASK(i) ? 0 : '-' );
		} else if(destp->wanton & SELMASK(i)) {
		    tail = selcat( st, tail, i, '^' );
		}
	    }
	}
	if(srcp)
	    tail += sprintf(tail, "= ");
    }
    if(srcp) {
	if(srcp->use == SEL_NONE) {
	    tail = selcat( st, tail, srcp->use, 0 );
	} else if(srcp->use == SEL_USE && srcp->wanted == 0) {
	    tail = selcat( st, tail, SEL_ALL, 0 );
	} else {
	    for(i = SEL_THRESH; i > 0; i--) {
		if(srcp->wanted & SELMASK(i)) {
		    tail = selcat( st, tail, i,
				srcp->wanton & SELMASK(i) ? 0 : '-' );
		} else if(srcp->wanton & SELMASK(i)) {
		    tail = selcat( st, tail, i, '^' );
		}
	    }
	}
    }
    if(tail - str > sroom) {
	if(sstr) Free(sstr);
	sroom = tail - str + 60;
	sstr = NewN( char, sroom );
    }
    if(tail == str) strcpy(sstr, "null");
    else {
	tail[-1] = '\0';
	strcpy(sstr, str);
    }
    return sstr;
}

enum SelToken {
    SEL_ERR,		/* unknown */
    SEL_EOF,
    SEL_WORD,		/* bare word or +word */
    SEL_EQUALS		/* = */
};

static enum SelToken seltoken( CONST char **sp, int *leadcp, char word[MAXSELNAME] ) {
    CONST char *p = *sp;
    int k;
    *leadcp = '\0';
    word[0] = '\0';
    while(isspace(*p)) p++;
    if(*p == '\0')
	return SEL_EOF;
    if(*p == '-' || *p == '^' || *p == '+')
	*leadcp = *p++;
    if(*p == '=') {
	*sp = p+1;
	return SEL_EQUALS;
    }
    if(!(isalnum(*p) || *p == '_'))
	return SEL_ERR;
    word[0] = *p++;
    for(k = 1; isalnum(*p) || *p == '_'; p++) {
	if(k < MAXSELNAME-1) word[k++] = *p;
    }
    word[k] = '\0';
    *sp = p;
    return SEL_WORD;
}

void seldest2src( struct stuff *st, CONST SelOp *dest, SelOp *src ) {
    selinit( src );
    if(dest->use == SEL_USE) {
	*src = *dest;
    } else if(dest->use == SEL_DEST) {
	src->wanted = ~dest->wanted | dest->wanton;
	src->wanton = dest->wanton;
	src->use = SEL_USE;
    }
}

void selsrc2dest( struct stuff *st, CONST SelOp *src, SelOp *dest ) {
    *dest = *src;
    if(src->use == SEL_USE) {
	dest->wanted = ~src->wanted;
	dest->wanton = src->wanton & src->wanted;
	dest->use = SEL_DEST;
    }
}

void seldestinvert( struct stuff *st, SelOp *dest ) {
    dest->wanton ^= ~dest->wanted;
}

int seldest( struct stuff *st, SelOp *dest, int selno, int selch ) {
    SelMask mask;
    if(dest == NULL)
	return 0;
    if(selno == SEL_ALL) {
	mask = ~0;
    } else if(selno <= 0 || selno > 32) {
	return 0;
    } else {
	mask = SELMASK(selno);
    }
    if(dest->use != SEL_DEST) {
	dest->use = SEL_DEST;
	dest->wanted = ~0;
	dest->wanton = 0;
    }
    switch(selch) {
    case '^':
	dest->wanted |= mask;
	dest->wanton |= mask;
	break;
    case '-':
	dest->wanted &= ~mask;
	dest->wanton &= ~mask;
	break;
    case '+':
    case '\0':
	dest->wanted &= ~mask;
	dest->wanton |= mask;
	break;
    default:
	return 0;
    }
    return 1;
}

int selsrc( struct stuff *st, SelOp *src, int selno, int selch ) {
    SelMask mask;
    if(src == NULL)
	return 0;
    if(src->use == SEL_DEST) {
	src->wanted = 0;
	src->wanton = 0;
	src->use = SEL_USE;
    }
    if(selno == SEL_OFF) {
	src->wanted = src->wanton = mask = ~0;
	src->use = SEL_NONE;
    } else if(selno == SEL_ALL) {
	src->wanted = src->wanton = mask = 0;
	src->use = SEL_USE;
    } else if(selno > 0 && selno <= 32) {
	mask = SELMASK(selno);
	src->use = SEL_USE;
    } else {
	return 0;
    }
    switch(selch) {
    case '-':
	src->wanted |= mask;
	src->wanton &= ~mask;
	break;
    case '+':
    case '\0':
	src->wanted |= mask;
	src->wanton |= mask;
	break;
    default:
	return 0;
    }
    return 1;
}

/*
 * [word [op]=] { [op]word }*
 */
int parse_selexpr( struct stuff *st, CONST char *str, SelOp *destp, SelOp *srcp, CONST char *plaint )
{
  SelOp dest, src;
  char wd[MAXSELNAME], w[MAXSELNAME];
  CONST char *p = str;
  int leadc, leadc2;
  enum SelToken tok, tok2;
  int ok = 1;
  int anysrc = 0, anydest = 0;

  selinit(&dest); selinit(&src);
  tok = seltoken( &p, &leadc, wd );
  if(tok == SEL_EOF)
      return 0;
  if(destp) {
      /* Had better begin with a word, and with no prefix */
    int destno;
    if(tok != SEL_WORD) goto fail;
    destno = selname( st, wd, 1 );
    tok2 = seltoken( &p, &leadc2, w );
    if(tok2 == SEL_ERR) goto fail;
    ok = seldest( st, &dest, destno, leadc ? leadc : leadc2 );
    anydest = 1;
    if(!ok) goto fail;
    if(tok2 == SEL_WORD) {
	tok = SEL_WORD;
	leadc = leadc2;
	strcpy(wd, w);
    } else {
	tok = seltoken( &p, &leadc, wd );
    }
  }
  while(srcp && tok == SEL_WORD) {
    ok = selsrc( st, &src, selname( st, wd, 0 ), leadc );
    if(!ok) goto fail;
    if(src.use == SEL_NONE && src.wanted != 0 && destp) {
	seldestinvert( st, &dest );
	selsrc( st, &src, SEL_ALL, 0 );	/* X = off  <=> -X = on */
    }
    anysrc = 1;
    tok = seltoken( &p, &leadc, wd );
  }
  if(tok != SEL_EOF)
      goto fail;
  if(destp && anydest) *destp = dest;
  if(srcp && anysrc) *srcp = src;
  return (anydest?1:0) + (anysrc?2:0);

fail:
  if(plaint)
      msg("%s: um, %s", plaint, str);
  return 0; /*XXX*/
}

CONST char *selcounts( struct stuff *st, struct specklist *sl, SelOp *selp ) {
    int i;
    static char counts[24];
    int yes, total;
    SelOp selop;

    if(selp == NULL) return "";
    if(selp->use == SEL_DEST)
	seldest2src( st, selp, &selop );
    else
	selop = *selp;
    for(yes = total = 0; sl != NULL; sl = sl->next) {
	SelMask *sel = sl->sel;
	if(sl->text != NULL || sl->nsel < sl->nspecks || sl->special != SPECKS)
	    continue;
	total += sl->nspecks;
	for(i = 0; i < sl->nspecks; i++)
	    if(SELECTED(sel[i], &selop))
		yes++;
    }
    if(selop.use == SEL_NONE) yes = 0;
    sprintf(counts, "%d of %d", yes, total);
    return counts;
}

SpecksPickFunc specks_all_picks( struct stuff *st, SpecksPickFunc func, void *arg ) {
    SpecksPickFunc was = st->picked;
    st->picked = func;
    st->pickinfo = arg;
    return was;
}


static void addchunk( struct stuff *st, int nsp, int bytesperspeck,
			float scaledby, struct speck *sp, char *text,
			int outbytesperspeck )
{
    struct specklist **slp, *sl = NewN( struct specklist, 1 );
    memset( sl, 0, sizeof(*sl) );
    sl->speckseq = ++st->speckseq;

    sl->bytesperspeck = outbytesperspeck;

    sl->specks = NewNSpeck( sl, nsp );
    sl->nspecks = nsp;
    sl->scaledby = scaledby;
    if(text) {
	sl->text = NewN( char, strlen(text)+1 );
	strcpy(sl->text, text);
    } else {
	sl->text = NULL;
    }
    if(bytesperspeck == outbytesperspeck) {
	memcpy( sl->specks, sp, nsp*sl->bytesperspeck );
    } else {
	int i;
	for(i = 0; i < nsp; i++)
	    memcpy(((char *)sl->specks) + i*outbytesperspeck,
		   ((char *)sp) + i*bytesperspeck,
		   outbytesperspeck);
    }
    sl->sel = NewN( SelMask, nsp );
    sl->nsel = nsp;
    memset(sl->sel, 0, nsp*sizeof(SelMask));

    specks_insertspecks(st, st->curdata, st->datatime, sl);
    st->sl = specks_timespecks(st, st->curdata, st->curtime); /* in case it changed */
    sl->colorseq = -1;		/* Force recomputing colors */
    sl->sizeseq = -1;		/* Force recomputing sizes */
}

int specks_count( struct specklist *sl ) {
    int n;
    for(n = 0; sl != NULL; sl = sl->next)
	if(sl->text == NULL)
	    n += sl->nspecks * (sl->subsampled>0 ? sl->subsampled : 1);
    return n;
}


static float speckscale = 1;


#define LINEBUFSIZE 2048

static char *get_line(FILE *f, char *buf) {
  buf[0] = '\0';
  while(fgets(buf, LINEBUFSIZE, f) != NULL) {
    char *cp = buf;
    while(isspace(*cp)) cp++;
    if(*cp != '#' && *cp != '\0')
	return cp;
  }
  return NULL;
}
 
static enum SurfStyle getstyle( char *str )
{
  if(str == NULL || !strcmp(str, "solid") || !strncmp(str, "fill", 4)
		 || !strncmp(str, "poly", 4))
    return S_SOLID;
  if(!strncmp(str, "wire", 4) || !strncmp(str, "line", 4))
    return S_LINE;
  if(!strncmp(str, "plane", 5) || !strncmp(str, "ax", 2))
    return S_PLANE;
  if(!strncmp(str, "point", 5))
    return S_POINT;
  if(!strcmp(str, "off"))
    return S_OFF;
  msg("Unknown surface style \"%s\": want solid|line|plane|point|off", str);
  return S_SOLID;
}

/*
 * Xcen Ycen Zcen ellipsoid [-t txno] [-c cindex] [-s surfstyle] [-n nu[,nv]] [-r sizex[,y,z]] [0-or-9-or-16-numbers-tfm]
 */
void specks_read_ellipsoid( struct stuff *st, Point *pos, int argc, char **argv, char *comment ) {
  int i;
  struct ellipsoid e, *ep;
  float m3[3*3];
  float rxyz[3];

  memset(&e, 0, sizeof(e));
  e.cindex = -1;
  e.level = -1;
  e.pos = *pos;
  e.size.x[0] = e.size.x[1] = e.size.x[2] = 1;

  if(comment) {
    e.title = (comment[1] == ' ') ? comment+2 : comment+1;
  }

  for(i = 1; i+1 < argc; i += 2) {
    if(!strncmp(argv[i], "-c", 2)) {
	sscanf(argv[i+1], "%d", &e.cindex);
    } else if(!strncmp(argv[i], "-s", 2)) {
	e.style = getstyle(argv[i+1]);
    } else if(!strcmp(argv[i], "-r")) {
	int k = sscanf(argv[i+1], "%f%*c%f%*c%f", &e.size.x[0],&e.size.x[1],&e.size.x[2]);
	switch(k) {
	case 1: e.size.x[2] = e.size.x[1] = e.size.x[0]; break;
	case 3: break;
	default: msg("ellipsoid -r: expected 1 or 3 comma-separated semimajor axes not %s", argv[i+1]);
	}
    } else if(!strcmp(argv[i], "-n")) {
	int k = sscanf(argv[i+1], "%d%*c%d", &e.nu, &e.nv);
	switch(k) {
	case 1: e.nv = e.nu/2 + 1; break;
	case 2: break;
	default: msg("ellipsoid -n: expected nu or nu,nv but not %s", argv[i+1]);
	}
    } else if(!strcmp(argv[i], "-l")) {
	sscanf(argv[i+1], "%d", &e.level);
    } else if(!strcmp(argv[i], "-w")) {
	sscanf(argv[i+1], "%f", &e.linewidth);
    } else {
	break;
    }
  }

  if(i == argc) {
    /* OK */
  } else if(i+9 == argc && 9==getfloats(m3, 9, i, argc, argv)) {
    memcpy(&e.ori.m[0*4+0], &m3[0], 3*sizeof(float));
    memcpy(&e.ori.m[1*4+0], &m3[3], 3*sizeof(float));
    memcpy(&e.ori.m[2*4+0], &m3[6], 3*sizeof(float));
    e.ori.m[3*4+3] = 1;
    e.hasori = 1;
  } else if(i+16 == argc && 16==getfloats(&e.ori.m[0], 16, i, argc, argv)) {
    e.hasori = 1;
  } else if(i+3 == argc && 3==getfloats(rxyz, 3, i, argc, argv)) {
    float aer[3];
    aer[0] = rxyz[1];
    aer[1] = rxyz[0];
    aer[2] = rxyz[2];
    xyzaer2tfm( &e.ori, NULL, aer );
    e.hasori = 1;
  } else {
    msg("ellipsoid: expected 0 or 3 (RxRyRz) or 9 or 16 numbers after options, not %s",
	rejoinargs(i, argc, argv));
    return;
  }

  /* OK, take it */
  ep = NewN( struct ellipsoid, 1 );
  *ep = e;
  if(e.title) ep->title = shmstrdup(e.title);
  ep->next = st->staticellipsoids;
  st->staticellipsoids = ep;
}


void specks_read_mesh( struct stuff *st, FILE *f, int argc, char **argv, char *buf )
{
  char *cp, *ep;
  int i, count, alloced, err = 0;
  int havetx = 0;
  Point pts[300], txs[300];
  struct mesh *m = NewN( struct mesh, 1 );
  int timestep = st->datatime;

  memset(m, 0, sizeof(*m));
  m->type = QUADMESH;
  m->cindex = -1;
  m->txno = -1;
  m->pts = pts; m->tx = txs;
  alloced = COUNT(pts);

  if(!strcmp(argv[0], "quadmesh") || !strcmp(argv[0], "mesh")) {
    m->type = QUADMESH;
  }

  for(i = 1; i+1 < argc; i += 2) {
    if(!strncmp(argv[i], "-time", 3)) {
	timestep = atoi(argv[i+1]);
    } else if(!strncmp(argv[i], "-t", 2)) {
	havetx = sscanf(argv[i+1], "%d", &m->txno);
    } else if(!strcmp(argv[i], "-static")) {
	timestep = -1;
	i--;
    } else if(!strncmp(argv[i], "-c", 2)) {
	sscanf(argv[i+1], "%d", &m->cindex);
    } else if(!strncmp(argv[i], "-s", 2)) {
	m->style = getstyle(argv[i+1]);
    } else if(!strncmp(argv[i], "-w", 2)) {
	m->linewidth = atof(argv[i+1]);
    } else {
	break;
    }
  }

  if(m->type == QUADMESH) {
    get_line(f, buf);
    if(sscanf(buf, "%d%d", &m->nu, &m->nv) != 2
			|| m->nu <= 0 || m->nv <= 0) {
	msg("quadmesh: expected nu nv, got %s", buf);
	Free(m);
	return;
    }
    alloced = m->nverts = m->nu * m->nv;
    m->pts = NewN( Point, m->nverts );
    m->tx = havetx ? NewN( Point, m->nverts ) : NULL;
  }
    

  count = 0;
  while((cp = get_line(f, buf)) != NULL) {
    int ngot;
    Point *tp;
#define BRA '{'
#define CKET '}'
    if(*cp == BRA) continue;
    if(*cp == CKET) break;

    if(count >= alloced) {
	if(m->type == QUADMESH) {
	    err = 1;
	    msg("Only expected %d*%d=%d vertices; what's %s",
		m->nu,m->nv, alloced, cp);
	    break;
	}
	/* otherwise, ask for more */
	/* XXX maybe later */
	break;
    }
	
    tp = &m->pts[count];
    for(ngot = 0; ngot < 3; ngot++, cp = ep) {
	tp->x[ngot] = strtod(cp, &ep);
	if(cp == ep) break;
    }
    if(havetx && ngot == 3) {
	tp = &m->tx[count];
	for(ngot = 0; ngot < 3; ngot++, cp = ep) {
	    tp->x[ngot] = strtod(cp, &ep);
	    if(cp == ep) break;
	}
	if(ngot == 2) {	/* accept either 2-D or 3-D texture coords */
	    tp->x[2] = 0;
	    ngot++;
	}
    }
    if(ngot != 3) {
	msg(havetx ? "Expected 5 or 6 numbers per line; what's %s"
		   : "Expected 3 numbers per line; what's %s",
		buf); 
	err = 1;
	break;
    }
    count++;
  }
  if(m->type == QUADMESH && count < alloced) {
    err = 1;
    msg("Expected %d*%d = %d vertices, only got %d", m->nu,m->nv, alloced, count);
  }
  if(err) {
    if(m->pts != pts) Free(m->pts);
    if(m->tx != txs && m->tx != NULL) Free(m->tx);
    Free(m);
    return;
  }

  if(timestep >= 0) {
    if(timestep >= st->ntimes)
	specks_ensuretime( st, st->curdata, timestep );
    m->next = st->meshes[st->curdata][timestep];
    st->meshes[st->curdata][timestep] = m;
  } else {
    m->next = st->staticmeshes;
    st->staticmeshes = m;
  }
}

void mesh_prepare_elements( struct mesh *m )
{
  static GLenum prim[6] = { GL_TRIANGLE_STRIP, GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS, GL_TRIANGLE_FAN };
  Elements els[6];
  int tristrips=0, trifans=0;
  int i, n, base;
 
  if(m->type != POLYMESH)
    return;

  memset( els, 0, sizeof(els) );

  /* how many of each type? */
  for(i = 0; i < m->nfaces; i++) {
    int n = m->fvn[i];
    if(n < 0) {
	els[0].count += -n;
	tristrips++;
    } else if(n >= 5) {
	els[5].count += n;
	trifans++;
    } else {
	els[n].count += n;
    }
  }
  /* starting positions */
  base = 0;
  for(i = 0; i <= 5; i++) {
    els[i].base = base;
    base += els[i].count;
  }
  /* allocate memory, copy fields into glInterleavedArrays form */
}

void specks_read_waveobj( struct stuff *st, int argc, char **argv, char *line, char *infname )
{
  struct mesh *m = NewN( struct mesh, 1 );
  int lno;
  int i, f;
  int anypolys;
  int timestep = st->datatime;
  FILE *inf;
  char *fname, *fullname;
#define CHUNK 500
  int kpts, ktxs, kvns, kfvs, kfv0, kfvn;
  int npts, ntxs, nvns, nfvs, nfv0, nfvn;
  struct chain {
	void *data;
	int pos;
	struct chain *next;
  } *cpts, *ctxs, *cvns, *cfvs, *cfv0, *cfvn;
  Point vpts[CHUNK], vtxs[CHUNK], vvns[CHUNK];
  int vfvs[CHUNK], vfv0[CHUNK], vfvn[CHUNK];


  memset(m, 0, sizeof(*m));
  m->type = POLYMESH;
  m->cindex = -1;
  m->txno = -1;

  for(i = 1; i+1 < argc; i += 2) {
    if(!strncmp(argv[i], "-time", 3)) {
	timestep = atoi(argv[i+1]);
    } else if(!strncmp(argv[i], "-texture", 3) || !strncmp(argv[i], "-tx", 3)) {
	sscanf(argv[i+1], "%d", &m->txno);
    } else if(!strcmp(argv[i], "-static")) {
	timestep = -1;
	i--;
    } else if(!strncmp(argv[i], "-c", 2)) {
	sscanf(argv[i+1], "%d", &m->cindex);
    } else if(!strncmp(argv[i], "-s", 2)) {
	m->style = getstyle(argv[i+1]);
    } else if(argv[i][0] == '-') {
	i = argc;
    } else {
	break;
    }
  }

  if(i >= argc) {
    msg("Usage: waveobj [-time T | -static] [-texture TXNO] [-c CINDEX] [-s solid|line|plane|point] wavefront.obj");
    return;
  }

  fname = argv[i];
  fullname = findfile( infname, fname );
  if(fullname == NULL || (inf = fopen(fullname, "r")) == NULL) {
    msg("waveobj: can't find .obj file %s", fname);
    return;
  }

#define VINIT(what) c##what = NULL, k##what = n##what = 0
#define VTOTAL(what) (k##what + n##what)
#define VNEXT(what) \
    if(++k##what >= CHUNK) { \
	struct chain *ct = (struct chain *)malloc(sizeof(*ct)); \
	ct->data = malloc(sizeof(v##what)); \
	memcpy(ct->data, v##what, sizeof(v##what)); \
	ct->pos = n##what; \
	ct->next = c##what; \
	c##what = ct; \
	k##what = 0; \
	n##what += CHUNK; \
    }
#define VSTUFF(dest, what) { \
	struct chain *ct, *cn; \
	memcpy( &dest[n##what], v##what, k##what * sizeof(v##what[0]) );  \
	for(ct = c##what; ct != NULL; ct = cn) {			  \
	    cn = ct->next;	\
	    memcpy( &dest[ct->pos], ct->data, CHUNK*sizeof(v##what[0]) ); \
	    free(ct->data);	\
	    free(ct);		\
	}			\
    }

  VINIT(pts);	/* vertices */
  VINIT(txs);	/* tx coordinates */
  VINIT(vns);	/* vertex normals */
  VINIT(fvs);	/* {vno, txno, normno} per vertex per face */
  VINIT(fv0);	/* starting offset in fvs per face */
  VINIT(fvn);	/* number of vertices per face */
  lno = 0;
  while(fgets(line, LINEBUFSIZE, inf) != NULL) {
    char *word, *s = line;
    int slen;
    lno++;
    while(isspace(*s)) s++;
    if(*s == '#' || *s == '\0') continue;
    word = s++;
    while(*s != '\0' && !isspace(*s)) s++;
    slen = s - word;
    if(slen == 1 && word[0] == 'v') {
	Point *p = &vpts[kpts];
	char *ep;
	p->x[0] = strtod(s, &s);
	p->x[1] = strtod(s, &s);
	p->x[2] = strtod(s, &ep);
	if(s == ep) {
	    msg("waveobj: %s line %d: bad v line %s", fullname, lno, line);
	    continue;
	}
	VNEXT(pts);

    } else if(slen == 2 && word[0] == 'v' && word[1] == 't') {
	/* "vt" lines */
	Point *txp = &vtxs[ktxs];
	char *ep;
	txp->x[0] = strtod(s, &s);
	txp->x[1] = strtod(s, &ep);
	txp->x[2] = 0;
	if(s == ep) {
	    msg("waveobj: %s line %d: bad vt line %s", fullname, lno, line);
	    continue;
	}
	VNEXT(txs);

    } else if(slen == 2 && word[0] == 'v' && word[1] == 'n') {
	/* "vn" lines */
	Point *vnp = &vvns[kvns];
	char *ep;
	vnp->x[0] = strtod(s, &s);
	vnp->x[1] = strtod(s, &s);
	vnp->x[2] = strtod(s, &ep);
	if(s == ep) {
	    msg("waveobj: %s line %d: bad vn line %s", fullname, lno, line);
	    continue;
	}
	VNEXT(vns);

    } else if(slen == 1 && word[0] == 'f') {
	char *ep;
	int myfv0 = VTOTAL( fvs );
	int myfvn = 0;
	int vno = -1, txvno = -1, normvno = -1;

	for(;;) {
	    while(isspace(*s)) s++;
	    if(*s == '\0') break;
	    vno = strtol(s, &ep, 10) - 1;
	    if(*ep == '/') {
		txvno = strtol(s=ep+1, &ep, 10) - 1;
		if(*ep == '/')
		    normvno = strtol(s=ep+1, &ep, 10) - 1;
	    }
	    if(s == ep) {
		msg("waveobj: %s line %d: bad f line %s", fullname, lno, line);
		myfvn = 0;
		break;
	    }
	    s = ep;
	    while(!isspace(*s)) s++;
	    vfvs[kfvs] = vno;
	    VNEXT(fvs);
	    vfvs[kfvs] = txvno;
	    VNEXT(fvs);
	    vfvs[kfvs] = normvno;
	    VNEXT(fvs);
	    myfvn++;
	}
	vfv0[kfv0] = myfv0;
	vfvn[kfvn] = myfvn;
	if(myfvn >= 2) {
	    VNEXT(fv0);
	    VNEXT(fvn);
	}
    }
  }
  fclose(inf);
  m->pts = NewN( Point, VTOTAL(pts) );
  VSTUFF( m->pts, pts );
  m->tx = NewN( Point, VTOTAL(txs) );
  VSTUFF( m->tx, txs );
  m->vnorms = NewN( Point, VTOTAL(vns) );
  VSTUFF( m->vnorms, vns );
  m->fv0 = NewN( int, VTOTAL(fv0) );
  VSTUFF( m->fv0, fv0 );
  m->fvn = NewN( int, VTOTAL(fvn) );
  VSTUFF( m->fvn, fvn );
  m->fvs = NewN( int, VTOTAL(fvs) );
  VSTUFF( m->fvs, fvs );
  m->nfaces = VTOTAL(fvn);
  m->nfv = VTOTAL(fvs);
  m->nverts = VTOTAL(pts);
  m->ntx = VTOTAL(txs);
  m->nvnorms = VTOTAL(vns);
  anypolys = 0;
  for(i = 0; i < m->nfaces; i++) {
    int nfv = m->fvn[i];
    int fv0 = m->fv0[i];
    int k;
    if(nfv >= 3)
	anypolys = 1;
    for(k = 0; k < nfv; k++) {
	int *myfvs = &m->fvs[fv0 + k*FVSTEP];
	int vno, txvno, normvno;

	vno = myfvs[FVS_VERT];
	if(vno < 0 || vno >= m->nverts) {
	    fprintf(stderr, "%s face %d: vert index %d out of range 1..%d\n",
		    fullname, i+1, vno+1, m->nverts);
	    myfvs[FVS_VERT] = 0;
	}
	txvno = myfvs[FVS_TX];
	if(txvno < -1 || txvno >= m->ntx) {
	    fprintf(stderr, "%s face %d: texture index %d out of range 1..%d\n",
		    fullname, i+1, txvno+1, m->ntx);
	    m->fvs[fv0+FVS_TX] = myfvs[FVS_TX] = -1;  /* wipe that texture off your face */
	}
	normvno = myfvs[FVS_VNORM];
	if(normvno < -1 || normvno >= m->nvnorms) {
	    fprintf(stderr, "%s face %d: vertex-normal index %d out of range 1..%d\n",
		    fullname, i+1, normvno+1, m->nvnorms);
	    m->fvs[fv0+FVS_VNORM] = myfvs[FVS_VNORM] = -1;  /* ignore all vertex normals, use facet */
	}
    }
  }
  if(anypolys) {
      m->fnorms = NewN( Point, m->nfaces );
      for(f = 0; f < m->nfaces; f++) {
	Point vab, vac, normal;
	int myfv0 = m->fv0[f];
	if(m->fvn[f] > 3 && m->fvs[myfv0] == m->fvs[myfv0+FVSTEP])
	    myfv0 += FVSTEP;		/* degenerate quad? */
	vsub( &vab, &m->pts[ m->fvs[myfv0+1*FVSTEP] ], &m->pts[ m->fvs[myfv0] ] );
	vsub( &vac, &m->pts[ m->fvs[myfv0+2*FVSTEP] ], &m->pts[ m->fvs[myfv0] ] );
	vcross( &normal, &vab, &vac );
	vunit( &m->fnorms[f], &normal );
      }
  }

  if(timestep >= 0) {
    if(timestep >= st->ntimes)
	specks_ensuretime( st, st->curdata, timestep );
    m->next = st->meshes[st->curdata][timestep];
    st->meshes[st->curdata][timestep] = m;
  } else {
    m->next = st->staticmeshes;
    st->staticmeshes = m;
  }

  mesh_prepare_elements( m );

}

void specks_read( struct stuff **stp, char *fname )
{
  FILE *f;
  char line[LINEBUFSIZE], oline[LINEBUFSIZE], *tcp;
  struct stuff *st = *stp;
  int maxfields = 0;

#ifdef FLHACK
 /* should be bigger, but too many machines have tiny stack areas! */
# define SPECKCHUNK 512
#else
# define SPECKCHUNK 1001
#endif
#define MAXARGS 48
  int argc;
  char *argv[MAXARGS+1];
  struct speck speckbuf[SPECKCHUNK];
  struct speck *sp;
  struct speck s;
  struct specklist tsl;
  int i, nsp, maxnsp;
  int ignorefirst = 0;
  int lno = 0;
  char *comment;

  if(fname == NULL) return;

  tcp = NewA( char, strlen(fname)+1 );	/* fname's space might get reused */
  strcpy(tcp, fname);
  fname = tcp;

  if((f = fopen(fname, "rb")) == NULL) {
    msg("%s: can't open: %s", fname, strerror(errno));
    return;
  }

  tsl.bytesperspeck = (st->maxcomment+1 +
			(sizeof(s) - sizeof(s.title)) + 3) & ~3;
  s.rgba = 0;
  s.size = 1;
  nsp = 0;
  sp = speckbuf;
  maxnsp = sizeof(speckbuf) / tsl.bytesperspeck;

#define SPFLUSH() \
    if(nsp > 0) {					\
	addchunk( st, nsp, tsl.bytesperspeck,		\
		speckscale, speckbuf, NULL,		\
		maxfields >= MAXVAL ? tsl.bytesperspeck \
			: SMALLSPECKSIZE(maxfields) );	\
    }							\
    nsp = maxfields = 0;				\
    sp = speckbuf;

  line[sizeof(line)-1] = '\1';
  while( fgets(line, sizeof(line), f) != NULL ) {
    lno++;
    if(line[sizeof(line)-1] != 1) {
	if(line[sizeof(line)-2] != '\n') {
	    while((i = getc(f)) != EOF && i != '\n')
		;
	}
	line[sizeof(line)-1] = '\1';
	line[sizeof(line)-2] = '\0';
	msg("%s line %d: truncated (>%d chars long!)",
		fname, lno, sizeof(line)-2);
    }

    argc = tokenize(line, oline, MAXARGS, argv, &comment);

    while(argc > 0 && !strcmp(argv[0], "add")) {
	for(i = 1; i <= argc; i++) argv[i-1] = argv[i];
	argc--;
    }

    if(argc == 0)
	continue;


    if(nsp >= maxnsp || (isalnum(argv[0][0]) && !isdigit(argv[0][0]))) {
	SPFLUSH();
    }

    if(!strcmp(argv[0], "include") || !strcmp(argv[0], "read")) {
	float oldscale = speckscale;
	char *infname = argv[1];
	char *realfile = findfile( fname, infname );
	if(realfile == NULL) {
	    msg("%s: Can't find include-file %s", fname, infname);
	} else {
	    specks_read( &st, realfile );
	}
	speckscale = oldscale;

    } else if(!strcmp(argv[0], "object") && argc>1) {

	char *eq = strchr(argv[1], '=');

	i = 1;
	parti_object( argv[1], &st, 1 );
	if(eq) {
	    parti_set_alias( st, eq+1 );
	} else if(argc >= 3 && !strcmp(argv[2], "=")) {
	    /* object g3 = alias */
	    parti_set_alias( st, argv[3] );
	}

    } else if(parti_read(stp, argc, argv, fname)) {
	st = *stp;	/* In case st changed */

    } else if(!strcmp(argv[0], "pb") && argc>1) {
	int tno = st->datatime;
	char *realfile;
	i = 1;
	if(argc>3 && !strcmp(argv[1], "-t")) {
	    if((tno = (int)getfloat(argv[2], st->datatime)) < 0) {
		msg("pb -t: expected timestepnumber(0-based), not %s", argv[2]);
		continue;
	    }
	    i = 3;
	}
	realfile = findfile( fname, argv[i] );
	if(realfile == NULL) {
	    msg("%s: pb: can't find file %s", fname, argv[i]);
	} else {
	    specks_read_pb( st, realfile, tno );
	}

    } else if(!strcmp(argv[0], "sdb") && argc>1) {
	int tno = st->datatime;
	char *realfile;
	i = 1;
	if(argc>3 && !strcmp(argv[1], "-t")) {
	    if((tno = (int)getfloat(argv[2], st->datatime)) < 0) {
		msg("sdb -t: expected timestepnumber(0-based), not %s", argv[2]);
		continue;
	    }
	    i = 3;
	}
	realfile = findfile( fname, argv[i] );
	if(realfile == NULL) {
	    msg("%s: sdb: can't find file %s", fname, argv[i]);
	} else {
	    specks_read_sdb( st, realfile, tno );
	}

    } else if(!strcmp(argv[0], "sdbvars")) {
	if(argc > 1) {
	    if(argv[1][strspn(argv[1], "mMcrogtxyzSn")] != '\0') {
		msg("sdbvars: pattern must contain only chars from: mMcrogtxyzSn, not %s",
			argv[1]);
	    } else {
		if(st->sdbvars) Free(st->sdbvars);
		st->sdbvars = shmstrdup(argv[1]);
	    }
	} else {
	    msg("sdbvars %s", st->sdbvars);
	}

    } else if(!strcmp(argv[0], "box") || !strcmp(argv[0], "boxes")) {
	Point cen, rad;
	struct AMRbox box;
	int k;

	int tno = -1;

	box.boxno = -1;
	box.level = 0;

	i = 1;
	while(i+2 < argc && argv[i][0] == '-') {
	    if(argv[i][1] == 'n' && sscanf(argv[i+1], "%d", &box.boxno)>0)
		i += 2;
	    else if(argv[i][1] == 'l' && sscanf(argv[i+1], "%d", &box.level)>0)
		i += 2;
	    else if(argv[i][1] == 't' && (tno = (int)getfloat(argv[i+1], st->datatime)) >= 0)
		i += 2;
	    else
		break;
	}
	if(i+2 == argc &&
	     3==sscanf(argv[i], "%f%*c%f%*c%f", &cen.x[0],&cen.x[1],&cen.x[2]) &&
	     0<(k=sscanf(argv[i+1], "%f%*c%f%*c%f", &rad.x[0],&rad.x[1],&rad.x[2]))) {
	    if(k<3) rad.x[1] = rad.x[2] = rad.x[0];	/* if scalar radius */
	    vsub(&box.p0, &cen, &rad);
	    vadd(&box.p1, &cen, &rad);
	    specks_add_box( st, &box, tno );

	} else if(i+3 == argc &&
		    2==sscanf(argv[i], "%f%*c%f",&box.p0.x[0],&box.p1.x[0]) &&
		    2==sscanf(argv[i+1], "%f%*c%f",&box.p0.x[1],&box.p1.x[1]) &&
		    2==sscanf(argv[i+2], "%f%*c%f",&box.p0.x[2],&box.p1.x[2])) {

	    specks_add_box( st, &box, tno );

	} else if(i+6 == argc) {
	    k = getfloats( &box.p0.x[0], 3, i, argc, argv )
	      + getfloats( &box.p1.x[0], 3, i+3, argc, argv );
	    if(k==6)
		specks_add_box( st, &box, tno );
	    else
		msg("box: expected xmin ymin zmin  xmax ymax zmax  -or- xmin,xmax ymin,ymax zmin,zmax -or- xcen,ycen,zcen xrad,yrad,zrad");

	} else if(i+1 == argc) {
	    char *realfile = findfile( fname, argv[i] );
	    if(realfile == NULL) {
		msg("%s: boxes: can't find file %s", fname, argv[i]);
	    } else {
		specks_read_boxes( st, realfile, tno );
	    }
	} else {
	    msg("usage: box [-t timestep] AMRboxfile  -or-");
	    msg("  box [-t time] [-n boxno] [-l level] xcen,ycen,zcen  xradius,yradius,zradius  -or-");
	    msg("  box [options]  xmin ymin zmin  xmax ymax zmax");
	    msg(" options: -n <boxno>  box number, for \"gobox\" and \"boxlabel\" cmds (dflt -1)");
	    msg("   -l <boxlevel>      level-number (0..31) for \"showboxlevel\" cmds (dflt 0)");
	    msg("   -t <time>		timestep, if animated; default eternal,");
	    msg("			or (for animated AMRboxfile) start time 0");
	}

    } else if(!strncmp(argv[0], "annot", 5)) {
	static char ausage[] = "Usage: annot [-t timestep]  string...";
	int tno = st->datatime;

	i = 1;
	if(argc>1 && !strcmp(argv[1], "-t")) {
	    tno = (int)getfloat(argv[2], -1);
	    i = 3;
	}
	if(i >= argc || tno < 0) {
	    msg(ausage);
	    continue;
	}
	specks_add_annotation( st, rejoinargs( i, argc, argv ), tno );

    } else if(!strcmp(argv[0], "cment") || !strcmp(argv[0], "textcment")
		|| !strcmp(argv[0], "boxcment")) {
	specks_parse_args( &st, argc, argv );

    } else if(!strcmp(argv[0], "size") && argc>1) {
	sscanf(argv[1], "%f", &s.size);

    } else if(!strcmp(argv[0], "scale") && argc>1) {
		/* "scale" in data file, not as VIRDIR command */
	float v = 0;
	sscanf(argv[1], "%f", &v);
	if(v != 0)
	    st->spacescale = speckscale = v;

    } else if(!strcmp(argv[0], "echo")) {
	msg( "%s", rejoinargs(1, argc, argv) );

    } else if(!strcmp(argv[0], "tfm") && argc>1) {
	int inv = 0;
	int mul = 0;
	int hpr = 0;
	int camparent = -1;
	int more, j, k;
	float scl = 1;
	int has_scl = 0;
	Matrix ot, t;
	Point xyz;
	float aer[3];
	char *key;
	i = 1;
	for(more = 1, key = argv[1]; *key != '\0' && more; key++) {
	  switch(*key) {
	  case '*': mul = 1; break;
	  case '/': inv = 1; break;
	  case 'h': case 'p': case 'r': hpr = 1; break;
	  case 'c': camparent = 1; i++; more = 0; break;
	  case 'w': camparent = 0; i++; more = 0; break;
	  case '\0': more = 0; i++; break;
	  default: more = 0; break;
	  }
	}

	if(camparent>=0)
	    parti_parent( st, camparent ? -1 : 0 );

	parti_geto2w( st, parti_idof( st ), &ot );

	k = getfloats( &t.m[0], 16, i, argc, argv );
	switch(k) {
	case 1:
		scl = t.m[0];
		t = Tidentity;
		t.m[0*4+0] = t.m[1*4+1] = t.m[2*4+2] = scl;
		break;
	case 7:		/* ignore fovy if included */
		scl = t.m[6];  has_scl = 1;
	case 6:
		xyz = *(Point *)&t.m[0];
		if(hpr) {
		    aer[0] = t.m[3], aer[1] = t.m[4], aer[2] = t.m[5];
		} else {
		    /* Note we permute: px py pz rx ry rz == px py pz e a r */
		    aer[1] = t.m[3], aer[0] = t.m[4], aer[2] = t.m[5];
		}
		xyzaer2tfm( &t, &xyz, aer );
		if(has_scl) {
		    for(i = 0; i < 12; i++)
			t.m[i] *= scl;
		}
		break;

	case 9:
		for(i = 2; i >= 0; i--)
		    for(j = 2; j >= 0; j--)
			t.m[i*4+j] = t.m[i*3+j];
		t.m[0*4+3] = t.m[1*4+3] = t.m[2*4+3] =
			t.m[3*4+0] = t.m[3*4+1] = t.m[3*4+2] = 0;
		t.m[3*4+3] = 1;
		break;

	case 16: break;
	case 0: if(camparent >= 0) continue;	/* allow "tfm c" or "tfm w" */
	default:
	    msg("Usage: tfm: expected 1 or 6 or 9 or 16 numbers not %s", line);
	    continue;
	}
	if(inv)
	    eucinv( &t, &t );
	if(mul)
	    mmmul( &t, &ot, &t );

	parti_seto2w( st, parti_idof( st ), &t );

    } else if((!strcmp(argv[0], "eval") || !strcmp(argv[0], "feed")
				   || !strcasecmp(argv[0], "VIRDIR"))
		&& argc > 1) {
#if THIEBAUX_VIRDIR
	if(0 == specks_parse_args( &st, argc-1, argv+1 )) {
	    /* grr, need to skip first word of line */
	    char *s = line;
	    while(isspace(*s)) s++;
	    while(!isspace(*s) && *s) s++;
	    VIDI_queue_commandstr( s );
	}

#else /* non-Thiebaux-virdir */
	specks_parse_args( &st, argc-1, argv+1 );

#endif

    } else if(!strcmp(argv[0], "filepath")) {
	int k = 0;
	char **oldpath = getfiledirs();
	char *path[128], *dir;
	for(i = 1, k = 0; i < argc && k < COUNT(path)-1; i++) {
	    dir = strtok(argv[i], ":");
	    do {
		if(!strcmp(dir,"+")) {
		    int m = 0;
		    while(k<COUNT(path)-1 && oldpath && oldpath[m])
			path[k++] = oldpath[m++];
		} else {
		    path[k++] = dir;
		}
		dir = strtok(NULL, ":");
	    } while(dir && k < COUNT(path)-1);
	}
	path[k] = NULL;
	if(k > 0) filedirs(path);

    } else if(!strcmp(argv[0], "setenv") && argc==3 && NULL==strchr(argv[1],'=')) {
	char *c = NewN( char, strlen(argv[1]) + strlen(argv[2]) + 2 );
	sprintf(c, "%s=%s", argv[1], argv[2]);
	putenv(c);

    } else if(!strcmp(argv[0], "texture")) {
	int txno = -1;
	int txflags = TXF_SCLAMP | TXF_TCLAMP | TXF_ADD;
	int txapply = TXF_DECAL;
	int qual = 7;
	char *txfname, *key;
	i = 1;
	if(argc > 1 && sscanf(argv[1], "%d", &txno) > 0)
	    i = 2;
	while(i < argc && (key = argv[i])[0] == '-' || key[0] == '+') {
	    int tqual = (strchr(key,'m') ? TXQ_MIPMAP:0) |
			(strchr(key,'l') ? TXQ_LINEAR:0) |
			(strchr(key,'n') ? TXQ_NEAREST:0);
	    if(key[0] == '-') qual &= ~tqual;
	    else qual |= tqual;
	    if(strchr(key,'a')) txflags |= TXF_ALPHA;
	    if(strchr(key,'i')) txflags |= TXF_INTENSITY;
	    if(strchr(key, 'A')) txflags |= TXF_ADD;
	    if(strchr(key, 'O')) txflags = (txflags & ~TXF_ADD) | TXF_OVER;
	    if(strchr(key, 'M')) txapply = TXF_MODULATE;
	    if(strchr(key, 'D')) txapply = TXF_DECAL;
	    if(strchr(key, 'B')) txapply = TXF_BLEND;
	    i++;
	}
	if((i < argc && txno < 0 && sscanf(argv[i++], "%d", &txno) <= 0) ||
	    (txfname = argv[i]) == NULL) {
		msg("Expected ``texture [-lmnaMDB] txno file.sgi'', got %s", line);
		msg(" opts: -l(inear) -m(ipmap) -n(earest) -i(intensity) -a(lpha) -A(dd) -O(ver(notAdd)) -M(odulate)|-D(ecal)|-B(lend))");
	} else {
	    txaddentry( &st->textures, &st->ntextures, fname, txno, txfname, txapply, txflags, qual );
	}

    } else if(!strcmp(argv[0], "polyorivar") && argc == 2) {
	if(1!=specks_set_byvariable( st, argv[1], &st->polyorivar0 ))
	    msg("polyorivar: unknown field %s", argv[1]);

    } else if(!strcmp(argv[0], "texturevar") && argc == 2) {
	if(1!=specks_set_byvariable( st, argv[1], &st->texturevar ))
	    msg("texturevar: unknown field %s", argv[1]);

    } else if(!strcmp(argv[0], "vecvar") && argc == 2) {
	if(1!=specks_set_byvariable( st, argv[1], &st->vecvar0 ))
	    msg("vecvar: unknown field %s", argv[1]);

    } else if(!strncmp(argv[0], "coord", 5) || !strncmp(argv[0], "altcoord", 8)) {
	float *t = &st->altcoord[0].w2coord.m[0];
	if(argc == 1) {
	    msg("coord %s  %g %g %g %g  %g %g %g %g  %g %g %g %g  %g %g %g %g",
		st->altcoord[0].name, t[0],t[1],t[2],t[3],
		t[4],t[5],t[6],t[7],
		t[8],t[9],t[10],t[11],
		t[12],t[13],t[14],t[15]);

	} else if(argc == 18) {
	    sprintf(st->altcoord[0].name, "%.9s", argv[1]);
	    i = getfloats( t, 16, 2, argc, argv );
	    if(i != 16) {
		msg("%s: expected 16 numbers; what's %s", argv[0],argv[i+2]);
		continue;
	    }
	} else {
	    msg("expected \"coord\" name  ... 16 world-to-coord tfm floats (GL order) ...");
	}

    } else if(!strncmp(argv[0], "dataset", 7) && argc == 3) {
	char name[12];
	int indexno;
	if(sscanf(argv[1], "%d", &indexno) <= 0
		|| sscanf(argv[2], "%11s", name) <= 0
		|| indexno < 0 || indexno >= MAXFILES) {
	    msg("%s: expected ``dataset <indexno> <datasetname>'' with 0<=indexno<=%d",
		fname, MAXFILES-1);
	    continue;
	}
	strncpyt(st->dataname[indexno], name, sizeof(st->dataname[indexno]));
	st->curdata = indexno;

    } else if(!strncmp(argv[0], "datavar", 7) && argc > 2) {
	int varno;
	struct valdesc vd;

	if(sscanf(argv[1], "%d", &varno)<=0
		|| sscanf(argv[2], "%19s", vd.name)<=0
		|| varno < 0 || varno >= MAXVAL) {
	    msg("%s: expected ``datavar <indexno> <variablename> [minval maxval]'' with 0<=indexno<=%d",
		line, MAXVAL-1);
	    continue;
	}
	strcpy(st->vdesc[st->curdata][varno].name, vd.name);
	if(argc == 5) {
	    st->vdesc[st->curdata][varno].min = atof(argv[3]);
	    st->vdesc[st->curdata][varno].max = atof(argv[4]);
	}

    } else if(!strcmp(argv[0], "datatime") && argc==2) {
	int newt = st->curtime;	/* so e.g. "datatime now" => apply to current data */
	sscanf(argv[1], "%d", &newt);
	if(sscanf(argv[1], "%d", &newt) <= 0 || newt < 0) {
	    msg("%s: datatime %s: timestep must be >= 0", fname, argv[1]);
	    continue;
	}
	if(newt == st->datatime)	/* no need to change anything */
	    continue;
	SPFLUSH();
	st->datatime = newt;

    } else if(!strcmp(argv[0], "mesh") || !strcmp(argv[0], "tstrip")
					|| !strcmp(argv[0], "tfan")) {
	specks_read_mesh(st, f, argc, argv, line);
	
    } else if(!strcmp(argv[0], "waveobj")) {
	specks_read_waveobj(st, argc, argv, line, fname);
	
	
    } else if(!strcmp(argv[0], "textcolor") && argc==2) {
	sscanf(argv[1], "%d", &s.rgba);

    } else if(!strcmp(argv[0], "maxcomment") && argc==2) {
	int newmax = st->maxcomment;
	sscanf(argv[1], "%d", &newmax);
	if(newmax != st->maxcomment) {
	    SPFLUSH();
	    st->maxcomment = newmax;
	    tsl.bytesperspeck =
		(st->maxcomment+1 + (sizeof(s) - sizeof(s.title)) + 3) & ~3;
	    maxnsp = sizeof(speckbuf) / tsl.bytesperspeck;
	}

    } else {
	struct valdesc *vdp;
	int k, m;

	k = getfloats( &s.p.x[0], 3, ignorefirst, argc, argv );
	if(k < 3) {
	    msg("Unrecognized datacmd: %s", line);
	    continue;
	}
	i = ignorefirst + k;
	vdp = &st->vdesc[st->curdata][0];
	k = getfloats( &s.val[0], COUNT(s.val), i, argc, argv );
	for(m = 0; m < k; m++) {
	    if(vdp->nsamples++ == 0) {
		vdp->min = vdp->max = s.val[m];
	    } else {
		if(vdp->min > s.val[m]) vdp->min = s.val[m];
		else if(vdp->max < s.val[m]) vdp->max = s.val[m];
	    }
	    vdp->sum += s.val[m];
	    vdp->mean = vdp->sum / vdp->nsamples;
	    vdp++;
	}

	if(maxfields < k) maxfields = k;
	m = i+k;
	s.title[0] = '\0';
	if(m < argc) {
	    if(!strcmp(argv[m], "text")) {
		s.size = 1;
		if(++m < argc-1 && !strcmp(argv[m], "-size")) {
		    sscanf(argv[m+1], "%f", &s.size);
		    m += 2;
		}
		addchunk( st, 1, SMALLSPECKSIZE(0),
			speckscale, &s,
			rejoinargs(m, argc, argv),
			SMALLSPECKSIZE(0) );
	    }
	    else if(!strcmp(argv[m], "ellipsoid")) {
		specks_read_ellipsoid( st, &s.p, argc-m, argv+m, comment );

	    } else {
		*sp = s;
		nsp++;
		sp = NextSpeck( speckbuf, &tsl, nsp );
	    }

	} else if(comment) {
	    char *title = comment + (comment[1] == ' ' ? 2 : 1);
	    *sp = s;
	    strncpyt(sp->title, title, st->maxcomment+1);
	    maxfields = MAXVAL+1;	/* "keep titles too" */
	    nsp++;
	    sp = NextSpeck( speckbuf, &tsl, nsp );

	} else {
	    *sp = s;
	    nsp++;
	    sp = NextSpeck( speckbuf, &tsl, nsp );
	}
    }
  }
  fclose(f);
  SPFLUSH();
  *stp = st;
}

/* Add a static (eternal) box to display list */
int specks_add_box( struct stuff *st, struct AMRbox *box, int timestep )
{
  int i;
  if(box->level >= 0 && st->boxlevels <= box->level)
    st->boxlevels = box->level+1;
  for(i = 0; i < st->staticboxroom && st->staticboxes[i].level >= 0; i++) {
    if(box->boxno != -1 && box->boxno == st->staticboxes[i].boxno) {
	st->staticboxes[i] = *box;
	return ~i;
    }
  }

  if(i >= st->staticboxroom-1) {
    st->staticboxroom = (st->staticboxroom>0) ? st->staticboxroom*2 : 12;
    st->staticboxes = RenewN( st->staticboxes, struct AMRbox, st->staticboxroom );
  }
  st->staticboxes[i] = *box;
  st->staticboxes[i+1].level = -1;
  return i;
}


void specks_read_boxes( struct stuff *st, char *fname, int timebase )
{
  FILE *f = fopen(fname, "rb");
  char line[1024];
  int ntimes = -1, maxbox = -1, curbox = 0, curtime = 0, level = -1, i;
  int lno = 0;
  int boxseq=1, boxno;
  struct AMRbox *boxes = NULL;
  struct AMRbox box;

  if(timebase < 0)	/* if unspecified time, start at first timestep */
    timebase = 0;

  if(f == NULL) {
    msg("Can't open AMRboxes file %s (as from hier2boxes.pl)", fname);
    return;
  }
  while(fgets(line, sizeof(line), f) != NULL) {
    lno++;
    boxno = -1;
    if(sscanf(line, "%f%f%f %f%f%f # %d",
		&box.p0.x[0],&box.p0.x[1],&box.p0.x[2],
		&box.p1.x[0],&box.p1.x[1],&box.p1.x[2], &boxno) >= 6) {
	for(i = 0; i < 3; i++) {
	    box.p0.x[i] *= st->spacescale;
	    box.p1.x[i] *= st->spacescale;
	}
	if(boxno != -1)
	    box.boxno = boxno;
	else
	    box.boxno = boxseq++;
	if(curbox < maxbox && boxes != NULL && level >= 0) {
	    boxes[curbox] = box;
	    curbox++;
	} else {
	    msg("%s line %d: Excess box (only expected %d at timestep %d)",
		fname, lno, maxbox, curtime);
	}
    }
    else if(sscanf(line, "AMRboxes %d timesteps", &ntimes) > 0 && ntimes >= 0) {
	int needtimes = ntimes + (timebase<0 ? 0 : timebase);
	if(st->boxtimes <= needtimes+1) {
	    needtimes += st->boxtimes + 15;	/* leave lots of extra room */
	    st->boxes = RenewN( st->boxes, struct AMRbox *, needtimes );
	    memset( &st->boxes[st->boxtimes], 0,
			(needtimes - st->boxtimes) * sizeof(struct AMRbox *) );
	    if(st->boxtimes < ntimes+timebase)
		st->boxtimes = ntimes+timebase;
	}
	boxes = NULL;
    }
    else if(sscanf(line, "timestep %d %d grids", &curtime, &maxbox) == 2) {
	boxseq = 1;
	if(ntimes < 0) {
	    msg(
"%s line %d: ``AMRboxes N timesteps'' must precede ``timestep'' lines!",
		    fname, lno);
	    curtime = -1;
	    boxes = NULL;
	    continue;
	} else if(curtime<0 || curtime>=ntimes || maxbox<0) {
	    msg("%s line %d: bad timestep number", fname, lno); 
	    curtime = -1;
	    boxes = NULL;
	    continue;
	}
	if((boxes = st->boxes[curtime+timebase]) != NULL) {
	    msg("%s line %d: Respecifying timestep %d",
		fname, lno, curtime);
	    Free( st->boxes[curtime+timebase] );
	}
	boxes = NewN( struct AMRbox, maxbox+1 );
	for(curbox = 0; curbox <= maxbox; curbox++)
	    boxes[curbox].level = -(maxbox+1);
	st->boxes[curtime+timebase] = boxes;

	/* Ensure that known span of time includes this timestep */
	specks_ensuretime( st, st->curdata, curtime+timebase );

	curbox = 0;
    }
    else if(sscanf(line, "level %d", &level) > 0) {
	if(boxes == NULL) {
	    msg("%s line %d: Must specify timestep before level",
		fname, lno);
	    level = -1;
	}
	if(st->boxlevels <= level)
	    st->boxlevels = level+1;
	box.level = level;
    }
  }
  fclose(f);

  /* st->boxlevelmask |= (1 << st->boxlevels) - 1;
   * Don't do this -- it turns on all boxlevels whenever we get a new box!
   */
}

static struct AMRbox *findboxno( struct AMRbox *box, int boxno, int *count, int *bmin, int *bmax ) {
  if(box == NULL) return NULL;
  while(box->level >= 0) {
    if(box->boxno == boxno)
	return box;
    if(*bmin > box->boxno) *bmin = box->boxno;
    if(*bmax < box->boxno) *bmax = box->boxno;
    ++*count;
    box++;
  } 
  return NULL;
}

int specks_gobox( struct stuff *st, int boxno, int argc, char *argv[] )
{
  Point pmid;
  int bmin = 1<<30, bmax = -1<<30;
  int count = 0;
  struct AMRbox *box = NULL;

  if(st->boxes && st->curtime < st->boxtimes)
    box = findboxno( st->boxes[st->curtime], boxno, &count, &bmin, &bmax );
  if(box == NULL)
    box = findboxno( st->staticboxes, boxno, &count, &bmin, &bmax );

  if(box == NULL) {
    if(count == 0)
	msg("No AMR boxes for timestep %d", st->curtime);
    else
	msg("%d AMR boxes (numbered %d..%d) for timestep %d, but no box number %d",
	    count, bmin, bmax, st->curtime, boxno);
    return 0;
  }

  vlerp( &pmid, .5*st->gscale, &box->p0, &box->p1 );
  parti_center( &pmid );

		/* round to 1 decimal */
#if THIEBAUX_VIRDIR
  {
    Point pmid, fwd, headv, pos, offset, newpos;
    static Point zvec = {0,0,1};
    float sz, scale;
    char cmd[128];
    sz = vdist( &box->p0, &box->p1 );
    sprintf(cmd, "%.1g", st->gscale * st->goboxscale * sz);
    scale = atof(cmd);
    CAVENavConvertVectorCAVEToWorld( zvec.x, fwd.x );
    CAVEGetPosition( CAVE_HEAD, headv.x );
    headv.x[2] -= 10.0;	/* a point in front of head, in CAVE coords */
    CAVENavConvertVectorCAVEToWorld( headv.x, offset.x );
    msg("scale %g caveunit %g offsetW %g %g %g", scale, vlength(&fwd), offset.x[0],offset.x[1],offset.x[2]);
    vsadd( &newpos, &pmid, -scale / vlength(&fwd), &offset );
    sprintf(cmd,
	sz > 0	? "jumpto %g %g %g  . . .  %g"
		: "jumpto %g %g %g",
	newpos.x[0], newpos.x[1], newpos.x[2], scale);
    VIDI_queue_commandstr( cmd );
  }
#endif

  return 1;
}

void specks_read_cmap( struct stuff *st, char *fname, int *ncmapp, struct cment **cmapp )
{
  int i;
  char *cp;
  char line[256];
  int k, count = 0;
  int *tcmap;
  struct cment *cm;
  float fr,fg,fb,fa;
  int big = 0;
  int lno = 0;
  int ncmap = 0;
  int cval;
  int csrc;

  FILE *f = fopen(fname, "rb");
  if(f == NULL) {
    msg("Can't open colormap file %s", fname);
    return;
  }
  tcmap = NULL;

  ncmap = 0;
  while(fgets(line, sizeof(line), f) != NULL) {
    lno++;
    cp = line;
   rescan:
    for( ; isspace(*cp); cp++)
	;
    if(*cp == '\0' || *cp == '#')
	continue;
    fa = 1.0;
    k = sscanf(cp, "%f%f%f", &fr,&fg,&fb/*,&fa*/);
    if(k == 1) {
	if(count == 0 && fr == (int)fr && fr > 0) {
	    count = (int) fr;
	    if(count >= 1 && count < 1000000)		/* OK */
		continue;
	    msg("Unreasonable number of colormap entries claimed in header");
	    /* and fall into error case */
	} else {
	    while(isspace(*cp) || isdigit(*cp)) cp++;
	    if(*cp == ':') {
		ncmap = (int)fr;
		if(ncmap < 0) {
		    ncmap = 0;		/* don't continue -- fall into error */
		} else if(ncmap >= count) {
		    msg("Colormap index %g exceeds size %d given in header",
			fr, count);	/* don't continue -- ditto */
		} else {
		    cp++;
		    goto rescan;		/* All's well */
		}
	    }
	}
    } else if(k >= 3) {
	if(fr > 2 || fg > 2 || fb > 2 || fa > 2)
	    big = 1;
	if(!big) {
	    fr *= 255; fg *= 255; fb *= 255; /*fa *= 255;*/
	}
	cval = PACKRGBA( (int)fr, (int)fg, (int)fb, 0 );
	if(tcmap == NULL) {
	    tcmap = NewA( int, count );
	    /* Fill unused entries with gray */
	    memset(tcmap, 0x80, count*sizeof(int));
	}
	tcmap[ncmap>=count ? count-1 : ncmap<0 ? 0 : ncmap] = cval;
	ncmap++;
	if(ncmap >= count)
	    break;
	continue;

    } else if(sscanf(cp, "=%d", &csrc) > 0) {
	if(csrc < 0 || csrc >= count || ncmap < 0 || ncmap >= count || tcmap == NULL) {
	    msg("Colormap index out of range: %d or %d isn't in 0..%d",
		ncmap, csrc, count-1);
	} else {
	    tcmap[ncmap++] = tcmap[csrc];
	    continue;
	}
	/* Fall through into error */
    }
    msg("Trouble reading colormap file %s: bad line %d: %s",
		fname, lno, line);
    fclose(f);
    return;
  }
  fclose(f);
  if(tcmap == NULL)
    return;

  *ncmapp = count;
  if(*ncmapp < 2) {
    *ncmapp = 2;
    tcmap[1] = tcmap[0];
  }
  *cmapp = cm = RenewN( *cmapp, struct cment, *ncmapp );
  k = *ncmapp;
  for(i = 0; i < k; i++) {
    cm[i].raw = tcmap[i];
    cm[i].cooked = specks_cookcment( st, tcmap[i] );
  }
}



int specks_set_byvariable( struct stuff *st, char *str, int *val )
{
  int i;
  char *ep;
  int best = -1;
  if(str == NULL || str[0] == '\0') return 0;

  if(!strcasecmp( str, "const" ) || !strcasecmp( str, "constant" )
				 || !strcasecmp( str, "rgb" )) {
    *val = CONSTVAL;
    return -1;
  }
	
  for(i = 0; i < MAXVAL; i++) {
    if(strncasecmp( str, st->vdesc[st->curdata][i].name, strlen(str) ) == 0) {
	best = i;
	if(!strcmp( str, st->vdesc[st->curdata][i].name ))
	    break;
    }
  }
  if(best >= 0) {
    *val = best;
    return 1;
  }
  i = strtol(str, &ep, 0);
  if(ep == str || i < 0 || i > MAXVAL || (*ep != '\0' && *ep != '('))
    return 0;
  *val = i;
  return 1;
}

static char *putcoords( char *buf, Point *pos, Matrix *T,
		int isvec, char *cartfmt,
		char *lonlatfmt, char *hmdmfmt )
{
  Point p;
  char *cp = buf;
  float lat, lon, r;
  if(T == NULL) T = &Tidentity;
  buf[0] = '\0';
  if(isvec)
    vtfmvector( &p, pos, T );
  else vtfmpoint( &p, pos, T );
  if(cartfmt)
    cp += sprintf(cp, cartfmt, p.x[0],p.x[1],p.x[2]);
  lon = atan2(p.x[1], p.x[0]) * 180/M_PI;
  if(lon < 0) lon += 360;
  lat = atan2(p.x[2], hypot(p.x[1],p.x[0])) * 180/M_PI;
  r = vlength(&p);
  if(lonlatfmt)
    cp += sprintf(cp, lonlatfmt, lon, lat, r);
  if(hmdmfmt) {
    CONST char *sign = (lat < 0) ? "-" : "+";
    lat = fabs(lat);
    lon /= 15;
    cp += sprintf(cp, hmdmfmt, (int)lon, 60. * (lon - (int)lon),
				sign, (int)lat, 60. * (lat - (int)lat),
				r);
  }
  return buf;
}

char *whereis(struct stuff *st, char *buf, Point *pos, int isvec)
{
  putcoords(buf, pos, NULL, isvec, "%.7g %.7g %.7g", NULL, NULL);
  if(strstr(st->altcoord[0].name, "2000")
	|| strstr(st->altcoord[0].name, "1950")
	|| strstr(st->altcoord[0].name, "eq")) {

    sprintf(buf + strlen(buf), "; %.9s: ", st->altcoord[0].name);
    putcoords(buf+strlen(buf), pos, &st->altcoord[0].w2coord, isvec,
		NULL, NULL, "%02d:%02.0f %s%02d:%02.0f %g");

  } else if(st->altcoord[0].name[0] != '\0') {
    sprintf(buf + strlen(buf), "; %.9s: ", st->altcoord[0].name);
    putcoords(buf+strlen(buf), pos, &st->altcoord[0].w2coord, isvec,
		NULL, "%.4f %.4f %g", NULL);
  }
  return buf;
}


static Point zero = {0,0,0}, forward = {0,0,-1};


static void tellwhere(struct stuff *st)
{
    Point pos, fwd, objpos, objfwd;
    float aer[3], scl;
    Matrix cam2w, obj2w, w2obj, cam2obj;
    int id;
    char buf[180], obuf[64];

#ifdef THIEBAUX_VIRDIR
    CAVENavConvertCAVEToWorld( zero.x, pos.x );
    msg("cave at %s", whereis(st, buf, &pos, 0));
    CAVEGetPosition( CAVE_HEAD_NAV, pos.x );
    msg("head at %s", whereis(st, buf, &pos, 0));
    CAVEGetPosition( CAVE_WAND_NAV, pos.x );
    msg("wand at %s", whereis(st, buf, &pos, 0));
    CAVEGetVector( CAVE_WAND_FRONT_NAV, fwd.x );
    VD_get_cam2world_matrix( cam2w.m );
#else
    parti_getc2w( &cam2w );
#endif

    id = parti_idof( st );
    parti_geto2w( st, id, &obj2w );
    eucinv( &w2obj, &obj2w );

    scl = tfm2xyzaer( &pos, aer, &cam2w );
    vtfmpoint( &objpos, &pos, &w2obj );
    vtfmvector( &fwd, &forward, &cam2w );
    vtfmvector( &objfwd, &fwd, &w2obj );
    vunit( &objfwd, &objfwd );
    msg("camera at %s (w) %s (g%d)", whereis(st, buf, &pos, 0), whereis(st, obuf, &objpos, 0), id);
    msg("looking to %s (w) %s (g%d)", whereis(st, buf, &fwd, 1), whereis(st, obuf, &objfwd, 1), id);
    msg("jump %g %g %g  %g %g %g  %g",
	pos.x[0],pos.x[1],pos.x[2],
	aer[1],aer[0],aer[2],
	scl);
    msg("c2w: %g %g %g %g  %g %g %g %g  %g %g %g %g  %g %g %g %g",
	cam2w.m[0], cam2w.m[1], cam2w.m[2], cam2w.m[3],
	cam2w.m[4], cam2w.m[5], cam2w.m[6], cam2w.m[7],
	cam2w.m[8], cam2w.m[9], cam2w.m[10], cam2w.m[11],
	cam2w.m[12], cam2w.m[13], cam2w.m[14], cam2w.m[15]);
    mmmul( &cam2obj, &cam2w, &w2obj );
    msg("c2obj: %g %g %g %g  %g %g %g %g  %g %g %g %g  %g %g %g %g",
	cam2obj.m[0], cam2obj.m[1], cam2obj.m[2], cam2obj.m[3],
	cam2obj.m[4], cam2obj.m[5], cam2obj.m[6], cam2obj.m[7],
	cam2obj.m[8], cam2obj.m[9], cam2obj.m[10], cam2obj.m[11],
	cam2obj.m[12], cam2obj.m[13], cam2obj.m[14], cam2obj.m[15]);
}


int getbool( char *str, int defval ) {
  int v;
  char *ep;
  if(str == NULL) return defval;

  if(!strcasecmp(str, "on")) return 1;
  if(!strcasecmp(str, "off")) return 0;
  if(!strcasecmp(str, "toggle")) return !defval;
  if(!strcasecmp(str, "all")) return -1;
  v = strtol(str, &ep, 0);
  if(str == ep) return defval;
  return v;
}

double getfloat( char *str, double defval ) {
  double v;
  char *ep;
  int prefix = 0;
  if(str == NULL) return defval;
  if(str[0] == '-' && (str[1] == '=' || str[1] == '-'))	/* -=, -- */
    prefix = *str++;
  if(str[0] == '*' || str[0] == '+' || str[0] == '/' || str[0] == 'x')
    prefix = *str++;
  if(str[0] == '=')
    str++;
  v = strtod(str, &ep);
  if(ep == str) {
    v = defval;
  } else {
    switch(prefix) {
    case '-': v = defval - v; break;
    case '+': v = defval + v; break;
    case '*': v = defval * v; break;
    case '/': v = (v != 0) ? defval / v : 0; break;
    }  /* default: just use v */
  }
  return v;
}

int getfloats( float *v, int nfloats, int arg0, int argc, char **argv ) {
  int i;
  char *ep;
  for(i = 0; i < nfloats && arg0+i < argc; i++) {
    float tv = strtod( argv[arg0+i], &ep );
    if(ep == argv[arg0+i])
	break;
    v[i] = tv;
  }
  return i;
}

void editcmap( struct stuff *st, int ncmap, struct cment *cmap, char *range,
		char *whose, char *colorstr )
{
    int i, min, max;
    char *cp;
    struct cment cm;

    cp = colorstr;
    while(isspace(*cp)) cp++;
    if(*cp == ':' || *cp == '=') {
	char junk;
	do cp++; while(*cp == '=');
	if(sscanf(cp, "%d%c", &i, &junk) != 1) {
	    msg("%s: expected entrynumber(s) := oldindex, not %s", whose, colorstr);
	    return;
	}
	if(i < 0 || i >= ncmap) {
	    msg("%s: clamping index %d to 0..%d", whose, i, ncmap-1);
	    i = (i < 0) ? 0 : ncmap-1;
	}
	cm = cmap[i];
    } else {
	float r, g, b;
	if(sscanf(cp, "%f%f%f", &r,&g,&b) != 3) {
	    msg("%s: expected entrynumber(s) r g b, not %s", whose, colorstr);
	    return;
	}
	cm.raw = PACKRGBA( (int)(r*255), (int)(g*255), (int)(b*255), 0 );
	cm.cooked = specks_cookcment( st, cm.raw );
    }

    for(cp = range; ; cp++) {
	while(isspace(*cp) || *cp == ',') cp++;
	if((i = sscanf(cp, "%d-%d", &min, &max)) > 0) {
	    if(i == 1) max = min;
	} else if((i = sscanf(cp, ">%d", &min)) > 0) {
	    max = ncmap-1;
	} else if((i = sscanf(cp, "<%d", &max)) > 0) {
	    min = 0;
	}
	if(i > 0) {
	    if(min < 0) min = 0;
	    if(max >= ncmap) max = ncmap-1;
	    for(i = min; i <= max; i++)
		cmap[i] = cm;
	}
	while(*cp != ',' && !isspace(*cp)) {
	    if(*cp == '\0') return;
	    cp++;
	}
    }
}

int
specks_parse_args( struct stuff **stp, int argc, char *argv[] )
{
  int i;
  struct stuff *st = *stp;

  if(parti_parse_args( stp, argc, argv, NULL ) > 0)
    return 1;

  while( argc>0 &&
	(!strncmp( argv[0], "specks", 4 ) ||
	 !strcmp( argv[0], "feed" ) ||
	 !strcmp( argv[0], "eval" )) ) {
    argc--, argv++;
    /* VD_select_menu( specks_menuindex ); */
  }

  if(argc <= 0)
    return 0;

  if( st->dyn.enabled && st->dyn.ctlcmd &&
		(*st->dyn.ctlcmd)( &st->dyn, st, argc, argv ) ) {
    /* OK, dyn command handled it */

  } else if(!strcmp( argv[0], "?" ) || !strcmp( argv[0], "help" )) {
    static char *help1[] = {
"specks commands:",
" speed   data-steps per VirDir second",
" step N  -or-  step +N  -or-  step -N  Go to data step N, or step fwd/back",
" trange on|off|MIN MAX [WRAP]	limit range of datastep times",
" run				toggle auto-play (run/step)",
" color VARNO-or-NAME		color particles by VARNO'th variable (0..%d)",
" color const R G B		set all particles to be that color",
" lum   VARNO-or-NAME		tie particle size/luminosity to VARNOth var",
" lum   const LUM		set all particles to be brightness LUM",
" slum  SCALEFACTOR		scale particle brightness by SCALEFACTOR",
" psize SIZE			scale particle brightness by SIZE * SCALEFACTOR",
" depthsort			sort polygons by depth",
" see   DATASETNO-or-NAME	show that dataset (e.g. \"seedata 0\" or \"seedata gas\")",
" read  [-t time] DATAFILENAME	read data file (e.g. to add new specks)",
" ieee  [-t time] IEEEIOFILE	read IEEEIO file (starting at given timestep)",
" sdb   [-t time] SDBFILE	read .sdb star-data file",
" annot [-t time] string	set annotation string (for given timestep)",
" add  DATAFILECOMMAND		enter a single datafile command (ditto)",
" every N			subsample: show every Nth particle",
" bound				show bounds (coordinate range of all particles)",
" clipbox {on | off | X0,X1 Y0,Y1 Z0,Z1 | CENX,Y,Z RADX,Y,Z | X0 Y0 Z0  X1 Y1 Z1} clipping region",
" add box [-n boxno] [-l level] CENX,Y,Z RX,RY,RZ | X0 Y0 Z0 X1 Y1 Z1  marker-box",
" boxlabels                     show box numbers",
" boxaxes                       show R/G/B axes from X0,Y0,Z0 box corner"
" boxes {off|on|only}		hide/show all AMR boxes",
" {hide|show} LEVELNO ...	hide/show AMR boxes of those levels (or \"all\")",
" {point|polygon|texture} {on|off}",
#if CAVEMENU
" fmenu HEIGHT  -or-  fmenu XPOS YPOS  -or- fmenu wall WALLNO",
#else
" readpath  FILENAME.wf		read Wavefront camera path (from virdir \"wfout\")",
" play  SPEED[f]		play path (at SPEED times normal speed)",
"			(with \"f\" suffix, play every SPEEDth frame)",
" frame FRAMENO			go to Nth frame",
" focal FOCALLEN		focal length (determines fly/tran speed)",
" clip  NEAR FAR		clipping distances",
" jump  X Y Z [RX RY RZ]	put viewpoint there",
" center X Y Z			set center of rotation for orbit/rotate",
" censize RADIUS		size of center marker",
" snapset  filestem [frameno]	set snapshot parameters",
" snapshot [frameno]		take snapshot [uses convert(1)]",
" kira {node|ring|size|scale|span|track}  starlab controls; try \"kira ?\"",
#endif
    };

    for(i = 0; i < COUNT(help1); i++)
	msg(help1[i], MAXVAL-1);

    if(st->dyn.enabled && st->dyn.help)
	(*st->dyn.help)(&st->dyn, st, 0);
  

  } else if(!strcmp( argv[0], "read" )) {
	if(argc > 1) {
	    char *foundfile = findfile( NULL, argv[1] );
	    if(foundfile == NULL)
		msg("read: can't find file %s", argv[1]);
	    else
		specks_read( &st, foundfile );
	}

  } else if(!strcmp( argv[0], "include" )) {
#ifdef NOTYET
#endif

  } else if( !strcmp(argv[0], "on") || !strcmp(argv[0], "off")
	    || !strcmp(argv[0], "enable") || !strcmp(argv[0], "disable") ) {

	st->useme = argc>1 ? getbool(argv[1], st->useme) : (argv[0][1]=='n');
	msg(st->useme ? "enabled" : "disabled");
	
  } else if(!strcmp(argv[0], "echo")) {
	msg( "%s", rejoinargs(1, argc, argv) );

  } else if(!strcmp(argv[0], "add")) {
	int k, io[2];
	FILE *tf;
	char fdname[64+L_tmpnam];

#ifdef WIN32
	tmpnam(fdname);
	tf = fopen(fdname, "w");
#else /* unix */
	if(pipe(io) < 0) {
	    msg("add: can't make pipe?");
	    tf = NULL;
	} else {
	    sprintf(fdname, "/dev/fd/%d", io[0]);
	    tf = fdopen(io[1], "w");
	}
#endif
	if(tf == NULL) {
	    fprintf(stderr, "Yeow: can't make temp file?\n");
	} else {
	    for(k = 1; k < argc; k++)
		fprintf(tf, "%s ", argv[k]);
	    fprintf(tf, "\n");
	    fclose(tf);
	    specks_read( &st, fdname );
#ifdef WIN32
	    //unlink(fdname);
#endif
	}

#ifndef WIN32
	close(io[0]);
#endif

  } else if(!strcmp( argv[0], "async" ) ||
	    !strcmp( argv[0], "|" )) {
	specks_add_async( st, rejoinargs( 1, argc, argv ),
				argv[0][0] == '|' );

  } else if(!strcmp( argv[0], "update" )) {
	parti_update();

  } else if(!strcmp( argv[0], "hist" )) {
	register struct specklist *sl;
	register struct speck *sp;
	int nspecks, nclipped, nthreshed, nlow, nhigh, nundefined;
	int clipping = (st->clipbox.level > 0);
	int threshing = (st->usethresh & P_USETHRESH) && (st->seesel.wanted != 0);
	int nbuckets = 11;
	int dolog = 0;
	int *bucket;
	float v, vmin, vmax, vrange;
	int histvar;
	int i, k, bno;
	struct valdesc *vd;

	for(i=1,k=1; i+1 < argc; k++) {
	    int yes = argv[i][0] == '-';
	    if(argv[i][0] != '-' && argv[i][1] != '+') break;
	    switch(argv[i][k]) {
		case 't': threshing = yes ? THRESHBIT : 0; break;
		case 'c':
		case 'b': clipping = yes; break;
		case 'n':
		    sscanf(argv[i][++k] ? &argv[i][k] : argv[++i],
			"%d", &nbuckets);
		    i++; k=0;
		    break;
		case 'l': dolog = yes; break;
		case '\0': i++; k=0;
		default: i=argc; break;
	    }
	}

	if(i>=argc) {
	    msg("Usage: hist [-t|+t(threshed)] [-c|+c(clipped)] [-n nbuckets] [-l(logscale)]  datavar [min max]");
	    return 1;
	}
	if(!specks_set_byvariable( st, argv[i], &histvar )) {
	    msg("hist %s: expected name/index of data variable", argv[i]);
	    return 1;
	}

	vd = &st->vdesc[st->curdata][histvar];
	vmin = vd->min;
	vmax = vd->max;

	if(i+1<argc) sscanf(argv[i+1], "%f", &vmin);
	if(i+2<argc) sscanf(argv[i+2], "%f", &vmax);

	if(vmax < vmin)
	    v = vmax, vmax = vmin, vmin = v;

	if(dolog && (vmin <= 0 || vmax <= 0)) {
	    msg("hist: can't take logs (-l) if range includes zero!");
	    dolog = 0;
	}
	if(dolog) {
	    vmin = log(vmin);
	    vmax = log(vmax);
	}
	vrange = (nbuckets-1) / ((vmax>vmin) ? vmax - vmin : 1);

	if(nbuckets<=0 || nbuckets>20000) {
	    msg("hist -n %d: Incredible number of histogram buckets", nbuckets);
	    return -1;
	}

	bucket = NewA(int, nbuckets);
	memset(bucket, 0, nbuckets*sizeof(int));

	nspecks = nclipped = nthreshed = nundefined = nlow = nhigh = 0;
	for(sl = st->sl; sl != NULL; sl = sl->next) {
	    if((sp = sl->specks) == NULL || sl->text != NULL)
		continue;
	    nspecks += sl->nspecks;
	    if(sl->bytesperspeck < SMALLSPECKSIZE(histvar+1)) {
		nundefined += sl->nspecks;
		continue;
	    }
	    for(i = sl->nspecks; --i >= 0; sp = NextSpeck( sp, sl, 1 )) {
		if(clipping &&
		  (sp->p.x[0] < st->clipbox.p0.x[0] ||
		   sp->p.x[0] > st->clipbox.p1.x[0] ||
		   sp->p.x[1] < st->clipbox.p0.x[1] ||
		   sp->p.x[1] > st->clipbox.p1.x[1] ||
		   sp->p.x[2] < st->clipbox.p0.x[2] ||
		   sp->p.x[2] > st->clipbox.p1.x[2])) {
			nclipped++;
			continue;
		}
		if(!SELECTED(sl->sel[i], &st->seesel)) {
		    nthreshed++;
		    continue;
		}
		v = sp->val[histvar];
		if(dolog) {
		    if(v <= 0) {
			nundefined++;
			continue;
		    }
		    v = log(v);
		}
		bno = (int) ((v - vmin)*vrange);
		if(bno < 0) nlow++;
		else if(bno >= nbuckets) nhigh++;
		else bucket[bno]++;
	    }
	}
	if(nspecks == 0) {
	    msg("No specks loaded yet");
	} else {
	    msg("hist -n %d %s%s%s%d(%s) %g %g => ",
		nbuckets, dolog?"-l ":"", clipping?"-c ":"", threshing?"-t ":"",
		histvar, vd->name,
		dolog ? exp(vmin) : vmin,
		dolog ? exp(vmax) : vmax);
	    msg("Total %d, %d < min, %d > max, %d undefined, %d clipped, %d threshed",
		nspecks, nlow, nhigh, nundefined, nclipped, nthreshed);
	    k = nlow;
	    msg("%d\t< %g", nlow, dolog ? exp(vmin) : vmin);
	    for(i = 0; i < nbuckets; i++) {
		v = vmin + ( (vrange>0) ? i / vrange : 0 );
		msg("%d\t>= %g", bucket[i], dolog ? exp(v) : v);
	    }
	    msg("%d\t> %g", nhigh, dolog ? exp(vmax) : vmax);
	}


  } else if(!strcmp( argv[0], "bound" )) {
	register struct specklist *sl;
	register struct speck *sp;
	Point xmin,xmax, mean, mid, radius;
	int minmaxk = MAXVAL+1, maxmaxk = 0;
	int nspecks = 0;
	Matrix To2w;
	int hasT = 0;
	if(argc > 1) {
	    hasT = 1;
	    parti_geto2w( st, parti_idof( st ), &To2w );
	}

	xmin.x[0] = xmin.x[1] = xmin.x[2] = 1e38;
	xmax.x[0] = xmax.x[1] = xmax.x[2] = -1e38;
	mean.x[0] = mean.x[1] = mean.x[2] = 0;
	for(sl = st->sl; sl != NULL; sl = sl->next) {
	    int i, maxk;
	    if((sp = sl->specks) == NULL)
		continue;
	    for(maxk=0; maxk<MAXVAL && SMALLSPECKSIZE(maxk)<sl->bytesperspeck; maxk++)
		;
	    if(minmaxk > maxk) minmaxk = maxk;
	    if(maxmaxk < maxk) maxmaxk = maxk;
	    for(i = sl->nspecks; --i >= 0; sp = NextSpeck( sp, sl, 1 )) {
		Point tp, *p = &sp->p;
		if(hasT) {
		    vtfmpoint( &tp, p, &To2w );
		    p = &tp;
		}
		if(xmin.x[0] > p->x[0]) xmin.x[0] = p->x[0];
		if(xmax.x[0] < p->x[0]) xmax.x[0] = p->x[0];
		mean.x[0] += p->x[0];
		if(xmin.x[1] > p->x[1]) xmin.x[1] = p->x[1];
		if(xmax.x[1] < p->x[1]) xmax.x[1] = p->x[1];
		mean.x[1] += p->x[1];
		if(xmin.x[2] > p->x[2]) xmin.x[2] = p->x[2];
		if(xmax.x[2] < p->x[2]) xmax.x[2] = p->x[2];
		mean.x[2] += p->x[2];
	    }
	    nspecks += sl->nspecks;
	}
	if(nspecks == 0) {
	    msg("No specks loaded yet");
	} else {
	    CONST char *which = hasT ? " (world)" : " (object)";
	    vscale( &mean, 1.0/nspecks, &mean );
	    vcomb( &mid, .5,&xmin, .5,&xmax );
	    vcomb( &radius, .5,&xmax, -.5,&xmin );
	    msg( "%d specks in range %g %g %g .. %g %g %g%s",
		nspecks, xmin.x[0],xmin.x[1],xmin.x[2],
		xmax.x[0],xmax.x[1],xmax.x[2], which);
	    msg( "midbbox %g %g %g  boxradius %g %g %g%s",
		mid.x[0],mid.x[1],mid.x[2],
		radius.x[0],radius.x[1],radius.x[2], which);
	    msg( "mean %g %g %g%s", mean.x[0],mean.x[1],mean.x[2], which );
	}

  } else if(!strcmp( argv[0], "fspeed" )) {
	if(argc>1) {
	    specks_set_fspeed( st, getfloat(argv[1], st->fspeed) );
	    st->playnext = 0.0;
	}
	msg("fspeed %g steps per real-time second", st->fspeed);

  } else if(!strcmp( argv[0], "speed" )) {
	if(argc>1)
	    specks_set_speed( st, getfloat(argv[1], clock_speed(st->clk)) );
	msg("speed %g steps per anim second", clock_speed(st->clk));

  } else if(!strcmp( argv[0], "timealign" )) {

	float datatime = 0, realtime = 0;
	/* Compute timebase so datastep <datatime> falls at <clocktime>,
	 * given current speed setting.
	 */
	double speed = clock_speed(st->clk) * st->clk->fwd;
	double timebase;
	if(getfloats( &datatime, 1, 1, argc,argv ) > 0 &&
		getfloats( &realtime, 1, 2, argc,argv ) > 0) {
	    specks_set_timebase( st, datatime - realtime*speed );
	    msg("Aligning datastep %g at realtime %g", datatime, realtime);
	} else if(argc>1) {
	    msg("Usage: timealign <datatime> <realtime>");
	}
	timebase = clock_timebase(st->clk);
	if(speed == 0)
	    msg("timebase %g (always at that datastep since speed=0)", timebase);
	else
	    msg("timebase %g (datastep at realtime 0), or datastep %g at realtime %g",
		timebase, datatime, (datatime - timebase) / speed);

  } else if(!strcmp( argv[0], "timebase" )) {
	if(argc>1)
	    specks_set_timebase( st, getfloat(argv[1], clock_timebase(st->clk)) );
	msg("timebase %g (datastep at time 0)", clock_timebase(st->clk));

  } else if(!strcmp( argv[0], "skipblanktimes" )) {
	if(argc>1)
	    st->skipblanktimes = getbool(argv[1], st->skipblanktimes);
	msg("blanktimes %s", st->skipblanktimes ? "on (skip blank timesteps)"
					: "off (show blankness at blank timesteps)");
	
  } else if(!strcmp( argv[0], "run" )) {
	parti_set_running( st, getbool( argv[1], 1 ) );
	st->playnext = 0.0;

  } else if(!strcmp( argv[0], "depthsort" )) {
	if(argc > 1) st->depthsort = getbool(argv[1], st->depthsort);
	msg("depthsort %s", st->depthsort ? "on" : "off" );

  } else if(!strcmp( argv[0], "fade" )) {
	char *fmt = "fade what?";
	if(argc>1) {
	    if(!strncmp(argv[1],"sph",3) || !strncmp(argv[1],"rad",3))
		st->fade = F_SPHERICAL;
	    else if(!strncmp(argv[1],"pla",3))
		st->fade = F_PLANAR;
	    else if(!strncmp(argv[1],"con",3) || !strncmp(argv[1],"ort",3))
		st->fade = F_CONSTANT;
	    else if(!strncmp(argv[1],"knee",3))
		st->fade = F_KNEE12;
	    else if(!strncmp(argv[1],"lin",3))
		st->fade = F_LINEAR;
	    else {
		msg("fade {sph|planar|const|linear|knees}");
		return -1;
	    }
	}
	if(argc>2) sscanf(argv[2], "%f", &st->fadeknee2);
	if(argc>3) sscanf(argv[3], "%f", &st->knee2steep);
	if(argc>4) sscanf(argv[4], "%f", &st->fadeknee1);
	if(argc>5) sscanf(argv[5], "%f%*c%f%*c%f",
			&st->fadecen.x[0], &st->fadecen.x[1], &st->fadecen.x[2]);
	if(argc>6) sscanf(argv[6], "%f", &st->fadecen.x[1]);
	if(argc>7) sscanf(argv[7], "%f", &st->fadecen.x[2]);
	switch(st->fade) {
	case F_SPHERICAL: fmt = "fade spherical  (1/r^2 from eyepoint)"; break;
	case F_PLANAR:    fmt = "fade planar     (1/r^2 from eye plane)"; break;
	case F_CONSTANT:  fmt = "fade const %g   (as if seen at given dist)"; break;
	case F_LINEAR:    fmt = "fade linear %g  (1/r, scaled to match planar at dist)"; break;
	case F_KNEE12:	  fmt = "fade knees %g %g  %g (fardist, steepness, neardist)"; break;
	case F_LREGION:	  fmt = "fade lregion %g  %g %g  %g %g %g (refdist; steepness, Rregion)"; break;
	}
	msg(fmt, st->fadeknee2, st->knee2steep, st->fadeknee1,
		st->fadecen.x[0], st->fadecen.x[1], st->fadecen.x[2]);

  } else if(!strcmp( argv[0], "clipbox" ) || !strcmp( argv[0], "cb" )) {
	Point cen, rad;
	int k;
	switch(argc) {
	case 2:	st->clipbox.level = getbool(argv[1], st->clipbox.level);
		if(!strcmp(argv[1], "hide")) st->clipbox.level = -1;
		break;
	case 3: if(3==sscanf(argv[1], "%f%*c%f%*c%f",
				&cen.x[0],&cen.x[1],&cen.x[2])
			&& 0 < (k = sscanf(argv[2], "%f%*c%f%*c%f",
				&rad.x[0],&rad.x[1],&rad.x[2]))) {
		    if(k==1) rad.x[1] = rad.x[2] = rad.x[0];
		    vsub( &st->clipbox.p0, &cen, &rad );
		    vadd( &st->clipbox.p1, &cen, &rad );
		    if(st->clipbox.level==0)
			st->clipbox.level = 1;	/* activate */
		} else {
		    msg("clipbox: xmin,xmax ymin,ymax zmin,zmax  or cenx,y,z radiusx,y,z");
		}
		break;
	case 4:
		if(2 == sscanf(argv[1], "%f%*c%f", &cen.x[0], &rad.x[0]) &&
		   2 == sscanf(argv[2], "%f%*c%f", &cen.x[1], &rad.x[1]) &&
		   2 == sscanf(argv[3], "%f%*c%f", &cen.x[2], &rad.x[2])) {
		    st->clipbox.p0 = cen;
		    st->clipbox.p1 = rad;
		    if(st->clipbox.level==0)
			st->clipbox.level = 1;	/* activate */
		} else {
		    msg("clipbox: xmin,xmax ymin,ymax zmin,zmax  or cenx,y,z radiusx,y,z");
		}
		break;

	case 7: for(k = 0; k < 3; k++) {
		    if(sscanf(argv[k+1], "%f", &st->clipbox.p0.x[k]) <= 0
			|| sscanf(argv[k+4], "%f", &st->clipbox.p1.x[k]) <= 0)
			break;
		}
		if(k == 3) {
		    if(st->clipbox.level == 0)
			st->clipbox.level = 1;
		} else {
		    msg("clipbox: xmin ymin zmin  xmax ymax zmax");
		}
		break;
	default:
		msg("clipbox: on|off|hide | x0,x1 y0,y1 z0,z1 | Cenx,y,z Rx,y,z | x0 y0 z0 x1 y1 z1");
		break;
	}
	vcomb( &cen, .5, &st->clipbox.p0, .5, &st->clipbox.p1 );
	vcomb( &rad, -.5, &st->clipbox.p0, .5, &st->clipbox.p1 );
	msg("clipbox %s  (%g,%g  %g,%g  %g,%g  or  %g,%g,%g %g,%g,%g)",
		st->clipbox.level ? (st->clipbox.level<0 ? "hide":"on") :"off",
		st->clipbox.p0.x[0], st->clipbox.p1.x[0],
		st->clipbox.p0.x[1], st->clipbox.p1.x[1],
		st->clipbox.p0.x[2], st->clipbox.p1.x[2],
		cen.x[0],cen.x[1],cen.x[2],  rad.x[0],rad.x[1],rad.x[2]);

  } else if(!strcmp( argv[0], "object" ) || sscanf( argv[0], "g%d", &i ) > 0) {
	struct stuff *tst = st;
	int a = argv[0][0]=='g' ? 0 : 1;
	char *ep;
	i = parti_object( argv[a], &tst, 0 );
	ep = argv[a] ? strchr(argv[a], '=') : NULL;

	if(a && argc==1) {
	    msg(tst->alias ? "object g%d=%s" : "object g%d", i, tst->alias);

	} else if(i<0) {
	    /* never mind */

	} else if(ep) {
	    parti_set_alias( tst, ep+1 );
	    msg("g%d = %s", i, ep+1);

	} else if(a+2 == argc && argv[a+1][0] == '=') {
	    parti_set_alias( tst, argv[a+2] );
	    msg("g%d = %s", i, argv[a+2]);

	} else if(a+1 < argc) {
	    specks_parse_args( &tst, argc-a-1, argv+a+1 );

	} else {
	    const char *alias = parti_get_alias( tst );
	    st = tst;
	    msg(alias ? "object g%d=%s selected (%d particles)" :
			"object g%d%.0s selected (%d particles)",
		i, alias,
		specks_count( st->sl ));
	}

  } else if(!strcmp( argv[0], "gall" ) || !strncmp( argv[0], "allobj", 6 )) {
	int verbose = (argc>1 && !strcmp(argv[1], "-v"));
	parti_allobjs( argc-1-verbose, argv+1+verbose, verbose );

  } else if(!strcmp(argv[0], "tfm")) {
	int inv = 0;
	int mulWorldside = 0, mulObjside = 0;
	int hpr = 0;
	int calc = 0;
	int any, more, a0;
	float scl;
	int has_scl = 0;
	Matrix ot, t;
	Point xyz;
	float aer[3];
	char *key;
	int verbose = 0;

	i = 1;
	if(argc > 1 && !strcmp(argv[1], "-v")) {
	    verbose = 1, i++;
	}
	key = argv[i];
	if(key == NULL) key = "";
	for(more = 1; *key != '\0' && more; key++) {
	  switch(*key) {
	  case '=': break;
	  case '*': mulWorldside = 1; break;
	  case '/': inv = 1; break;
	  case 'h': case 'p': case 'r': hpr = 1; break;
	  case ' ': case '\t': break;
	  case '\0':
		if(i < argc-1) {
		    key = argv[++i];
		    break;
		}
		/* else fall into default */
	  default: more = 0; key--; break;
	  }
	}

	parti_geto2w( st, parti_idof( st ), &ot );

	
	a0 = i;
	argv[a0] = key;
	for(any = 0; any < 16 && a0+any<argc
			&& sscanf(argv[a0+any], "%f", &t.m[any]) > 0; any++)
		;
	switch(any) {
	case 1:
		scl = t.m[0];
		t = Tidentity;
		t.m[0*4+0] = t.m[1*4+1] = t.m[2*4+2] = scl;
		break;

	case 7:	
		scl = t.m[6]; has_scl = 1; /* and fall into... */
	case 6:
		xyz = *(Point *)&t.m[0];
		if(hpr) {
		    aer[0] = t.m[3], aer[1] = t.m[4], aer[2] = t.m[5];
		} else {
		    /* Note we permute: px py pz rx ry rz == px py pz e a r */
		    aer[1] = t.m[3], aer[0] = t.m[4], aer[2] = t.m[5];
		}
		xyzaer2tfm( &t, &xyz, aer );
		if(has_scl) {
		    for(i = 0; i < 12; i++)
			t.m[i] *= scl;
		}
		break;

	case 16: break;
	case 0:  t = ot; break;
	default:
	    msg("Usage: tfm [*] [/] [\"hpr\"] [16 numbers or 6 or 7 numbers] [=]");
	    return -1;
	}

	for(i = a0+any-1; i < argc; i++) {
	    if(strchr(argv[i], '=')) calc = 1;
	    if(strchr(argv[i], '*')) mulObjside = 1;
	}

	if(any) {
	    if(inv)
		eucinv( &t, &t );
	    if(mulWorldside)
		mmmul( &t, &ot, &t );
	    if(mulObjside)
		mmmul( &t, &t, &ot );

	    /* with trailing '=', just print result without assignment */

	    if(!calc)
		parti_seto2w( st, parti_idof( st ), &t );
	}

	scl = tfm2xyzaer( &xyz, aer, &t );
	msg("obj2w:  x y z %s   %g %g %g  %g %g %g  %g",
		hpr?"h p r":"rx ry rz",
		xyz.x[0],xyz.x[1],xyz.x[2],  aer[1-hpr],aer[hpr],aer[2], scl );
	if(verbose) {
	    for(i = 0; i < 3; i++)
		msg(" %10.7g %10.7g %10.7g %10.7g", t.m[i*4+0],t.m[i*4+1],t.m[i*4+2],t.m[i*4+3]);
	    msg(" %10.7g %10.7g %10.7g %10.7g", t.m[3*4+0],t.m[3*4+1],t.m[3*4+2],t.m[3*4+3]);
	    eucinv( &t, &t );
	    scl = tfm2xyzaer( &xyz, aer, &t );
	    msg("w2obj:  x y z %s   %g %g %g  %g %g %g  %g",
		    hpr?"h p r":"rx ry rz",
		    xyz.x[0],xyz.x[1],xyz.x[2],  aer[1-hpr],aer[hpr],aer[2], scl );
	    for(i = 0; i < 3; i++)
		msg(" %10.7f %10.7f %10.7f %10.7f", t.m[i*4+0],t.m[i*4+1],t.m[i*4+2],t.m[i*4+3]);
	    msg(" %10.7g %10.7g %10.7g %10.7g", t.m[3*4+0],t.m[3*4+1],t.m[3*4+2],t.m[3*4+3]);
	}

  } else if(!strcmp( argv[0], "bgcolor" )) {
	char *result, given[64];
	if(argc == 4) {
	    sprintf(given, "%.10s,%.10s,%.10s", argv[1],argv[2],argv[3]);
	    result = parti_bgcolor( given );
	} else {
	    result = parti_bgcolor( argv[1] );
	}
	msg("bgcolor %s", result);

#if !CAVEMENU
  } else if(!strcmp( argv[0], "move" ) || !strcmp( argv[0], "move-objects" )) {
	i = parti_move( argv[1], &st );
	msg(i>=0 ? "move-objects on ; g%d selected" : "move-objects off", i);
#endif

#if MATT_VIRDIR
  } else if(!strcmp( argv[0], "time" )) {
	float newnow, now;
	if(argc > 1 && sscanf(argv[1], "%f", &newnow) > 0)
	    now = vd_camera_time( &newnow );
	else 
	    now = vd_camera_time( NULL );
	msg("time %g", now);

  } else if(!strcmp( argv[0], "key" )) {
	int curkey;
	if(argc == 1) {
	    curkey = vd_create_key();
	} else {
	    curkey = vd_select_key( atoi(argv[1]), 0 );
	}
	msg("key %d", curkey);

  } else if(!strcmp( argv[0], "attach" )) {
	vd_attach( getbool( argv[1], 1 ) );

  } else if(!strcmp( argv[0], "rem" )) {
	vd_remove_key( argc, argv );
  
#endif /* MATT_VIRDIR */

  } else if(!strcmp( argv[0], "step" )) {
	if(argc>1) {
	    if(argv[1][0] == '+') {
		if(argv[1][1] != '\0')
		    clock_set_step( st->clk, getfloat(&argv[1][1], 0) );
		clock_step( st->clk, 1 );
	    } else if(!strcmp(argv[1], "-")) {
		clock_step( st->clk, -1 );
	    } else {
		clock_set_time( st->clk, getfloat(argv[1], 0) );
	    }
	}


#if !CAVEMENU
	parti_set_running( st, 0 );
	specks_set_timestep(st);
#endif
	msg("step %g", st->currealtime);

  } else if(!strcmp( argv[0], "trange" ) || !strcmp( argv[0], "steprange" )) {
	double utmin = st->utmin;
	double utmax = st->utmax;
	double utwrap = st->utwrap;
	if(argc>1) {
	    if(!strcmp(argv[1],"on")) st->usertrange = 1;
	    else if(!strcmp(argv[1],"off")) st->usertrange = 0;
	    else {
		st->utmin = getfloat( argv[1], utmin );
		if(argc>2) st->utmax = getfloat( argv[2], utmax );
		if(argc>3) st->utwrap = getfloat( argv[3], utwrap );
		if(utmin != st->utmin || utmax != st->utmax)
		    st->usertrange = 1;
	    }
	}
	msg(st->usertrange ?
	    "trange %lg %lg  %lg (tmin tmax twrap)"
	  : "trange off   (%lg %lg  %lg  tmin tmax twrap)",
		st->utmin, st->utmax, st->utwrap);

  } else if(!strcmp( argv[0], "fwd" )) {
	if(argc>1)
	    clock_set_fwd( st->clk, getbool(argv[1], 1) );
	IFCAVEMENU( set_fwd( clock_fwd(st->clk), NULL, st ) );

  } else if(!strcmp( argv[0], "gscale" )) {
	if(argc>1) sscanf(argv[1], "%f", &st->gscale);
	msg("gscale %g (scaling particles), then translating by %g %g %g",
		st->gscale, st->gtrans.x[0],st->gtrans.x[1],st->gtrans.x[2]);

  } else if(!strcmp( argv[0], "clearobj" )) {
	struct specklist **slp;
	struct mesh *m, *nm;
	struct ellipsoid *e, *ne;
	specks_clearspecks( st, st->curdata, st->curtime );
	st->sl = NULL;
	for(m = st->staticmeshes, st->staticmeshes = NULL; m; m = nm) {
	    nm = m->next;
	    Free(m);
	}
	for(e = st->staticellipsoids, st->staticellipsoids = NULL; e; e = ne) {
	    ne = e->next;
	    Free(e);
	}

  } else if(!strcmp( argv[0], "every" )) {
	if(argc>1) sscanf(argv[1], "%d", &st->subsample);
	if(st->subsample <= 0) st->subsample = 1;
	msg("display every %dth particle (of %d)",
			st->subsample, specks_count(st->sl));

  } else if(!strncmp( argv[0], "everycomp", 9 )) {
	st->everycomp = getbool(argv[1], st->everycomp);
	msg("everycomp %d (%s compensate for \"every\" subsampling)",
		st->everycomp,
		st->everycomp ? "do" : "don't");

  } else if(!strncmp( argv[0], "color", 5 )) {
	struct valdesc *vd;
	if(argc>1) {
	    if(!specks_set_byvariable( st, argv[1], &st->coloredby ))
		st->coloredby = CONSTVAL;
	}
	vd = &st->vdesc[st->curdata][st->coloredby];
	if(argc>2 && (!strncmp(argv[2], "int", 3) ||
		      !strncmp(argv[2], "exact", 5))) {
	    vd->call = 0;
	    vd->cexact = 1;
	    vd->cmin = 0;
	    argc--, argv++;
	}
	if(argc>2 && (!strncmp(argv[2], "cont", 4) || !strcmp(argv[2], "-exact"))) {
	   vd->cexact = 0;
	   argc--, argv++;
	}
	if(argc>2 && sscanf(argv[2], "%f", &vd->cmin))
	    vd->call = 0;
	if(argc>3 && sscanf(argv[3], "%f", &vd->cmax))
	    vd->call = 0;
	if((vd->cmin == vd->cmax && vd->cmin == 0) ||
	   (argc>2 && (!strcmp(argv[2],"-") || !strcmp(argv[2],"all")))) {

	    vd->cmin = vd->min, vd->cmax = vd->max, vd->call = 1;
	}
	if(argc>4 && st->coloredby == CONSTVAL) sscanf(argv[4], "%f", &vd->mean);
	if(st->coloredby == CONSTVAL) {
	    msg("coloring-by rgb %.3f %.3f %.3f",
		vd->cmin, vd->cmax, vd->mean);
	} else if(vd->cexact) {
	    msg("coloring-by %d(%s) exactly (cindex=data+%g; data %g..%g, cmap 0..%d)",
		st->coloredby, vd->name, vd->cmin,
		vd->min,vd->max, st->ncmap-1);
	} else {
	    if(vd->min == vd->cmin && vd->max == vd->cmax || vd->nsamples==0) {
		msg("coloring-by %d(%s) %g.. %g mean %.3g]",
		    st->coloredby, vd->name,
		    vd->cmin, vd->cmax, vd->mean);
	    } else {
		msg("coloring-by %d(%s) %g.. %g (data %g..%g mean %.3g)",
		    st->coloredby, vd->name,
		    vd->cmin, vd->cmax, vd->min, vd->max, vd->mean);
	    }
	}
	if(argc>1)
	    st->colorseq++;

  } else if(!strcmp( argv[0], "datavar" ) || !strcmp( argv[0], "dv" )) {
	int min = 0, max = CONSTVAL-1;
	if(argc>1 && specks_set_byvariable( st, argv[1], &min ))
	    max = min;
	if(st->ndata>1)
	    msg("In dataset %d %s:", st->curdata, st->dataname[st->curdata]);
	for(i = min; i <= max; i++) {
	    struct valdesc *vd = &st->vdesc[st->curdata][i];
	    if(max>min && vd->name[0] == '\0' && vd->max == vd->min)
		continue;
	    msg("datavar %d %s  %g.. %g mean %g%s%s",
		i, vd->name[0]=='\0'?"\"\"" : vd->name,
		vd->min, vd->max, vd->mean,
		st->sizedby==i ? "[lum]" : "",
		st->coloredby==i ? "[color]" : "");
	}

  } else if(!strcmp( argv[0], "datawait" )) {
	if(argc>1) {
	    switch(i = getbool(argv[1],-1)) {
	    case 0:
	    case 1:
		st->datasync = i;
		break;
	    }
	    msg("datawait %s", st->datasync ? "on" : "off");
	}
	specks_datawait(st);

  } else if(!strncmp( argv[0], "lum", 3 )) {
	struct valdesc *vd;
	char absstuff[32];
	int a;
	enum lnext { ENONE, EMIN, EMAX, EBASE } expect;
	if(argc>1) {
	    if(!specks_set_byvariable( st, argv[1], &st->sizedby ))
		st->sizedby = CONSTVAL;
	}
	vd = &st->vdesc[st->curdata][st->sizedby];
	for(a = 2, expect = EMIN; a < argc; a++) {
	    char *ep;
	    float v = strtod(argv[a], &ep);
	    if(*ep == '\0') {
		switch(expect) {
		    case EMIN: vd->lmin = v; vd->lmax = vd->max; vd->lall = 0; expect = EMAX; break;
		    case EMAX: vd->lmax = v; vd->lall = 0; expect = ENONE; break;
		    case EBASE: vd->lbase = v; expect = ENONE; break;
		    default: msg("What's \"%s\" in command \"%s\"?", argv[a], rejoinargs( 0, argc, argv ));
			     a = argc;
			     break;
		}
	    } else if(!strcmp(argv[a], "-") || !strcmp(argv[a], "all")) {
		vd->lall = 1;
	    } else if(!strncmp(argv[a], "abs", 3)) {
		vd->lop = L_ABS;
		vd->lbase = 0;
		expect = EBASE;
	    } else if(!strcmp(argv[a], "base") || !strncmp(argv[a], "lin", 3)) {
		vd->lop = L_LIN;
		vd->lbase = 0;
		expect = EBASE;
	    } else if(argv[a][0] == '[')
		break;
	}
	if(vd->lmin == vd->lmax && argc > 1)
	    vd->lall = 1;

	absstuff[0] = '\0';
	if(vd->lop == L_ABS)
	    sprintf(absstuff, " abs %g", vd->lbase);
	if(st->sizedby == CONSTVAL) {
	    msg("lum-by constant %g", vd->lmin);
	} else if(vd->lall) {
	    msg("lum-by %d(%s) all%s [%g..%g mean %.3g over %d]",
		    st->sizedby, vd->name, absstuff,
		    vd->min, vd->max, vd->mean, vd->nsamples);
	} else {
	    msg("lum-by %d(%s) %g %g%s [%g..%g mean %.3g over %d]",
		    st->sizedby, vd->name, vd->lmin, vd->lmax, absstuff,
		    vd->min, vd->max, vd->mean, vd->nsamples);
	}
	if(argc>1)
	    st->sizeseq++;

  } else if(!strcmp( argv[0], "cmap" ) || !strcmp( argv[0], "boxcmap" )
			|| !strcmp( argv[0], "textcmap" ) || !strcmp( argv[0], "chromacmap")) {
	char *tfname = argv[1];
	char *realfile = findfile( NULL, argv[1] );
	if(argc != 2) {
	    msg("Usage: %s <ascii-file-of-RGB-triples>", argv[0]);
	} else {
	    if(argc > 1 && realfile == NULL) {
		tfname = (char *)alloca( strlen(argv[1]) + 6 );
		sprintf(tfname, "%s.cmap", argv[1]);
		realfile = findfile( NULL, tfname );
	    }
	    if(realfile != NULL) {
		if(argv[0][0] == 'b') {
		    specks_read_cmap( st, realfile, &st->boxncmap, &st->boxcmap );
		} else if(argv[0][0] == 't') {
		    specks_read_cmap( st, realfile, &st->textncmap, &st->textcmap );
		} else if(argv[0][1] == 'h') {
		  specks_read_cmap( st, realfile, &st->nchromacm, &st->chromacm );
		}
		else {
		    specks_read_cmap( st, realfile, &st->ncmap, &st->cmap );
		    st->colorseq++; /* Must recompute particle coloring */
		}
	    } else {
		msg("%s: can't find \"%s\" nor ....cmap", argv[0], argv[1]);
	    }
	}

  } else if(!strcmp( argv[0], "vcmap" )) {
	int k = 1;
	int coloredby = st->coloredby;
	char *tfname, *realfile;
	struct valdesc *vd;
	if(argc > 3 && !strcmp( argv[1], "-v" )) {
	    if(!specks_set_byvariable( st, argv[2], &coloredby )) {
		msg("vcmap -v %s: unknown colorby-variable", argv[2]);
		return -1;
	    }
	    k = 3;
	}
	vd = &st->vdesc[st->curdata][coloredby];
	if(coloredby < 0 || coloredby >= MAXVAL) {
	    msg("vcmap: bad colorby-variable %d", coloredby);
	    return -1;
	}
	if(argc != k+1) {
	    msg("Usage: vcmap [-v varname] <ascii-file-of-RGB-triples>", argv[0]);
	} else {
	    tfname = argv[k];
	    realfile = findfile( NULL, tfname );
	    if(realfile == NULL) {
		tfname = (char *)alloca( strlen(argv[k]) + 6 );
		sprintf(tfname, "%s.cmap", argv[k]);
		realfile = findfile( NULL, tfname );
	    }
	    if(realfile != NULL) {
		specks_read_cmap( st, realfile, &vd->vncmap, &vd->vcmap );
		st->colorseq++; /* Must recompute particle coloring */
	    } else {
		msg("vcmap: can't find \"%s\" nor ....cmap", argv[k]);
	    }
	}

  } else if(!strncmp( argv[0], "cment", 5 ) || !strncmp( argv[0], "boxcment", 8 )
		|| !strncmp( argv[0], "textcment", 6 )) {
	int i;
	unsigned char *cvp;
	char *prefix = "";
	int ncmap = st->ncmap;
	struct cment *cmap = st->cmap;

	switch(argv[0][0]) {
	case 'b': prefix = "box"; ncmap = st->boxncmap; cmap = st->boxcmap; break;
	case 't': prefix = "text"; ncmap = st->textncmap; cmap = st->textcmap; break;
	}

	if(argc>1 && argv[1][0] != '?') {
	    i = atoi(argv[1]);
	    if(i < 0 || i > ncmap) {
		msg("cment: %d out of range (must be 0..%d)", i, ncmap-1);
	    } else {
		if(argc>2) {
		    char *colorstr = rejoinargs( 2, argc, argv );
		    editcmap( st, ncmap, cmap, argv[1], argv[0], colorstr );
		    if(st->sl) st->colorseq++;	/* force recolor */
		}
		cvp = (unsigned char *)&cmap[i].raw;
		msg( "%scment %d  %.3f %.3f %.3f  (of cmap 0..%d",
			prefix,
			i, cvp[0]/255., cvp[1]/255., cvp[2]/255.,
			ncmap-1 );
	    }
	} else {
	    msg("Usage: %scment colorindex [r g b]  -or-  c1-c2,c3,c4  r g b -or- c1-c2,... = cindex", prefix);
	    msg(" (no blanks around commas for color ranges)");
	}

  } else if(!strcmp( argv[0], "chromaparams")) {
	getfloats( &st->chromaslidestart, 1,  1, argc, argv );
	getfloats( &st->chromaslidelength, 1, 2, argc, argv );
	msg("chromaparams %g %g # (chromadepth Z-start and Z-range for Chromatek display; chromadepth %s)",
		st->chromaslidestart,
		st->chromaslidelength,
		st->use_chromadepth ? "on" : "off");

  } else if(!strcmp( argv[0], "chromadepth")) {
	char *onoff = argv[1];
	st->use_chromadepth = getbool( argv[1], st->use_chromadepth );
	getfloats( &st->chromaslidestart, 1,  2, argc, argv );
	getfloats( &st->chromaslidelength, 1, 3, argc, argv );
	msg("chromadepth %s # Chromatek chromostereo display; chromaparams %g %g",
		st->use_chromadepth ? "on" : "off",
		st->chromaslidestart, st->chromaslidelength);

  } else if(!strncmp( argv[0], "only", 4 )) {
	int plus = !strcmp(argv[0], "only+");
	int minus = !strcmp(argv[0], "only-");
	int exact = !strcmp(argv[0], "only=");
	char *selstr = NULL;
	int tvar = -1;
	int all = 0;
	int k;
	int nspecks, nvisible, needthresh;
	struct specklist *sl;
	struct speck *sp;

	if(argc>2 && !strcmp(argv[1], "-s")) {
	    selstr = argv[2];
	    argc -= 2, argv += 2;
	}
	if(argc>1) {
	    if(!strcmp(argv[1], "all") || !strcmp(argv[1], "*")) all = 1;
	    if(!strcmp(argv[1], "none")) all = -1;
	}
	if(!all && ((!plus && !minus && !exact) ||
		!specks_set_byvariable( st, argv[1], &tvar))) {
	    msg("only+/only-/only=  \"all\"|\"none\"|varname value minvalue-maxvalue ...");
	}

	plus |= exact;

	st->selseq++;
	if(exact || all) {
	    /* preset all values */
	    SelMask selbit = (plus || all<0) ? 0 : SELMASK(SEL_THRESH);
	    for(sl = st->sl; sl != NULL; sl = sl->next) {
		SelMask *sel = sl->sel;
		if((sp = sl->specks) == NULL || sl->text != NULL || sel == NULL)
		    continue;
		for(k = sl->nspecks; --k >= 0; sp = NextSpeck(sp, sl, 1)) {
		    sel[k] = (sel[k] & ~SELMASK(SEL_THRESH)) | selbit;
		}
		sl->selseq = st->selseq;
	    }
	}

	/*
	 * If a thresh variable is given, use "plus" to choose
	 * whether to make that speck visible.
	 */
	for(i = 2; i < argc; i++) {
	    float vmin = -1e38, vmax = 1e38;
	    if(argv[i][0] == '<') {
		/* ugh */
		vmax = atof(argv[i][1]==0 && i+1<argc
			? argv[++i] : &argv[i][1]);
	    } else if(argv[i][0] == '>') {
		vmin = atof(argv[i][1]==0 && i+1<argc
			? argv[++i] : &argv[i][1]);
	    } else {
		switch(sscanf(argv[i], "%f%*c%f", &vmin, &vmax)) {
		case 1: vmax = vmin; break;
		case 2: break;
		default:
		    msg("%s: \"%s\": expected N or <N or >N or N-M",
			argv[0], argv[i]);
		    return -1;
		}
	    }

	    for(sl = st->sl; sl != NULL; sl = sl->next) {
		SelMask *sel = sl->sel;
		if((sp = sl->specks) == NULL || sl->text != NULL || tvar<0
			|| SMALLSPECKSIZE(tvar+1) > sl->bytesperspeck)
		    continue;
		for(k = 0; k < sl->nspecks; sp = NextSpeck(sp, sl, 1), k++) {
		    if(sp->val[tvar]>=vmin && sp->val[tvar]<=vmax) {
			sel[k] = plus ? sel[k] | SELMASK(SEL_THRESH)
			    	      : sel[k] & ~SELMASK(SEL_THRESH);
		    }
		}
		sl->selseq = st->selseq;
	    }
	}

	parse_selexpr( st, "thresh", &st->threshsel, NULL, NULL );
	seldest2src( st, &st->threshsel, &st->seesel );
	msg("only %s (selecting \"see %s\")",
		selcounts( st, st->sl, &st->threshsel ),
		show_selexpr( st, NULL, &st->seesel ));

  } else if(!strncmp( argv[0], "thresh", 6 )) {
	int i, expect = -1;
	float v;
	char smin[16], smax[16], *arg;
	char chosen[40];
	int changed = 0;
	CONST char *selstr = NULL;
	for(i = 1; i < argc; i++) {
	    arg = argv[i];
	    if(!strcmp(arg,"-s")) {
		selstr = argv[++i];
	    } else if(!strcmp(arg,"off")) {
		selinit( &st->seesel );
		expect = -1;
	    } else if(!strcmp(arg,"on")) {
		seldest2src( st, &st->threshsel, &st->seesel );
		if(st->usethresh == 0)
		    msg("thresh on -- but no threshold active?");
	    } else if(expect<0 && specks_set_byvariable(st, arg, &st->threshvar)) {
		expect = 0;
		st->usethresh = P_USETHRESH;	/* neither MIN nor MAX set */
		changed = 1;
	    } else if(!strcmp(arg,"min") || !strcmp(arg, ">"))
		expect = 0;
	    else if(!strcmp(arg,"max") || !strcmp(arg, "<"))
		expect = 1;
	    else if(!strcmp(arg,"="))
		expect = 2;
	    else if(!strcmp(arg, "-")) {
		st->usethresh &= expect ? ~P_THRESHMAX : ~P_THRESHMIN;
		expect = 1;
	    } else if(sscanf(arg, "%f", &v) > 0) {
		if(expect != 1) {	/* -1, 0, or 2 */
		    st->thresh[0] = v;
		    st->usethresh |= P_THRESHMIN | P_USETHRESH;
		    changed = 1;
		}
		if(expect >= 1) {	/* 1 or 2 */
		    st->thresh[1] = v;
		    st->usethresh |= P_THRESHMAX | P_USETHRESH;
		    changed = 1;
		}
		expect = 1;
	    } else if(sscanf(arg, ">%f", &st->thresh[0]) > 0) {
		st->usethresh |= P_THRESHMIN | P_USETHRESH;
		changed = 1;
	    } else if(sscanf(arg, "<%f", &st->thresh[1]) > 0) {
		st->usethresh |= P_THRESHMAX | P_USETHRESH;
		changed = 1;
	    } else if(sscanf(arg, "=%f", &st->thresh[0]) > 0) {
		st->thresh[1] = st->thresh[0];
		st->usethresh |= P_THRESHMIN | P_THRESHMAX | P_USETHRESH; 
		changed = 1;
	    }
	}
	if(changed) {
	    st->threshseq++;
	    st->selseq++;
	    if(!parse_selexpr( st, (char *) (selstr ? selstr : "thresh"), &st->threshsel, NULL, NULL )) {
		msg("thresh -s %s: expected selection variable name", selstr);
		return 1;
	    }

	    /* "thresh" with no -s implies also "see thresh" */
	    if(selstr == NULL && st->seesel.wanted == 0)
		seldest2src( st, &st->threshsel, &st->seesel );
	    specks_reupdate( st, st->sl );
	}
	chosen[0] = '\0';
	sprintf(chosen, " (%s selected)", selcounts( st, st->sl, &st->threshsel ));
	sprintf(smin, st->usethresh&P_THRESHMIN ? "%g" : "-", st->thresh[0]);
	sprintf(smax, st->usethresh&P_THRESHMAX ? "%g" : "-", st->thresh[1]);

	msg(st->usethresh == 0 ? "thresh off"
		: "thresh%s%s%s %d(%s) min %s max %s%s",
		selstr ? " -s " : "",
		selstr ? selstr : "",
		st->usethresh&P_USETHRESH ? "" : " off ",
		st->threshvar, st->vdesc[st->curdata][st->threshvar].name,
		smin, smax, chosen);
	
  } else if(!strcmp( argv[0], "rawdump" )) {
	int all = 0;
	FILE *f;
	struct specklist *sl;
	int i, j, nmine, net;
	if(argc>1 && !strcmp(argv[1], "-a")) {
	    all = 1, argc--, argv++;
	}
	f = argc>1 ? fopen(argv[1], "w") : NULL;
	if(f == NULL) {
	    msg("%s: can't create", argv[1]);
	} else {
	    fprintf(f, "# from dataset %d \"%s\" timestep %d  time %g\n",
		st->curdata, CURDATATIME(fname), st->curtime, st->currealtime);
	    if(all)
		fprintf(f, "x y z\txRGB\tSize\t");
	    for(i = 0; st->vdesc[st->curdata][i].name[0] != '\0'; i++) {
		fprintf(f, "%s\t", st->vdesc[st->curdata][i].name);
	    }
	    fprintf(f, "\n");
	    nmine = i;
	    net = 0;
	    sl = specks_timespecks( st, st->curdata, st->curtime );
	    if(sl == NULL && st->ntimes <= 1)
		sl = st->sl;
	    for(; sl != NULL; sl = sl->next) {
		for(j = 0; j < sl->nspecks; j++) {
		    struct speck *sp = NextSpeck(sl->specks, sl, j);
		    if(all) {
			fprintf(f, "%.8g %.8g %.8g\t%02x%02x%02x\t%g",
			    sp->p.x[0],sp->p.x[1],sp->p.x[2],
			    RGBA_R(sp->rgba), RGBA_G(sp->rgba), RGBA_B(sp->rgba),
			    sp->size);
		    }
		    if(nmine > 0)
			fprintf(f, "%g", sp->val[0]);
		    for(i = 1; i < nmine; i++)
			fprintf(f, "\t%g", sp->val[i]);
		    fprintf(f, "\n");
		    net++;
		}
	    }
	    fclose(f);
	    msg("Wrote %d particles (%d fields each) to %s",
		net, nmine, argv[1]);
	}
	
  } else if(!strncmp( argv[0], "slum", 3 ) || !strcmp( argv[0], "scale-lum" )) {
	float *lp;
	int by = st->sizedby, cd = st->curdata;
	if(argc>1) {
	    if(argc>2) specks_set_byvariable( st, argv[2], &by );
	    if(argc>3) sscanf(argv[3], "%d", &cd);
	    if((unsigned int)cd >= MAXFILES) cd = 0;
	    lp = &st->vdesc[cd][by].lum;
	    *lp = getfloat( argv[1], *lp!=0 ? *lp : 1 );
	}
	msg("slum %g (var %d %s dataset %s (%d))",
	    st->vdesc[cd][by].lum, by, st->vdesc[cd][by].name,
	    st->dataname[cd], cd);

  } else if(!strcmp( argv[0], "seedata" )) {
	if(argc>1) {
	    int v = -1, len = strlen(argv[1]);
		/* search st->dataname[] array! XXX */
	    if(sscanf(argv[1], "%d", &v) > 0) {
		/* fine */
	    } else {
		for(v = st->ndata; --v >= 0; ) {
		    if(!strncasecmp( argv[1], st->dataname[v], len ))
			break;
		}
		if(v < 0) {
		    char msgstr[2048];
		    sprintf(msgstr,
			"Never heard of dataset \"%s\"; we have these %d:",
			argv[1], st->ndata);
		    for(v = 0; v < st->ndata; v++)
			sprintf(msgstr+strlen(msgstr), " \"%s\"(%d) ", st->dataname[v], v);
		    msg(msgstr);
		    v = -1;
		}
	    }
	    if(v >= 0 && v < st->ndata)
		st->curdata = v;
	}
	msg("showing dataset %d (%s)", st->curdata,
		st->dataname[st->curdata]);

  } else if(!strncmp( argv[0], "seesel", 3 )) {
    if(argc>1)
	parse_selexpr( st, rejoinargs( 1, argc, argv ), NULL, &st->seesel, "see" );
    msg("see %s (%s)", show_selexpr( st, NULL, &st->seesel ),
	    	selcounts( st, st->sl, &st->seesel ));

  } else if(!strcmp( argv[0], "emph" )) {
    if(argc>2 && !strcmp(argv[1], "-f"))
	st->emphfactor = getfloat( argv[2], st->useemph ),
        argc -= 2, argv += 2;
    if(argc>1) {
	if(!strcmp(argv[1], "on")) st->useemph = 1;
	else if(!strcmp(argv[1], "off")) st->useemph = 0;
	else {
	    parse_selexpr( st, rejoinargs( 1, argc, argv ), NULL, &st->emphsel, "emph" );
	    st->useemph = 1;
	}
    }
    if(st->useemph) {
	st->sizeseq++;
	msg("emph -f %g  %s (%s emphasized)", st->emphfactor,
	    	show_selexpr( st, NULL, &st->emphsel ),
		selcounts( st, st->sl, &st->emphsel ));
    } else {
	msg("emph off");
    }

  } else if(!strcmp( argv[0], "sel" )) {
    SelOp src, dest;
    char *rest = rejoinargs( 1, argc, argv );

    /*
     * sel a = b -c d  (existing names, or "see" or "emph" or maybe "thresh")
     * sel a = [all|none]
     */
    if(strchr(rest, '=')) {
	if(parse_selexpr( st, rest, &dest, &src, "sel" )) {
	    struct specklist *sl;
	    /* Is this the right place to look?  Do we want to update more than st->sl? */
	    int selcount = 0;
	    st->selseq++;
	    for(sl = st->sl; sl != NULL; sl = sl->next) {
		int k, ns = sl->nspecks;
		SelMask *sel = sl->sel;
		if(sl->text != NULL || sl->nsel < ns) continue;
		for(k = 0; k < ns; k++) {
		    if(src.use != SEL_NONE && SELECTED(sel[k], &src)) {
			SELSET(sel[k], &dest);
			selcount++;
		    }
		}
		if(st->useemph)
		    specks_resize( st, sl, sl->sizedby );  /* XXX could anything else depend on this? */
		sl->selseq = st->selseq;
	    }
	    msg("sel %s : %d of %d selected",
		show_selexpr(st, &dest, &src), selcount, specks_count(st->sl));
	}
    } else {
	/* No '=' -- just asking how many match some pattern */
	if(parse_selexpr( st, rest, NULL, &src, "sel")) {
	    msg("sel %s : %s matched", show_selexpr( st, NULL, &src ),
		    	selcounts(st, st->sl, &src));
	}
    }

  } else if(!strcmp( argv[0], "pick" )) {
    /* pick [-a] [note|center|sel <name>] */
    /* kira pick track */
      msg("pick not yet implemented XXX");

  } else if(!strcmp( argv[0], "show" ) || !strcmp( argv[0], "hide" )
	  || !strncasecmp( argv[0], "showbox", 7 )
	  || !strncasecmp( argv[0], "hidebox", 7 )) {
	int doshow = argv[0][0] == 's';
	int i, level, mask = 0;
	if(doshow) st->useboxes = 1;
	for(i = 1; i < argc; i++) {
	    level = getbool( argv[i], -2 );
	    if(level == -1)
		mask = ~0;
	    else if(level >= 0)
		mask |= (1 << level);
	}
	if(doshow) st->boxlevelmask |= mask;
	else st->boxlevelmask &= ~mask;
	/* st->boxlevelmask &= (1 << st->boxlevels) - 1;
	 *  Don't do this -- we want "show all" to apply to levels that
	 *  we haven't read in yet.
	 */
	IFCAVEMENU( partimenu_boxlevels( st ) );

  } else if(!strcmp( argv[0], "box" ) || !strcmp( argv[0], "boxes" )) {
	st->useboxes = getbool( argv[1], (st->useboxes+1)%3 );
	msg("boxes %s", st->useboxes==2 ? "ONLY"
				: st->useboxes ? "ON" : "off");

  } else if(!strncmp( argv[0], "boxlabel", 6 )) {
	if(argc>1) st->boxlabels = getbool( argv[1], !st->boxlabels );
	else st->boxlabels = !st->boxlabels;
	msg("boxlabels %s", st->boxlabels?"on":"off");

  } else if(!strncmp( argv[0], "boxaxes", 5 )) {
	if(argc>1) st->boxaxes = getbool( argv[1], !st->boxaxes );
	else st->boxaxes = !st->boxaxes;
	msg("boxaxes %s", st->boxaxes?"on":"off");

  } else if(!strcmp(argv[0], "boxscale")) {
	char buf[512];
	int from, to;
	if(argc>1) {
	    float v = atof(argv[1]);
	    from = getbool(argc>2?argv[2]:NULL, 0);
	    to = getbool(argc>3?argv[3]:NULL, MAXBOXLEV-1);
	    if(from<0) from = 0;
	    if(to>MAXBOXLEV-1) to=MAXBOXLEV-1;
	    while(from<=to)
		st->boxscale[from++] = v;
	}
	sprintf(buf, "boxscales:");
	for(to=MAXBOXLEV-1; to>1 && st->boxscale[to]==st->boxscale[to-1]; to--)
	    ;
	for(from = 0; from <= to; from++)
	    sprintf(buf+strlen(buf), " %g", st->boxscale[from]);
	msg("%s", buf);

  } else if(!strcmp( argv[0], "go" ) || !strcmp( argv[0], "gobox" )) {
	int boxno = getbool( argv[1], -1 );
	if(boxno < 0) {
	    msg("Usage: gobox <boxnumber> -- sets POI and jumps to view that AMR box");
	} else {
	    specks_gobox( st, boxno, argc-2, argv+2 );
	}

  } else if(!strcmp( argv[0], "goboxscale" )) {
	if(argc>1) sscanf(argv[1], "%f", &st->goboxscale);
	msg("goboxscale %g  (\"gobox\" sets scale to %g * box diagonal; 0 => leave scale intact)",
		st->goboxscale, st->goboxscale);

  } else if(!strcmp( argv[0], "psize" ) || !strcmp(argv[0], "pointsize")) {
	if(argc>1) {
	    st->psize = getfloat( argv[1], st->psize );
	    IFCAVEMENU( set_psize( st->psize, NULL, st ) );
	}
	msg("pointsize %g pixels (times scale-lum value)", st->psize );

  } else if(!strcmp( argv[0], "polysize" )) {
	if(argc>1) {
	    st->polysize = getfloat(argv[1], st->polysize);
	    IFCAVEMENU( set_polysize( st->polysize, NULL, st ) );
	}
	if(argc>2) {
	    if(argv[2][0] == 'a' || argv[2][0] == 's') st->polyarea = 1;
	    else if(argv[2][0] == 'r') st->polyarea = 0;
	}
	msg("polysize %g%s", st->polysize, st->polyarea ? " area":"" );

  } else if(!strncmp( argv[0], "polylum", 7 )) {
	char *area = "";
	if(argc>1) {
	    if(!specks_set_byvariable( st, argv[1], &st->polysizevar ))
		if(!strcmp(argv[1], "-1") ||
		   !strcmp(argv[1], "point-size") ||
		   !strcmp(argv[1], "pointsize"))
		    st->polysizevar = -1;
	    if(st->polysizevar == CONSTVAL)
		st->polysizevar = -1;	/* Can't do const */
	}
	if(argc>2)
	    sscanf(argv[2], "%f", &st->polysize);
	switch(argv[argc-1][0]) {
	    case 'r':
	    case 'd':
	    case 's': st->polyarea = 0; break;
	    case 'a': st->polyarea = 1; break;
	}
	if(st->polyarea) area = " area";
	if(st->polysizevar < 0)
	    msg("polylumvar point-size%s", area);
	else
	    msg("polylumvar %d(%s)%s", st->polysizevar,
		st->vdesc[st->curdata][st->polysizevar].name,
		area);

  } else if(!strncmp( argv[0], "polyminpixels", 7 )) {
	if(argc>1) st->polymin = getfloat(argv[1], st->polymin);
	if(argc>2) st->polymax = getfloat(argv[2], st->polymax);
	msg("polyminpixels %g %g (minpixels maxpixels)", st->polymin, st->polymax );

  } else if(!strncmp( argv[0], "labelminpixels", 7 )) {
	st->textmin = getfloat(argv[1], st->textmin);
	msg("labelminpixels %g", st->textmin );


  } else if(!strcmp( argv[0], "labelsize" ) || !strcmp(argv[0], "lsize")) {
	st->textsize = getfloat(argv[1], st->textsize);
	msg("labelsize %g", st->textsize);

  } else if(!strcmp( argv[0], "point" ) || !strcmp( argv[0], "points" )) {
	st->usepoint = getbool( argv[1], !st->usepoint );
	IFCAVEMENU( set_point( st->usepoint, NULL, st ) );
	msg("points %s", st->usepoint ? "on":"off");

  } else if(!strcmp( argv[0], "poly" ) || !strncmp( argv[0], "polygon", 7 )) {
	st->usepoly = getbool( argv[1], !st->usepoly );
	IFCAVEMENU( set_poly( st->usepoly, NULL, st ) );
	msg("polygons %s", st->usepoly ? "on":"off");

  } else if(!strcmp( argv[0], "vec" ) || !strncmp(argv[0], "vector", 6)) {
	struct valdesc *vd = &st->vdesc[st->curdata][st->vecvar0];
	if(argc>1) {
	    if(!strcmp(argv[1], "arrow")) {
		st->usevec = VEC_ARROW;
	    } else {
		int v = getbool( argv[1], -1 );
		if(v >= 0)
		    st->usevec = v ? VEC_ON : VEC_OFF;
		else
		    msg("vec {off|on|arrow} -- draw vectors (w/optional arrows) on points if \"vecvar\" is set");
	    }
	}
	msg("vectors %s (vecscale %g %g (vecscale arrowscale)) (vecalpha %g) (vecvar %s(%d))",
		st->usevec==VEC_ARROW ? "arrow" : st->usevec==VEC_ON ? "on":"off",
		st->vecscale,
		st->vecarrowscale,
		st->vecalpha,
		vd->name, st->vecvar0);

  } else if(!strcmp(argv[0], "vecalpha")) {
	struct valdesc *vd = &st->vdesc[st->curdata][st->vecvar0];
	st->vecalpha = getfloat( argv[1], st->vecalpha );
	msg("vecalpha %g (vec %s) (vecvar %s(%d))",
	    st->vecalpha,
	    st->usevec==VEC_ARROW ? "arrow" : st->usevec==VEC_ON ? "on":"off",
	    vd->name, st->vecvar0);

  } else if(!strcmp(argv[0], "vecscale")) {
	struct valdesc *vd = &st->vdesc[st->curdata][st->vecvar0];
	st->vecscale = getfloat( argv[1], st->vecscale );
	if(argc > 2)
	    st->vecarrowscale = getfloat( argv[2], st->vecarrowscale );
	msg("vecscale %g %g (vecscale arrowscale) (vec %s) (vecvar %s(%d))",
	    st->vecscale, st->vecarrowscale,
	    st->usevec==VEC_ARROW ? "arrow" : st->usevec==VEC_ON ? "on":"off",
	    vd->name, st->vecvar0);

  } else if(!strncmp( argv[0], "texture", 7 ) || !strcmp( argv[0], "tx" )) {
	if(argc > 1 && !strcmp(argv[1], "preload")) {
	    msg("Preloading textures...");
	    for(i = 0; i < st->ntextures; i++)
		if(st->textures[i]) txload( st->textures[i] );
	} else if(argc > 1 && !strncmp(argv[1], "report", 3)) {
	    if(argc == 2) {
		for(i = 0; i < st->ntextures; i++)
			if(st->textures[i]) st->textures[i]->report = ~0;
	    } else {
		for(i = 2; i < argc; i++) {
		    int t = atoi(argv[i]);
		    if(t >= 0 && t < st->ntextures && st->textures[t])
			st->textures[t]->report = ~0;
		}
	    }
	} else {
	    st->usetextures = getbool( argv[1], !st->usetextures );
	    msg("textures %s", st->usetextures ? "on":"off");
	}

  } else if(!strcmp( argv[0], "txscale" )) {
	if(argc>1) sscanf(argv[1], "%f", &st->txscale);
	msg("txscale %.3f", st->txscale);

  } else if(!strncmp( argv[0], "polyorivar", 10)) {
	if(argc>1) st->polyorivar0 = getbool(argv[1], -1);
	msg(st->polyorivar0 >= 0
	  ? "polyorivar %d : polygon orientations from var %d..%d"
	  : "polyorivar %d : polygon orientations parallel to screen",
	  st->polyorivar0, st->polyorivar0, st->polyorivar0+5);

  } else if(!strcmp( argv[0], "texturevar")) {
	if(argc>1) st->texturevar = getbool(argv[1], -1);
	msg("texturevar %d", st->texturevar);

  } else if(!strncmp( argv[0], "ellipsoids", 5 )) {
	st->useellipsoids = getbool(argv[1], !st->useellipsoids);
	msg("ellipsoids %s", st->useellipsoids ? "on":"off");
	
  } else if(!strncmp( argv[0], "meshes", 4 )) {
	st->usemeshes = getbool(argv[1], !st->usemeshes);
	msg("meshes %s", st->usemeshes ? "on":"off");

  } else if(!strncmp( argv[0], "mullions", 3 )) {
      	st->mullions = getfloat( argv[1], st->mullions );
	msg("mullions %g", st->mullions);

  } else if(!strcmp( argv[0], "label" ) || !strcmp( argv[0], "labels" )) {
	st->usetext = getbool( argv[1], !st->usetext );
	IFCAVEMENU( set_label( st->usetext, NULL, st ) );
	msg("labels %s", st->usetext ? "on":"off");

  } else if(!strcmp( argv[0], "laxes" )) {
	st->usetextaxes = getbool(argv[1], !st->usetextaxes);
	msg("laxes %s  (axes on each label)", st->usetextaxes ? "on":"off");

  } else if(!strncmp( argv[0], "polyside", 8 )) {
	if(argc>1) sscanf(argv[1], "%d", &st->npolygon);
	msg("polysides %d (polygons drawn with %d sides)",
		st->npolygon,st->npolygon);

  } else if(!strcmp( argv[0], "gamma" )) {
	if(argc>1) sscanf(argv[1], "%f", &st->gamma);
	msg("gamma %g", st->gamma);

  } else if(!strncmp( argv[0], "cgamma", 4 ) || !strncmp(argv[0], "setgamma", 6)) {
	float *p = NULL;
	int any = 0;
	for(i = 1; i < argc; i++) {
	    if(!strcmp(argv[i], "-b")) i++, p = &st->rgbright[0];
	    else if(!strcmp(argv[i], "-g")) i++, p = &st->rgbgamma[0];
	    else if(i == argc-1) p = &st->rgbgamma[0];
	    else continue;
	    switch(sscanf(argv[i], "%f%*c%f%*c%f", &p[0],&p[1],&p[2])) {
	    case 1: p[1] = p[2] = p[0];  /* and fall into... */
	    case 3: any = 1; break;
	    default: msg("setgamma [-b BRIGHT] [-g GAMMA]: want 1 or 3 floats not %s", argv[i]);
	    }
	}
	msg("setgamma -b %g,%g,%g  -g %g,%g,%g",
		st->rgbright[0],st->rgbright[1],st->rgbright[2],
		st->rgbgamma[0],st->rgbgamma[1],st->rgbgamma[2]);
	if(any)
	    specks_rgbremap( st );

  } else if(!strcmp( argv[0], "alpha" )) {
	if(argc>1)
	    st->alpha = getfloat(argv[1], st->alpha);
	IFCAVEMENU( set_alpha( st->alpha, NULL, st ) );
	msg("alpha %g", st->alpha);

  } else if(!strncmp( argv[0], "fast", 4 )) {
	st->fast = getbool(argv[1], !st->fast);
	if(argc > 2) sscanf(argv[2], "%f", &st->pfaint);
	if(argc > 3) sscanf(argv[3], "%f", &st->plarge);
	msg("fast %s  (ptsize %.3g %.3g (faintest largest))",
		st->fast ? "on" : "off (better rendering)",
		st->pfaint, st->plarge);

  } else if(!strncmp( argv[0], "ptsize", 6 )) {
	if(argc > 1) sscanf(argv[1], "%f", &st->pfaint);
	if(argc > 2) sscanf(argv[2], "%f", &st->plarge);
	msg("ptsize  %.3g %.3g  (faintest largest)", st->pfaint, st->plarge);

  } else if(!strcmp( argv[0], "fog" ) && argc > 1) {
	IFCAVEMENU( set_fog( st->fog, NULL, st ) );

  } else if(!strcmp( argv[0], "menu" ) || !strcmp( argv[0], "fmenu" )) {
	IFCAVEMENU( partimenu_fmenu( st, argc, argv ) );

  } else if(!strcmp( argv[0], "datascale" ) && argc>1) {
	float s, v = atof(argv[1]);
	if(v!=0) {
	    struct specklist *sl;
	    struct speck *p;
	    int i;
	    for(sl = st->sl; sl != NULL; sl = sl->next) {
		s = v / sl->scaledby;
		for(i = 0, p = sl->specks; i < sl->nspecks; i++, p = NextSpeck(p, sl, 1)) {
		    p->p.x[0] *= s; p->p.x[1] *= s; p->p.x[2] *= s;
		}
		sl->scaledby = v;
	    }
	}
  } else if(!strcmp( argv[0], "where" ) || !strcmp( argv[0], "w" )) {
	tellwhere(st);

  } else if(!strcmp( argv[0], "version" )) {
	extern char partiview_version[];
	char *cp = strchr(local_id, ',');	/* ... partibrains.c,v N.NN */
	if(cp) {
	    cp += 3;
	    msg("version %s (brains %.*s)  ", partiview_version, strcspn(cp, " "), cp);
	    msg("# Copyright (C) 2002 NCSA, University of Illinois");
	}
  }
  else
    return 0;

  *stp = st;	/* in case it changed, e.g. because of an "object" cmd */
  return 1;
}

#ifdef __cplusplus
};	// end extern "C"
#endif
