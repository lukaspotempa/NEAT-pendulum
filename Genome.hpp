#pragma once
#include <vector>
#include "Node.hpp"
#include "ConnectionGene.hpp"
#include "Math.hpp"
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <random>
#include <algorithm>

// NodeInnovation struct for tracking node mutations
struct NodeInnovation {
    int nodeId;
    int conn1Innov;
    int conn2Innov;
};

// Forward declarations for innovation tracking functions
int getConnectionInnovation(int nodeIn, int nodeOut);
NodeInnovation getNodeInnovation(int connectionInnov);

class Genome {
public:
    Genome(std::unordered_map<int, Node> nodes, std::vector<ConnectionGene> connections, float fitness = 0.0f)
        : nodes(std::move(nodes)), connections(std::move(connections)), fitness(fitness) {
    }

    Genome copy() const {
        std::unordered_map<int, Node> nodesCopy;
        for (const auto& [id, node] : nodes)
            nodesCopy.emplace(id, node.copy());

        std::vector<ConnectionGene> connsCopy;
        connsCopy.reserve(connections.size());
        for (const auto& conn : connections)
            connsCopy.push_back(conn.copy());

        return Genome(std::move(nodesCopy), std::move(connsCopy), fitness);
    }

    std::vector<float> evaluate(std::vector<float>& inputValues) {
        std::vector<Node*> inputNodes;
        std::vector<Node*> outputNodes;

        for (auto& [id, node] : nodes) {
            if (node.getLayer() == Layer::INPUT)
                inputNodes.push_back(&node);
            else if (node.getLayer() == Layer::OUTPUT)
                outputNodes.push_back(&node);
        }

        if (inputValues.size() != inputNodes.size())
            throw std::invalid_argument("Number of inputs does not match number of input nodes");

        std::unordered_map<int, float> nodeValues;
        std::unordered_map<int, std::vector<ConnectionGene*>> nodeInputs;
        std::unordered_map<Node*, std::vector<Node*>> edges;

        // Initialise edge list for all nodes
        for (auto& [id, node] : nodes)
            edges[&node] = {};

        // Assign input values
        for (int i = 0; i < inputNodes.size(); i++)
            nodeValues[inputNodes[i]->getId()] = inputValues[i];

        // Build graph from enabled connections only
        for (ConnectionGene& conn : connections) {
            if (!conn.isEnabled()) continue;

            Node* inNode = getNode(conn.getInNodeId());
            Node* outNode = getNode(conn.getOutNodeId());

            edges[inNode].push_back(outNode);
            nodeInputs[conn.getOutNodeId()].push_back(&conn);
        }

        std::vector<Node*> sortedNodes = topologicalSort(edges);

        for (Node* node : sortedNodes) {
            if (nodeValues.count(node->getId())) continue; // already assigned (input nodes)

            float total = node->getBias();
            for (ConnectionGene* conn : nodeInputs[node->getId()])
                total += nodeValues[conn->getInNodeId()] * conn->getWeight();

            nodeValues[node->getId()] = static_cast<float>(node->activate(total));
        }

        std::vector<float> outputs;
        outputs.reserve(outputNodes.size());
        for (Node* outNode : outputNodes)
            outputs.push_back(nodeValues.count(outNode->getId()) ? nodeValues[outNode->getId()] : 0.0f);

        return outputs;
    }

    // Check if a path exists from startNodeId to endNodeId (used for cycle detection)
    bool pathExists(int startNodeId, int endNodeId, std::unordered_set<int>* checkedNodes = nullptr) const {
        std::unordered_set<int> localChecked;
        if (!checkedNodes) checkedNodes = &localChecked;

        if (startNodeId == endNodeId)
            return true;

        checkedNodes->insert(startNodeId);

        for (const auto& conn : connections) {
            if (conn.isEnabled() && conn.getInNodeId() == startNodeId) {
                if (checkedNodes->find(conn.getOutNodeId()) == checkedNodes->end()) {
                    if (pathExists(conn.getOutNodeId(), endNodeId, checkedNodes))
                        return true;
                }
            }
        }
        return false;
    }

    void mutateAddConnection() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

        std::vector<Node*> nodeList;
        for (auto& [id, node] : nodes)
            nodeList.push_back(&node);

