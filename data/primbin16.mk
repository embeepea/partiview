# Recipe to create primbin16.out, from Piet's mail:
# Use as in:
#   make -f primbin16.mk

SHELL = /bin/sh

primbin16.out:
	makeplummer -n 8 -i -s 123 | \
	  makesecondary -i -f 1 | \
	  scale -m 1 -e -0.25 -q 0.5 | \
	  makebinary -l 2 -u 2 | \
	  kira -t 10 -d 30 -n 0 -D x25 \
	    > primbin16.out  2> primbin16.log
