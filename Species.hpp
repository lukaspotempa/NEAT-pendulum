#pragma once
#include "Genome.hpp"
#include <vector>
#include <limits>
#include <numeric>

class Species {
public:
    Species(Genome* representative) : representative(representative) {
        members.push_back(representative);
    }

    void addMember(Genome* genome) {
        members.push_back(genome);
    }

    void clearMembers() {
        members.clear();
    }

    void updateFitnessStats() {
        if (members.empty()) {
            adjustedFitness = 0.0f;
            return;
        }

        float currentBestFitness = -std::numeric_limits<float>::infinity();
        for (Genome* member : members)
            currentBestFitness = std::max(currentBestFitness, member->getFitness());

        if (currentBestFitness > bestFitness) {
            bestFitness = currentBestFitness;
            stagnantGenerations = 0;
        }
        else {
            stagnantGenerations++;
        }

        float fitnessSum = 0.0f;
        for (Genome* member : members)
            fitnessSum += member->getFitness();

        adjustedFitness = fitnessSum / members.size();
    }

    Genome* getRepresentative() const { return representative; }
    const std::vector<Genome*>& getMembers() const { return members; }
    float getAdjustedFitness() const { return adjustedFitness; }
    float getBestFitness() const { return bestFitness; }
    int getStagnantGenerations() const { return stagnantGenerations; }
    void setRepresentative(Genome* representative) { this->representative = representative; }

private:
    Genome* representative;
    std::vector<Genome*> members;
    float adjustedFitness = 0.0f;
    float bestFitness = -std::numeric_limits<float>::infinity();
    int stagnantGenerations = 0;
};