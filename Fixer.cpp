#include <regex>
#include <string>
#include <iostream>
#include <fstream>
//#include <cstdlib>

//std::string binary = std::bitset<8>(n).to_string();

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
//windows platforms
	#define slash "\\"
	#define FFMPEG "ffmpeg.exe"
	#define FFPROBE "ffprobe.exe"
#else
//linux and others
	#define slash "/"
	#define FFMPEG "ffmpeg"
	#define FFPROBE "ffprobe"
#endif

#define WAX std::cout << "WAX" << std::endl
#define XAW(n) std::cout << "WAX " << n << std::endl

void die(const std::string& fname){
	std::cerr << fname << ": could not open file, error." << std::endl;
	exit(42);
}

void open_input_file(std::ifstream& inputFileStream, const std::string& inputFileName){
	try{
		inputFileStream.exceptions ( std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit );
		//inputFileStream.open(inputFileName, std::ifstream::binary | std::ifstream::in);
		inputFileStream.open(inputFileName, std::ios::in | std::ios::out | std::ios::binary);
	}
	catch (const std::ifstream::failure& e) {
		die(inputFileName + " " + e.what());
	}
	catch (...){
		die(inputFileName + " unknown exception");
	}
}

void open_output_file(std::ofstream& outputFileStream, const std::string& outputFileName){
	try{
		outputFileStream.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
		outputFileStream.open(outputFileName, std::ios::binary | std::ios::out);
	}
	catch (const std::ofstream::failure& e) {
		die(outputFileName + " " + e.what());
	}
	catch (...){
		die(outputFileName + " unknown exception");
	}
}

void buildIntermediateFile(const std::string& fileName, const std::string& cmdLine){
	std::ifstream ifile;
	ifile.open(fileName, std::ifstream::binary | std::ifstream::in);
	//if file does not exist, then we have to build it
	if(!ifile.is_open() || !ifile.good() || ifile.fail()){
		std::cout << "buildIntermediate: " << cmdLine << std::endl;
		system(cmdLine.c_str());
	}
	else{
		ifile.close();
	}
}

