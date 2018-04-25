#include <iostream>
#include <fstream>

//'iops' in comments means "in original perl script"
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

void die(const std::string& fname);

void debugWriteIntoFile(const char* bufferToDebug,
								const std::string& debugFileName,
								int bufferSize);

void open_input_file(std::ifstream& inputFileStream,
		const std::string& inputFileName);


void open_output_file(std::ofstream& outputFileStream,
		const std::string& outputFileName);

void buildIntermediateFile(const std::string& fileName,
		const std::string& cmdLine);

/*
//Nals map is a list of maps
//underlying maps can be either string to int maps
//or string to another underlying map (checkout 'printbytes' keyword in output--nals-stats.txt to understand)
//script languages such as perl may have totally different types inside a same array
//we are emulating the same thing here, by defining this ambiguous type
//*/
class stringOrInt{
	public:
		enum disambig{
			E_INTT,
			E_MAPP
		};
		typedef std::map<const std::string, int> t_hashsMap;

	private:
		int intVal;
		t_hashsMap mapVal;
		disambig dd;

	public:
		stringOrInt(){
			intVal = 0;
			dd = E_INTT;
		}
		stringOrInt(int value){
			intVal = value;
			dd = E_INTT;
		}
		stringOrInt(t_hashsMap& value){
			mapVal = value;
			dd = E_MAPP;
		}
		int get_intVal(){
			if(dd == E_INTT)
				return intVal;
			else{
				die("tried to access stringOrInt value as int, whereas it is a map");
				return 0;
			}
		}
		t_hashsMap& get_mapVal(){
			if(dd != E_MAPP)
				die("tried to access stringOrInt value as map, whereas it is an int");
			return mapVal;
		}
		disambig get_stringOrIntDisambig() const{
			return dd;
		}
		void set_intVal(int value){
			dd = E_INTT;
			intVal = value;
		}
		void set_mapVal(t_hashsMap& value){
			dd = E_MAPP;
			mapVal = value;
		}
};
