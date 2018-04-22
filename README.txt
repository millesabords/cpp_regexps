What this is? is a portable C++ translation of Perl 'mp4fixer' tool from Bookkojot at: https://github.com/bookkojot/mp4fixer

Why this is? I don't really know. Still not sure if this is a good idea, though.

My advice? pattern matching with boost/C++11 regexp api is not the tool.
- If you need to write something quickly, prefer using long approved scripting tools like sed, awk, grep, Perl, etc
- If you need performance, use Flex/Bison, or Boost Spirit
- Use C++11 regexp api only if you don't have the choice.

Requirements:
- make sure to have ffmpeg and ffprobe installed on your system
- For windows users under linux: you need 'i686-w64-mingw32-g++' and 'wine' to cross-compile and test under linux
- For windows users under windows (make sense?): don't know. Not tested under a real windows environment yet. TODO

How to use:
$ make
$ ./linuxFixer good.mp4 broken.mp4 output
$ #for windows users, replace 'linuxFixer' by 'winFixer.exe'

Methodology used in order to adapt c++ regexp to Perl ones:
C++11 regexp, which come from Boost regexp, uses PCRE format ("perl compatible regular expressions").
So basically, copy-pasting regexps from perl script to C++ source file should be ok.
Nevertheless, there are some inconveniences when working with binary files:
- C++ regexp api only works with strings, and binary file streams are hard to turn correctly into std::strings (constructor might stubornely interpret some control characters)
- What took me hours to figure out was that binary zeros in a C++ regexp must be written '\\0',
	unlike with C++ strings with which it needs to be written '\0'.

Furthermore, if you want to check binary output file after C++ treatment, you can make use of 'hexdump -C' on it:




Following is my debug code buffer:
	//auto headerPattern {R""s};
	std::string containme;
	for(int i = 0; i< 0x100; i++){
		containme += (unsigned char) header[i];
	}
	std::cout << "containme size: " << containme.size() << std::endl;


	size_t trailing_zeros = result.find_last_not_of('\0') + 1;//note that C++ string api interprets binary zero as '\0'
	result.erase(trailing_zeros, result.size()-1);
