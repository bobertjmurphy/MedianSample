// CommandLine.cpp
// Video analysis sample program
// Bob Murphy, May 2019

#include <getopt.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex>

#include "CommandLine.h"

static bool prvFileIsNormalFile(std::string &posixPath)
{
    // Fail for an empty path
    if (posixPath.empty()) {
        return false;
    }
    
    // Query the OS about what kind of item is at the path location
    struct stat statbuf;
    int status = lstat(posixPath.c_str(), &statbuf);    // lstat doesn't resolve symlinks
    if (status != 0) {      // Can't analyze anything at the location
        return false;
    }
    
    // Look for a regular file at that location. This sample doesn't try to resolve symlinks.
    mode_t fileType = statbuf.st_mode & S_IFMT;
    bool result = fileType == S_IFREG;
    return result;
}

static bool prvFileIsReadable(std::string &posixPath)
{
    bool result = false;
    int fd = open(posixPath.c_str(), O_RDONLY);
    if (fd >= 0) {
        result = true;
        close(fd);
    }
    return result;
}
static struct option sLongLoptions[] =
{
    {    "input",     required_argument, NULL, 'i'    },
    {    "dim",       required_argument, NULL, 'd'    },
    {    "output",    required_argument, NULL, 'o'    },
    {     NULL, 0, NULL, 0                        }
};

CommandLineArguments ProcessCommandLine(int argc, char **argv)
{
    CommandLineArguments result;
    result.m_Rows = 0;
    result.m_Cols = 0;
    
    // -------- Parse the command line arguments -------- 
    
    std::string dimensionStr;
    int ch = getopt_long(argc, argv, "i:d:o:", sLongLoptions, NULL);
    bool specifiedOutputFilepath = false;
    while (ch != -1)
    {
        // check to see if a single character or long option came through
        switch (ch)
        {
                // Input filepath
            case 'i':
                result.m_InputFilepath = optarg;
                break;
                
                // Dimensions
            case 'd':
                dimensionStr = optarg;
                break;
                
                // Output
            case 'o':
                specifiedOutputFilepath = true;
                result.m_OutputFilepath = optarg;
                break;
                
            default:
                usage(argv[0]);
                break;
        }
        
        // Prepare for the next iteration
        ch = getopt_long(argc, argv, "i:d:o:", sLongLoptions, NULL);
    }
    
    
    // -------- Interpret and validate the command line arguments -------- 
    
    bool errorFound = false;
    
    // Does the input filepath point to a readable file?
    if (result.m_InputFilepath.empty()) {
        fprintf(stderr, "Empty input filepath\n");
        errorFound = true;
    }
    else if (!prvFileIsNormalFile(result.m_InputFilepath)) {
        fprintf(stderr, "No input file at \"%s\"\n", result.m_InputFilepath.c_str());
        errorFound = true;
    }
    else if (!prvFileIsReadable(result.m_InputFilepath)) {
        fprintf(stderr, "Can't read input file at \"%s\"\n", result.m_InputFilepath.c_str());
        errorFound = true;
    }
    
    // Validate and interpret the dimensions string
    if (dimensionStr.empty()) {
        fprintf(stderr, "Empty dimensions string\n");
        errorFound = true;
    }
    else {
        const std::string cDimensionsRegexStr = "(\\d+)x(\\d+)";
        std::regex dimensionsRegex(cDimensionsRegexStr, std::regex_constants::ECMAScript);
        std::smatch smatch;
        bool matches = std::regex_match(dimensionStr, smatch, dimensionsRegex);
        if (!matches) {
            fprintf(stderr, "Invalid dimensions string\n");
            errorFound = true;
        }
        else {
            result.m_Rows = atoi(smatch[1].str().c_str());
            result.m_Cols = atoi(smatch[2].str().c_str());
            if (result.m_Cols == 0 || result.m_Rows == 0) {
                fprintf(stderr, "Zero values not allowed in dimensions string\n");
                errorFound = true;
            }
        }
    }
    
    // If the output filepath is specified, make sure the location can be written to
    if (specifiedOutputFilepath) {
        if (result.m_OutputFilepath.empty()) {
            fprintf(stderr, "Empty output filepath\n");
            errorFound = true;
        }
        else {
            // If a file is there, try to open it; if not, try to create a temporary file there
            bool createdAFile = false;
            int fd = open(result.m_OutputFilepath.c_str(), O_WRONLY);    // This will succeed if there's a writable file there
            if (fd < 0) {                                       // No file there, so try to create one
                fd = open(result.m_OutputFilepath.c_str(), O_WRONLY | O_CREAT, 0600);    // Readable and writable, but not executable
                createdAFile = (fd >= 0);
            }
            if (fd < 0) {
                fprintf(stderr, "Can't write output file at \"%s\"\n", result.m_OutputFilepath.c_str());
                errorFound = true;
            }
            else {
                close(fd);
                if (createdAFile) {             // Clean up the temporarily-created file
                    unlink(result.m_OutputFilepath.c_str());
                }
            }
        }
    }
    
    if (errorFound) {
        usage(argv[0]);
    }
    
    return result;
}

void usage(const char* exeName)
{
    fprintf(stderr, "Usage: %s --input <input movie file> --dim <NxM> [--output <output file>]\n", exeName);
    exit(-1);
};
