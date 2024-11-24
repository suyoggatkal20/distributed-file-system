#include "dfs_file_operations.h"
#include <map>
#include <iostream>
#include "json.hpp" // Include nlohmann/json header
#include <curl/curl.h>
#include "config.h"
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>  // For open()
#include <unistd.h> // For read() and close()
#define TRAILER_SIZE 78
#define ACTUAL_TRAILER_SIZE 88
#define BUFFER_SIZE 1024

using json = nlohmann::json;

// Constructor implementation
DfsFileOperations::DfsFileOperations()
{
}
void putFileChunk(const std::string &fileName, const std::string &ip,
                  int chunkSize, int chunkId, int replicaId, int chunkOrder, int fileId, const std::string &destFilePath, size_t defaultChunkSize);
int getAndCombileFile(size_t replicaId, std::string &nodeIp, size_t default_chunk_size, int fileFd);

bool parseStatusMessage(char *buffer, int &status, std::string &statusMsg);

int findAndExtractStatus(char *buffer, size_t bufferSize, size_t x, size_t y, size_t &status_code, std::string &msg);

std::map<std::string, std::string> headers_map;
size_t headerCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    // The size of one header line is size * nmemb
    size_t headerSize = size * nmemb;
    if (headerSize == 0)
        return 0; // Return if no header data

    // Convert the header data (ptr) to a C++ string
    std::string headerLine(static_cast<char *>(ptr), headerSize);

    // Find the position of the colon that separates the key and value in the header
    size_t separatorPos = headerLine.find(':');
    if (separatorPos != std::string::npos)
    {
        // Extract the header name and value
        std::string headerName = headerLine.substr(0, separatorPos);
        std::string headerValue = headerLine.substr(separatorPos + 1);

        // Trim spaces from the header name and value
        headerName.erase(headerName.find_last_not_of(" \t\r\n") + 1);
        headerValue.erase(0, headerValue.find_first_not_of(" \t\r\n"));
        headerValue.erase(headerValue.find_last_not_of("\r\n") + 1);

        // Store the header in the global map
        headers_map[headerName] = headerValue;
    }

    return headerSize;
}

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    std::string string = (char *)contents;
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

size_t writeCallbackHugeData(void *contents, size_t size, size_t nmemb, void *userp)
{
    // File descriptor passed as user data
    int fd = *((int *)userp);

    // Total size of data received in this callback
    size_t totalSize = size * nmemb;

    // Write all the received data to the file in one go
    ssize_t written = write(fd, contents, totalSize);
    if (written == -1)
    {
        std::cerr << "Error writing to file!" << std::endl;
        return 0; // Return 0 to indicate failure to libcurl
    }

    // Return the total size of data processed
    return totalSize;
}

