#include "http_controller.h"
#include "http_request_json.h"
#include "json.hpp"
#include "dbhelper.h"
#include "FileUtils.h"
#include <fcntl.h>
#include "custom_exceptions.h"
#include <boost/filesystem.hpp>
#include "models.h"
#include "config.h"
#include "base64.h"
#include <sys/epoll.h>
#include "stdlib.h"
#include "unistd.h"
#include <xxhash.h>
#include <sys/socket.h>
#include <algorithm>

using json = nlohmann::json;

std::string responseTemplateString =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: META/BLOCK\r\n"
    "status: 500\r\n"
    "status-msg: Server error\r\n"
    "\r\n";
std::string responseTemplateStringSuccess =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: META/BLOCK\r\n"
    "status: 200\r\n"
    "status-msg: ok\r\n"
    "\r\n";

#define BUFFERSIZE 4096
#define TIMEOUT_MS 5000

namespace fs = boost::filesystem;

HttpResponseJson HttpController::get_block(HttpRequestJson request)
{
    HttpResponseJson response;
    MySQLDatabase db;
    db.connect();
    FileUtils fileUtils(db);
    Base64 base64;
    ChunkReplicasSlaveModel chunkReplicasSlaveModel;
    ChunkReplicasSlaveModel chunkReplicasSlaveModelTemplate(0, 0, 0, 0, 0, 0, "", "", "");
    size_t replica_id = atoll(request.headers["replica-id"].c_str());
    std::cout << "Delivering replica. Replica ID: " << replica_id << std::endl;
    chunkReplicasSlaveModel = db.executeTemplateQuery("SELECT * FROM chunk_replicas_slave where replica_id = " + std::to_string(replica_id), chunkReplicasSlaveModelTemplate).front();
    string path = fileUtils.findPathToNumberInNaryTree(chunkReplicasSlaveModel.id / config.getLong("dfs.fs-files-per-dir"), config.getLong("dfs.fs-tree-branch-number"));
    string finalPath = "/home/suyog" + path;

    string filePath = finalPath + "/replica" + std::to_string(chunkReplicasSlaveModel.replica_id) + ".blk";
    std::cout << "Reading file from File Path: " << filePath << std::endl;
    int file_fd = open(filePath.c_str(), O_RDONLY);
    if (file_fd == -1)
    {
        perror("Failed to open file");
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Can not open file");
        return response;
    }

    XXH64_state_t *state = XXH64_createState();
    if (state == nullptr)
    {
        std::cerr << "Error creating XXH64 state." << std::endl;
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Hash failed");
        return response;
    }
    // Initialize state with a seed value (optional, 0 means no custom seed)
    XXH64_reset(state, 0);

    char buffer[BUFFERSIZE];
    size_t total_byte_read = 0;

    ssize_t bytesSent = send(request.clientSocket, responseTemplateStringSuccess.c_str(), responseTemplateStringSuccess.length(), 0);
    int error = 0;
    if (bytesSent == -1 || bytesSent == 0)
    {
        perror("Failed to send data.");
        error = 1;
    }
    while (total_byte_read < chunkReplicasSlaveModel.chunk_size)
    {
        if (error > 0)
        {
            break;
        }
        if (chunkReplicasSlaveModel.chunk_size - total_byte_read == 0)
        {
            break;
        }
        ssize_t byte_read = read(file_fd, buffer, std::min(static_cast<size_t>(BUFFERSIZE), chunkReplicasSlaveModel.chunk_size - total_byte_read));
        if (byte_read == 0 || byte_read == -1)
        {
            perror("Reading data failed: ");
            error = 2;
            break;
        }

        total_byte_read += byte_read;

        XXH64_update(state, buffer, byte_read);

        ssize_t bytesSent = send(request.clientSocket, buffer, byte_read, 0);

        if (bytesSent == -1 || bytesSent != byte_read)
        {
            perror("Data sending failed: ");
            error = 1;
            break;
        }
    }

    uint64_t hash = XXH64_digest(state);
    string encoded = base64.encode(hash);
    if (encoded != chunkReplicasSlaveModel.data_hash)
    {
        error = 3;
    }
    std::ostringstream oss;
    if (error == 0)
    {
        oss << std::setw(30) << std::setfill(' ') << config.getString("dfs.magic-number") << "\n" // Magic number fixed to 30 bytes
            << "status_code:" << std::setw(3) << std::setfill('0') << 200 << "\n"
            << "status_msg:" << std::setw(30) << std::setfill(' ') << std::left << "Valid data";
        send(request.clientSocket, oss.str().c_str(), oss.str().length(), 0);
    }
    else if (error == 1)
    {
        printf("Error not recoverable.");
    }
    else if (error == 2)
    {
        oss << std::setw(30) << std::setfill(' ') << config.getString("dfs.magic-number") << "\n" // Magic number fixed to 30 bytes
            << "status_code:" << std::setw(3) << std::setfill('0') << 501 << "\n"
            << "status_msg:" << std::setw(30) << std::setfill(' ') << std::left << "Data block reading failed";
        send(request.clientSocket, oss.str().c_str(), oss.str().length(), 0);
    }
    else if (error == 3)
    {
        oss << std::setw(30) << std::setfill(' ') << config.getString("dfs.magic-number") << "\n" // Magic number fixed to 30 bytes
            << "status_code:" << std::setw(3) << std::setfill('0') << 502 << "\n"
            << "status_msg:" << std::setw(30) << std::setfill(' ') << std::left << "Data block is corrupted";
        send(request.clientSocket, oss.str().c_str(), oss.str().length(), 0);
    }
    close(file_fd);
    XXH64_freeState(state);
    if (shutdown(request.clientSocket, SHUT_WR) == -1)
    {
        perror("shutdown failed");
    }
    close(request.clientSocket);
    response.status_code = -1;
    return response;
}

