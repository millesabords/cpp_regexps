What this is? is a portable C++ translation of Perl 'mp4fixer' tool from Bookkojot at: https://github.com/bookkojot/mp4fixer

Requirements:
- make sure to have ffmpeg and ffprobe installed on your system
- For windows users under linux: you need 'i686-w64-mingw32-g++' and 'wine' to cross-compile and test under linux
- For windows users under windows (make sense?): don't know. Not tested under a real windows environment yet. TODO

How to use:
$ make
$ ./linuxFixer good.mp4 broken.mp4 output
$ #for windows users, replace 'linuxFixer' by 'winFixer.exe'

Remarks:
- C++11 regexp, which come from Boost regexp, uses PCRE format ("perl compatible regular expressions").
	So basically, copy-pasting regexps from perl script to C++ source file should be ok.
	Nevertheless, some characters better be escaped twice, such as '\0' which has to be written '\\0'
- C++ regexp api only works with strings, and std::strings are not friends with binary info data
- 'hexdump -C' is often used to check results when working on binary data
- binary files are not read using conventionnal strings because c++ strings misinterpret binary files and should be avoided
- Speaking about Perl and regexps, here is a good reading suggestion:
	https://www.perl.com/pub/2003/06/06/regexps.html/
	https://www.perl.com/pub/2003/07/01/regexps.html/
- External libraries (could be) used:
	libpack C portable library than emulates "pack" utility from Perl, with MIT license:http://www.leonerd.org.uk
- debugs & todos:
	//auto headerPattern {R""s};

	size_t trailing_zeros = result.find_last_not_of('\0') + 1;//note that C++ string api interprets binary zero as '\0'
	result.erase(trailing_zeros, result.size()-1);

			//todo, try efficiency of using this syntax:
			/*
std::string subject("This is a test");
try {
  std::regex re("\\w+");
  std::sregex_iterator next(subject.begin(), subject.end(), re);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    std::cout << match.str() << "\n";
    next++;
  } 
} catch (std::regex_error& e) {
  // Syntax error in the regular expression
}
*/