// List directory in DFS
void DfsFileOperations::listDirectory(const std::string &path)
{
    CURL *curl;
    CURLcode res;

    // Retrieve server address (this can be done using your configuration method)
    std::string serverAddr = config.getString("server-addr"); // e.g., "172.30.117.150:8080"
    std::string url = "http://" + serverAddr + "/ls";         // Construct the URL for the POST request

    // Prepare the JSON data for the POST request using nlohmann/json
    json jsonData;
    jsonData["path"] = path;
    std::string jsonString = jsonData.dump(); // Convert json object to string

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set the HTTP headers
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());

        // Prepare a string to store the response from the server
        std::string responseString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, nullptr); // We don't need any data to be passed to the callback

        res = curl_easy_perform(curl);

        // Check for errors in the request
        if (res != CURLE_OK)
        {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // std::map<std::string, std::string>::const_iterator it = headers_map.begin();
            // while (it != headers_map.end())
            // {
            //     std::cout << "Header: " << it->first << " Value: " << it->second << std::endl;
            //     ++it;
            // }

            if (headers_map["status"] == "200")
            {
                json responseJson = json::parse(responseString);
                std::cout << "\nDirectories:\n";
                std::cout << std::left << std::setw(50) << "Name"
                          << std::setw(30) << "Created" << "\n";
                std::cout << std::string(70, '-') << "\n";

                for (const auto &dir : responseJson["dirs"])
                {
                    std::cout << std::left << std::setw(50) << dir["name"].get<std::string>()
                              << std::setw(30) << dir["created"].get<std::string>() << "\n";
                }

                std::cout << "\nFiles:\n";
                std::cout << std::left << std::setw(50) << "Name"
                          << std::setw(10) << "Size"
                          << std::setw(10) << "Chunks"
                          << std::setw(30) << "Created"
                          << std::setw(30) << "Updated" << "\n";
                std::cout << std::string(120, '-') << "\n";

                for (const auto &file : responseJson["files"])
                {
                    std::cout << std::left << std::setw(50) << file["name"].get<std::string>()
                              << std::setw(10) << file["size"].get<int>()
                              << std::setw(10) << file["chunks"].get<int>()
                              << std::setw(30) << file["created"].get<std::string>()
                              << std::setw(30) << file["updated"].get<std::string>() << "\n";
                }
            }
            else
            {
                std::cout << "Error: " << headers_map["status-msg"] << std::endl;
            }
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

// Create directory in DFS
void DfsFileOperations::createDirectory(const std::string &path)
{
    CURL *curl = nullptr;
    CURLcode res;

    // Retrieve the server address
    std::string serverAddr = config.getString("server-addr");
    if (serverAddr.empty())
    {
        std::cerr << "Server address is empty. Check configuration.\n";
        return;
    }

    // Construct the URL
    std::string url = "http://" + serverAddr + "/mkdir";

    // Prepare JSON data
    json jsonData;
    jsonData["path"] = path;
    std::string jsonString = jsonData.dump();

    // Initialize CURL
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        std::cerr << "CURL global initialization failed: " << curl_easy_strerror(res) << "\n";
        return;
    }

    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize CURL.\n";
        curl_global_cleanup();
        return;
    }

    struct curl_slist *headers = nullptr;
    std::string responseString;

    try
    {
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!headers)
        {
            throw std::runtime_error("Failed to set CURL headers.");
        }

        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if (res != CURLE_OK)
        {
            throw std::runtime_error("Failed to set CURL HTTP headers: " + std::string(curl_easy_strerror(res)));
        }
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
        if (res != CURLE_OK)
        {
            throw std::runtime_error("Failed to set POST data: " + std::string(curl_easy_strerror(res)));
        }

        // Set up the write callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, nullptr); // We don't need any data to be passed to the callback

        // Perform the CURL request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
        }

        // Parse the JSON response
        try
        {
            if (headers_map["status"] == "200")
            {
                std::cerr << "Directory created" << "\n";
            }
            else
            {
                std::cerr << "Directory creation failed: " << headers_map["status-msg"] << "\n";
            }
        }
        catch (const json::exception &e)
        {
            std::cerr << "Failed to parse server response: " << e.what() << "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    // Clean up
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    if (headers)
    {
        curl_slist_free_all(headers);
    }

    // Ensure CURL global cleanup even in case of errors
    curl_global_cleanup();
}

