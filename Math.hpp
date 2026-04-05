#pragma once
#include <queue>
#include <vector>
#include <unordered_map>
#include "Node.hpp"

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