        constexpr int maxTries = 10;
        for (int attempt = 0; attempt < maxTries; attempt++) {
            std::uniform_int_distribution<int> nodeDist(0, static_cast<int>(nodeList.size()) - 1);
            int idx1 = nodeDist(rng);
            int idx2 = nodeDist(rng);
            if (idx1 == idx2) continue;

            Node* node1 = nodeList[idx1];
            Node* node2 = nodeList[idx2];

            // Ensure proper directionality (input -> hidden -> output)
            if (node1->getLayer() == Layer::OUTPUT ||
                (node1->getLayer() == Layer::HIDDEN && node2->getLayer() == Layer::INPUT)) {
                std::swap(node1, node2);
            }

            // Skip invalid configurations
            if (node1->getId() == node2->getId()) continue;
            if (node1->getLayer() == node2->getLayer()) continue;
            if (node1->getLayer() == Layer::OUTPUT || node2->getLayer() == Layer::INPUT) continue;

            // Check if connection already exists
            bool connExists = false;
            for (const auto& c : connections) {
                if ((c.getInNodeId() == node1->getId() && c.getOutNodeId() == node2->getId()) ||
                    (c.getInNodeId() == node2->getId() && c.getOutNodeId() == node1->getId())) {
                    connExists = true;
                    break;
                }
            }
            if (connExists) continue;

            // Check for cycles: if path exists from node2 to node1, adding node1->node2 creates cycle
            if (pathExists(node2->getId(), node1->getId())) continue;

            int innovNum = getConnectionInnovation(node1->getId(), node2->getId());
            connections.emplace_back(node1->getId(), node2->getId(), weightDist(rng), innovNum, true);
            return;
        }
    }

    void mutateAddNode() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> biasDist(-1.0f, 1.0f);

        // Get enabled connections
        std::vector<ConnectionGene*> enabledConns;
        for (auto& conn : connections) {
            if (conn.isEnabled())
                enabledConns.push_back(&conn);
        }
        if (enabledConns.empty()) return;

        std::uniform_int_distribution<int> connDist(0, static_cast<int>(enabledConns.size()) - 1);
        ConnectionGene* connection = enabledConns[connDist(rng)];
        connection->setEnabled(false);

        NodeInnovation ni = getNodeInnovation(connection->getInnovation());

        // Create new hidden node
        nodes.emplace(ni.nodeId, Node(Layer::HIDDEN, ni.nodeId, relu, biasDist(rng)));

        // Create two new connections: in -> newNode (weight 1.0), newNode -> out (original weight)
        connections.emplace_back(connection->getInNodeId(), ni.nodeId, 1.0f, ni.conn1Innov, true);
        connections.emplace_back(ni.nodeId, connection->getOutNodeId(), connection->getWeight(), ni.conn2Innov, true);
    }

    void mutateWeights(float rate = 0.8f, float power = 0.5f) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);
        std::normal_distribution<float> gauss(0.0f, power);

        for (auto& conn : connections) {
            if (prob(rng) < rate) {
                if (prob(rng) < 0.1f) {
                    conn.setWeight(weightDist(rng));
                }
                else {
                    float newWeight = conn.getWeight() + gauss(rng);
                    newWeight = std::clamp(newWeight, -5.0f, 5.0f);
                    conn.setWeight(newWeight);
                }
            }
        }
    }

    void mutateBias(float rate = 0.7f, float power = 0.5f) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> biasDist(-1.0f, 1.0f);
        std::normal_distribution<float> gauss(0.0f, power);

        for (auto& [id, node] : nodes) {
            if (node.getLayer() != Layer::INPUT && prob(rng) < rate) {
                if (prob(rng) < 0.1f) {
                    node.setBias(biasDist(rng));
                }
                else {
                    float newBias = node.getBias() + gauss(rng);
                    newBias = std::clamp(newBias, -5.0f, 5.0f);
                    node.setBias(newBias);
                }
            }
        }
    }

    void mutate(float connMutationRate = 0.05f, float nodeMutationRate = 0.03f,
                float weightMutationRate = 0.8f, float biasMutationRate = 0.7f) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);

        mutateWeights(weightMutationRate);
        mutateBias(biasMutationRate);

        if (prob(rng) < connMutationRate)
            mutateAddConnection();

        if (prob(rng) < nodeMutationRate)
            mutateAddNode();
    }

    Node* getNode(int nodeId) { return &nodes.at(nodeId); }
    const Node* getNode(int nodeId) const { return &nodes.at(nodeId); }
    bool hasNode(int nodeId) const { return nodes.count(nodeId) > 0; }
    std::unordered_map<int, Node>& getNodes() { return nodes; }
    const std::unordered_map<int, Node>& getNodes() const { return nodes; }
    std::vector<ConnectionGene>& getConnections() { return connections; }
    const std::vector<ConnectionGene>& getConnections() const { return connections; }
    float getFitness() const { return fitness; }
    void setFitness(float val) { fitness = val; }

private:
    std::unordered_map<int, Node> nodes;
    std::vector<ConnectionGene> connections;
    float fitness = 0.0f;
};