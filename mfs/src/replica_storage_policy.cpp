#include "replica_storage_policy.h"
#include <algorithm>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include "config.h"
#include "json.hpp"
using json = nlohmann::json;

static std::string generateQuery(long long chunk_size, int k);

std::vector<NodeModel> RandomizedRoundRobinPolicy::selectReplicaNodes(MySQLDatabase &db, size_t numberOfReplicas, size_t chunk_size)
{
    std::string query = generateQuery(chunk_size, config.getInt("dfs.mins-to-inactive"));
    NodeModel templateNode(0, "", 0, 0, "");

    std::vector<NodeModel> nodes = db.executeTemplateQuery(query, templateNode);

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::random_shuffle(nodes.begin(), nodes.end());

    std::vector<NodeModel> selectedNodes;
    for (size_t i = 0; i < numberOfReplicas && i < nodes.size(); ++i)
    {
        selectedNodes.push_back(nodes[i]);
    }
    return selectedNodes;
}

std::vector<NodeModel> LeastLoadedNodePolicy::selectReplicaNodes(MySQLDatabase &db, size_t numberOfReplicas, size_t chunk_size)
{
    std::string query = generateQuery(chunk_size, config.getInt("dfs.mins-to-inactive"));
    NodeModel templateNode(0, "", 0, 0, "");
    std::vector<NodeModel> nodes = db.executeTemplateQuery(query, templateNode);

    std::sort(nodes.begin(), nodes.end(), [](const NodeModel &a, const NodeModel &b)
              { return a.free_space > b.free_space; });

    std::vector<NodeModel> selectedNodes;
    for (size_t i = 0; i < numberOfReplicas && i < nodes.size(); ++i)
    {
        selectedNodes.push_back(nodes[i]);
    }
    return selectedNodes;
}

// ReplicaPolicyManager constructor that initializes the selected policy
ReplicaPolicyManager::ReplicaPolicyManager()
{
    std::string policyConfig = config.getString("dfs.replica-storage-policy");

    if (policyConfig == "randomized_round_robin")
    {
        policy = std::unique_ptr<RandomizedRoundRobinPolicy>(new RandomizedRoundRobinPolicy());
    }
    else if (policyConfig == "least_loaded")
    {
        policy = std::unique_ptr<LeastLoadedNodePolicy>(new LeastLoadedNodePolicy());
    }
    else
    {
        throw std::invalid_argument("Unknown replication policy: " + policyConfig);
    }
}

// Select replica nodes for multiple chunks, each with a replication factor
std::vector<std::vector<NodeModel>> ReplicaPolicyManager::selectReplicaNodesForChunks(MySQLDatabase &db, size_t chunkCount, size_t replicationFactor, size_t chunk_size)
{
    std::vector<std::vector<NodeModel>> result;
    for (size_t i = 0; i < chunkCount; ++i)
    {
        result.push_back(policy->selectReplicaNodes(db, replicationFactor, chunk_size));
    }
    return result;
}

static std::string generateQuery(long long chunk_size, int k)
{
    std::time_t currentTime = std::time(nullptr);

    std::time_t thresholdTime = currentTime - (k * 60);

    char thresholdTimeStr[20];
    std::strftime(thresholdTimeStr, sizeof(thresholdTimeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&thresholdTime));

    std::string query = "SELECT * FROM nodes WHERE status = 'ACTIVE' AND free_space >= " +
                        std::to_string(chunk_size) +
                        " AND last_heartbeat >= '" + thresholdTimeStr + "';";

    return query;
}

