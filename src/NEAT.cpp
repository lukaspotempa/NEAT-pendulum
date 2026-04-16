#include <algorithm>
#include <unordered_map>
#include "Genome.hpp"
#include "Node.hpp"
#include "Math.hpp"
#include <random>
#include "ConnectionGene.hpp"
#include <unordered_set>
#include "Species.hpp"
#include "Speciator.hpp"
#include <iostream>
#include <mutex>

inline uint64_t makeKey(int nodeIn, int nodeOut) {
    return (static_cast<uint64_t>(nodeIn) << 32) | static_cast<uint32_t>(nodeOut);
}


static std::mutex innovationMutex;
int currentInnovation = 0;
std::unordered_map<uint64_t, int> connectionInnovations;
std::unordered_map<int, NodeInnovation> nodeInnovations;
int nodeIdCounter = 0;

int getConnectionInnovation(int nodeIn, int nodeOut) {
    std::lock_guard<std::mutex> lock(innovationMutex);
    uint64_t key = makeKey(nodeIn, nodeOut);
    auto [it, inserted] = connectionInnovations.emplace(key, currentInnovation);
    if (inserted) currentInnovation++;
    return it->second;
}

NodeInnovation getNodeInnovation(int connectionInnov) {
    std::lock_guard<std::mutex> lock(innovationMutex);
    auto it = nodeInnovations.find(connectionInnov);
    if (it == nodeInnovations.end()) {
        NodeInnovation ni;
        ni.nodeId = nodeIdCounter++;
        ni.conn1Innov = currentInnovation++;
        ni.conn2Innov = currentInnovation++;
        nodeInnovations[connectionInnov] = ni;
        return ni;
    }

    return it->second;
}

void updateGlobalInnovationState(int maxInnovation, int maxNodeId) {
    std::lock_guard<std::mutex> lock(innovationMutex);
    if (maxInnovation >= currentInnovation) currentInnovation = maxInnovation + 1;
    if (maxNodeId >= nodeIdCounter) nodeIdCounter = maxNodeId + 1;
}

static std::mt19937& getRng() {
    thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

Genome createInitialGenome(int nInputs, int nOutputs) {
    std::unordered_map<int, Node> nodes;
    std::vector<ConnectionGene> connections;

    nodes.reserve(nInputs + nOutputs);

    for (int i = 0; i < nInputs; i++) {
        nodes.emplace(i, Node(Layer::INPUT, i, identity, 0.0f, ActivationType::IDENTITY));
    }

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (int i = 0; i < nOutputs; i++) {
        int id = nInputs + i;
        // Use tanh for output nodes to get [-1. 1]
        nodes.emplace(id, Node(Layer::OUTPUT, id, tanhActivation, dist(getRng()), ActivationType::TANH));
    }

    // Update nodeIdCounter
    nodeIdCounter = std::max(nodeIdCounter, nInputs + nOutputs);

    for (int i = 0; i < nInputs; i++) {
        for (int j = 0; j < nOutputs; j++) {
            int innov = getConnectionInnovation(i, nInputs + j);
            connections.emplace_back(i, nInputs + j, dist(getRng()), innov, true);
        }
    }

    return Genome(std::move(nodes), std::move(connections));
}

std::vector<Genome> createInitialPopulation(int size, int inputs, int outputs) {
    std::vector<Genome> population;
    population.reserve(size);

    for (int i = 0; i < size; i++) {
        population.emplace_back(createInitialGenome(inputs, outputs));
    }

    return population;
}

float genomeDistance(const Genome& g1, const Genome& g2, float c1, float c2, float c3) {
    std::unordered_map<int, const ConnectionGene*> genes1;
    std::unordered_map<int, const ConnectionGene*> genes2;

    for (const ConnectionGene& gene : g1.getConnections())
        genes1[gene.getInnovation()] = &gene;

    for (const ConnectionGene& gene : g2.getConnections())
        genes2[gene.getInnovation()] = &gene;

    int maxInnov1 = genes1.empty() ? 0 : std::max_element(genes1.begin(), genes1.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; })->first;
    int maxInnov2 = genes2.empty() ? 0 : std::max_element(genes2.begin(), genes2.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; })->first;
    int maxInnov = std::min(maxInnov1, maxInnov2);

    int matchingCount = 0;
    int disjointCount = 0;
    int excessCount = 0;
    float weightDiffSum = 0.0f;

    // Check all innovations from both genomes
    std::unordered_set<int> allInnovations;
    for (const auto& [innov, _] : genes1) allInnovations.insert(innov);
    for (const auto& [innov, _] : genes2) allInnovations.insert(innov);

    for (int innov : allInnovations) {
        bool inG1 = genes1.count(innov);
        bool inG2 = genes2.count(innov);

        if (inG1 && inG2) {
            matchingCount++;
            weightDiffSum += std::abs(genes1[innov]->getWeight() - genes2[innov]->getWeight());
        }
        else {
            if (innov > maxInnov)
                excessCount++;
            else
                disjointCount++;
        }
    }

    float avgWeightDiff = matchingCount > 0 ? weightDiffSum / matchingCount : 0.0f;

    size_t N = std::max(g1.getConnections().size(), g2.getConnections().size());
    if (N < 20) N = 1;

    return (c1 * excessCount) / static_cast<float>(N) + (c2 * disjointCount) / static_cast<float>(N) + c3 * avgWeightDiff;
}

