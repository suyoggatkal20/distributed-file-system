#include "http_controller.h"
#include "http_request_json.h"
#include "json.hpp"
#include "dbhelper.h"
#include "FileUtils.h"
#include "custom_exceptions.h"
#include <boost/filesystem.hpp>
#include "models.h"
#include "config.h"
#include "replica_storage_policy.h"
using json = nlohmann::json;

namespace fs = boost::filesystem;

HttpResponseJson HttpController::listing(HttpRequestJson request)
{
    HttpResponseJson responce;
    json j = request.getJsonBody();
    string path = j["path"];
    cout << "Serving request:\nFile listing:\n"
         << path << endl;
    MySQLDatabase db;
    db.connect();
    FileUtils file_utils(db);
    int dirID;
    try
    {
        dirID = file_utils.getDirectoryIdByPath(path);
    }
    catch (const DirectoryNotFoundException &e)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "Invalid path"}};
        responce.status_code = 200;
        return responce;
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }

    // cout << "dir id: " << dirID << endl;
    if (dirID > 0)
    {
        vector<FileModel> files;
        FileModel fileModelTemplate(0, "", "", 0L, 0, 0, "", "");
        try
        {
            files = db.executeTemplateQuery("select * from files where directory_id = " + std::to_string(dirID), fileModelTemplate);
        }
        catch (...)
        {
            responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
            responce.status_code = 200;
            return responce;
        }
        json files_json = json::array();
        for (size_t i = 0; i < files.size(); i++)
        {
            json file;
            file["name"] = files[i].file_name;
            file["size"] = files[i].file_size;
            file["chunks"] = files[i].num_chunks;
            file["created"] = files[i].created_at;
            file["updated"] = files[i].updated_at;
            files_json.push_back(file);
        }
        vector<DirectoryModel> dirs;
        DirectoryModel dirModel(0, 0, "", "", "");
        try
        {
            dirs = db.executeTemplateQuery("select * from directories where parent_directory_id = " + std::to_string(dirID), dirModel);
        }
        catch (...)
        {
            responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
            responce.status_code = 200;
            return responce;
        }
        json dirs_json = json::array();
        for (size_t i = 0; i < dirs.size(); i++)
        {
            json dir;
            dir["name"] = dirs[i].directory_name;
            dir["created"] = dirs[i].created_at;
            dirs_json.push_back(dir);
        }
        json responce_body;
        responce_body["files"] = files_json;
        responce_body["dirs"] = dirs_json;
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "200"}, {"status-msg", "Ok"}};
        responce.status_code = 200;
        responce.setJsonBody(responce_body);
        return responce;
    }
    else
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
}

HttpResponseJson HttpController::mkdir(HttpRequestJson request)
{
    HttpResponseJson responce;
    json j = request.getJsonBody();
    string path_string = j["path"];
    fs::path full_path(path_string);
    cout << "Serving request:\nMaking directory:\n"
         << path_string << endl;
    MySQLDatabase db;
    db.connect();

    string last_dir = full_path.filename().string();
    string rest_of_path = full_path.parent_path().string();

    FileUtils file_utils(db);
    int dirID;
    try
    {
        dirID = file_utils.getDirectoryIdByPath(rest_of_path);
    }
    catch (const DirectoryNotFoundException &e)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "Path not found"}};
        responce.status_code = 200;
        return responce;
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
    FileModel fileModelTemplate(0, "", "", 0L, 0, 0, "", "");
    vector<FileModel> files;
    try
    {
        files = db.executeTemplateQuery("select * from files where directory_id = " + std::to_string(dirID) + " and file_name = '" + last_dir + "'", fileModelTemplate);
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
    if (files.size() > 0)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "400"}, {"status-msg", "File of same name exist"}};
        responce.status_code = 200;
        return responce;
    }
    if (dirID > 0)
    {
        std::string query = "INSERT INTO directories (parent_directory_id, directory_name) VALUES (" + std::to_string(dirID) + ", ";
        query += "'" + last_dir + "');";

        try
        {
            db.executeQuery(query);
        }
        catch (const DatabaseConnectionException &e)
        {
            responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
            responce.status_code = 200;
            return responce;
        }
        catch (const DuplicateDbEntry &e)
        {
            responce.headers = {{"Content-Type", "META/JSON"}, {"status", "400"}, {"status-msg", "Directory already exist"}};
            responce.status_code = 200;
            return responce;
        }

        responce.status_code = 200;
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "200"}, {"status-msg", "Directory added"}};
        cout << "Directory added.\n\n";
        return responce;
    }
    else
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "Path does'nt exist"}};
        responce.status_code = 200;
        return responce;
    }
}

