#pragma once
#include "Species.hpp"
#include "Genome.hpp"
#include <vector>
#include <algorithm>

// Forward declaration of genomeDistance (defined in NEAT.cpp, defaults in NEAT.hpp)
float genomeDistance(const Genome& g1, const Genome& g2, float c1, float c2, float c3);

class Speciator {
public:
    Speciator(float compatibilityThreshold = 3.0f) : compatibilityThreshold(compatibilityThreshold) {}

    void speciate(std::vector<Genome>& population) {
        // Clear members but keep representatives
        for (Species& s : species)
            s.clearMembers();

        // Assign each genome to a species
        for (Genome& genome : population) {
            bool foundSpecies = false;
            for (Species& s : species) {
                if (genomeDistance(genome, *s.getRepresentative(), 1.0f, 1.0f, 0.4f) < compatibilityThreshold) {
                    s.addMember(&genome);
                    foundSpecies = true;
                    break;
                }
            }

            if (!foundSpecies)
                species.emplace_back(&genome);
        }

        // Remove empty species
        species.erase(
            std::remove_if(species.begin(), species.end(),
                [](const Species& s) { return s.getMembers().empty(); }),
            species.end()
        );

        // Update fitness stats and representative
        for (Species& s : species) {
            s.updateFitnessStats();

            // Set representative to best member
            const std::vector<Genome*>& members = s.getMembers();
            Genome* best = *std::max_element(members.begin(), members.end(),
                [](const Genome* a, const Genome* b) {
                    return a->getFitness() < b->getFitness();
                });
            s.setRepresentative(best);
        }
    }

    const std::vector<Species>& getSpecies() const { return species; }
    std::vector<Species>& getSpecies() { return species; }

private:
    std::vector<Species> species;
    float compatibilityThreshold;
};