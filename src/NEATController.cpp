#include "NEATController.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <fstream>
#include <sstream>

static std::mt19937& getRng() {
    thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

NEATController::NEATController(int populationSize, float compatibilityThreshold)
    : speciator(compatibilityThreshold)
{
    // Create initial population
    population = createInitialPopulation(populationSize, NUM_INPUTS, NUM_OUTPUTS);
    
    // Verify genome input count matches
    if (!population.empty()) {
        int actualInputs = 0;
        for (const auto& [id, node] : population[0].getNodes()) {
            if (node.getLayer() == Layer::INPUT) actualInputs++;
        }
        std::cout << "NEAT: Created population with " << populationSize << " genomes, "
                  << actualInputs << " inputs, " << NUM_OUTPUTS << " outputs" << std::endl;
        if (actualInputs != NUM_INPUTS) {
            std::cerr << "ERROR: Input count mismatch! Expected " << NUM_INPUTS 
                      << " but got " << actualInputs << std::endl;
        }
    }
    
    m_simManager = std::make_unique<SimulationManager>(populationSize);
    
    std::cout << "NEAT Controller initialized with " << populationSize << " genomes (parallel)" << std::endl;
}

void NEATController::setEnabled(bool val) {
    enabled = val;
    if (enabled) {
        m_state = NEATState::Idle;
    } else {
        m_state = NEATState::Idle;
    }
}

void NEATController::runGeneration() {
    if (!enabled || population.empty()) return;
    
    m_state = NEATState::Simulating;
    
    std::cout << "\n=== Running Generation " << generation << " (parallel) ===" << std::endl;
    
    // Start generation
    m_simManager->startGeneration(population);
}

bool NEATController::updateSimulation(float dt) {
    if (m_state != NEATState::Simulating) return false;
    
    // Update simulation
    bool complete = m_simManager->updateGeneration(dt);
    
    if (complete) {
        // Generation finished
        float genBest = m_simManager->getBestFitness();
        if (genBest > bestFitness) {
            bestFitness = genBest;
            std::cout << "New best fitness: " << bestFitness << std::endl;
        }
        
        // Save to history
        saveGenerationToHistory();
        
        int bestIdx = m_simManager->getBestAgentIndex();
        
        if (m_onGenerationComplete) {
            m_onGenerationComplete(generation, genBest);
        }
        
        // Select best agent for viewing
        m_selectedAgentIndex = bestIdx;
        
        bool inBatchMode = (m_batchTotalGens > 0 && !m_batchStopRequested);
        
        if (inBatchMode) {
            m_state = NEATState::BatchTraining;
            std::cout << "Generation " << generation << " complete (batch " << (m_batchCurrentGen + 1)
                      << "/" << m_batchTotalGens << ")" << std::endl;
        } else {
            m_viewingHistory = false;
            m_currentSnapshot = m_history.getLatestGeneration();
            
            std::cout << "Generation " << generation << " complete - starting replay" << std::endl;
            startReplay(m_selectedAgentIndex);
        }
        
        return true;
    }
    
    return false;
}

void NEATController::evolve() {
    if (!enabled || population.empty()) return;
    
    std::cout << "\n=== Evolving Generation " << generation << " ===" << std::endl;
    
    population = createNextGeneration();
    
    generation++;
    
    std::cout << "Starting Generation " << generation << std::endl;
}

std::vector<Genome> NEATController::createNextGeneration() {
    std::vector<Genome> newPopulation;
    newPopulation.reserve(population.size());
    
    std::vector<int> sortedIndices = m_simManager->getSortedAgentIndices();
    
    // Calculate counts
    int totalSize = static_cast<int>(population.size());
    int eliteCount = static_cast<int>(totalSize * ELITE_RATIO);
    int selectedCount = totalSize - eliteCount;
    
    std::cout << "Elite (unchanged): " << eliteCount << ", Selected+Mutated: " << selectedCount << std::endl;
    
    for (int i = 0; i < eliteCount && i < static_cast<int>(sortedIndices.size()); ++i) {
        int idx = sortedIndices[i];
        newPopulation.push_back(population[idx].copy());
    }
    
    std::vector<float> wheelScores;
    wheelScores.reserve(population.size());
    
    float totalFitness = 0.0f;
    for (const auto& genome : population) {
        totalFitness += genome.getFitness();
    }
    
    // probability distribution
    float cumulativeSum = 0.0f;
    if (totalFitness > 0.0f) {
        for (const auto& genome : population) {
            cumulativeSum += genome.getFitness() / totalFitness;
            wheelScores.push_back(cumulativeSum);
        }
    } else {
        float uniformScore = 1.0f / static_cast<float>(population.size());
        for (size_t i = 0; i < population.size(); ++i) {
            cumulativeSum += uniformScore;
            wheelScores.push_back(cumulativeSum);
        }
    }

    std::uniform_real_distribution<float> rouletteDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    
    constexpr int MUTATION_ATTEMPTS = 4;      
    constexpr float NEW_NODE_PROB = 0.05f;   
    constexpr float NEW_CONN_PROB = 0.80f;   
    constexpr float WEIGHT_MUT_PROB = 0.25f;  
    
    for (int i = 0; i < selectedCount; ++i) {

        float threshold = rouletteDist(getRng());
        int selectedIdx = 0;
        for (size_t j = 0; j < wheelScores.size(); ++j) {
            if (wheelScores[j] > threshold) {
                selectedIdx = static_cast<int>(j);
                break;
            }
        }
        
        // Copy selected genome
        Genome child = population[selectedIdx].copy();
        
        // Apply multiple mutation attempts
        for (int mut = 0; mut < MUTATION_ATTEMPTS; ++mut) {
            if (prob(getRng()) < WEIGHT_MUT_PROB) {
                if (prob(getRng()) < 0.5f) {
                    child.mutateBias();
                } else {
                    child.mutateWeights();
                }
            }
        }
        
        if (prob(getRng()) < NEW_CONN_PROB) {
            child.mutateAddConnection();
        }
        
        if (prob(getRng()) < NEW_NODE_PROB) {
            child.mutateAddNode();
        }
        
        newPopulation.push_back(std::move(child));
    }
    
    return newPopulation;
}

void NEATController::startReplay(int agentIndex) {
    Genome* genome = nullptr;
    int maxIndex = 0;
    float agentFitness = 0.0f;
    
    if (m_currentSnapshot) {
        maxIndex = static_cast<int>(m_currentSnapshot->genomes.size());
        if (agentIndex >= 0 && agentIndex < maxIndex) {

            genome = const_cast<Genome*>(&m_currentSnapshot->genomes[agentIndex]);
            agentFitness = m_currentSnapshot->getAgentFitness(agentIndex);
        }
    } else if (!population.empty()) {
        maxIndex = static_cast<int>(population.size());
        if (agentIndex >= 0 && agentIndex < maxIndex) {
            genome = &population[agentIndex];
            agentFitness = genome->getFitness();
        }
    }
    
    if (!genome) {
        std::cerr << "Cannot start replay: no valid genome at index " << agentIndex << std::endl;
        m_state = NEATState::WaitingForNext;
        return;
    }
    
    m_selectedAgentIndex = agentIndex;
    m_replayTime = 0.0f;
    m_replayTimeAccumulator = 0.0f;
    m_replayGenome = genome;
    m_state = NEATState::Replaying;
    
    // Reset replay simulation to initial state
    m_replaySimulation.reset();
    
    std::cout << "Replaying agent " << agentIndex << " (fitness: " << agentFitness << ") [LIVE]" << std::endl;
}

bool NEATController::updateReplay(float dt) {
    if (m_state != NEATState::Replaying) return true;
    
    if (!m_replayGenome) {
        m_state = NEATState::WaitingForNext;
        return true;
    }
    

    constexpr float SIM_DT = 0.016f;
    m_replayTimeAccumulator += dt;
    
    // Run physics simulation with genome controlling cart
    // Step multiple times if frame longer than SIM_DT
    while (m_replayTimeAccumulator >= SIM_DT && !m_replaySimulation.done) {
        std::vector<float> inputs = prepareInputs(m_replaySimulation);
        
        std::vector<float> outputs = m_replayGenome->evaluate(inputs);
        float control = outputs.empty() ? 0.0f : std::clamp(outputs[0], -1.0f, 1.0f);
        
        // Step physics
        m_replaySimulation.step(SIM_DT, control, true);
        m_replayTimeAccumulator -= SIM_DT;
    }
    
    m_replayTime += dt;
    

    if (m_replaySimulation.done || m_replayTime >= AgentSimulation::MAX_SIMULATION_TIME + 0.5f) {
        m_state = NEATState::WaitingForNext;
        return true;
    }
    
    return false;
}

std::vector<float> NEATController::prepareInputs(const AgentSimulation& sim) const {

    float normalizedCartX = std::clamp(sim.cartX / sim.rightBound, -1.0f, 1.0f);
    float sinTheta = std::sin(sim.theta);
    float cosTheta = std::cos(sim.theta);
    
    constexpr float SIM_DT = 0.016f;
    float angularDisplacement = sim.thetaDot * SIM_DT;
    
    return { normalizedCartX, sinTheta, cosTheta, angularDisplacement };
}

float NEATController::getReplayCartX() const {
    return m_replaySimulation.cartX;
}

float NEATController::getReplayTheta() const {
    return m_replaySimulation.theta;
}

void NEATController::selectAgent(int index) {
    int maxIndex = m_currentSnapshot 
        ? static_cast<int>(m_currentSnapshot->genomes.size())
        : static_cast<int>(population.size());
        
    if (index >= 0 && index < maxIndex) {
        startReplay(index);
    }
}

void NEATController::selectNextAgent() {
    int maxIndex = m_currentSnapshot 
        ? static_cast<int>(m_currentSnapshot->genomes.size())
        : static_cast<int>(population.size());
    if (maxIndex == 0) return;
    int next = (m_selectedAgentIndex + 1) % maxIndex;
    selectAgent(next);
}

void NEATController::selectPrevAgent() {
    int maxIndex = m_currentSnapshot 
        ? static_cast<int>(m_currentSnapshot->genomes.size())
        : static_cast<int>(population.size());
    if (maxIndex == 0) return;
    int prev = (m_selectedAgentIndex - 1 + maxIndex) % maxIndex;
    selectAgent(prev);
}

void NEATController::selectBestAgent() {
    if (m_currentSnapshot) {
        selectAgent(m_currentSnapshot->getBestAgentIndex());
    } else if (m_simManager) {
        selectAgent(m_simManager->getBestAgentIndex());
    }
}

float NEATController::getAgentFitness(int index) const {
    if (m_currentSnapshot) {
        return m_currentSnapshot->getAgentFitness(index);
    }
    
    if (m_simManager && index >= 0 && index < m_simManager->getPopulationSize()) {
        return m_simManager->getAgent(index).fitness;
    }
    return 0.0f;
}

void NEATController::skipToNextGeneration() {
    if (!enabled) return;
    
    // If currently replaying or waiting, advance next generation
    if (m_state == NEATState::Replaying || m_state == NEATState::WaitingForNext || m_state == NEATState::Idle) {
        evolve();
        runGeneration();
    }
}

void NEATController::saveGenerationToHistory() {
    std::vector<float> fitnesses;
    fitnesses.reserve(population.size());
    for (size_t i = 0; i < population.size(); ++i) {
        fitnesses.push_back(m_simManager->getAgent(static_cast<int>(i)).fitness);
    }
    
    m_history.addGeneration(generation, fitnesses, population);
}

void NEATController::startBatchTraining(int numGenerations) {
    if (!enabled || numGenerations <= 0) return;
    
    m_batchTotalGens = numGenerations;
    m_batchCurrentGen = 0;
    m_batchStopRequested = false;
    m_state = NEATState::BatchTraining;
    
    std::cout << "\n=== Starting Batch Training: " << numGenerations << " generations ===" << std::endl;
    
    if (m_history.isEmpty()) {
        runGeneration();
        m_batchCurrentGen = 1;
    } else {
        int lastSavedGen = m_history.getLastGenerationNumber();
        generation = lastSavedGen + 1;
        std::cout << "Resuming from generation " << generation << " (last saved: " << lastSavedGen << ")" << std::endl;

        runGeneration();
        m_batchCurrentGen = 1;
    }
    
}

void NEATController::stopBatchTraining() {
    if (m_state == NEATState::BatchTraining || m_state == NEATState::Simulating) {
        m_batchStopRequested = true;
        
        std::cout << "Batch training stopped by user at generation " << generation << std::endl;
        
        m_batchTotalGens = 0;
        m_batchCurrentGen = 0;
        
        if (!m_history.isEmpty()) {
            int latestGen = m_history.getLastGenerationNumber();
            loadGenerationForReplay(latestGen);
            m_selectedAgentIndex = m_currentSnapshot ? m_currentSnapshot->getBestAgentIndex() : 0;
            startReplay(m_selectedAgentIndex);
        } else {
            m_state = NEATState::WaitingForNext;
        }
    }
}

void NEATController::advanceBatchTraining() {
    if (!shouldContinueBatchTraining()) return;
    
    // Evolve and run next generation
    evolve();
    runGeneration();
    
    m_batchCurrentGen++;
    
    // Check if training is complete
    if (m_batchCurrentGen >= m_batchTotalGens) {
        completeBatchTraining();
    }
}

void NEATController::completeBatchTraining() {
    m_batchStopRequested = true;
    m_state = NEATState::WaitingForNext;
    
    std::cout << "Batch training complete! Generation " << generation << std::endl;
    
    // Reset batch state
    m_batchTotalGens = 0;
    m_batchCurrentGen = 0;
    
    // Load latest generation for replay and select best agent
    if (!m_history.isEmpty()) {
        int latestGen = m_history.getLastGenerationNumber();
        loadGenerationForReplay(latestGen);
        m_selectedAgentIndex = m_currentSnapshot ? m_currentSnapshot->getBestAgentIndex() : 0;
        startReplay(m_selectedAgentIndex);
    }
    
    // Notify UI that batch training is complete
    if (m_onBatchComplete) {
        m_onBatchComplete();
    }
}

bool NEATController::loadGenerationForReplay(int genNumber) {
    const GenerationSnapshot* snapshot = m_history.getGeneration(genNumber);
    if (!snapshot) {
        return false;
    }
    
    m_viewingHistory = true;
    m_viewingGeneration = genNumber;
    m_currentSnapshot = snapshot;
    
    int bestIdx = snapshot->getBestAgentIndex();
    m_selectedAgentIndex = bestIdx;
    
    // Start real time replay using genome
    startReplay(bestIdx);
    
    std::cout << "Loaded generation " << genNumber << " for replay" << std::endl;
    return true;
}

std::vector<int> NEATController::getCurrentSortedAgentIndices() const {
    if (m_viewingHistory && m_currentSnapshot) {
        std::vector<int> indices;
        indices.reserve(m_currentSnapshot->agentRecords.size());
        for (const auto& record : m_currentSnapshot->agentRecords) {
            indices.push_back(record.originalIndex);
        }
        return indices;
    }
    return m_simManager->getSortedAgentIndices();
}

bool NEATController::saveSimulation(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "{\"generation\":" << generation
         << ",\"bestFitness\":" << bestFitness
         << ",\"history\":" << m_history.toJson()
         << ",\"population\":[";
         
    for (size_t i = 0; i < population.size(); ++i) {
        if (i > 0) file << ",";
        file << population[i].toJson();
    }
    file << "]}";
    
    std::cout << "Simulation saved to " << filepath << std::endl;
    return true;
}

bool NEATController::loadSimulation(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filepath << " for loading" << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    // Parse JSON
    try {
        size_t genPos = json.find("\"generation\":");
        if (genPos != std::string::npos) {
            generation = std::stoi(json.substr(genPos + 13));
        }
        
        size_t bestFitPos = json.find("\"bestFitness\":");
        if (bestFitPos != std::string::npos) {
            bestFitness = std::stof(json.substr(bestFitPos + 14));
        }
        
        size_t histStart = json.find("\"history\":{");
        size_t histEnd = json.find("},\"population\":");
        if (histStart != std::string::npos && histEnd != std::string::npos) {
            // Find end of history safely
            std::string histStr = json.substr(histStart + 10, histEnd - (histStart + 10) + 1);
            m_history = TrainingHistory::fromJson(histStr);
        }
        
        size_t popStart = json.find("\"population\":[");
        size_t popEnd = json.rfind("]}");
        if (popStart != std::string::npos && popEnd != std::string::npos) {
            std::string popStr = json.substr(popStart + 13, popEnd - (popStart + 13) + 1);
            
            std::vector<Genome> newPopulation;
            GenerationSnapshot::parseJsonArray(popStr, [&newPopulation](const std::string& item) {
                newPopulation.push_back(Genome::fromJson(item));
            });
            
            if (!newPopulation.empty()) {
                population = std::move(newPopulation);
            }
        }
        
        // Reinitialize logic
        m_simManager = std::make_unique<SimulationManager>(static_cast<int>(population.size()));
        
        // Find max node and innovation IDs
        int maxNodeId = 0;
        int maxInnov = 0;
        for (const auto& genome : population) {
            for (const auto& [id, node] : genome.getNodes()) {
                maxNodeId = std::max(maxNodeId, id);
            }
            for (const auto& conn : genome.getConnections()) {
                maxInnov = std::max(maxInnov, conn.getInnovation());
            }
        }
        
        // Also check history for max IDs just in case
        for (size_t i = 0; i < m_history.getGenerationCount(); ++i) {
            auto snap = m_history.getGenerationByIndex(i);
            if (snap) {
                for (const auto& genome : snap->genomes) {
                    for (const auto& [id, node] : genome.getNodes()) {
                        maxNodeId = std::max(maxNodeId, id);
                    }
                    for (const auto& conn : genome.getConnections()) {
                        maxInnov = std::max(maxInnov, conn.getInnovation());
                    }
                }
            }
        }
        
        updateGlobalInnovationState(maxInnov, maxNodeId);
        
        m_state = NEATState::Idle;
        m_batchTotalGens = 0;
        m_batchCurrentGen = 0;
        m_selectedAgentIndex = 0;
        m_viewingHistory = false;
        
        std::cout << "Simulation loaded from " << filepath << " (Gen: " << generation 
                  << ", Pop: " << population.size() << ")" << std::endl;
                  
        // Setup initial view
        if (!m_history.isEmpty()) {
            int latestGen = m_history.getLastGenerationNumber();
            loadGenerationForReplay(latestGen);
        } else if (!population.empty()) {
            m_state = NEATState::WaitingForNext;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse simulation JSON: " << e.what() << std::endl;
        return false;
    }
}
