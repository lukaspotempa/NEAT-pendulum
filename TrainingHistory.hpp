#pragma once

#include "Genome.hpp"
#include <vector>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>

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
        
        file << "{\"bestEver\":" << m_bestFitnessEver
             << ",\"bestGen\":" << m_bestGenerationEver
             << ",\"generations\":[";
        
        bool first = true;
        for (const auto& gen : m_generations) {
            if (!first) file << ",";
            file << gen.toJson();
            first = false;
        }
        file << "]}";
        return true;
    }

private:
    std::vector<GenerationSnapshot> m_generations;
    float m_bestFitnessEver = 0.0f;
    int m_bestGenerationEver = 0;
};