Genome crossover(const Genome& parent1, const Genome& parent2) {
    // Assumes parent1 is the fitter parent
    std::vector<ConnectionGene> offspringConnections;
    std::unordered_set<int> offspringNodeIds;
    std::unordered_map<int, Node> allNodes;

    for (const auto& [id, node] : parent1.getNodes()) {
        allNodes.emplace(id, node.copy());

        if (node.getLayer() == Layer::INPUT || node.getLayer() == Layer::OUTPUT || node.getLayer() == Layer::HIDDEN) {
            offspringNodeIds.insert(id);
        }
    }
    for (const auto& [id, node] : parent2.getNodes()) {
        if (allNodes.find(id) == allNodes.end()) {
            allNodes.emplace(id, node.copy());
        }
    }

    std::unordered_map<int, const ConnectionGene*> genes1;
    std::unordered_map<int, const ConnectionGene*> genes2;

    for (const auto& g : parent1.getConnections())
        genes1[g.getInnovation()] = &g;

    for (const auto& g : parent2.getConnections())
        genes2[g.getInnovation()] = &g;

    // Combine all innovation numbers
    std::unordered_set<int> allInnovs;
    for (const auto& [innov, _] : genes1) allInnovs.insert(innov);
    for (const auto& [innov, _] : genes2) allInnovs.insert(innov);

    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    std::vector<int> sortedInnovs(allInnovs.begin(), allInnovs.end());
    std::sort(sortedInnovs.begin(), sortedInnovs.end());

    for (int innov : sortedInnovs) {
        const ConnectionGene* gene1 = genes1.count(innov) ? genes1[innov] : nullptr;
        const ConnectionGene* gene2 = genes2.count(innov) ? genes2[innov] : nullptr;

        ConnectionGene geneCopy(0, 0, 0.0f, 0, false); // placeholder

        if (gene1 && gene2) {
            const ConnectionGene* selected = (prob(getRng()) < 0.5f) ? gene1 : gene2;
            geneCopy = selected->copy();

            if (!gene1->isEnabled() || !gene2->isEnabled()) {
                if (prob(getRng()) < 0.75f) {
                    geneCopy.setEnabled(false);
                }
            }
        }
        else if (gene1 && !gene2) {
            geneCopy = gene1->copy();
        }
        else {
            continue;
        }

        int inNodeId = geneCopy.getInNodeId();
        int outNodeId = geneCopy.getOutNodeId();

        if (allNodes.find(inNodeId) != allNodes.end() && allNodes.find(outNodeId) != allNodes.end()) {
            offspringConnections.push_back(geneCopy);
            offspringNodeIds.insert(inNodeId);
            offspringNodeIds.insert(outNodeId);
        }
    }

    // Build final node map
    std::unordered_map<int, Node> offspringNodes;
    for (int nodeId : offspringNodeIds) {
        offspringNodes.emplace(nodeId, allNodes.at(nodeId).copy());
    }

    Genome offspring(std::move(offspringNodes), std::move(offspringConnections));
    offspring.cleanupInvalidConnections(); 
    return offspring;
}