int main(int argc, char* argv[]){
	if(argc != 4){
		std::cout << "Usage: " << argv[0] << " <good_file.mp4> <bad_file.mp4> <output_prefix>" << std::endl;
		exit(0);
	}

	const std::string ffmpeg(FFMPEG),
							ffprobe(FFPROBE),
							goodfile(argv[1]),
							badfile(argv[2]),
							outfile_prefix(argv[3]),
							sample_h264(outfile_prefix + "-headers.h264"),
							sample_stat_h264(outfile_prefix + "-stat.mp4"),
							sample_aac(outfile_prefix + "-headers.aac"),
							sample_nals(outfile_prefix + "-nals.txt"),
							sample_nals_stat(outfile_prefix + "-nals-stat.txt"),
							out_video(outfile_prefix + "-out-video.h264"),
							out_audio(outfile_prefix + "-out-audio.raw"),
							sample_h264_cmdLine(ffmpeg + " -i " + goodfile + \
									" -c copy -frames 1 -bsf h264_mp4toannexb " + sample_h264),
							sample_stat_h264_cmdline(ffmpeg + " -i " + goodfile + \
									" -c copy -t 20 -an " + sample_stat_h264),
							sample_nals_cmdline(ffprobe + \
									" -select_streams 0 -show_packets -show_data " + \
									sample_stat_h264 + " > " + sample_nals),
							sample_aac_cmdline(ffmpeg + " -i " + goodfile + \
									" -c copy -t 1 -f adts " + sample_aac);
	//const std::array<const std::stringstream, 4> cmdlines = {

	std::cout << "Build intermediates..." << std::endl;
	buildIntermediateFile(sample_h264, sample_h264_cmdLine);
	buildIntermediateFile(sample_stat_h264, sample_stat_h264_cmdline);
	buildIntermediateFile(sample_nals, sample_nals_cmdline);
	buildIntermediateFile(sample_aac, sample_aac_cmdline);

	std::cout << "Opening files..." << std::endl;
	std::ifstream bfile, vhead, nals;
	std::ofstream vout, aout;
	open_input_file(bfile, badfile);
	open_input_file(vhead, sample_h264);
	open_input_file(nals, sample_nals);
	open_output_file(vout, out_video);
	open_output_file(aout, out_audio);

	//std::streamsize headerSize = 0x100;
	unsigned int headerSize = 0x100;
	char header[headerSize];
	std::string str;
		std::cout << "before fuck: " << header << std::endl;
	if(vhead.fail()){
		std::cerr << "fuck";
		exit(24);
	}
	try{
		//vhead.read(header, headerSize);
		/*
		 * std::string containme;
		vhead >> containme;
		std::cout << "containme size: " << containme.size() << std::endl;
		*/
WAX;
        str = std::string(headerSize, '\0'); // construct string to stream size
        vhead.seekg(0);
        vhead.read(&str[0], headerSize);
		  if(str.size() != headerSize){
		std::cout << "sizes: " << str.size() << " and " << headerSize << std::endl;
			die("could not read header from binary file.");
		  }

/*  std::filebuf* pbuf = vhead.rdbuf();
  std::size_t size = pbuf->pubseekoff (0,vhead.end,vhead.in);
  pbuf->pubseekpos (0,vhead.in);
	
		std::cout << "size: " << size << std::endl;
  char* buffer=new char[size];
  pbuf->sgetn (buffer,size);
  vhead.close();
  // write content to stdout
  //std::cout.write (buffer,size);
  delete[] buffer;
*/

			  /*
std::ostringstream out;
out << vhead.rdbuf();
std::string str = out.str();
		std::cout << "size: " << str.size() << std::endl;
*/

	}
	catch (const std::ifstream::failure& e) {
			die(e.what());
	}
	catch (...){
			die("unknown exception while reading");
	}

	std::cout << "(debug) number of characters read: " << vhead.gcount() << std::endl;
	std::cout << "(debug) str size: " << str.size() << std::endl;

	vhead.close();
	//todo: check why vhead is reopened in bin mode after its already read in original script


	/********************************************************************/
	/**************************** first RegExp **************************/
	/********************************************************************/

	std::regex pattern { "\\0\\0+\x01[\x65\x45\x25].+$" };
	//std::regex pattern { "^(.+)\x01\x65\xb8\x41.+$" };//another pattern that does the job but leaves trailing binary zeros, let's not use it
	
	//2 methods to do perform our substitution:
	// - using 'match' structure after a regexp_search
	// - using regexp_replace(): more intuitive and readable, but less powerful
	
	/* -> 1
	std::smatch mr;
	bool search_res = std::regex_search(str, mr, pattern);
	std::cout << "(debug) regex_search result: " << std::boolalpha << search_res << std::endl;
  if (mr.ready()) {
    std::cout << mr[0] << " found!\n";
    std::cout << mr[1] << " opla!\n";
    std::cout << mr[2] << " un dos tres!\n";
    std::cout << "prefix: [" << mr.prefix() << "]\n";
    std::cout << "suffix: [" << mr.suffix() << "]\n";
    std::cout << "mr.size: " << mr.size() << "]\n";
  }
	//mr already holds our result!
	//According to the way your substitution pattern looks like:
	// - if there are no look-aheads (groups formed using parenthesis): result is in mr.prefix or sufix
	// - if there are look-aheads: result is in mr[i] with i being the index of the look-ahead in the pattern
  	//So, if we were to use this method, result would be in mr.prefix()
	*/

	// -> 2
	const std::string replacement = "";
	std::string result;
	result = std::regex_replace(str, pattern, "$1");
	//std::regex_replace(back_inserter(result), str.begin(), str.end(), pattern, replacement, std::regex_constants::format_sed);



	//debug
	std::ofstream mydebugresult("mydebugresult", std::ios::out | std::ios::binary);
	mydebugresult << result;
	mydebugresult.close();
	std::ofstream mydebug("mydebug", std::ios::out | std::ios::binary);
	mydebug << str;
	mydebug.close();

	return 0;
}
