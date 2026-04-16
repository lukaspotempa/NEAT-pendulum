#pragma once

#include "Genome.hpp"
#include <vector>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <functional>
#include <string>

// Stores a snapshot of a single generation (genome)
struct GenerationSnapshot {
    int generationNumber = 0;
    float bestFitness = 0.0f;
    float avgFitness = 0.0f;
    
    struct AgentRecord {
        int originalIndex;
        float fitness;
        
        bool operator<(const AgentRecord& other) const {
            return fitness > other.fitness; 
        }
    };
    std::vector<AgentRecord> agentRecords;
    
    std::vector<Genome> genomes;
    
    void sortAgentsByFitness() {
        std::sort(agentRecords.begin(), agentRecords.end());
    }
    
    int getBestAgentIndex() const {
        if (agentRecords.empty()) return 0;
        return agentRecords[0].originalIndex;
    }
    
    float getAgentFitness(int originalIndex) const {
        for (const auto& record : agentRecords) {
            if (record.originalIndex == originalIndex) {
                return record.fitness;
            }
        }
        return 0.0f;
    }
    
    std::string toJson() const {
        std::ostringstream ss;
        ss << "{\"gen\":" << generationNumber
           << ",\"best\":" << bestFitness
           << ",\"avg\":" << avgFitness
           << ",\"records\":[";
        
        bool first = true;
        for (const auto& r : agentRecords) {
            if (!first) ss << ",";
            ss << "{\"i\":" << r.originalIndex << ",\"f\":" << r.fitness << "}";
            first = false;
        }
        
        ss << "],\"genomes\":[";
        first = true;
        for (const auto& g : genomes) {
            if (!first) ss << ",";
            ss << g.toJson();
            first = false;
        }
        ss << "]}";
        return ss.str();
    }
    
    static GenerationSnapshot fromJson(const std::string& json) {
        GenerationSnapshot snapshot;
        
        size_t genPos = json.find("\"gen\":");
        if (genPos != std::string::npos) snapshot.generationNumber = std::stoi(json.substr(genPos + 6));
        
        size_t bestPos = json.find("\"best\":");
        if (bestPos != std::string::npos) snapshot.bestFitness = std::stof(json.substr(bestPos + 7));
        
        size_t avgPos = json.find("\"avg\":");
        if (avgPos != std::string::npos) snapshot.avgFitness = std::stof(json.substr(avgPos + 6));
        
        size_t recordsStart = json.find("\"records\":[");
        size_t recordsEnd = json.find("],\"genomes\"");
        if (recordsStart != std::string::npos && recordsEnd != std::string::npos) {
            std::string recordsStr = json.substr(recordsStart + 11, recordsEnd - (recordsStart + 11));
            parseJsonArray(recordsStr, [&snapshot](const std::string& item) {
                AgentRecord r;
                size_t iPos = item.find("\"i\":");
                if (iPos != std::string::npos) r.originalIndex = std::stoi(item.substr(iPos + 4));
                size_t fPos = item.find("\"f\":");
                if (fPos != std::string::npos) r.fitness = std::stof(item.substr(fPos + 4));
                snapshot.agentRecords.push_back(r);
            });
        }
        
        size_t genomesStart = json.find("\"genomes\":[");
        size_t genomesEnd = json.rfind("]}");
        if (genomesStart != std::string::npos && genomesEnd != std::string::npos) {
            std::string genomesStr = json.substr(genomesStart + 11, genomesEnd - (genomesStart + 11));
            parseJsonArray(genomesStr, [&snapshot](const std::string& item) {
                snapshot.genomes.push_back(Genome::fromJson(item));
            });
        }
        
        return snapshot;
    }

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

class TrainingHistory {
public:
    static constexpr size_t MAX_HISTORY_SIZE = 100; // Keep last 100 generations
    
    TrainingHistory() = default;
    
