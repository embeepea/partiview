# webpage update
RDIR = teuben@grus.astro.umd.edu:public_html/nemo/amnh
EXPORT = partiview*html partiview.ps partiview.sgml partiview.txt \
	                partiview2.ps partiview.pdf

help:
	@echo all:  ps html
	@echo export: copy EXPORT to RDIR
	@echo ""
	@echo EXPORT=$(EXPORT)
	@echo RDIR=$(RDIR)

.PHONY:  txt ps pdf html

all:    txt ps pdf html 

txt:
	sgml2txt partiview.sgml

ps:
	sgml2latex partiview.sgml
	latex partiview
	dvips partiview -o
	gzip -c partiview.ps > partiview.ps.gz
	rm partiview.dvi

# 	note ps2pfg as well as pstopdf exist
pdf:
	ps2pdf partiview.ps

#		need psutils for pnsup (prints 2 pages on 1)
ps2:	ps
	psnup -2 partiview.ps > partiview2.ps


html:
	sgml2html partiview.sgml

clean:
	rm -f core *~  partiview.dvi partiview.ps

cleanall: clean
	rm -f partiview*.html partiview.txt partiview.ps.gz

# merely to update the webpage (only useful for teuben right now)

export:
	scp -C $(EXPORT) $(RDIR)