HttpResponseJson HttpController::put_file_req(HttpRequestJson request)
{
    HttpResponseJson responce;
    json j = request.getJsonBody();
    string path_string = j["path"];
    long long size = atoll(((string)j["size"]).c_str());

    fs::path full_path(path_string);

    MySQLDatabase db;
    db.connect();
    cout << "Serving request:\nPut file:\n"
         << path_string << " Size:" << size << endl;

    string last_dir = full_path.filename().string();
    string rest_of_path = full_path.parent_path().string();
    FileUtils file_utils(db);
    int dirID;
    try
    {
        dirID = file_utils.getDirectoryIdByPath(rest_of_path);
    }
    catch (const DirectoryNotFoundException &e)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "Invalid path"}};
        responce.status_code = 200;
        return responce;
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
    // checking if filename collide with dir
    int flag = 0;
    try
    {
        dirID = file_utils.getDirectoryIdByPath(path_string);
    }
    catch (const DirectoryNotFoundException &e)
    {
        flag = 1;
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
    if (flag == 0)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "400"}, {"status-msg", "Path already exist."}};
        responce.status_code = 200;
        return responce;
    }
    size_t replication = config.getInt("dfs.replication");
    string storage_policy = config.getString("dfs.replica-storage-policy");
    long long chunkSize = config.getLongLong("dfs.chunk-size");
    long long chunks = (size + chunkSize - 1) / chunkSize;
    ReplicaPolicyManager replicaPolicyManager;

    vector<std::vector<NodeModel>> nodes = replicaPolicyManager.selectReplicaNodesForChunks(db, chunks, replication, chunkSize);

    FileModel newFile;
    newFile.file_name = last_dir;
    newFile.file_path = rest_of_path;
    newFile.file_size = size;
    newFile.num_chunks = chunks;
    newFile.directory_id = dirID;
    try
    {
        newFile.insertRecord(db.getConnection());
    }
    catch (const DuplicateDbEntry &e)
    {
        responce.status_code = 200;
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "405"}, {"status-msg", "File already exist."}};
        return responce;
    }
    catch (...)
    {
        responce.status_code = 200;
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "400"}, {"status-msg", "Not added"}};
        return responce;
    }
    json body = replicaPolicyManager.updateDbAndGenerateResponceJson(db, nodes, newFile);
    // printf("Json : %s\n\n", body.dump().c_str());

    responce.status_code = 200;
    responce.headers = {{"Content-Type", "META/JSON"}, {"status", "200"}, {"status-msg", "Ok"}};
    responce.setJsonBody(body);
    return responce;
}