    void addGeneration(int genNumber, 
                       const std::vector<float>& fitnesses,
                       const std::vector<Genome>& genomes) {
        GenerationSnapshot snapshot;
        snapshot.generationNumber = genNumber;
        
        // Calculate fitness stats
        float totalFitness = 0.0f;
        float maxFitness = 0.0f;
        
        for (size_t i = 0; i < fitnesses.size(); ++i) {
            float fitness = fitnesses[i];
            snapshot.agentRecords.push_back({static_cast<int>(i), fitness});
            totalFitness += fitness;
            maxFitness = std::max(maxFitness, fitness);
        }
        
        snapshot.bestFitness = maxFitness;
        snapshot.avgFitness = fitnesses.empty() ? 0.0f : totalFitness / fitnesses.size();
        snapshot.sortAgentsByFitness();
        
        snapshot.genomes = genomes;
        
        m_generations.push_back(std::move(snapshot));
        
        // Track best fitness ever
        if (maxFitness > m_bestFitnessEver) {
            m_bestFitnessEver = maxFitness;
            m_bestGenerationEver = genNumber;
        }
        
        if (m_generations.size() > MAX_HISTORY_SIZE) {
            m_generations.erase(m_generations.begin());
        }
    }
    
    const GenerationSnapshot* getGeneration(int genNumber) const {
        for (const auto& gen : m_generations) {
            if (gen.generationNumber == genNumber) {
                return &gen;
            }
        }
        return nullptr;
    }
    
    const GenerationSnapshot* getLatestGeneration() const {
        if (m_generations.empty()) return nullptr;
        return &m_generations.back();
    }
    
    // Get generation at index
    const GenerationSnapshot* getGenerationByIndex(size_t index) const {
        if (index >= m_generations.size()) return nullptr;
        return &m_generations[index];
    }
    

    std::vector<std::pair<int, float>> getBestFitnessHistory() const {
        std::vector<std::pair<int, float>> history;
        history.reserve(m_generations.size());
        for (const auto& gen : m_generations) {
            history.emplace_back(gen.generationNumber, gen.bestFitness);
        }
        return history;
    }
    
    // Get all average fitness values for graphing
    std::vector<std::pair<int, float>> getAvgFitnessHistory() const {
        std::vector<std::pair<int, float>> history;
        history.reserve(m_generations.size());
        for (const auto& gen : m_generations) {
            history.emplace_back(gen.generationNumber, gen.avgFitness);
        }
        return history;
    }
    
    size_t getGenerationCount() const { return m_generations.size(); }
    bool isEmpty() const { return m_generations.empty(); }
    float getBestFitnessEver() const { return m_bestFitnessEver; }
    int getBestGenerationEver() const { return m_bestGenerationEver; }
    
    int getFirstGenerationNumber() const {
        return m_generations.empty() ? 0 : m_generations.front().generationNumber;
    }
    
    int getLastGenerationNumber() const {
        return m_generations.empty() ? 0 : m_generations.back().generationNumber;
    }
    
    // Check if a generation exists
    bool hasGeneration(int genNumber) const {
        return getGeneration(genNumber) != nullptr;
    }
    
    // Clear all history
    void clear() {
        m_generations.clear();
        m_bestFitnessEver = 0.0f;
        m_bestGenerationEver = 0;
    }
    
    // Save history to file
    bool saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        
        file << toJson();
        return true;
    }

    std::string toJson() const {
        std::ostringstream ss;
        ss << "{\"bestEver\":" << m_bestFitnessEver
           << ",\"bestGen\":" << m_bestGenerationEver
           << ",\"generations\":[";
        
        bool first = true;
        for (const auto& gen : m_generations) {
            if (!first) ss << ",";
            ss << gen.toJson();
            first = false;
        }
        ss << "]}";
        return ss.str();
    }
    
    static TrainingHistory fromJson(const std::string& json) {
        TrainingHistory history;
        
        size_t bestEverPos = json.find("\"bestEver\":");
        if (bestEverPos != std::string::npos) history.m_bestFitnessEver = std::stof(json.substr(bestEverPos + 11));
        
        size_t bestGenPos = json.find("\"bestGen\":");
        if (bestGenPos != std::string::npos) history.m_bestGenerationEver = std::stoi(json.substr(bestGenPos + 10));
        
        size_t genStart = json.find("\"generations\":[");
        size_t genEnd = json.rfind("]}");
        if (genStart != std::string::npos && genEnd != std::string::npos) {
            std::string genStr = json.substr(genStart + 15, genEnd - (genStart + 15));
            GenerationSnapshot::parseJsonArray(genStr, [&history](const std::string& item) {
                history.m_generations.push_back(GenerationSnapshot::fromJson(item));
            });
        }
        
        return history;
    }

private:
    std::vector<GenerationSnapshot> m_generations;
    float m_bestFitnessEver = 0.0f;
    int m_bestGenerationEver = 0;
};
