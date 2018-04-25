#include <map>

#include "Fixer.h"

void die(const std::string& fname){
	std::cerr << fname << ": could not open file, error." << std::endl;
	exit(42);
}

void debugWriteIntoFile(const char* bufferToDebug,
								const std::string& debugFileName,
								int bufferSize){
	std::ofstream debugMePlease(debugFileName, std::ios::out | std::ios::binary);
	debugMePlease.write(bufferToDebug, bufferSize);
	debugMePlease.close();
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
