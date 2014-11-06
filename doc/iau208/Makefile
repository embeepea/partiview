FIGS = starfield.ps primbin16.ps 0607.ps ncsa-haydenCollab2.ps hyades-hrdiag.ps

dvi: ${FIGS}
	latex partiview

ps: dvi
	dvips -f partiview.dvi > partiview.ps+ && mv partiview.ps+ partiview.ps

pdf: ${FIGS}
	pdflatex partiview

tar: ${FIGS}
	gnutar cvfz slevy.tar.gz  partiview.tex ${FIGS}

.SUFFIXES: .ps .png

.png.ps:
	convert $*.png pgm:- | pnminvert | pnmtops -nosetpage -noturn > $@+ && mv $@+ $@
