default_target: all
.PHONY : default_target
LIBPACK=libpack/libpack.o
LIBPACKCCFLAGS=-Iinclude -Wall -std=c99 -g

all: compileLinux testLinux

compileLinux: $(LIBPACK)
	g++ -Wall -Wfatal-errors -g -std=c++1y -std=gnu++17 $(LIBPACK) Fixer.cpp -o linuxFixer

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


#todo: $(CC) must be addapted for mingw when compile for windows, and compileWin rule must be adapted to include compiled dll
libpack/%.o: libpack/%.c
	$(CC) -o $@ $(LIBPACKCCFLAGS) -c $<
$(LIBPACK): libpack/parse.o libpack/pack.o libpack/unpack.o libpack/stream.o libpack/file.o libpack/string.o
	$(LD) -r -o $@ $^