json ReplicaPolicyManager::updateDbAndGenerateResponceJson(MySQLDatabase &db, std::vector<std::vector<NodeModel>> nodes, FileModel &newFile)
{
    json body_json;
    body_json["chunk-size"] = config.getLongLong("dfs.chunk-size");
    body_json["file-id"] = newFile.file_id;
    json chunks_json = json::array();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        ChunkModel newChunk;
        newChunk.file_id = newFile.file_id;
        newChunk.chunk_order = i + 1;
        if (i == nodes.size() - 1)
        {
            newChunk.chunk_size = newFile.file_size - (i * config.getLongLong("dfs.chunk-size"));
        }
        else
        {
            newChunk.chunk_size = config.getLongLong("dfs.chunk-size");
        }
        newChunk.insertRecord(db.getConnection());
        json chunk_json;
        chunk_json["chunk-order"] = newChunk.chunk_order;
        chunk_json["chunk-size"] = newChunk.chunk_size;
        json replicas_json = json::array();
        for (size_t j = 0; j < nodes[i].size(); ++j)
        {
            ChunkReplicaModel newChunkReplica;
            newChunkReplica.node_id = nodes[i][j].node_id;
            newChunkReplica.chunk_id = newChunk.chunk_id;
            newChunkReplica.insertRecord(db.getConnection());
            json replica_json;
            replica_json["replica-id"] = newChunkReplica.replica_id;
            replica_json["chunk-id"] = newChunkReplica.chunk_id;
            replica_json["node-id"] = newChunkReplica.node_id;
            replica_json["node-ip"] = nodes[i][j].node_ip;

            replicas_json.push_back(replica_json);
            // std::cout << "ip: " << nodes[i][j].node_ip << " " << std::endl;
        }
        chunk_json["replicas"] = replicas_json;
        chunks_json.push_back(chunk_json);
        std::cout << std::endl;
    }
    body_json["chunks"] = chunks_json;
    return body_json;
}

json ReplicaPolicyManager::getReplicaInfo(MySQLDatabase &db, FileModel &newFile)
{
    ChunkModel chunksTemplate;
    std::string fileQuery = "select * from chunks where file_id=" + std::to_string(newFile.file_id);
    std::string replicasQueryTemplate = "select * from chunk_replicas where chunk_id=";
    std::string nodeQueryTemplate = "select * from nodes where node_id=";
    std::string replicasQuery, nodeQuery;

    std::vector<ChunkModel> fileChunks = db.executeTemplateQuery(fileQuery, chunksTemplate);

    json body_json;
    body_json["chunk-size"] = config.getLongLong("dfs.chunk-size");
    body_json["file-size"] = newFile.file_size;
    body_json["file-num-chunks"] = newFile.num_chunks;
    json chunks_json = json::array();
    for (size_t i = 0; i < fileChunks.size(); ++i)
    {
        ChunkModel newChunk = fileChunks[i];
        ChunkReplicaModel chunkReplicaTemplate;
        replicasQuery = replicasQueryTemplate + std::to_string(newChunk.chunk_id);
        std::vector<ChunkReplicaModel> fileReplicas = db.executeTemplateQuery(replicasQuery, chunkReplicaTemplate);
        json chunk_json;
        chunk_json["chunk-order"] = newChunk.chunk_order;
        chunk_json["chunk-size"] = newChunk.chunk_size;
        json replicas_json = json::array();
        for (size_t j = 0; j < fileReplicas.size(); ++j)
        {
            ChunkReplicaModel newChunkReplica = fileReplicas[j];
            json replica_json;
            replica_json["replica-id"] = newChunkReplica.replica_id;
            replica_json["chunk-id"] = newChunkReplica.chunk_id;
            replica_json["node-id"] = newChunkReplica.node_id;
            NodeModel templateNodeModel;
            nodeQuery = nodeQueryTemplate + std::to_string(newChunkReplica.node_id);
            NodeModel nodeModel = db.executeTemplateQuery(nodeQuery, templateNodeModel).front();
            replica_json["node-ip"] = nodeModel.node_ip;
            replicas_json.push_back(replica_json);
        }
        chunk_json["replicas"] = replicas_json;
        chunks_json.push_back(chunk_json);
        std::cout << std::endl;
    }
    body_json["chunks"] = chunks_json;
    return body_json;
}
