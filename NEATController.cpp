#include "NEATController.hpp"
#include <algorithm>
#include <iostream>

NEATController::NEATController(int populationSize, float compatibilityThreshold)
    : speciator(compatibilityThreshold)
{
    // Create initial population
    population = createInitialPopulation(populationSize, NUM_INPUTS, NUM_OUTPUTS);
    std::cout << "NEAT Controller initialized with " << populationSize << " genomes" << std::endl;
}

std::vector<float> NEATController::normalizeInputs(float theta, float thetaDot, float cartX, float cartVel) {
    constexpr float PI = 3.14159265f;
    
    // Normalize theta relative to upright position (PI)
    float normalizedTheta = std::fmod(theta, 2.0f * PI);
    if (normalizedTheta > PI) normalizedTheta -= 2.0f * PI;
    if (normalizedTheta < -PI) normalizedTheta += 2.0f * PI;
    
    // Use sin to represent angle deviation from upright
    // sin(theta) = 0 when upright (theta = PI), ranges -1 to 1
    float sinTheta = std::sin(normalizedTheta);

    // Normalize angular velocity (clamp to reasonable range)
    float normalizedThetaDot = std::clamp(thetaDot / 10.0f, -1.0f, 1.0f);

    // Normalize cart position (assuming window width ~1580, center ~790)
    float normalizedCartX = std::clamp(cartX / 600.0f, -1.0f, 1.0f);

    // Normalize cart velocity
    float normalizedCartVel = std::clamp(cartVel / 500.0f, -1.0f, 1.0f);

    return { sinTheta, normalizedThetaDot, normalizedCartX, normalizedCartVel };
}

float NEATController::getControl(float theta, float thetaDot, float cartX, float cartVel) {
    if (!enabled || population.empty() || currentGenomeIndex >= static_cast<int>(population.size())) {
        return 0.0f;
    }

    std::vector<float> inputs = normalizeInputs(theta, thetaDot, cartX, cartVel);

    try {
        std::vector<float> outputs = population[currentGenomeIndex].evaluate(inputs);
        if (!outputs.empty()) {
            // Map output from [0, 1] (sigmoid output) to [-1, 1]
            float control = (outputs[0] - 0.5f) * 2.0f;
            return std::clamp(control, -1.0f, 1.0f);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "NEAT evaluation error: " << e.what() << std::endl;
    }

    return 0.0f;
}

bool NEATController::isUpright(float theta) const {
    // Theta measured from vertical down position
    // Upright position is theta = PI (or -PI)
    // We need to check if the pendulum is within ±5° of upright
    
    constexpr float PI = 3.14159265f;
    
    // Normalize theta to [-PI, PI]
    float normalizedTheta = std::fmod(theta, 2.0f * PI);
    if (normalizedTheta > PI) normalizedTheta -= 2.0f * PI;
    if (normalizedTheta < -PI) normalizedTheta += 2.0f * PI;

    // Distance from upright (PI or -PI)
    float distFromUpright = std::min(std::abs(normalizedTheta - PI), std::abs(normalizedTheta + PI));
    
    return distFromUpright <= UPRIGHT_THRESHOLD;
}

bool NEATController::updateFitness(float dt, float theta) {
    if (!enabled || population.empty()) return false;

    currentEvaluationTime += dt;
    generationTime += dt;  // Track total generation time
    
    constexpr float PI = 3.14159265f;

    // Reward for being upright
    if (isUpright(theta)) {
        currentFitness += dt; // Add time spent upright as fitness
    }

    // Normalize theta to check how far from upright
    float normalizedTheta = std::fmod(theta, 2.0f * PI);
    if (normalizedTheta > PI) normalizedTheta -= 2.0f * PI;
    if (normalizedTheta < -PI) normalizedTheta += 2.0f * PI;
    
    // Distance from upright position (PI)
    float distFromUpright = std::min(std::abs(normalizedTheta - PI), std::abs(normalizedTheta + PI));
    
    // If pendulum is more than 45 degrees from upright, end evaluation early
    bool fellOver = distFromUpright > (45.0f * PI / 180.0f);
    
    if (currentEvaluationTime >= MAX_EVALUATION_TIME || fellOver) {
        // Store fitness and move to next genome
        population[currentGenomeIndex].setFitness(currentFitness);
        
        if (currentFitness > bestFitness) {
            bestFitness = currentFitness;
            std::cout << "New best fitness: " << bestFitness << " (Gen " << generation 
                      << ", Genome " << currentGenomeIndex << ")" << std::endl;
        }

        nextGenome();
        return true; // Signal that simulation should reset
    }
    
    return false;
}

void NEATController::nextGenome() {
    currentGenomeIndex++;
    currentFitness = 0.0f;
    currentEvaluationTime = 0.0f;

    if (currentGenomeIndex >= static_cast<int>(population.size())) {
        // All genomes evaluated - check if minimum generation time has passed
        if (generationTime >= MIN_GENERATION_TIME) {
            evolve();
        } else {
            // Restart from first genome until time requirement met
            currentGenomeIndex = 0;
            std::cout << "Restarting generation (time: " << generationTime << "s / " 
                      << MIN_GENERATION_TIME << "s)" << std::endl;
        }
    }
    else {
        std::cout << "Testing genome " << currentGenomeIndex << "/" << population.size() 
                  << " (Gen " << generation << ", time: " << static_cast<int>(generationTime) << "s)" << std::endl;
    }
}

void NEATController::skipToNextGeneration() {
    if (!enabled || population.empty()) return;
    
    // Assign current fitness to current genome
    population[currentGenomeIndex].setFitness(currentFitness);
    
    // Assign 0 fitness to remaining genomes
    for (int i = currentGenomeIndex + 1; i < static_cast<int>(population.size()); i++) {
        population[i].setFitness(0.0f);
    }
    
    std::cout << "Skipping to next generation..." << std::endl;
    evolve();
}

void NEATController::evolve() {
    std::cout << "\n=== Evolving Generation " << generation << " (total time: " 
              << generationTime << "s) ===" << std::endl;
    
    // Evolve population
    population = evolution(population, speciator);
    
    generation++;
    currentGenomeIndex = 0;
    currentFitness = 0.0f;
    currentEvaluationTime = 0.0f;
    generationTime = 0.0f;  // Reset generation timer

    std::cout << "Starting Generation " << generation << std::endl;
    std::cout << "Testing genome 0/" << population.size() << std::endl;
}

void NEATController::resetCurrentGenome() {
    currentFitness = 0.0f;
    currentEvaluationTime = 0.0f;
}
