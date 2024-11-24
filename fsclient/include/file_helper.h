#ifndef FILE_HELPER
#define FILE_HELPER
#include <fstream>
using namespace std;

long findNearestNewline(ifstream& file, long offset, size_t chunkSize);

#endif 
