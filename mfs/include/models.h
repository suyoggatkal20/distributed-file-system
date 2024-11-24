#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <mysql/mysql.h>
#include <dbhelper.h>

// ChunkReplicaModel Class Declaration
class ChunkReplicaModel : public DBEntity<ChunkReplicaModel>
{
public:
    int replica_id;
    int chunk_id;
    int node_id;
    std::string created_at;

    ChunkReplicaModel(int replica_id, int chunk_id, int node_id, const std::string &created_at);
    ChunkReplicaModel();

    // Override mapRowToEntity function
    ChunkReplicaModel mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const override;
    bool insertRecord(MYSQL* conn);
};

// Chunk Class Declaration
class ChunkModel : public DBEntity<ChunkModel>
{
public:
    int chunk_id;
    int file_id;
    int chunk_order;
    long long chunk_size;
    std::string created_at;

    ChunkModel(int chunk_id, int file_id, int chunk_order, long long chunk_size, const std::string &created_at);
    ChunkModel();

    ChunkModel mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const override;

    bool insertRecord(MYSQL *conn);
};

// Directory Class Declaration
class Directory : public DBEntity<Directory>
{
public:
    int directory_id;
    int parent_directory_id;
    std::string directory_name;
    std::string created_at;
    std::string updated_at;

    // Constructor to initialize the Directory object
    Directory(int directory_id, int parent_directory_id, const std::string &directory_name,
              const std::string &created_at, const std::string &updated_at);

    // Override mapRowToEntity function
    Directory mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const override;
};

class FileModel : public DBEntity<FileModel>
{
public:
    int file_id;
    std::string file_name;
    std::string file_path;
    long long file_size;
    long long num_chunks;
    int directory_id;
    std::string created_at;
    std::string updated_at;

    // Constructor declaration
    FileModel();
    FileModel(int file_id, const std::string &file_name, const std::string &file_path,
              long long file_size, long long num_chunks, int directory_id,
              const std::string &created_at, const std::string &updated_at);

    // Method to map row to entity declaration
    FileModel mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const;
    bool insertRecord(MYSQL *conn);
};

class DirectoryModel : public DBEntity<DirectoryModel>
{
public:
    int directory_id;
    int parent_directory_id;
    std::string directory_name;
    std::string created_at;
    std::string updated_at;

    // Constructor declaration
    DirectoryModel(int directory_id, int parent_directory_id, const std::string &directory_name,
                   const std::string &created_at, const std::string &updated_at);

    // Method to map row to entity declaration
    DirectoryModel mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const;
};

class NodeModel : public DBEntity<NodeModel>
{
public:
    // Constructor
    NodeModel(int id, const std::string &ip, long totalStorage, long freeSpace, const std::string &status);
    NodeModel();

    // Method to map MYSQL_ROW to NodeModel object
    NodeModel mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const;

    // Member variables
    int node_id;
    std::string node_ip;
    long total_storage;
    long free_space;
    std::string status;
};

#endif