void DfsFileOperations::putFile(const std::string &filePath, const std::string &dirPath)
{
    CURL *curl = nullptr;
    CURLcode res;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    // Check if the file was opened successfully
    if (!file.is_open())
    {
        std::cerr << "Failed to open the file: " << filePath << std::endl;
    }

    // Get the file size by calling tellg() after seeking to the end
    size_t fileSize = file.tellg();
    // std::cout << "File Size: " << fileSize << std::endl;
    std::string serverAddr = config.getString("server-addr");
    if (serverAddr.empty())
    {
        std::cerr << "Server address is empty. Check configuration.\n";
        return;
    }

    std::string url = "http://" + serverAddr + "/put/req";

    json jsonData;
    jsonData["path"] = dirPath;
    jsonData["size"] = std::to_string(fileSize);
    file.close();
    std::string jsonString = jsonData.dump();

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl)
    {
        std::cerr << "Failed to initialize CURL.\n";
        curl_global_cleanup();
        return;
    }

    struct curl_slist *headers = nullptr;
    std::string responseString;

    try
    {
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!headers)
        {
            throw std::runtime_error("Failed to set CURL headers.");
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());

        // Set up the write callback to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, nullptr); // We don't need any data to be passed to the callback

        // Perform the CURL request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
        }

        // Parse the JSON response
        try
        {
            // printf("json:\n%s\n\n", responseString.c_str());
            if (headers_map["status"] == "200")
            {
                json responseJson = json::parse(responseString);
                size_t file_id = responseJson["file-id"];
                size_t chunk_size_default = responseJson["chunk-size"];
                for (const auto &chunk : responseJson["chunks"])
                {
                    int chunkOrder = chunk["chunk-order"];
                    int chunkSize = chunk["chunk-size"];

                    // Iterate over each replica for the current chunk
                    for (const auto &replica : chunk["replicas"])
                    {
                        int chunkId = replica["chunk-id"];
                        int replicaId = replica["replica-id"];
                        std::string nodeIp = replica["node-ip"];
                        // std::cout << "chunk-size: " << chunkSize << "\n"
                        //           << "chunk-id: " << chunkId << "\n"
                        //           << "replica-id: " << replicaId << "\n"
                        //           << "chunk-order: " << chunkOrder << "\n"
                        //           << "file-id: " << file_id << "\n" // Assuming file-id is known, you can adjust this accordingly
                        //           << "file-path: /new/doc/hello1.py\n\n";
                        putFileChunk(filePath, nodeIp, chunkSize, chunkId, replicaId, chunkOrder, file_id, dirPath, chunk_size_default);
                        // Print the desired information
                    }
                }
            }
            else
            {
                std::cerr << "Failed to submit file upload request: " << headers_map["status-msg"] << "\n";
            }
        }
        catch (const json::exception &e)
        {
            std::cerr << "Failed to parse server response: " << e.what() << "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    // Clean up
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

// Get file from DFS directory
void DfsFileOperations::getFile(const std::string &remoteFilePath, const std::string &localPath)
{
    CURL *curl = nullptr;
    CURLcode res;
    std::string serverAddr = config.getString("server-addr");
    if (serverAddr.empty())
    {
        std::cerr << "Server address is empty. Check configuration.\n";
        return;
    }

    std::string url = "http://" + serverAddr + "/get/req";

    json jsonData;
    jsonData["path"] = remoteFilePath;
    std::string jsonString = jsonData.dump();

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl)
    {
        std::cerr << "Failed to initialize CURL.\n";
        curl_global_cleanup();
        return;
    }

    struct curl_slist *headers = nullptr;
    std::string responseString;

    try
    {
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!headers)
        {
            throw std::runtime_error("Failed to set CURL headers.");
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());

        // Set up the write callback to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, nullptr); // We don't need any data to be passed to the callback

        // Perform the CURL request
        res = curl_easy_perform(curl);
        std::string noNConstPath = localPath;
        int fileFd = open(noNConstPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600); // Mode 0600 for minimum permissions
        if (fileFd == -1)
        {
            if (errno == EEXIST)
            {
                std::cout << "File already exists, not opening it." << std::endl;
            }
            else
            {
                std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            }
            return;
        }
        if (res != CURLE_OK)
        {
            throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
        }

        // Parse the JSON response
        try
        {
            printf("json:\n%s\n\n", responseString.c_str());
            json responseJson = json::parse(responseString);
            size_t default_chunk_size = responseJson["chunk-size"];
            for (const auto &chunk : responseJson["chunks"])
            {
                int chunkOrder = chunk["chunk-order"];
                int chunkSize = chunk["chunk-size"];
                int flag = 0;
                for (const auto &replica : chunk["replicas"])
                {
                    std::string nodeIp = replica["node-ip"];
                    int replicaId = replica["replica-id"];

                    // Print details for each replica
                    std::cout << "Chunk Order: " << chunkOrder << "\n"
                              << "Chunk Size: " << chunkSize << "\n"
                              << "Node IP: " << nodeIp << "\n"
                              << "Replica ID: " << replicaId << "\n"
                              << "default chunk size: " << default_chunk_size << "\n"
                              << "--------------------------" << std::endl;
                    size_t recordOffset = lseek(fileFd, 0, SEEK_CUR);

                    if (getAndCombileFile(replicaId, nodeIp, default_chunk_size, fileFd))
                    {
                        flag = 1;
                        break;
                    }
                    else
                    {
                        if (ftruncate(fileFd, recordOffset) == -1)
                        {
                            perror("Failed to truncate file");
                            return;
                        }
                        else
                        {
                            std::cout << "Data retrieval failed from replica." << "\n";
                        }
                    }
                }
                if (flag == 0)
                {
                    std::cout << "Tried all replicas but data can not be retrievd." << "\n";
                }
            }
        }
        catch (const json::exception &e)
        {
            std::cerr << "Failed to parse server response: " << e.what() << "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    // Clean up
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

void putFileChunk(const std::string &fileName, const std::string &ip_port, int chunkSize, int chunkId, int replicaId,
                  int chunkOrder, int fileId, const std::string &destFilePath, size_t defaultChunkSize)
{
    long long offset = static_cast<long long>(chunkOrder - 1) * defaultChunkSize;

    size_t dataSize = chunkSize;
    size_t colonPos = ip_port.find(':');
    int port;
    std::string ip;
    if (colonPos != std::string::npos)
    {
        ip = ip_port.substr(0, colonPos);
        port = std::stoi(ip_port.substr(colonPos + 1));
    }
    // Open the file
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 1)
    {
        std::cerr << "file open failed" << std::endl;
    }
    off_t fileSize = lseek(fd, 0, SEEK_END);

    size_t totalFileSize = lseek(fd, 0, SEEK_CUR);

    // printf("offset: %lld", offset);
    if (offset >= totalFileSize)
    {
        std::cerr << "Offset exceeds file size. Cannot read beyond file end." << std::endl;
        return;
    }
    lseek(fd, offset, SEEK_SET);
    // printf("offset last: %lld", lseek(fd, 0, SEEK_CUR));
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Configure server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(sock);
        return;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection to server failed");
        close(sock);
        return;
    }

    // Construct HTTP POST request headers
    std::ostringstream requestStream;
    requestStream << "POST /put HTTP/1.1\r\n"
                  << "Host: " << ip << ":" << port << "\r\n"
                  << "chunk-size: " << chunkSize << "\r\n"
                  << "chunk-id: " << chunkId << "\r\n"
                  << "replica-id: " << replicaId << "\r\n"
                  << "chunk-order: " << chunkOrder << "\r\n"
                  << "file-id: " << fileId << "\r\n"
                  << "file-path: " << destFilePath << "\r\n"
                  << "Content-Length: " << dataSize << "\r\n"
                  << "Connection: close\r\n\r\n";

    // Send the request headers
    std::string request = requestStream.str();
    if (send(sock, request.c_str(), request.size(), 0) < 0)
    {
        perror("Failed to send headers");
        close(sock);
        return;
    }

    // Send the file chunk
    char buffer[1024];
    size_t remaining = dataSize;
    while (remaining > 0)
    {
        // printf("Remaining bytes: %lld", remaining);
        size_t toRead = std::min(remaining, sizeof(buffer) - 1);
        // printf("to read bytes: %lld", toRead);
        size_t bytesRead = read(fd, buffer, toRead);
        // printf("actual read bytes: %lld", bytesRead);
        buffer[bytesRead] = '\0';
        // printf("New buff: %s ", buffer);
        if (bytesRead > 0)
        {
            size_t bytes_sent = send(sock, buffer, bytesRead, 0);
            // std::cout << "Output: bytes send: " << bytes_sent;
            if (bytes_sent < 0)
            {
                perror("Failed to send file chunk");
                close(sock);
                return;
            }
            remaining -= bytesRead;
        }
        else
        {
            printf("error: %zu ", bytesRead);
            break;
        }
    }
    if (shutdown(sock, SHUT_WR) < 0)
    {
        perror("Shutdown failed");
        close(sock);
    }
    // Receive the server's response
    char response[4096];
    int received = recv(sock, response, sizeof(response) - 1, 0);
    int status = 0;
    std::string status_msg;
    if (received > 0)
    {
        response[received] = '\0';
        parseStatusMessage(response, status, status_msg);
        if (status == 200)
        {
            std::cout << "Chunk stored successfully. Chunk ID: " << chunkId << " Replica ID: " << replicaId << std::endl;
        }
        else
        {
            std::cout << "Error in storing chunk: " << status_msg << std::endl;
        }
    }

    close(sock);
    close(fd);
}

