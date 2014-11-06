# top level Makefile for partiview

VERSION = `cat src/VERSION`

TAG = partiview-`date +%Y-%m-%d-%H%M`
#TAG = partiview-2000-11-29

EDIR =  partiview-$(VERSION)
EFILE = $(EDIR).tar.gz

RDIR = teuben@apus:/apus1/nemo/local/www/amnh


# things to export

FILES = HISTORY Makefile VERSION README.FIRST
DIRS =  src data doc nemo starlab windows tutor

help:
	@echo 'Try "make build" to rebuild src, if you already ran "configure".'
	@echo Nohelp for VERSION=$(VERSION)
	@echo Exportdir = $(EDIR)

build:
	(cd src; make depend; make)


.PHONY:  tar export


dist:
	rm -rf $(EDIR)
	cvs -q tag $(TAG)
	cvs -q export -r $(TAG) -d $(EDIR) partiview
	tar -zcf $(EFILE) $(EDIR)
	rm -rf $(EDIR)

export:
	scp -C $(EFILE) $(RDIR)


