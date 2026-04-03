#pragma once

#include "Genome.hpp"
#include "Speciator.hpp"
#include "NEAT.hpp"
#include <vector>
#include <cmath>

// NEAT Controller for pendulum balancing
// Inputs: pendulum angle (normalized), angular velocity (normalized), cart position (normalized), cart velocity (normalized)
// Output: force direction (-1 to 1)

class NEATController {
public:
    NEATController(int populationSize = 50, float compatibilityThreshold = 3.0f);

    // Get control output from current genome (-1 to 1 force)
    float getControl(float theta, float thetaDot, float cartX, float cartVel);

    // Update fitness based on current pendulum state
    // Call this every frame while NEAT is active
    // Returns true if simulation should be reset (genome evaluation ended)
    bool updateFitness(float dt, float theta);

    // Advance to next genome in population (call when current genome fails)
    void nextGenome();

    // Evolve to next generation (call when all genomes evaluated)
    void evolve();

    // Check if pendulum is within upright threshold (radians)
    bool isUpright(float theta) const;

    // Reset for new evaluation
    void resetCurrentGenome();
    
    // Manual advance to next generation (N key)
    void skipToNextGeneration();

    // Getters
    int getCurrentGenomeIndex() const { return currentGenomeIndex; }
    int getPopulationSize() const { return static_cast<int>(population.size()); }
    int getGeneration() const { return generation; }
    float getCurrentFitness() const { return currentFitness; }
    float getBestFitness() const { return bestFitness; }
    float getGenerationTime() const { return generationTime; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool val) { enabled = val; }

    // Constants
    static constexpr int NUM_INPUTS = 4;  // theta, thetaDot, cartX, cartVel
    static constexpr int NUM_OUTPUTS = 1; // force
    static constexpr float UPRIGHT_THRESHOLD = 5.0f * 3.14159265f / 180.0f; // 5 degrees in radians
    static constexpr float MAX_EVALUATION_TIME = 60.0f; // Max seconds per genome (increased)
    static constexpr float MIN_GENERATION_TIME = 40.0f; // Minimum seconds per generation

private:
    // Normalize inputs to [-1, 1] range
    std::vector<float> normalizeInputs(float theta, float thetaDot, float cartX, float cartVel);

    std::vector<Genome> population;
    Speciator speciator;

    int currentGenomeIndex = 0;
    int generation = 1;
    float currentFitness = 0.0f;
    float currentEvaluationTime = 0.0f;
    float generationTime = 0.0f;  // Total time spent on current generation
    float bestFitness = 0.0f;
    bool enabled = false;
};