int getAndCombileFile(size_t replicaId, std::string &nodeIp, size_t default_chunk_size, int fileFd)
{
    size_t colonPos = nodeIp.find(':');
    int port;
    std::string ip;
    if (colonPos != std::string::npos)
    {
        ip = nodeIp.substr(0, colonPos);
        port = std::stoi(nodeIp.substr(colonPos + 1));
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return -1;
    }
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection to server failed");
        close(sock);
        return -1;
    }

    // Construct HTTP POST request headers
    std::ostringstream requestStream;
    requestStream << "POST /get HTTP/1.1\r\n"
                  << "Host: " << ip << ":" << port << "\r\n"
                  << "replica-id: " << replicaId << "\r\n"
                  << "Content-Length: " << 0 << "\r\n"
                  << "Connection: close\r\n\r\n";

    // Send the request headers
    std::string request = requestStream.str();
    if (send(sock, request.c_str(), request.size(), 0) < 0)
    {
        perror("Failed to send headers");
        close(sock);
        return -1;
    }
    if (shutdown(sock, SHUT_WR) < 0)
    {
        perror("Shutdown failed");
        close(sock);
        return -1;
    }

    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char *buffer;
    int byteRead = 0, prevByteRead;
    long i = 0;
    // printf("started reading:\n\n");
    char headers[BUFFER_SIZE];
    int k = 0;
    char a1 = 0, a2 = 0, a3 = 0, a4 = 0;
    while (k < BUFFER_SIZE)
    {
        prevByteRead = byteRead;
        byteRead = read(sock, headers + k, 1);
        a1 = a2;
        a2 = a3;
        a3 = a4;
        a4 = *(headers + k);
        if (a1 == '\r' && a2 == '\n' && a3 == '\r' && a4 == '\n')
        {
            break;
        }
        k++;
    }

    if (k >= BUFFER_SIZE)
    {
        printf("Error: Very long headers\n");
        return 0;
    }
    else
    {
        headers[k + 1] = '\0';
    }
    while (1)
    {
        i = i + 1;
        if (i % 2)
        {
            buffer = buffer1;
        }
        else
        {
            buffer = buffer2;
        }
        prevByteRead = byteRead;
        byteRead = read(sock, buffer, BUFFER_SIZE);
        if (byteRead <= 0)
        {
            break;
        }

        if (write(fileFd, buffer, byteRead) < 0)
        {
            perror("write to file system failed");
            return -1;
        }
    }
    // printf("\n\nfinished reading\n\n");
    char result[2 * BUFFER_SIZE + 1];
    // printf("i=%ld\n", i);
    int status = 0;
    size_t status_code;
    std::string status_message;
    if (i <= 2)
    {
        std::memcpy(result, buffer1, prevByteRead);
        result[prevByteRead] = '\0';
        status = findAndExtractStatus(result, prevByteRead, TRAILER_SIZE, TRAILER_SIZE, status_code, status_message);
    }
    else if (i % 2)
    {
        std::memcpy(result, buffer1, BUFFER_SIZE);
        std::memcpy(result + BUFFER_SIZE, buffer2, prevByteRead);
        result[BUFFER_SIZE + prevByteRead] = '\0';
        status = findAndExtractStatus(result, BUFFER_SIZE + prevByteRead, TRAILER_SIZE, TRAILER_SIZE, status_code, status_message);
    }
    else
    {
        std::memcpy(result, buffer2, BUFFER_SIZE);
        std::memcpy(result + BUFFER_SIZE, buffer1, prevByteRead);
        result[BUFFER_SIZE + prevByteRead] = '\0';
        status = findAndExtractStatus(result, BUFFER_SIZE + prevByteRead, TRAILER_SIZE, TRAILER_SIZE, status_code, status_message);
    }
    size_t currentOffset = lseek(fileFd, 0, SEEK_CUR);
    if (ftruncate(fileFd, currentOffset - ACTUAL_TRAILER_SIZE) == -1)
    {
        perror("Failed to truncate file");
        return 0;
    }
    currentOffset = lseek(fileFd, -ACTUAL_TRAILER_SIZE, SEEK_CUR);

    if (status)
    {
        std::cout << "\n\nBlock retrived\nserver status code: " << status_code << "\nServer status Message: " << status_message << "\n\n";
        close(sock);
        return 1;
    }
    close(sock);
    return 0;
}

