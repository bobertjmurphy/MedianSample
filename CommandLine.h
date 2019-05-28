// CommandLine.h
// Video analysis sample program
// Bob Murphy, May 2019

#ifndef __COMMANDLINE_H__
#define __COMMANDLINE_H__ 1

#include <string>

struct CommandLineArguments
{
    std::string m_InputFilepath;
    std::string m_OutputFilepath;
    int         m_Cols;
    int         m_Rows;
};

CommandLineArguments    ProcessCommandLine(int argc, char **argv);
void usage(const char* exeName);

#endif // __COMMANDLINE_H__
