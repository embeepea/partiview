# TARGET
TARGET   = partiview.exe

FLTK_DIR   = D:\wx\fltk114
FLTK_INC   = -I "$(FLTK_DIR)"
FLTK_LIB   = $(FLTK_DIR)\lib\fltk.lib $(FLTK_DIR)\lib\fltk_gl.lib $(FLTK_DIR)\lib\fltk_images.lib

ELUMENS_LIB = .\spiclops.lib
ELUMENS_FLAGS = -DUSE_ELUMENS -DDEFAULT_ELUMENS

GL_LIB   = opengl32.lib glu32.lib
SYS_LIB	 = libc.lib kernel32.lib wsock32.lib user32.lib gdi32.lib comctl32.lib ole32.lib shell32.lib advapi32.lib \
		/nologo /subsystem:console /incremental:no

# FLAGS and BINARIES
PV_FLAGS    = -DUSE_WARP # -DUSE_MODEL -DUSE_KIRA
DEFS	    = -DNOCAVE -DWIN32 $(PV_FLAGS)
INCL	    = $(FLTK_INC)
CC          = cl /TC
CFLAGS      = $(OPT) $(DEFS) $(INCL)
CXX	    = cl /TP
CXXFLAGS    = $(CFLAGS) 
OPT         = /Zi /O2
LINK        = link
LIBS        = $(KIRA_LIB) $(SPICLOPS_LIB) $(FLTK_LIB) $(GL_LIB) $(SYS_LIB)

APP_CSRCS   = geometry.c partibrains.c mgtexture.c textures.c \
		findfile.c sfont.c version.c shmem.c \
		winjunk.c \
		plugins.c warp.c async.c
APP_CXXSRCS = partiview.cc partiviewc.cc partipanel.cc Gview.cc Hist.cc \
		Fl_Log_Slider.cxx kira_parti.cc Plot.cc \
		Fl_Scroll_Thin.cxx genericslider.cc \
		elumens.cc # parti_model.cc cat_model.cc cat_modelutil.cc parti-ieee.cc

APP_OBJS    = partiview.obj partiviewc.obj partipanel.obj Gview.obj Hist.obj \
		Plot.obj geometry.obj partibrains.obj \
		genericslider.obj Fl_Scroll_Thin.obj \
		mgtexture.obj textures.obj futil.obj findfile.obj sfont.obj \
		sclock.obj notify.obj async.obj Fl_Log_Slider.obj \
		version.obj winjunk.obj shmem.obj \
		plugins.obj warp.obj # parti_model.obj cat_model.obj cat_modelutil.obj

ELUMENS_OBJS = partiview_elumens.obj elumens.obj

all:	partiview.exe # partiviewelum.exe

$(TARGET):  $(APP_OBJS)
	$(LINK) $(APP_OBJS) $(LIBS)  /out:$@

partiviewelum.exe: $(APP_OBJS:partiview.obj=) $(ELUMENS_OBJS)
	$(CC) -c plugins.c $(CFLAGS) $(ELUMENS_FLAGS)
	$(LINK) $(APP_OBJS:partiview.obj=) $(ELUMENS_OBJS) $(ELUMENS_LIB) $(LIBS)  /out:$@
	del plugins.obj

partiview_elumens.obj: partiview.cc
	$(CXX) -c /Fo$@ partiview.cc $(CXXFLAGS) $(ELUMENS_FLAGS)

elumens.obj: elumens.cc
	$(CXX) -c /Fo$@ elumens.cc $(CXXFLAGS) $(ELUMENS_FLAGS)

clean:
	del *.obj
	del $(TARGET)
	touch Makedepend

.SUFFIXES: .C .cc .cxx .cpp

.c.obj:
	$(CC) -c $< $(CFLAGS) 
.cc.obj:
	$(CXX) -c $< $(CXXFLAGS) 
.cxx.obj:
	$(CXX) -c $< $(CXXFLAGS)
.cpp.obj:
	$(CXX) -c $< $(CXXFLAGS) 

include Makedepend
