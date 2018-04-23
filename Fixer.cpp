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

#define NALS_FILE_COLUMNS_LENGTH_MAX	68


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

	unsigned int headerSize = 0x100;
	char header[headerSize];
	std::string str;
	try{
		//several failed attempts to put binary data into string:
		/*
			1 vhead.read(header, headerSize);
			2 vhead >> header;
			*/
		str = std::string(headerSize, '\0');
		vhead.seekg(0);
		vhead.read(&str[0], headerSize);
		if(str.size() != headerSize)
			die("could not read header from binary file.");
	}
	catch (const std::ifstream::failure& e) {
		die(e.what());
	}
	catch (...){
		die("unknown exception while reading");
	}

	/*
	std::cout << "(debug) number of characters read: " << vhead.gcount() << std::endl;
	std::cout << "(debug) str size: " << str.size() << std::endl;
*/

	vhead.close();


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
	std::string result;
	result = std::regex_replace(str, pattern, "$1");
	//const std::string replacement = "";
	//std::regex_replace(back_inserter(result), str.begin(), str.end(), pattern, replacement, std::regex_constants::format_sed);

	//debug
	std::ofstream mydebugresult("mydebugresult", std::ios::out | std::ios::binary);
	mydebugresult << result;
	mydebugresult.close();
	std::ofstream mydebug("mydebug", std::ios::out | std::ios::binary);
	mydebug << str;
	mydebug.close();

	//todo: check why vhead is reopened in bin mode after its already read in original script


	//get nals
	std::string buf;
	int size;
	typedef std::map<const std::string, int> t_nals_map;
	std::array<t_nals_map, 32> nalsMaps;//32 == 0b11111 + 0b1
	int i = 0;
	//for(nals_map& nm: nals){
	std::for_each(nalsMaps.begin(), nalsMaps.end(), [&i](t_nals_map& nm){
			nm = t_nals_map({
					{"min", 0xFFFFFF}, 
					{"max", 0x0},
					{"id", i++}
					});
			});

	std::string temp, nals_buf;

	//all of the following is made to prepare reading nals file
	//file is too big to be read with simple 'getline' function (in case you wondered)
	std::filebuf* pbuf = nals.rdbuf();
	std::size_t nalsSize = pbuf->pubseekoff (0,nals.end,nals.in);
	pbuf->pubseekpos (0,nals.in);
	char* nalsBuf=new char[nalsSize];
	pbuf->sgetn (nalsBuf,nalsSize);
	std::istringstream nals_iss(nalsBuf);
	char* line_buf=new char[NALS_FILE_COLUMNS_LENGTH_MAX];
	std::string fub = "";
	bool firstPacket = true;
	std::string result2;
	std::regex packetregex1 { "^\\[PACKET\\]" , std::regex::extended };
	std::regex packetregex2 { " ([0-9a-fA-F]{4})" , std::regex::extended };
   char *token = nullptr;
	//let's read it
	while(nals_iss.getline(line_buf, NALS_FILE_COLUMNS_LENGTH_MAX)){
		if(nals_iss.gcount() < NALS_FILE_COLUMNS_LENGTH_MAX){//header packet
			if(std::regex_match(line_buf, packetregex1) && !firstPacket){//header packet "PACKET"
				//std::cout << line_buf << std::endl;



				/*this solution takes much too much time
				 * result2 = std::regex_replace(fub, packetregex2, "");
				 std::cout << result2 << std::endl;
				 */
				//+ a lot of stuff to do here
			}
		}
		else{//not header packet -> binary data
			firstPacket = false;
			token = std::strtok(line_buf, " ");
			int ii = 0;
			while (ii < 8 && token != NULL) {
				token = std::strtok(NULL, " ");
				fub += token;
				++ii;
			}
		}
	}


	return 0;
}
