#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include "config.h"
#include "file_helper.h"
#include "http_client_helper.h"
#include "json.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include "dfs_file_operations.h"
using json = nlohmann::json;

#define PORT 31004
#define MAX_CONNECTIONS 10

Properties config;

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <option> <path> [file]" << std::endl;
        std::cerr << "Options: -l <dfs path>  : List directory" << std::endl;
        std::cerr << "         -c <dfs path>  : Create directory" << std::endl;
        std::cerr << "         -p <local source file path> <dfs destination path> : Put file into directory" << std::endl;
        std::cerr << "         -g <dfs source path> <local destination path> : Get file from directory" << std::endl;
        return 1;
    }
    config.loadProperties("config.properties");
    // Parse arguments
    int opt;
    std::string path, filePath, dirPath;
    DfsFileOperations dfsOps;
    auto start = std::chrono::high_resolution_clock::now();
    while ((opt = getopt(argc, argv, "l:c:p:g:")) != -1)
    {
        switch (opt)
        {
        case 'l':
            path = optarg;
            dfsOps.listDirectory(path);
            break;
        case 'c':
            path = optarg;
            dfsOps.createDirectory(path);
            break;
        case 'p':
            if (argc < 4)
            {
                std::cerr << "Error: File path and directory are required for -p option." << std::endl;
                return 1;
            }
            filePath = argv[2];
            dirPath = argv[3];
            dfsOps.putFile(filePath, dirPath);
            break;
        case 'g':
            if (argc < 4)
            {
                std::cerr << "Error: File path and directory are required for -g option." << std::endl;
                return 1;
            }
            filePath = argv[2];
            dirPath = argv[3];
            dfsOps.getFile(filePath, dirPath);
            break;
        default:
            std::cerr << "Invalid option. Use -l, -c, -p, or -g." << std::endl;
            return 1;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duration.count() << " miliseconds" << std::endl;
    return 0;
}
