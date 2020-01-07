LDLIBS = -lcurses
CFLAGS = -g

# sgxstat is just a link to sgxtop
sgxstat:    sgxtop
	rm -f sgxstat
	ln sgxtop sgxstat

all:	sgxtop sgxstat
