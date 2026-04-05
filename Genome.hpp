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
#include <fstream>
#include <sstream>

struct NodeInnovation {
    int nodeId;
    int conn1Innov;
    int conn2Innov;
};

// Forward declarations for innovation tracking functions
int getConnectionInnovation(int nodeIn, int nodeOut);
NodeInnovation getNodeInnovation(int connectionInnov);

struct CompiledNetwork {
    struct CompiledNode {
        int id;
        float bias;
        std::function<double(double)> activation;
        std::vector<std::pair<int, float>> inputs; // (source node id, weight)
    };
    
    std::vector<int> inputNodeIds;
    std::vector<int> outputNodeIds;
    std::vector<CompiledNode> evaluationOrder;
    std::vector<float> nodeValues;             // Reusable buffer
    std::unordered_map<int, size_t> nodeIdToIndex;
    bool valid = false;
    
    void clear() {
        inputNodeIds.clear();
        outputNodeIds.clear();
        evaluationOrder.clear();
        nodeValues.clear();
        nodeIdToIndex.clear();
        valid = false;
    }
};

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

        if (!m_compiled.valid) {
            compileNetwork();
        }

        if (m_compiled.valid) {
            return evaluateCompiled(inputValues);
        }
        
        return evaluateSlow(inputValues);
    }
    
    void invalidateCompiled() {
        m_compiled.valid = false;
    }
    
private:
    mutable CompiledNetwork m_compiled;
    
    void compileNetwork() const {
        m_compiled.clear();
        
        for (auto& [id, node] : nodes) {
            if (node.getLayer() == Layer::INPUT)
                m_compiled.inputNodeIds.push_back(id);
            else if (node.getLayer() == Layer::OUTPUT)
                m_compiled.outputNodeIds.push_back(id);
        }
        
        // Build node index map
        size_t idx = 0;
        for (const auto& [id, node] : nodes) {
            m_compiled.nodeIdToIndex[id] = idx++;
        }
        m_compiled.nodeValues.resize(nodes.size(), 0.0f);
        
        // Build edge map for topological sort
        std::unordered_map<Node*, std::vector<Node*>> edges;
        std::unordered_map<int, std::vector<std::pair<int, float>>> nodeInputConns;
        
        for (auto& [id, node] : nodes)
            edges[const_cast<Node*>(&node)] = {};
        
        for (const ConnectionGene& conn : connections) {
            if (!conn.isEnabled()) continue;
            if (!hasNode(conn.getInNodeId()) || !hasNode(conn.getOutNodeId())) continue;
            
            Node* inNode = const_cast<Node*>(getNode(conn.getInNodeId()));
            Node* outNode = const_cast<Node*>(getNode(conn.getOutNodeId()));
            
            edges[inNode].push_back(outNode);
            nodeInputConns[conn.getOutNodeId()].emplace_back(conn.getInNodeId(), conn.getWeight());
        }
        
        std::vector<Node*> sortedNodes = ::topologicalSort(edges);
        
        // Build evaluation order
        for (Node* node : sortedNodes) {
            if (node->getLayer() == Layer::INPUT) continue;
            
            CompiledNetwork::CompiledNode cn;
            cn.id = node->getId();
            cn.bias = node->getBias();
            cn.activation = node->getActivationFn();
            cn.inputs = nodeInputConns[node->getId()];
            m_compiled.evaluationOrder.push_back(std::move(cn));
        }
        
        m_compiled.valid = true;
    }
    
    std::vector<float> evaluateCompiled(std::vector<float>& inputValues) const {
        if (inputValues.size() != m_compiled.inputNodeIds.size()) {
            throw std::invalid_argument("Input size mismatch");
        }
        
        std::fill(m_compiled.nodeValues.begin(), m_compiled.nodeValues.end(), 0.0f);
        
        // Set input values
        for (size_t i = 0; i < m_compiled.inputNodeIds.size(); ++i) {
            int nodeId = m_compiled.inputNodeIds[i];
            m_compiled.nodeValues[m_compiled.nodeIdToIndex.at(nodeId)] = inputValues[i];
        }
        
        // Evaluate
        for (const auto& cn : m_compiled.evaluationOrder) {
            float sum = cn.bias;
            for (const auto& [srcId, weight] : cn.inputs) {
                sum += m_compiled.nodeValues[m_compiled.nodeIdToIndex.at(srcId)] * weight;
            }
            m_compiled.nodeValues[m_compiled.nodeIdToIndex.at(cn.id)] = static_cast<float>(cn.activation(sum));
        }
        
        // Collect outputs
        std::vector<float> outputs;
        outputs.reserve(m_compiled.outputNodeIds.size());
        for (int outId : m_compiled.outputNodeIds) {
            outputs.push_back(m_compiled.nodeValues[m_compiled.nodeIdToIndex.at(outId)]);
        }
        
        return outputs;
    }
    
    std::vector<float> evaluateSlow(std::vector<float>& inputValues) {
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
        for (size_t i = 0; i < inputNodes.size(); i++)
            nodeValues[inputNodes[i]->getId()] = inputValues[i];

        // Build graph from enabled connections only
        for (ConnectionGene& conn : connections) {
            if (!conn.isEnabled()) continue;

            if (!hasNode(conn.getInNodeId()) || !hasNode(conn.getOutNodeId())) {
                continue;  // Skip invalid connections
            }

            Node* inNode = getNode(conn.getInNodeId());
            Node* outNode = getNode(conn.getOutNodeId());

            edges[inNode].push_back(outNode);
            nodeInputs[conn.getOutNodeId()].push_back(&conn);
        }

        std::vector<Node*> sortedNodes = topologicalSort(edges);

        for (Node* node : sortedNodes) {
            if (nodeValues.count(node->getId())) continue; // already assigned (input nodes)

            float total = node->getBias();
            for (ConnectionGene* conn : nodeInputs[node->getId()]) {
                // Verify input node value exists
                if (nodeValues.count(conn->getInNodeId())) {
                    total += nodeValues[conn->getInNodeId()] * conn->getWeight();
                }
            }

            nodeValues[node->getId()] = static_cast<float>(node->activate(total));
        }

        std::vector<float> outputs;
        outputs.reserve(outputNodes.size());
        for (Node* outNode : outputNodes)
            outputs.push_back(nodeValues.count(outNode->getId()) ? nodeValues[outNode->getId()] : 0.0f);

        return outputs;
    }

