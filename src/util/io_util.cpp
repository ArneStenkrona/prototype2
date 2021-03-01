#include "io_util.h"

prt::vector<char> io_util::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t) file.tellg();
    prt::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    
    file.close();
    
    return buffer;
}

bool io_util::is_file_exist(char const * fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

static constexpr size_t MAX_FILENAME_LEN = 512;
static constexpr size_t ABSOLUTE_NAME_START = 1; // Perhaps should be 3 for windos systems
#define SLASH '/' // Perhaps should be '\\' for windows systems
// Thanks: https://www.codeguru.com/cpp/misc/misc/fileanddirectorynaming/article.php/c263/Function-to-Determine-a-Files-Relative-Path.htm
void io_util::getRelativePath(char const * currentDirectory, char const * absoluteFilename, char * relativeFilename) {
	// declarations - put here so this should work in a C compiler
	size_t afMarker = 0, rfMarker = 0;
	size_t cdLen = 0, afLen = 0;
	size_t i = 0;
	size_t levels = 0;
	cdLen = strlen(currentDirectory);
	afLen = strlen(absoluteFilename);
	
	// make sure the names are not too long or too short
	if(cdLen > MAX_FILENAME_LEN || cdLen < ABSOLUTE_NAME_START+1 || 
		afLen > MAX_FILENAME_LEN || afLen < ABSOLUTE_NAME_START+1) {
		assert(false && "Relative path is to long or short");
        return;
	}
	
	// Handle DOS names that are on different drives:
	if(currentDirectory[0] != absoluteFilename[0]) {
		// not on the same drive, so only absolute filename will do
		strcpy(relativeFilename, absoluteFilename);
		return;
	}
	// they are on the same drive, find out how much of the current directory
	// is in the absolute filename
	i = ABSOLUTE_NAME_START;
	while(i < afLen && i < cdLen && currentDirectory[i] == absoluteFilename[i])
	{
		i++;
	}
	if(i == cdLen && (absoluteFilename[i] == SLASH || absoluteFilename[i-1] == SLASH))
	{
		// the whole current directory name is in the file name,
		// so we just trim off the current directory name to get the
		// current file name.
		if(absoluteFilename[i] == SLASH)
		{
			// a directory name might have a trailing slash but a relative
			// file name should not have a leading one...
			i++;
		}
		strcpy(relativeFilename, &absoluteFilename[i]);
		return;
	}
	// The file is not in a child directory of the current directory, so we
	// need to step back the appropriate number of parent directories by
	// using "..\"s.  First find out how many levels deeper we are than the
	// common directory
	afMarker = i;
	levels = 1;
	// count the number of directory levels we have to go up to get to the
	// common directory
	while(i < cdLen)
	{
		i++;
		if(currentDirectory[i] == SLASH)
		{
			// make sure it's not a trailing slash
			i++;
			if(currentDirectory[i] != '\0')
			{
				levels++;
			}
		}
	}
	// move the absolute filename marker back to the start of the directory name
	// that it has stopped in.
	while(afMarker > 0 && absoluteFilename[afMarker-1] != SLASH)
	{
		afMarker--;
	}
	// check that the result will not be too long
	if(levels * 3 + afLen - afMarker > MAX_FILENAME_LEN)
	{
		assert(false && "Result is too long");
		return;
	}
	
	// add the appropriate number of "..\"s.
	rfMarker = 0;
	for(i = 0; i < levels; i++)
	{
		relativeFilename[rfMarker++] = '.';
		relativeFilename[rfMarker++] = '.';
		relativeFilename[rfMarker++] = SLASH;
	}
	// copy the rest of the filename into the result string
	strcpy(&relativeFilename[rfMarker], &absoluteFilename[afMarker]);
	return;
}