std::vector<Genome> reproduceSpecies(Species& species, int offspringCount) {
    std::vector<Genome> offspring;
    offspring.reserve(offspringCount);

    const std::vector<Genome*>& members = species.getMembers();
    if (members.empty()) return offspring;

    std::vector<Genome*> sortedMembers = members;
    std::sort(sortedMembers.begin(), sortedMembers.end(),
        [](const Genome* a, const Genome* b) { return a->getFitness() > b->getFitness(); });

    // Use top 50% as potential parents
    size_t numParents = std::max(static_cast<size_t>(2), sortedMembers.size() / 2);
    numParents = std::min(numParents, sortedMembers.size());

    std::uniform_int_distribution<size_t> parentDist(0, numParents - 1);

    for (int i = 0; i < offspringCount; i++) {
        size_t idx1 = parentDist(getRng());
        size_t idx2 = parentDist(getRng());

        if (numParents > 1 && idx1 == idx2) {
            idx2 = (idx2 + 1) % numParents;
        }

        Genome* parent1 = sortedMembers[idx1];
        Genome* parent2 = sortedMembers[idx2];

        // Ensure parent1 is the fitter one
        if (parent2->getFitness() > parent1->getFitness()) {
            std::swap(parent1, parent2);
        }

        Genome child = crossover(*parent1, *parent2);
        child.mutate();
        offspring.push_back(std::move(child));
    }

    return offspring;
}

std::vector<Genome> evolution(std::vector<Genome>& population, Speciator& speciator, int stagnationLimit) {
    std::vector<Genome> newPopulation;
    newPopulation.reserve(population.size());

    // Speciate population
    speciator.speciate(population);
    std::vector<Species> speciesList = speciator.getSpecies();

    // Sort species by best fitness
    std::sort(speciesList.begin(), speciesList.end(),
        [](const Species& a, const Species& b) { return a.getBestFitness() > b.getBestFitness(); });

    std::cout << "Species created: " << speciesList.size() << std::endl;

    // Remove stagnant species
    std::vector<Species*> survivingSpecies;
    std::vector<Species> survivingSpeciesOwned;

    if (!speciesList.empty()) {
        survivingSpeciesOwned.push_back(speciesList[0]); // Keep best regardless of stagnation
        for (size_t i = 1; i < speciesList.size(); i++) {
            if (speciesList[i].getStagnantGenerations() < stagnationLimit) {
                survivingSpeciesOwned.push_back(speciesList[i]);
            }
        }
    }

    std::cout << "Species that survived: " << survivingSpeciesOwned.size() << std::endl;

    // Calculate total adjusted fitness
    float totalAdjustedFitness = 0.0f;
    for (const auto& s : survivingSpeciesOwned) {
        totalAdjustedFitness += s.getAdjustedFitness();
    }

    std::cout << "Total adjusted fitness: " << totalAdjustedFitness << std::endl;

    // Elitism
    for (const auto& species : survivingSpeciesOwned) {
        if (!species.getMembers().empty()) {
            Genome* best = *std::max_element(species.getMembers().begin(), species.getMembers().end(),
                [](const Genome* a, const Genome* b) { return a->getFitness() < b->getFitness(); });
            newPopulation.push_back(best->copy());
        }
    }

    int remainingOffspring = static_cast<int>(population.size()) - static_cast<int>(newPopulation.size());

    // Allocate remaining offsprin
    for (auto& species : survivingSpeciesOwned) {
        int offspringCount = 0;
        if (totalAdjustedFitness > 0) {
            offspringCount = static_cast<int>((species.getAdjustedFitness() / totalAdjustedFitness) * remainingOffspring);
        }
        else if (!survivingSpeciesOwned.empty()) {
            offspringCount = remainingOffspring / static_cast<int>(survivingSpeciesOwned.size());
        }

        if (offspringCount > 0) {
            std::vector<Genome> offspring = reproduceSpecies(species, offspringCount);
            for (auto& child : offspring) {
                newPopulation.push_back(std::move(child));
            }
        }
    }

    while (newPopulation.size() < population.size() && !survivingSpeciesOwned.empty()) {
        auto& bestSpecies = *std::max_element(survivingSpeciesOwned.begin(), survivingSpeciesOwned.end(),
            [](const Species& a, const Species& b) { return a.getAdjustedFitness() < b.getAdjustedFitness(); });
        std::vector<Genome> offspring = reproduceSpecies(bestSpecies, 1);
        for (auto& child : offspring) {
            newPopulation.push_back(std::move(child));
        }
    }

    return newPopulation;
}

float fitnessXor(Genome& genome) {
    // XOR Problem data 
    std::vector<std::vector<float>> X = { {0, 0}, {0, 1}, {1, 0}, {1, 1} };
    std::vector<float> y = { 0, 1, 1, 0 };

    float totalError = 0.0f;
    for (size_t i = 0; i < X.size(); i++) {
        try {
            std::vector<float> output = genome.evaluate(X[i]);
            if (!output.empty()) {
                float error = std::abs(output[0] - y[i]);
                totalError += error;
            }
            else {
                totalError += y[i];
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 0.0f;
        }
    }

    float fitness = 4.0f - totalError;
    return std::max(0.0f, fitness);
}
