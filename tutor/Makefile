SHELL = /bin/sh

showme: memdemo
	@echo "*** Starting consumer program (which prints to stdout) ***"
	memdemo -c &
	@echo "*** Starting producer program (which reads from stdin) ***"
	@echo "*** Type a few lines of text, and then press control-D ***"
	memdemo -p

memdemo:	memdemo.c
	${CC} -o $@ $@.c