public:
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
        auto& rng = getGlobalRng();
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

            // Ensure proper order (input -> hidden -> output)
            if (node1->getLayer() == Layer::OUTPUT ||
                (node1->getLayer() == Layer::HIDDEN && node2->getLayer() == Layer::INPUT)) {
                std::swap(node1, node2);
            }

            // Skip invalid configurations
            if (node1->getId() == node2->getId()) continue;
            if (node1->getLayer() == node2->getLayer()) continue;
            if (node1->getLayer() == Layer::OUTPUT || node2->getLayer() == Layer::INPUT) continue;

            bool connExists = false;
            for (const auto& c : connections) {
                if ((c.getInNodeId() == node1->getId() && c.getOutNodeId() == node2->getId()) ||
                    (c.getInNodeId() == node2->getId() && c.getOutNodeId() == node1->getId())) {
                    connExists = true;
                    break;
                }
            }
            if (connExists) continue;

            if (pathExists(node2->getId(), node1->getId())) continue;

            int innovNum = getConnectionInnovation(node1->getId(), node2->getId());
            connections.emplace_back(node1->getId(), node2->getId(), weightDist(rng), innovNum, true);
            return;
        }
    }

    void mutateAddNode() {
        auto& rng = getGlobalRng();
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

        nodes.emplace(ni.nodeId, Node(Layer::HIDDEN, ni.nodeId, relu, biasDist(rng), ActivationType::RELU));

        // Create two new connections: in -> newNode (weight 1.0), newNode -> out (original weight)
        connections.emplace_back(connection->getInNodeId(), ni.nodeId, 1.0f, ni.conn1Innov, true);
        connections.emplace_back(ni.nodeId, connection->getOutNodeId(), connection->getWeight(), ni.conn2Innov, true);
    }

    void mutateWeights(float power = 0.3f) {
        if (connections.empty()) return;
        
        auto& rng = getGlobalRng();
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);
        
        std::uniform_int_distribution<size_t> connDist(0, connections.size() - 1);
        auto& conn = connections[connDist(rng)];
        
        if (prob(rng) < 0.2f) {
            conn.setWeight(weightDist(rng));
        }
        else {
            if (prob(rng) < 0.75f) {
                float delta = 0.01f * weightDist(rng);
                float newWeight = conn.getWeight() + delta;
                newWeight = std::clamp(newWeight, -5.0f, 5.0f);
                conn.setWeight(newWeight);
            } else {
                float delta = power * weightDist(rng);
                float newWeight = conn.getWeight() + delta;
                newWeight = std::clamp(newWeight, -5.0f, 5.0f);
                conn.setWeight(newWeight);
            }
        }
    }

    void mutateBias(float power = 0.3f) {
        if (nodes.empty()) return;
        
        auto& rng = getGlobalRng();
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> biasDist(-1.0f, 1.0f);
        
        // Pick random node to mutate
        std::vector<int> nodeIds;
        for (const auto& [id, node] : nodes) {
            if (node.getLayer() != Layer::INPUT) {
                nodeIds.push_back(id);
            }
        }
        if (nodeIds.empty()) return;
        
        std::uniform_int_distribution<size_t> nodeDist(0, nodeIds.size() - 1);
        int nodeId = nodeIds[nodeDist(rng)];
        auto& node = nodes.at(nodeId);
        
        if (prob(rng) < 0.2f) {
            // 20% chance for completely new bias
            node.setBias(biasDist(rng));
        }
        else {
            if (prob(rng) < 0.75f) {
                float delta = 0.01f * biasDist(rng);
                float newBias = node.getBias() + delta;
                newBias = std::clamp(newBias, -5.0f, 5.0f);
                node.setBias(newBias);
            } else {
                float delta = power * biasDist(rng);
                float newBias = node.getBias() + delta;
                newBias = std::clamp(newBias, -5.0f, 5.0f);
                node.setBias(newBias);
            }
        }
    }

    void mutate(float connMutationRate = 0.05f, float nodeMutationRate = 0.03f) {
        auto& rng = getGlobalRng();
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);

        mutateWeights();
        mutateBias();

        if (prob(rng) < connMutationRate)
            mutateAddConnection();

        if (prob(rng) < nodeMutationRate)
            mutateAddNode();
        
        cleanupInvalidConnections();

        invalidateCompiled();
    }
    

    void cleanupInvalidConnections() {
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [this](const ConnectionGene& conn) {
                    return !hasNode(conn.getInNodeId()) || !hasNode(conn.getOutNodeId());
                }),
            connections.end()
        );
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
    

    static std::mt19937& getGlobalRng() {
        thread_local std::mt19937 rng(std::random_device{}());
        return rng;
    }

    
    std::string toJson() const {
        std::ostringstream ss;
        ss << "{\"fitness\":" << fitness << ",\"nodes\":[";
        
        bool first = true;
        for (const auto& [id, node] : nodes) {
            if (!first) ss << ",";
            ss << node.toJson();
            first = false;
        }
        
        ss << "],\"connections\":[";
        first = true;
        for (const auto& conn : connections) {
            if (!first) ss << ",";
            ss << conn.toJson();
            first = false;
        }
        ss << "]}";
        return ss.str();
    }
    
    static Genome fromJson(const std::string& json) {
        std::unordered_map<int, Node> parsedNodes;
        std::vector<ConnectionGene> parsedConns;
        float parsedFitness = 0.0f;
        
        // Parse fitness
        size_t pos = json.find("\"fitness\":");
        if (pos != std::string::npos) {
            parsedFitness = std::stof(json.substr(pos + 10));
        }
        
        // Parse nodes array
        size_t nodesStart = json.find("\"nodes\":[");
        size_t nodesEnd = json.find("],\"connections\"");
        if (nodesStart != std::string::npos && nodesEnd != std::string::npos) {
            std::string nodesStr = json.substr(nodesStart + 9, nodesEnd - nodesStart - 9);
            parseJsonArray(nodesStr, [&parsedNodes](const std::string& item) {
                Node n = Node::fromJson(item);
                parsedNodes.emplace(n.getId(), std::move(n));
            });
        }
        
        // Parse connections array
        size_t connsStart = json.find("\"connections\":[");
        size_t connsEnd = json.rfind("]}");
        if (connsStart != std::string::npos && connsEnd != std::string::npos) {
            std::string connsStr = json.substr(connsStart + 15, connsEnd - connsStart - 15);
            parseJsonArray(connsStr, [&parsedConns](const std::string& item) {
                parsedConns.push_back(ConnectionGene::fromJson(item));
            });
        }
        
        return Genome(std::move(parsedNodes), std::move(parsedConns), parsedFitness);
    }
    
    bool saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        file << toJson();
        return true;
    }
    
    static Genome loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return fromJson(buffer.str());
    }

private:
    std::unordered_map<int, Node> nodes;
    std::vector<ConnectionGene> connections;
    float fitness = 0.0f;
    
    static void parseJsonArray(const std::string& arrayStr, std::function<void(const std::string&)> callback) {
        int braceCount = 0;
        size_t itemStart = 0;
        
        for (size_t i = 0; i < arrayStr.size(); ++i) {
            char c = arrayStr[i];
            if (c == '{') {
                if (braceCount == 0) itemStart = i;
                braceCount++;
            } else if (c == '}') {
                braceCount--;
                if (braceCount == 0) {
                    callback(arrayStr.substr(itemStart, i - itemStart + 1));
                }
            }
        }
    }
};