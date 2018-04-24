default_target: all
.PHONY : default_target

all: compileLinux testLinux

compileLinux:
	g++ -Wall -Wfatal-errors -g -std=c++1y -std=gnu++17 Fixer.cpp -o linuxFixer

supercompile:
	g++ -Wall -Wfatal-errors -std=gnu++17 -O4 Fixer.cpp -o linuxFixer

testLinux:
	./linuxFixer good.mp4 broken.mp4 output

#portable filesystem api "experimental::filesystem::exists()" C++17
#can be used linking "-lstdc++fs" and including "<experimental/filesystem>" with recent gcc versions
#but still does not exist in mingw yet, too bad
#...or make use of <boost/filesystem.hpp>?

compileWin: $(LIBPACK)
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -static-libgcc -static-libstdc++ -o winFixer.exe Fixer.cpp

testWin:
	wine winFixer.exe good.mp4 broken.mp4 output


#if you need pack() functionnality in C++, compile and integrate (adapt the compilation directives accordingly) the following library:
LIBPACK=libpack/libpack.o
LIBPACKCCFLAGS=-Iinclude -Wall -std=c99 -g
libpack/%.o: libpack/%.c
	$(CC) -o $@ $(LIBPACKCCFLAGS) -c $<
$(LIBPACK): libpack/parse.o libpack/pack.o libpack/unpack.o libpack/stream.o libpack/file.o libpack/string.o
	$(LD) -r -o $@ $^

