#ifndef REPLICA_POLICY_H
#define REPLICA_POLICY_H

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include "dbhelper.h"
#include "models.h"
#include "json.hpp"
using json = nlohmann::json;



// Base class for replica storage policy
class ReplicaStoragePolicy
{
public:
    virtual std::vector<NodeModel> selectReplicaNodes(MySQLDatabase &db, size_t numberOfReplicas, size_t chunk_size) = 0;
    virtual ~ReplicaStoragePolicy() = default;
};

// Randomized Round-Robin Policy
class RandomizedRoundRobinPolicy : public ReplicaStoragePolicy
{
public:
    std::vector<NodeModel> selectReplicaNodes(MySQLDatabase &db, size_t numberOfReplicas, size_t chunk_size) override;
};

// Least-Loaded Node Policy
class LeastLoadedNodePolicy : public ReplicaStoragePolicy
{
public:
    std::vector<NodeModel> selectReplicaNodes(MySQLDatabase &db, size_t numberOfReplicas, size_t chunk_size) override;
};

// ReplicaPolicyManager to select the policy based on configuration
class ReplicaPolicyManager
{
public:
    ReplicaPolicyManager();

    std::vector<std::vector<NodeModel>> selectReplicaNodesForChunks(MySQLDatabase &db, size_t chunkCount, size_t replicationFactor, size_t chunk_size);
    json updateDbAndGenerateResponceJson(MySQLDatabase& db, std::vector<std::vector<NodeModel>> nodes,FileModel &newFile);
    json getReplicaInfo(MySQLDatabase& db,FileModel &newFile);
private:
    std::unique_ptr<ReplicaStoragePolicy> policy;
};

#endif // REPLICA_POLICY_H
