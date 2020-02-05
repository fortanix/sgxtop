CC = clang
LDLIBS = -lcurses
CFLAGS = -g -Wall

# sgxstat is just a link to sgxtop
sgxstat:    sgxtop
	rm -f sgxstat
	ln sgxtop sgxstat

all:	sgxtop sgxstat