int findAndExtractStatus(char *buffer, size_t bufferSize, size_t x, size_t y, size_t &status_code, std::string &msg)
{
    const std::string targetSequence = "32876735353287673535";
    const size_t targetLength = targetSequence.length();

    // Check if buffer is large enough for x bytes
    if (bufferSize < x)
    {
        std::cerr << "Buffer is smaller than " << x << " bytes. Cannot proceed." << std::endl;
        return -1;
    }

    // Search within the last x bytes
    const char *searchStart = buffer + bufferSize - x;
    const char *foundPos = std::strstr(searchStart, targetSequence.c_str());

    if (!foundPos)
    {
        std::cout << "Target sequence not found in the last " << x << " bytes." << std::endl;
        return -1;
    }

    // Calculate position of target sequence from the end
    size_t positionFromEnd = x - (foundPos - searchStart) - targetLength;

    // std::cout << "Target sequence found at position " << positionFromEnd << " bytes from the end." << std::endl;

    if (positionFromEnd == y - targetLength)
    {
        // Exactly y bytes from the end
        // std::cout << "Sequence is exactly " << y << " bytes from the end. Extracting data..." << std::endl;

        // Extract status_code and status_msg
        std::string remainingData(foundPos + targetLength, searchStart + x);
        size_t statusCodePos = remainingData.find("status_code:");
        size_t statusMsgPos = remainingData.find("status_msg:");

        if (statusCodePos == std::string::npos || statusMsgPos == std::string::npos)
        {
            std::cerr << "Status code or status message not found in the expected format." << std::endl;
            return -1;
        }

        // Extract status code
        size_t codeStart = statusCodePos + std::string("status_code:").length();
        size_t codeEnd = remainingData.find('\n', codeStart);
        std::string statusCode = remainingData.substr(codeStart, codeEnd - codeStart);

        // Extract status message
        size_t msgStart = statusMsgPos + std::string("status_msg:").length();
        std::string statusMsg = remainingData.substr(msgStart);
        status_code = atoi(statusCode.c_str());
        msg = statusMsg;
        return 1;
    }
    else if (positionFromEnd < y - targetLength)
    {
        // Less than y bytes from the end
        std::cout << "Sequence is less than " << y << " bytes from the end. Printing last " << y << " bytes..." << std::endl;

        std::string lastYBytes(searchStart + x - y, y);
        std::cout << "Last " << y << " bytes: " << lastYBytes << std::endl;
        return 0;
    }
    else
    {
        // More than y bytes from the end
        std::cout << "Sequence is more than " << y << " bytes from the end. Ignoring." << std::endl;
        return 0;
    }
}

bool parseStatusMessage(char *buffer, int &status, std::string &statusMsg)
{
    const char *statusKey = "status: ";
    const char *statusMsgKey = "status-msg: ";

    // Find the position of the status and status-msg keys in the buffer
    const char *statusPos = strstr(buffer, statusKey);
    const char *statusMsgPos = strstr(buffer, statusMsgKey);

    // Check if both keys are found in the buffer
    if (statusPos && statusMsgPos)
    {
        // Extract the status value (move the pointer to the value)
        statusPos += strlen(statusKey);
        status = std::atoi(statusPos); // Convert to integer

        // Extract the status-msg value (move the pointer to the value)
        statusMsgPos += strlen(statusMsgKey);
        const char *endOfStatusMsg = strchr(statusMsgPos, '\n'); // Find newline or end of string
        if (endOfStatusMsg)
        {
            statusMsg = std::string(statusMsgPos, endOfStatusMsg);
        }
        else
        {
            statusMsg = std::string(statusMsgPos); // Until end of input
        }

        return true;
    }

    return false; // If format not found
}