#pragma once

#include <vector>
#include <algorithm>
#include <iostream>
#include "AgentSimulation.hpp"
#include "ThreadPool.hpp"
#include "Genome.hpp"


class SimulationManager {
private:
    static constexpr float SIM_DT = 0.016f; // ~60 FPS physics timestep
    
public:
    explicit SimulationManager(int populationSize)
        : m_populationSize(populationSize)
        , m_threadPool(0)  // Auto-detect thread count
    {
        m_agents.resize(populationSize);
        std::cout << "SimulationManager: " << populationSize << " agents, " 
                  << m_threadPool.getThreadCount() << " threads" << std::endl;
    }
    
    // Reset all agents for a new generation
    void resetAll() {
        for (auto& agent : m_agents) {
            agent.reset();
        }
        m_allDone = false;
        m_generationComplete = false;
        m_generationTime = 0.0f;
    }
    
    void startGeneration(std::vector<Genome>& genomes) {
        if (genomes.size() != m_agents.size()) {
            std::cerr << "SimulationManager: genome count mismatch!" << std::endl;
            return;
        }
        
        m_currentGenomes = &genomes;
        resetAll();
        m_isRunning = true;
    }
    
    bool updateGeneration(float dt) {
        if (!m_isRunning || m_generationComplete) return true;
        
        constexpr float MAX_GEN_TIME = 40.0f;
        
        constexpr int STEPS_PER_FRAME = 20;
        
        m_threadPool.dispatch(static_cast<uint32_t>(m_agents.size()), 
            [this, STEPS_PER_FRAME, MAX_GEN_TIME](uint32_t start, uint32_t end) {
                for (int step = 0; step < STEPS_PER_FRAME; ++step) {
                    bool anyActive = false;
                    for (uint32_t i = start; i < end; ++i) {
                        if (!m_agents[i].done) {
                            // Get control from genome
                            float control = evaluateGenome((*m_currentGenomes)[i], m_agents[i]);
                            m_agents[i].step(SIM_DT, control);
                            anyActive = true;
                        }
                    }
                    // Early exit if all agents in this batch are done
                    if (!anyActive) break;
                }
            });
        
        m_generationTime += SIM_DT * STEPS_PER_FRAME;
        
        // Check if all done (after dispatch completes)
        m_allDone = true;
        for (const auto& agent : m_agents) {
            if (!agent.done) {
                m_allDone = false;
                break;
            }
        }
        
        // Check if generation complete
        if (m_allDone || m_generationTime >= MAX_GEN_TIME) {
            finishGeneration();
            return true;
        }
        
        return false;
    }
    
    // Run all agents until all are done (parallel, fast simulation) - BLOCKING
    // genomes: vector of genomes to evaluate (must match population size)
    void runGeneration(std::vector<Genome>& genomes) {
        startGeneration(genomes);
        
        // Block until complete
        while (!updateGeneration(0.016f)) {
            // Keep updating
        }
    }
    
    // Accessors
    int getPopulationSize() const { return m_populationSize; }
    bool isGenerationComplete() const { return m_generationComplete; }
    bool isRunning() const { return m_isRunning; }
    float getGenerationProgress() const { return m_generationTime / 60.0f; }
    
    const AgentSimulation& getAgent(int index) const {
        return m_agents[index];
    }
    
    AgentSimulation& getAgent(int index) {
        return m_agents[index];
    }
    
    // Get sorted indices by fitness (descending)
    std::vector<int> getSortedAgentIndices() const {
        std::vector<int> indices(m_agents.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = static_cast<int>(i);
        }
        
        std::sort(indices.begin(), indices.end(), [this](int a, int b) {
            return m_agents[a].fitness > m_agents[b].fitness;
        });
        
        return indices;
    }
    
    // Get best agent index
    int getBestAgentIndex() const {
        int bestIdx = 0;
        float bestFitness = 0.0f;
        for (size_t i = 0; i < m_agents.size(); ++i) {
            if (m_agents[i].fitness > bestFitness) {
                bestFitness = m_agents[i].fitness;
                bestIdx = static_cast<int>(i);
            }
        }
        return bestIdx;
    }
    
    float getBestFitness() const {
        float best = 0.0f;
        for (const auto& agent : m_agents) {
            best = std::max(best, agent.fitness);
        }
        return best;
    }

private:
    void finishGeneration() {
        if (!m_currentGenomes) return;
        
        // Transfer fitness scores to genomes
        for (size_t i = 0; i < m_agents.size(); ++i) {
            (*m_currentGenomes)[i].setFitness(m_agents[i].fitness);
        }
        
        m_generationComplete = true;
        m_isRunning = false;
        
        // Find best fitness
        float bestFitness = 0.0f;
        int bestIdx = 0;
        for (size_t i = 0; i < m_agents.size(); ++i) {
            if (m_agents[i].fitness > bestFitness) {
                bestFitness = m_agents[i].fitness;
                bestIdx = static_cast<int>(i);
            }
        }
        
        std::cout << "Generation complete. Best fitness: " << bestFitness 
                  << " (agent " << bestIdx << ")" << std::endl;
    }

    float evaluateGenome(Genome& genome, const AgentSimulation& agent) {
        // Normalize cart position to [-1, 1]
        float normalizedCartX = std::clamp(agent.cartX / agent.rightBound, -1.0f, 1.0f);
        
        // Get pendulum direction
        float sinTheta = std::sin(agent.theta);
        float cosTheta = std::cos(agent.theta);
        
        float angularDisplacement = agent.thetaDot * SIM_DT;
        
        std::vector<float> inputs = { 
            normalizedCartX,        // Cart position [-1, 1]
            sinTheta,               // Pendulum direction x [-1, 1]
            cosTheta,               // Pendulum direction y [-1, 1]
            angularDisplacement     // Angular velocity * dt
        };
        
        try {
            std::vector<float> outputs = genome.evaluate(inputs);
            if (!outputs.empty()) {
                return std::clamp(outputs[0], -1.0f, 1.0f);
            }
        } catch (const std::exception& e) {
            // Log error once to help debugging
            static bool errorLogged = false;
            if (!errorLogged) {
                std::cerr << "Genome evaluation error: " << e.what() 
                          << " (inputs=" << inputs.size() << ")" << std::endl;
                errorLogged = true;
            }
        }
        
        return 0.0f;
    }
    
    std::vector<AgentSimulation> m_agents;
    int m_populationSize;
    tp::ThreadPool m_threadPool;
    bool m_allDone = false;
    bool m_generationComplete = false;
    bool m_isRunning = false;
    float m_generationTime = 0.0f;
    std::vector<Genome>* m_currentGenomes = nullptr;
};
