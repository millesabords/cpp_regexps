#include <regex>
#include <string>
#include <ctime>

#include "Fixer.h"


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
	std::string header;
	try{
		header = std::string(headerSize, '\0');
		vhead.seekg(0);
		vhead.read(&header[0], headerSize);
		if(header.size() != headerSize)
			die("could not read header from binary file.");
	}
	catch (const std::ifstream::failure& e) {
		die(e.what());
	}
	catch (...){
		die("unknown exception while reading");
	}

	vhead.close();

	std::regex pattern { "\\0\\0+\x01[\x65\x45\x25].+$" };

	//2 methods to do perform our substitution:
	// - using 'match' structure after a regexp_search
	// - using regexp_replace(): more intuitive and readable, but less powerful

	/* -> 1
		std::smatch mr;
		bool search_res = std::regex_search(header, mr, pattern);
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
	result = std::regex_replace(header, pattern, "$1");
	//const std::string replacement = "";
	//std::regex_replace(back_inserter(result), header.begin(), header.end(), pattern, replacement, std::regex_constants::format_sed);

	//todo: check why vhead is reopened in bin mode after its already read iops


	//get nals   ---> not used for the moment
	std::string buf;
	typedef std::map<const std::string, stringOrInt> t_nals_map;
	std::array<t_nals_map, 32> nalsMaps;//32 == 0b11111 + 0b1
	int i = 0;
	std::for_each(nalsMaps.begin(), nalsMaps.end(), [&i](t_nals_map& nm){
			nm = t_nals_map({
					{"min", stringOrInt(0xFFFFFF)}, 
					{"max", stringOrInt(0x0)},
					{"id", stringOrInt(i++)}
					});
			});

	//the following is made to prepare reading nals file
	//file is too big to be read with direct 'ifstream::getline' (too many syscalls I guess)
	//getlines will have to be made on a buffered stream
	std::filebuf* pbuf = nals.rdbuf();
	std::size_t nalsSize = pbuf->pubseekoff (0,nals.end,nals.in);
	pbuf->pubseekpos (0,nals.in);
	char* nalsBuf=new char[nalsSize];
	pbuf->sgetn (nalsBuf,nalsSize);
	std::istringstream nals_iss(nalsBuf);
	char* line_buf=new char[NALS_FILE_COLUMNS_LENGTH_MAX];
	std::string fub = "";
	std::string bytes = "";
	bool firstPacket = true;
	std::regex packetregex1 { "^\\[PACKET\\]" , std::regex::extended };
	std::regex packetregex2 { " ([0-9a-fA-F]{4})" , std::regex::extended };
   char *token = nullptr;
	int ii, local_size, type;
	//let's read it
	while(nals_iss.getline(line_buf, NALS_FILE_COLUMNS_LENGTH_MAX)){

		if(nals_iss.gcount() < NALS_FILE_COLUMNS_LENGTH_MAX){//header packet
			if(nals_iss.gcount() > 1 &&
					!strstr(line_buf, "PACKET") &&
					!firstPacket){//header packet "PACKET"

				while(1){
					std::istringstream(fub.substr(0,8)) >> std::hex >> local_size;
					std::cout << "NAL: " << local_size << "\n";
					if((int)fub.length() >= (local_size*2) + 8){

						std::istringstream(fub.substr(8,2)) >> std::hex >> type;
						type &= 0b11111;
						std::istringstream(fub.substr(8,type==5?6:4)) >> std::hex >> bytes;
						std::cout << "type= " << type << "\n";
						std::cout << "bytes= " << bytes << "\n";
						short unsigned int cbb[3];

						//packing by hand (turning back ascii hex value as binary hex value)
						//I wish there was something like Perl pack("H*", ...) in C++
						//check out and test libpack if really necessary
						for ( unsigned int j = 0; j < bytes.length() / 2 ; j++) {
							sscanf( (bytes.substr(j*2,2)).c_str(), "%hx", &cbb[j]);
						}


						std::cout << "cbb=" << cbb << "wax\n";

/*
if(type != 5){
						std::cout << "debugging pack for 4 digits hex number when type != 5 TODO: cbb=" << cbb << "\n";
						exit(45);
}

						t_nals_map& localNalsMapRef = nalsMaps[type];//localNalsMapRef is '$n' iops
						if(localNalsMapRef["min"].get_intVal() > local_size)
							localNalsMapRef["min"].set_intVal(local_size);
						if(localNalsMapRef["max"].get_intVal() < local_size)
							localNalsMapRef["max"].set_intVal(local_size);
						*TODO put {$bytes} and {$printbytes} hash maps into localNalsMapRef
						 *
						 * {$bytes} is needed to build audio restitution file later.
						 * {$printbytes} doesn't seem to be used iops
						 *
						 * {$bytes} is a list of dereferences, that is still unclear to me
						 *	t_hashsMap tmp_map;
						 * for i in ? do {
						 *	std::make_pair<std::string, int> tmp_pair(something with $bytes);
						 *	tmp_map[tmp_pair] = 1;
						 * }
						 * localNalsMapRef["bytes"].set_mapVal(tmp_map));
						 * localNalsMapRef["printbytes"].set_mapVal(samesamebutuseless)
						 */
							
						fub = fub.substr(8+(local_size*2), 0);
					}
					else{
						break;
					}
						
						std::cout << "Remain " << fub.length() << ": " << fub.substr(0, 32) << "\n";
						fub = "";//todo check if deleting this doesn't crash anything
				}

			}
		}
		else{//not header packet -> binary data

			firstPacket = false;
			ii = 0;
			token = std::strtok(line_buf, " ");//forget first token
			while (ii < 8 && token != NULL) {//forget last token (number 9)
				token = std::strtok(NULL, " ");
				fub += token;
				++ii;
			}
		}
	}

	vout << header;

	//int was_key = 0;
	//std::string shit = "";//(still iops)
	int blocksize = 10000000;
	
	try{
		pbuf = bfile.rdbuf();
		char* fileContentBuf = new char[blocksize];
		char* concatBuffer = nullptr, *swapBuffer = nullptr;
		int concatBufferSize = 0, swapBufferSize = 0;
		while(1){
			//filling B
			std::streamsize sizeRead = pbuf->sgetn (fileContentBuf,blocksize);
			if(sizeRead <= 10)
				break;
			sizeRead -= 10;//iops
			//let's do S = A + B:
			swapBufferSize = concatBufferSize + sizeRead;
			swapBuffer = (char*) realloc((char*) swapBuffer,(1 + swapBufferSize) * sizeof(char*));
			if(concatBuffer != nullptr){
				strncpy(swapBuffer, concatBuffer, concatBufferSize);//(S=A)
				free(concatBuffer);
				concatBuffer = nullptr;
			}
			memcpy((char*)(swapBuffer + concatBufferSize), (const char *) fileContentBuf, sizeRead);//(S+=B)
			concatBuffer = swapBuffer;//A = S
			swapBuffer = nullptr;//S = null
			concatBufferSize = swapBufferSize;

//debugWriteIntoFile(concatBuffer, "mydebug", concatBufferSize);

			//now we have our $file in 'concatBuffer' and $fsize in 'concatBufferSize'
		  	//(iops equivalent is '$file=$file.$buf;')

			//std::time_t s_time = std::time(nullptr);
			//std::cout << "quest-ce que le temps?" << std::asctime(std::localtime(&s_time));
		   for(int jj = 0; jj < concatBufferSize; jj++){//todo: find out why iops the guy is iterating one by one whereas he is reading four by four
				//let's unpack things

//in construction
unsigned long mylong = 42;
unsigned char myshort = 'Z';
char *qbuf = new char[4];
memcpy((char*)qbuf, (char*)concatBuffer, 4);
sscanf(qbuf+3, "%hhu", &myshort);//can't get myshort this way...
sscanf(qbuf+2, "%d", &mylong);
std::cout << "mylong=" << mylong << std::endl;
std::cout << "myshort=" << myshort << std::endl;
std::cout << "qbuf[0]=" << qbuf[0] << std::endl;
std::cout << "qbuf[1]=" << qbuf[1] << std::endl;
std::cout << "qbuf[2]=" << qbuf[2] << std::endl;
std::cout << "qbuf[3]=" << qbuf[3] << std::endl;
exit(23);

			}
		}
		}
	catch (const std::ifstream::failure& e) {
		die(e.what());
	}
	catch (...){
		die("unknown exception while reading");
	}

	return 0;
}
