NAME    = busdataloader
OUTDIR  = bin

CC=gcc -g
CPP=g++ -g
CFLAGS  = -W -Wall -I/usr/include/sqlite3 -I/usr/include/stdlib.h -I/usr/include/stdio.h 
LDFLAGS += -arch x86_64
LIBS = -L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/usr/lib -L/usr/lib -lstdc++ -lsqlite3 -lcrt1.o -lc
CC  = clang
SRC = $(shell find . \( -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.m" -o -name "*.mm" -o -name "*.mx" -o -name "*.mxx" \) )
OBJ = $(addsuffix .o, $(basename $(SRC)))

%.o:    %.c
	@echo " + Compiling $<..."; $(CC) $(CFLAGS) -o $@ -c $<
%.o:    %.cc
	@echo " + Compiling $<..."; $(CC) $(CFLAGS) -o $@ -c $<
%.o:    %.cpp
	@echo " + Compiling $<..."; $(CC) $(CFLAGS) -o $@ -c $<
%.o:    %.cxx
	@echo " + Compiling $<..."; $(CC) $(CFLAGS) -o $@ -c $<

all:    prebuild $(OBJ) link end

prebuild:
	@echo " + Removing old files..."
	@rm -f $(OUTDIR)/$(NAME)

link:
	@echo " + Linking..."
	@test -d $(OUTDIR) || mkdir -p $(OUTDIR)
	$(LD) $(LDFLAGS) $^ $(LIBS) -u _main -o $(OUTDIR)/$(NAME) $(OBJ)

end:
	@echo " + Done. "
	
clean:
	@echo " + Cleaning project intermediate files..."
	@rm -f $(OBJ) *~ *\#
	@echo " + Done."