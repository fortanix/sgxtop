AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = sgxtop
sgxtop_SOURCES = sgxtop.c

install-exec-hook:
	(cd ${bindir}; ln -sf sgxtop sgxstat)

uninstall-hook:
	(cd ${bindir}; rm -fv sgxtop; rm -fv sgxstat)
