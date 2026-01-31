#
# Makefile
#

PROGRAM=r4dcb08
VERSION=$(shell grep '^#define VERSION' revision.h | sed 's/.*"\(.*\)".*/\1/')

# Files - odstranění read_temp.c ze zdrojových souborů
SRC=packet.c serial.c monada.c now.c median_filter.c maf_filter.c main.c config.c error.c signal_handler.c help_functions.c read_functions.c write_functions.c scan.c
OBJ=$(SRC:.c=.o)
# Odstranění read_temp.h z hlavičkových souborů
HEAD=typedef.h revision.h define_error_resp.h packet.h serial.h monada.h now.h median_filter.h maf_filter.h config.h error.h signal_handler.h help_functions.h read_functions.h write_functions.h constants.h scan.h


# C compiler
CC = clang

# Compiler optimalisation (-O0 for debug, -O2 for ussual opt.)
OPT = -O2  

# For gdb, OPT must be -O0  (-g for gdb, -pd for gproof)
DBG = #-g

# Ostatni parametry prekladace (-Wall -Wextra -pedantic)
CFLAGS = -Wall -Wextra #-pedantic

# Linkovane knihovny  libefence.a = -lefence
LIBINCLUDE = -I ~/include
LIBPATH = -L ~/lib
LIB = # -lm -lfftw3 -l_matrix  #-lefence

# Cilum build, install, uninstall, clean a dist neodpovida primo zadny soubor
# (predstirany '.PHONY' target)

.PHONY: build
.PHONY: install
.PHONY: uninstall
.PHONY: clean
.PHONY: dist

# Prvni cil je implicitni, neni treba volat 'make build', staci 'make'.
# Cil build nema zadnou akci, jen zavislost.

build: $(PROGRAM) $(PROGRAM1)

# install závisi na prelozeni projektu, volat ho muze jen root
install: build
	cp $(PROGRAM) /usr/local/bin

# uninstall (only for root)
uninstall:
	rm -f /usr/local/bin/$(PROGRAM) 

# Clean files
clean:
	rm -f *.o $(PROGRAM) 

# Source package
dist:
	tar --exclude='*.o' --exclude='r4dcb08-mqtt' -czf $(PROGRAM)-$(VERSION).tgz $(SRC) $(HEAD) Makefile README.md LICENSE .gitignore doc/ mqtt_daemon/

# Linked
$(PROGRAM): $(OBJ) Makefile
	$(CC) $(LIBPATH) $(OBJ) $(DBG) $(LIB) -o $(PROGRAM)

.c.o: Makefile $(HEAD)
	$(CC) $(CFLAGS) $(OPT) $(LIBINCLUDE) $(DBG) -c $<
#
