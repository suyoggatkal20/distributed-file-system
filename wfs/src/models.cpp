#include "dbhelper.h"
#include "models.h"
#include "custom_exceptions.h"
#include "custom_exceptions.h"

// ChunkReplica Class Definition
ChunkReplicaModel::ChunkReplicaModel(int replica_id, int chunk_id, int node_id, const std::string &created_at)
    : replica_id(replica_id), chunk_id(chunk_id), node_id(node_id), created_at(created_at) {}
ChunkReplicaModel::ChunkReplicaModel() {}

ChunkReplicaModel ChunkReplicaModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{
    return ChunkReplicaModel(
        std::stoi(row[0]),  // replica_id
        std::stoi(row[1]),  // chunk_id
        std::stoi(row[2]),  // node_id
        std::string(row[3]) // created_at (assuming it is the fourth column)
    );
}

// Insert record into the 'chunk_replicas' table
bool ChunkReplicaModel::insertRecord(MYSQL *conn)
{
    // Prepare the insert query for the 'chunk_replicas' table
    std::string insert_query = "INSERT INTO chunk_replicas (chunk_id, node_id) "
                               "VALUES (" +
                               std::to_string(chunk_id) + ", " +
                               std::to_string(node_id) + ")";

    // Execute the insert query
    if (mysql_query(conn, insert_query.c_str()))
    {
        // Check for duplicate entry violation
        if (mysql_errno(conn) == 1062)
        {
            throw DuplicateDbEntry("Duplicate entry violation: " + std::string(mysql_error(conn)));
        }
        else
        {
            std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
            return false;
        }
    }

    // Get the generated replica_id after the insert
    replica_id = mysql_insert_id(conn); // The replica_id is auto-incremented
    std::cout << "Insert successful. Generated replica_id: " << replica_id << std::endl;
    return true;
}

// Chunk Class Definition
ChunkModel::ChunkModel(int chunk_id, int file_id, int chunk_order, long long chunk_size, const std::string &created_at)
    : chunk_id(chunk_id), file_id(file_id), chunk_order(chunk_order), chunk_size(chunk_size), created_at(created_at) {}
ChunkModel::ChunkModel() {}

ChunkModel ChunkModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{
    return ChunkModel(
        std::stoi(row[0]),  // chunk_id
        std::stoi(row[1]),  // file_id
        std::stoi(row[2]),  // chunk_order
        std::stoll(row[3]), // chunk_size
        std::string(row[4]) // created_at
    );
}
bool ChunkModel::insertRecord(MYSQL *conn)
{
    // Prepare the insert query for the 'chunks' table
    std::string insert_query = "INSERT INTO chunks (file_id, chunk_order, chunk_size) "
                               "VALUES (" +
                               std::to_string(file_id) + ", " +
                               std::to_string(chunk_order) + ", " +
                               std::to_string(chunk_size) + ")";

    // Execute the insert query
    if (mysql_query(conn, insert_query.c_str()))
    {
        // Check for duplicate entry violation
        if (mysql_errno(conn) == 1062)
        {
            throw DuplicateDbEntry("Duplicate entry violation: " + std::string(mysql_error(conn)));
        }
        else
        {
            std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
            return false;
        }
    }

    // Get the generated chunk_id after the insert
    chunk_id = mysql_insert_id(conn); // The chunk_id is auto-incremented
    std::cout << "Insert successful. Generated chunk_id: " << chunk_id << std::endl;
    return true;
}

// Directory Class Definition
// Directory::Directory(int directory_id, int parent_directory_id, const std::string &directory_name,
//                      const std::string &created_at, const std::string &updated_at)
//     : directory_id(directory_id), parent_directory_id(parent_directory_id),
//       directory_name(directory_name), created_at(created_at), updated_at(updated_at) {}

// Directory Directory::mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const
// {
//     return Directory(
//         std::stoi(row[0]),                 // directory_id
//         (row[1] ? std::stoi(row[1]) : -1), // parent_directory_id (NULL is handled by -1)
//         std::string(row[2]),               // directory_name
//         std::string(row[3]),               // created_at
//         std::string(row[4])                // updated_at
//     );
// }

FileModel::FileModel(int file_id, const std::string &file_name, const std::string &file_path,
                     long long file_size, long long num_chunks, int directory_id,
                     const std::string &created_at, const std::string &updated_at)
    : file_id(file_id), file_name(file_name), file_path(file_path),
      file_size(file_size), num_chunks(num_chunks), directory_id(directory_id),
      created_at(created_at), updated_at(updated_at) {}

// mapRowToEntity definition
FileModel FileModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{
    return FileModel(
        std::stoi(row[0]),   // file_id
        std::string(row[1]), // file_name
        std::string(row[2]), // file_path
        std::stoll(row[3]),  // file_size
        std::stoi(row[4]),   // num_chunks
        std::stoi(row[5]),   // directory_id
        std::string(row[6]), // created_at
        std::string(row[7])  // updated_at
    );
}

FileModel::FileModel() {};