HttpResponseJson HttpController::get_file_req(HttpRequestJson request)
{
    HttpResponseJson responce;
    json j = request.getJsonBody();
    string path_string = j["path"];
    fs::path full_path(path_string);
    string last_dir = full_path.filename().string();
    string rest_of_path = full_path.parent_path().string();
    MySQLDatabase db;
    db.connect();
    cout << "path: " << full_path << endl;

    FileUtils file_utils(db);
    string query = "SELECT * FROM files WHERE file_name = '" + last_dir +
                   "' AND file_path = '" + rest_of_path + "'";
    FileModel newFile;
    try
    {
        FileModel fileTemplate;
        vector<FileModel> listOfFiles = db.executeTemplateQuery(query, fileTemplate);
        if (listOfFiles.size() < 1)
        {
            responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "File not found"}};
            responce.status_code = 200;
            return responce;
        }
        else
        {
            newFile = listOfFiles.front();
        }
    }
    catch (const DirectoryNotFoundException &e)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "404"}, {"status-msg", "Path not found"}};
        responce.status_code = 200;
        return responce;
    }
    catch (...)
    {
        responce.headers = {{"Content-Type", "META/JSON"}, {"status", "500"}, {"status-msg", "Database error"}};
        responce.status_code = 200;
        return responce;
    }
    ReplicaPolicyManager replicaPolicyManager;

    json body = replicaPolicyManager.getReplicaInfo(db, newFile);
    printf("Json : %s\n\n", body.dump().c_str());

    responce.status_code = 200;
    responce.headers = {{"Content-Type", "META/JSON"}, {"status", "200"}, {"status-msg", "Ok"}};
    responce.setJsonBody(body);
    return responce;
}

void findAndExtractStatus(const char *buffer, size_t bufferSize, size_t x, size_t y)
{
    const std::string targetSequence = "3287673535";
    const size_t targetLength = targetSequence.length();

    // Ensure the buffer is large enough
    if (bufferSize < x)
    {
        std::cerr << "Buffer is smaller than the specified size x. Cannot proceed." << std::endl;
        return;
    }

    // Search in the last 'x' bytes of the buffer
    const char *searchStart = buffer + bufferSize - x;

    // Find the target sequence within the last 'x' bytes
    const char *foundPos = std::strstr(searchStart, targetSequence.c_str());
    if (!foundPos)
    {
        std::cout << "Target sequence not found in the last " << x << " bytes." << std::endl;
        return;
    }

    // Calculate the position from the end
    size_t positionFromEnd = x - (foundPos - searchStart) - targetLength;
    std::cout << "Target sequence found at position " << positionFromEnd << " bytes from the end." << std::endl;

    // If position is exactly 'y' bytes from the end, extract status code and status message
    if (positionFromEnd == y)
    {
        std::cout << "Position is exactly " << y << " bytes from the end. Extracting data..." << std::endl;

        // Extract status_code and status_msg from the remaining part after the target sequence
        std::string remainingData(foundPos + targetLength, searchStart + x);

        size_t statusCodePos = remainingData.find("status_code:");
        size_t statusMsgPos = remainingData.find("status_msg:");

        if (statusCodePos == std::string::npos || statusMsgPos == std::string::npos)
        {
            std::cerr << "Status code or status message not found in the expected format." << std::endl;
            return;
        }

        // Extract status code
        size_t codeStart = statusCodePos + std::string("status_code:").length();
        size_t codeEnd = remainingData.find('\n', codeStart);
        std::string statusCode = remainingData.substr(codeStart, codeEnd - codeStart);

        // Extract status message
        size_t msgStart = statusMsgPos + std::string("status_msg:").length();
        std::string statusMsg = remainingData.substr(msgStart);

        // Print extracted values
        std::cout << "Extracted Status Code: " << statusCode << std::endl;
        std::cout << "Extracted Status Message: " << statusMsg << std::endl;
    }
    else if (positionFromEnd > y)
    {
        // If position is less than 'y' bytes from the end, print the last 'y' bytes as a string
        std::string lastYBytes(searchStart + x - y, searchStart + x);
        std::cout << "Position is less than " << y << " bytes from the end. Last " << y << " bytes: " << lastYBytes << std::endl;
    }
    else
    {
        // If position is more than 'y' bytes from the end, ignore it
        std::cout << "Position is more than " << y << " bytes from the end. Ignoring..." << std::endl;
    }
}
