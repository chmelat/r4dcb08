#
# Makefile
#

PROGRAM=modbus
VERSION=0.1

# Files
SRC=packet.c serial.c monada.c now.c main.c
OBJ=$(SRC:.c=.o)
HEAD=typedef.h revision.h packet.h serial.h monada.h now.h


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

# List of valid suffixes through the use of the .SUFFIXES special target.
#.SUFFIXES: .c .o

# Prvni cil je implicitni, neni treba volat 'make build', staci 'make'.
# Cil build nema zadnou akci, jen zavislost.

build: $(PROGRAM) $(PROGRAM1)

# install závisi na prelozeni projektu, volat ho muze jen root
install: build
	cp $(PROGRAM) ~/bin

# uninstall (only for root)
uninstall:
	rm -f ~/bin/$(PROGRAM) 

# Clean files
clean:
	rm -f *.o $(PROGRAM) 

# Source package
dist:
	tar czf $(PROGRAM)-$(VERSION).tgz $(SRC) $(HEAD) Makefile

# Linked
$(PROGRAM): $(OBJ) Makefile
	$(CC) $(LIBPATH) $(OBJ) $(DBG) $(LIB) -o $(PROGRAM)

.c.o: Makefile $(HEAD)
	$(CC) $(CFLAGS) $(OPT) $(LIBINCLUDE) $(DBG) -c $<
