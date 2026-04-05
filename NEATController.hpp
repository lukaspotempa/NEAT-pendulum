#pragma once

#include "Genome.hpp"
#include "Speciator.hpp"
#include "NEAT.hpp"
#include "SimulationManager.hpp"
#include "TrainingHistory.hpp"
#include "AgentSimulation.hpp"
#include <vector>
#include <cmath>
#include <memory>
#include <functional>

enum class NEATState {
    Idle,           // Not running
    Simulating,     // Running fast parallel simulation
    Replaying,      // Running genome in real time (live replay of genome performing task)
    WaitingForNext, // Waiting for user to advance 
    BatchTraining   // Running multiple generations
};

class NEATController {
public:
    NEATController(int populationSize = DEFAULT_POPULATION_SIZE, float compatibilityThreshold = 3.0f);

    void runGeneration();
    
    bool updateSimulation(float dt);

    void evolve();

    void startReplay(int agentIndex);
    bool updateReplay(float dt);
    
    float getReplayCartX() const;
    float getReplayTheta() const;
    bool isReplaying() const { return m_state == NEATState::Replaying; }
    
    void selectAgent(int index);
    int getSelectedAgentIndex() const { return m_selectedAgentIndex; }
    void selectNextAgent();
    void selectPrevAgent();
    void selectBestAgent();
    

    int getPopulationSize() const { return static_cast<int>(population.size()); }
    int getGeneration() const { return generation; }
    float getBestFitness() const { return bestFitness; }
    float getAgentFitness(int index) const;
    NEATState getState() const { return m_state; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool val);
    
    // Get agent simulation data
    const AgentSimulation& getAgent(int index) const {
        return m_simManager->getAgent(index);
    }
    
    // Get replay simulation for rreplay
    const AgentSimulation& getReplayAgent() const { return m_replaySimulation; }
    
    std::vector<int> getSortedAgentIndices() const { return m_simManager->getSortedAgentIndices(); }
    
    // Get current replay time
    float getReplayTime() const { return m_replayTime; }
    
    // Manual controls
    void skipToNextGeneration();
    
    // Batch training
    void startBatchTraining(int numGenerations);
    void stopBatchTraining();
    void advanceBatchTraining(); 
    void completeBatchTraining(); 
    bool isBatchTraining() const { return m_state == NEATState::BatchTraining || 
                                          (m_state == NEATState::Simulating && m_batchTotalGens > 0); }
    int getBatchProgress() const { return m_batchCurrentGen; }
    int getBatchTotal() const { return m_batchTotalGens; }
    bool shouldContinueBatchTraining() const { 
        return (m_state == NEATState::BatchTraining || 
                (m_state == NEATState::Simulating && m_batchTotalGens > 0)) && 
               m_batchCurrentGen < m_batchTotalGens && 
               !m_batchStopRequested; 
    }
    
    // Training history
    TrainingHistory& getHistory() { return m_history; }
    const TrainingHistory& getHistory() const { return m_history; }
    
    // Load generation for replay
    bool loadGenerationForReplay(int genNumber);
    
    bool isViewingHistory() const { return m_viewingHistory; }
    int getViewingGeneration() const { return m_viewingHistory ? m_viewingGeneration : generation; }
    const GenerationSnapshot* getCurrentSnapshot() const { return m_currentSnapshot; }
    

    std::vector<int> getCurrentSortedAgentIndices() const;
    
    // Callback when a generation completes
    void setOnGenerationComplete(std::function<void(int, float)> callback) {
        m_onGenerationComplete = callback;
    }
    
    // Callback when batch training completes
    void setOnBatchComplete(std::function<void()> callback) {
        m_onBatchComplete = callback;
    }
    
    bool showAllAgents = false;  // "see all" mode
    
    // network inputs
    static constexpr int NUM_INPUTS = 4;
    static constexpr int NUM_OUTPUTS = 1;
    static constexpr float ELITE_RATIO = 0.30f;  
    static constexpr float MUTATION_RATIO = 0.70f;
    static constexpr int DEFAULT_POPULATION_SIZE = 1000; // Experimenting with high populations

private:
    std::vector<Genome> createNextGeneration();
    
    void saveGenerationToHistory();
    
    std::vector<float> prepareInputs(const AgentSimulation& sim) const;

    std::vector<Genome> population;
    Speciator speciator;
    std::unique_ptr<SimulationManager> m_simManager;

    int generation = 1;
    float bestFitness = 0.0f;
    bool enabled = false;
    
    NEATState m_state = NEATState::Idle;
    
    int m_selectedAgentIndex = 0;
    float m_replayTime = 0.0f;
    
    // Real-time replay state
    AgentSimulation m_replaySimulation;  // Live replay
    Genome* m_replayGenome = nullptr;    // Pointer to genome being replayed
    
    // Batch training state
    int m_batchTotalGens = 0;
    int m_batchCurrentGen = 0;
    bool m_batchStopRequested = false;
    
    // Training history
    TrainingHistory m_history;

    bool m_viewingHistory = false;
    int m_viewingGeneration = 0;
    const GenerationSnapshot* m_currentSnapshot = nullptr;
    
    std::function<void(int, float)> m_onGenerationComplete;
    std::function<void()> m_onBatchComplete;
};
