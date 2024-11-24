#ifndef DFSFILEOPERATIONS_H
#define DFSFILEOPERATIONS_H

#include <string>

class DfsFileOperations
{
public:
    // Constructor
    DfsFileOperations();

    // Member functions
    void listDirectory(const std::string &path);
    void createDirectory(const std::string &path);
    void putFile(const std::string &filePath, const std::string &dirPath);
    void getFile(const std::string &remoteFilePath, const std::string &localPath);

private:
    // You can add any private member variables here if needed
};

#endif // DFSFILEOPERATIONS_H
