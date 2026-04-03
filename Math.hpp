// Activations.hpp
#pragma once
#include <cmath>
#include "Node.hpp"
#include <queue>

inline double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

inline double identity(double x) {
    return x;
}

inline double relu(double x) {
    return x > 0.0 ? x : 0.0;
}

inline std::vector<Node*> topologicalSort(std::unordered_map<Node*, std::vector<Node*>>& edges) {
    std::unordered_map<Node*, int> inDegree;

    for (auto& [node, neighbours] : edges) {
        if (!inDegree.count(node)) inDegree[node] = 0;
        for (Node* neighbour : neighbours)
            inDegree[neighbour]++;
    }

    std::queue<Node*> queue;
    for (auto& [node, degree] : inDegree)
        if (degree == 0) queue.push(node);

    std::vector<Node*> sorted;
    while (!queue.empty()) {
        Node* current = queue.front(); queue.pop();
        sorted.push_back(current);
        for (Node* neighbour : edges[current]) {
            if (--inDegree[neighbour] == 0)
                queue.push(neighbour);
        }
    }

    return sorted;
}