HttpResponseJson HttpController::put_block(HttpRequestJson request)
{
    HttpResponseJson response;
    MySQLDatabase db;
    db.connect();
    FileUtils fileUtils(db);
    Base64 base64;
    ChunkReplicasSlaveModel chunkReplicasSlaveModel;
    chunkReplicasSlaveModel.chunk_size = atoll(request.headers["chunk-size"].c_str());
    chunkReplicasSlaveModel.chunk_id = atoll(request.headers["chunk-id"].c_str());
    chunkReplicasSlaveModel.chunk_order = atoll(request.headers["chunk-order"].c_str());
    chunkReplicasSlaveModel.file_id = atoll(request.headers["file-id"].c_str());
    chunkReplicasSlaveModel.file_path = request.headers["file-path"];
    chunkReplicasSlaveModel.replica_id = atoll(request.headers["replica-id"].c_str());
    size_t content_length = atoll(request.headers["Content-Length"].c_str());

    std::cout << "Accepting block:\n"
              << "Chunk size:" << chunkReplicasSlaveModel.chunk_size << "\n"
              << "chunk_id:" << chunkReplicasSlaveModel.chunk_id << "\n"
              << "chunk_order:" << chunkReplicasSlaveModel.chunk_order << "\n"
              << "file_path:" << chunkReplicasSlaveModel.file_path << "\n"
              << "replica_id:" << chunkReplicasSlaveModel.replica_id << "\n"
              << "content_length:" << content_length << "\n";
    chunkReplicasSlaveModel.insertRecord(db.getConnection());

    string path = fileUtils.findPathToNumberInNaryTree(chunkReplicasSlaveModel.id / config.getLong("dfs.fs-files-per-dir"), config.getLong("dfs.fs-tree-branch-number"));

    string finalPath = "/home/suyog" + path;

    fileUtils.createDirectories(finalPath);

    string filePath = finalPath + "/replica" + std::to_string(chunkReplicasSlaveModel.replica_id) + ".blk";
    std::cout << "Storing block as a file: " << filePath << std::endl;
    std::ofstream newfile(filePath, std::ios::binary | std::ios::app);

    if (!newfile.is_open())
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Block reading failed.");
        return response;
    }
    XXH64_state_t *state = XXH64_createState();
    if (state == nullptr)
    {
        std::cerr << "Error creating XXH64 state." << std::endl;
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Hash function failed");
        return response;
    }
    XXH64_reset(state, 0);
    size_t byte_read = -1;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("Failed to create epoll file descriptor");
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Epoll failed");
        return response;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = request.clientSocket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, request.clientSocket, &event) == -1)
    {
        perror("Failed to add file descriptor to epoll");
        response.status_code = 200;
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Epoll failed");
        return response;
    }

    char buffer[BUFFERSIZE];
    size_t total_byte_read;
    while (1)
    {
        struct epoll_event events;
        int nfds = epoll_wait(epoll_fd, &events, 1, TIMEOUT_MS);

        if (nfds == -1)
        {
            perror("Failed epoll:");
            response.status_code = 200;
            response.addHeader("Content-Type", "META/JSON");
            response.addHeader("status", "500");
            response.addHeader("status-msg", "Epoll failed");
            return response;
        }
        else if (nfds == 0)
        {
            perror("No data from client:");
            response.status_code = 200;
            response.addHeader("Content-Type", "META/JSON");
            response.addHeader("status", "403");
            response.addHeader("status-msg", "No data from client");
            return response;
        }
        else
        {
            if (events.events & EPOLLIN)
            {
                byte_read = read(request.clientSocket, buffer, BUFFERSIZE < (content_length - total_byte_read) ? BUFFERSIZE : content_length - total_byte_read);
                total_byte_read += byte_read;

                if (byte_read > 0)
                {
                    XXH64_update(state, buffer, byte_read);
                    fileUtils.writeToFile(newfile, buffer, byte_read);
                    if (content_length == total_byte_read)
                    {
                        break;
                    }
                }
                else if (byte_read == 0)
                {
                    printf("End of data stream.\n");
                    break;
                }
                else
                {
                    perror("Reading data from client failed:");
                    response.status_code = 200;
                    response.addHeader("Content-Type", "META/JSON");
                    response.addHeader("status", "500");
                    response.addHeader("status-msg", "Reading data from client failed.");
                    return response;
                }
            }
        }
    }
    close(epoll_fd);
    uint64_t hash = XXH64_digest(state);
    string encoded = base64.encode(hash);
    chunkReplicasSlaveModel.data_hash = encoded;
    std::cout << "Hash of block:\n"
              << chunkReplicasSlaveModel.data_hash << "\n";
    chunkReplicasSlaveModel.updateHash(db.getConnection());
    string blockPrefix = fileUtils.getForattedString(chunkReplicasSlaveModel);

    fileUtils.writeToFile(newfile, blockPrefix.c_str(), blockPrefix.size());
    newfile.close();
    XXH64_freeState(state);
    response.status_code = 200;
    response.addHeader("Content-Type", "META/JSON");
    response.addHeader("status", "200");
    response.addHeader("status-msg", "Data written to file");
    std::cout << "Request of service finished\n"
              << "\n\n";
    return response;
}
