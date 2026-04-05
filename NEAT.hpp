#pragma once
#include "Genome.hpp"
#include "Species.hpp"
#include <vector>

// Forward declaration
class Speciator;

// Population
Genome createInitialGenome(int nInputs, int nOutputs);
std::vector<Genome> createInitialPopulation(int size, int inputs, int outputs);

// Distance
float genomeDistance(const Genome& g1, const Genome& g2, float c1 = 1.0f, float c2 = 1.0f, float c3 = 0.4f);

// Crossover
Genome crossover(const Genome& parent1, const Genome& parent2);

// Reproduction
std::vector<Genome> reproduceSpecies(Species& species, int offspringCount);

// Evolution
std::vector<Genome> evolution(std::vector<Genome>& population, Speciator& speciator, int stagnationLimit = 15);

// Fitness
float fitnessXor(Genome& genome);