bool FileModel::insertRecord(MYSQL *conn)
{
    std::string insert_query = "INSERT INTO files (file_name, file_path, file_size, num_chunks, directory_id) "
                               "VALUES ('" +
                               file_name + "', '" + file_path + "', " +
                               std::to_string(file_size) + ", " +
                               std::to_string(num_chunks) + ", " +
                               std::to_string(directory_id) +
                               ")";

    // Execute the insert query with RETURNING
    if (mysql_query(conn, insert_query.c_str()))
    {
        if (mysql_errno(conn) == 1062)
        {
            throw DuplicateDbEntry("Duplicate entry violation: " + std::string(mysql_error(conn)));
        }
        else
        {
            std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
            return false;
        }
    }

    file_id = mysql_insert_id(conn);
    std::cout << "Insert successful. Generated file_id: " << file_id << std::endl;
    return true;
}

DirectoryModel::DirectoryModel(int directory_id, int parent_directory_id, const std::string &directory_name,
                               const std::string &created_at, const std::string &updated_at)
    : directory_id(directory_id), parent_directory_id(parent_directory_id),
      directory_name(directory_name), created_at(created_at), updated_at(updated_at) {}

// mapRowToEntity definition
DirectoryModel DirectoryModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{
    return DirectoryModel(
        std::stoi(row[0]),                 // directory_id
        (row[1] ? std::stoi(row[1]) : -1), // parent_directory_id (NULL is handled by -1)
        std::string(row[2]),               // directory_name
        std::string(row[3]),               // created_at
        std::string(row[4])                // updated_at
    );
}

NodeModel::NodeModel(int id, const std::string &ip, long totalStorage, long freeSpace, const std::string &status)
    : node_id(id), node_ip(ip), total_storage(totalStorage), free_space(freeSpace), status(status) {}

NodeModel::NodeModel() {};

// mapRowToEntity method definition
NodeModel NodeModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{
    return NodeModel(
        std::stoi(row[0]),   // node_id
        std::string(row[1]), // node_ip
        std::stol(row[2]),   // total_storage
        std::stol(row[3]),   // free_space
        std::string(row[4])  // status
    );
}

ChunkReplicasSlaveModel::ChunkReplicasSlaveModel() {}

ChunkReplicasSlaveModel::ChunkReplicasSlaveModel(size_t id, size_t replica_id, size_t chunk_id,
                                                 size_t chunk_order, size_t chunk_size, size_t file_id,
                                                 const std::string &file_path, const std::string &data_hash,
                                                 const std::string &created_at)
    : id(id), replica_id(replica_id), chunk_id(chunk_id), chunk_order(chunk_order),
      chunk_size(chunk_size), file_id(file_id), file_path(file_path),
      data_hash(data_hash), created_at(created_at) {}

// Map a row from the result set to a ChunkReplicasSlaveModel entity
ChunkReplicasSlaveModel ChunkReplicasSlaveModel::mapRowToEntity(const MYSQL_ROW &row, [[maybe_unused]] MYSQL_FIELD *fields) const
{

    ChunkReplicasSlaveModel chunkReplicasSlaveModel;
    chunkReplicasSlaveModel.id = std::stoi(row[0]);
    chunkReplicasSlaveModel.replica_id = std::stoi(row[1]);
    chunkReplicasSlaveModel.chunk_id = std::stoi(row[2]);
    chunkReplicasSlaveModel.chunk_order = std::stoi(row[3]);
    chunkReplicasSlaveModel.chunk_size = std::stoi(row[4]);
    chunkReplicasSlaveModel.file_id = std::stoi(row[5]);
    chunkReplicasSlaveModel.file_path = row[6];
    chunkReplicasSlaveModel.data_hash = row[7];
    chunkReplicasSlaveModel.created_at = row[8];
    return chunkReplicasSlaveModel;
}

// Insert a new record into the chunk_replicas_slave table
bool ChunkReplicasSlaveModel::insertRecord(MYSQL *conn)
{
    // Prepare the insert query for the 'chunk_replicas_slave' table
    std::string insert_query = "INSERT INTO chunk_replicas_slave (replica_id, chunk_id, chunk_order, chunk_size, file_id, file_path) "
                               "VALUES (" +
                               std::to_string(replica_id) + ", " +
                               std::to_string(chunk_id) + ", " +
                               std::to_string(chunk_order) + ", " +
                               std::to_string(chunk_size) + ", " +
                               std::to_string(file_id) + ", '" +
                               file_path + "'" +
                               ")"; // Use string format for BINARY(16)
                                    //    std::to_string(chunk_size) +

    // Execute the insert query
    if (mysql_query(conn, insert_query.c_str()))
    {
        // Check for duplicate entry violation
        if (mysql_errno(conn) == 1062)
        {
            throw DuplicateDbEntry("Duplicate entry violation: " + std::string(mysql_error(conn)));
        }
        else
        {
            std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
            return false;
        }
    }

    // Get the generated id after the insert (auto-increment)
    id = mysql_insert_id(conn); // The id is auto-incremented
    std::cout << "Insert successful. Generated id: " << id << std::endl;
    return true;
}

bool ChunkReplicasSlaveModel::updateHash(MYSQL *conn)
{
    // Prepare the insert query for the 'chunk_replicas_slave' table
    std::string update_query = "UPDATE chunk_replicas_slave SET data_hash = '" + data_hash + "' " + "WHERE id = " +
                               std::to_string(id);

    // Execute the insert query
    if (mysql_query(conn, update_query.c_str()))
    {
        std::cerr << "Update failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    std::cout << "Updated successful. for id: " << id << std::endl;
    return true;
}