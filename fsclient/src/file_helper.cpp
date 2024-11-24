#include <iostream>
#include <fstream>
#include <vector>
#include "file_helper.h"
#include "config.h"
using namespace std;

long findNearestNewline(ifstream &file, long offset, size_t chunkSize)
{
    // ifstream file(filePath, ios::binary);
    long originalOffset = offset;
    long max_record_size = config.getLong("max-record-size");

    // Ensure we do not seek beyond the start of the file
    if (offset < 0)
    {
        offset = 0;
    }

    // Read in chunks backwards
    while (offset > 0)
    {
        long readSize = min(static_cast<long>(chunkSize), offset);
        offset -= readSize;

        // Move to the new offset
        // file.seekg();
        file.seekg(offset);
        // Read the chunk into a buffer
        vector<char> buffer(readSize);
        file.read(buffer.data(), readSize);

        // Search for a newline character in the chunk
        for (long i = readSize - 1; i >= 0; --i)
        {
            if (readSize - i > max_record_size)
            {
                return -1;
            }
            if (buffer[i] == '\n')
            {
                return offset + i; // Return the position of the newline
            }
        }
    }

    // No newline found before the start of the fil
    return -1;
}
