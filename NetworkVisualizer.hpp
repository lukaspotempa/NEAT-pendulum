#pragma once

#include <SFML/Graphics.hpp>
#include "Genome.hpp"
#include "Constants.hpp"
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <cmath>
#include <algorithm>

// Visualizes a neural network
class NetworkVisualizer : public sf::Drawable {
public:
    static constexpr float NODE_RADIUS = 18.0f;
    static constexpr float LAYER_SPACING = 160.0f;
    static constexpr float NODE_SPACING = 65.0f;
    static constexpr float CONNECTION_WIDTH = 3.0f;
    static constexpr float CONTAINER_PADDING = 15.0f;
    static constexpr float CORNER_RADIUS = 12.0f;
    
    NetworkVisualizer(float x, float y, float width, float height, const sf::Font& font)
        : m_bounds({x, y}, {width, height})
        , m_font(font)
    {
        m_background.setPosition({x, y});
        m_background.setSize({width, height});
        m_background.setFillColor(Constants::COLOR_ZINC_800);
        m_background.setOutlineColor(Constants::COLOR_ZINC_600);
        m_background.setOutlineThickness(2.0f);
    }
    
    // Set the genome to visualize
    void setGenome(const Genome* genome) {
        m_genome = genome;
        if (genome) {
            computeLayout();
        }
    }
    
    void updateValues(const std::vector<float>& inputs, float output) {
        m_inputValues = inputs;
        m_outputValue = output;
    }
    
    void setInputLabels(const std::vector<std::string>& labels) {
        m_inputLabels = labels;
    }
    
    void setOutputLabel(const std::string& label) {
        m_outputLabel = label;
    }

private:
    void computeLayout() {
        if (!m_genome) return;
        
        m_nodePositions.clear();
        m_layers.clear();
        m_nodeDepths.clear();
        
        const auto& nodes = m_genome->getNodes();
        const auto& connections = m_genome->getConnections();
        
        std::vector<int> inputNodes, hiddenNodes, outputNodes;
        
        for (const auto& [id, node] : nodes) {
            switch (node.getLayer()) {
                case Layer::INPUT:
                    inputNodes.push_back(id);
                    break;
                case Layer::HIDDEN:
                    hiddenNodes.push_back(id);
                    break;
                case Layer::OUTPUT:
                    outputNodes.push_back(id);
                    break;
            }
        }
        
        // Sort by id
        std::sort(inputNodes.begin(), inputNodes.end());
        std::sort(hiddenNodes.begin(), hiddenNodes.end());
        std::sort(outputNodes.begin(), outputNodes.end());
        
        std::unordered_map<int, std::vector<int>> outgoingEdges;  // node -> nodes it connects to
        std::unordered_map<int, std::vector<int>> incomingEdges;  // node -> nodes that connect to it
        
        for (const auto& conn : connections) {
            if (conn.isEnabled()) {
                outgoingEdges[conn.getInNodeId()].push_back(conn.getOutNodeId());
                incomingEdges[conn.getOutNodeId()].push_back(conn.getInNodeId());
            }
        }

        std::unordered_map<int, int> nodeDepth;
        
        for (int id : inputNodes) {
            nodeDepth[id] = 0;
        }
        
        // BFS to get max depth from any input
        std::queue<int> queue;
        for (int id : inputNodes) {
            queue.push(id);
        }
        
        int maxHiddenDepth = 0;
        while (!queue.empty()) {
            int current = queue.front();
            queue.pop();
            
            int currentDepth = nodeDepth[current];
            
            for (int next : outgoingEdges[current]) {
                // update if we found a longer path
                if (nodeDepth.find(next) == nodeDepth.end() || nodeDepth[next] < currentDepth + 1) {
                    nodeDepth[next] = currentDepth + 1;
                    queue.push(next);
                    
                    if (nodes.count(next) && nodes.at(next).getLayer() == Layer::HIDDEN) {
                        maxHiddenDepth = std::max(maxHiddenDepth, nodeDepth[next]);
                    }
                }
            }
        }
        
        for (int id : hiddenNodes) {
            if (nodeDepth.find(id) == nodeDepth.end()) {
                nodeDepth[id] = 1;
            }
        }
        
        std::map<int, std::vector<int>> hiddenByDepth;
        for (int id : hiddenNodes) {
            hiddenByDepth[nodeDepth[id]].push_back(id);
        }
        
        int numHiddenLayers = hiddenByDepth.empty() ? 0 : static_cast<int>(hiddenByDepth.size());
        int numLayers = 2 + numHiddenLayers; 
        
        float totalWidth = (numLayers - 1) * LAYER_SPACING;
        float startX = m_bounds.position.x + (m_bounds.size.x - totalWidth) / 2.0f;
        
        int layerIndex = 0;
        
        float inputLayerX = startX;
        positionNodesVertically(inputNodes, inputLayerX, layerIndex++);
        
        for (const auto& [depth, nodesAtDepth] : hiddenByDepth) {
            float layerX = startX + layerIndex * LAYER_SPACING;
            std::vector<int> sortedNodes = nodesAtDepth;
            std::sort(sortedNodes.begin(), sortedNodes.end());
            positionNodesVertically(sortedNodes, layerX, layerIndex++);
        }
        
        float outputLayerX = startX + (numLayers - 1) * LAYER_SPACING;
        positionNodesVertically(outputNodes, outputLayerX, layerIndex++);
    }
    
    void positionNodesVertically(const std::vector<int>& nodeIds, float x, int layerIndex) {
        if (nodeIds.empty()) return;
        
        float totalHeight = (nodeIds.size() - 1) * NODE_SPACING;
        float startY = m_bounds.position.y + (m_bounds.size.y - totalHeight) / 2.0f;
        
        for (size_t i = 0; i < nodeIds.size(); ++i) {
            float y = startY + i * NODE_SPACING;
            m_nodePositions[nodeIds[i]] = sf::Vector2f(x, y);
            m_layers[nodeIds[i]] = layerIndex;
        }
    }
    
    sf::Color getValueColor(float value) const {
        value = std::clamp(value, -1.0f, 1.0f);
        
        if (value > 0) {
            float intensity = value;
            return sf::Color(
                static_cast<uint8_t>(39 + (16 - 39) * intensity),    
                static_cast<uint8_t>(39 + (185 - 39) * intensity),   
                static_cast<uint8_t>(42 + (129 - 42) * intensity)    
            );
        } else if (value < 0) {
            float intensity = -value;
            return sf::Color(
                static_cast<uint8_t>(39 + (244 - 39) * intensity),  
                static_cast<uint8_t>(39 + (63 - 39) * intensity),   
                static_cast<uint8_t>(42 + (94 - 42) * intensity)    
            );
        } else {
            return Constants::COLOR_ZINC_600;
        }
    }
    
    // Get connection color based on weight
    sf::Color getWeightColor(float weight) const {
        weight = std::clamp(weight, -5.0f, 5.0f) / 5.0f; // Normalized to [-1, 1]
        
        if (weight > 0) {
            uint8_t intensity = static_cast<uint8_t>(weight * 255);
            return sf::Color(
                static_cast<uint8_t>(16 + (52 - 16) * weight),
                static_cast<uint8_t>(150 + (211 - 150) * weight),
                static_cast<uint8_t>(105 + (153 - 105) * weight),
                static_cast<uint8_t>(120 + intensity * 0.5f)
            );
        } else if (weight < 0) {
            uint8_t intensity = static_cast<uint8_t>(-weight * 255);
            return sf::Color(
                static_cast<uint8_t>(180 + (244 - 180) * (-weight)),
                static_cast<uint8_t>(80 + (63 - 80) * (-weight)),
                static_cast<uint8_t>(80 + (94 - 80) * (-weight)),
                static_cast<uint8_t>(120 + intensity * 0.5f)
            );
        } else {
            return sf::Color(Constants::COLOR_ZINC_600.r, Constants::COLOR_ZINC_600.g, Constants::COLOR_ZINC_600.b, 80);
        }
    }
    
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        if (!m_genome) return;
        
        // Draw background container
        target.draw(m_background, states);
        
        // Title
        sf::Text title(m_font, "Neural Network", 16);
        title.setFillColor(Constants::COLOR_EMERALD_400);
        title.setStyle(sf::Text::Bold);
        title.setPosition({m_bounds.position.x + CONTAINER_PADDING, m_bounds.position.y + 8});
        target.draw(title, states);
        
        const auto& connections = m_genome->getConnections();
        for (const auto& conn : connections) {
            if (!conn.isEnabled()) continue;
            
            auto it1 = m_nodePositions.find(conn.getInNodeId());
            auto it2 = m_nodePositions.find(conn.getOutNodeId());
            
            if (it1 == m_nodePositions.end() || it2 == m_nodePositions.end()) continue;
            
            sf::Vector2f start = it1->second;
            sf::Vector2f end = it2->second;
            
            sf::Color lineColor = getWeightColor(conn.getWeight());
            float thickness = std::max(2.0f, std::min(6.0f, std::abs(conn.getWeight()) * 1.2f));
            

            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length > 0) {
                dir = dir / length;
                sf::Vector2f perp(-dir.y * thickness / 2, dir.x * thickness / 2);
                
                sf::VertexArray line(sf::PrimitiveType::TriangleStrip, 4);
                line[0].position = start + perp;
                line[1].position = start - perp;
                line[2].position = end + perp;
                line[3].position = end - perp;
                
                for (int i = 0; i < 4; ++i) {
                    line[i].color = lineColor;
                }
                
                target.draw(line, states);
            }
        }
        
        // Draw nodes
        const auto& nodes = m_genome->getNodes();
        for (const auto& [id, node] : nodes) {
            auto it = m_nodePositions.find(id);
            if (it == m_nodePositions.end()) continue;
            
            sf::Vector2f pos = it->second;
            
            sf::Color nodeColor;
            sf::Color outlineColor;
            std::string label;
            
            if (node.getLayer() == Layer::INPUT) {
                float value = 0.0f;
                int inputIndex = id;
                if (inputIndex >= 0 && inputIndex < static_cast<int>(m_inputValues.size())) {
                    value = m_inputValues[inputIndex];
                }
                nodeColor = getValueColor(value);
                outlineColor = Constants::COLOR_EMERALD_500;
                
                if (inputIndex < static_cast<int>(m_inputLabels.size())) {
                    label = m_inputLabels[inputIndex];
                }
            }
            else if (node.getLayer() == Layer::OUTPUT) {
                nodeColor = getValueColor(m_outputValue);
                outlineColor = Constants::COLOR_EMERALD_400;
                label = m_outputLabel;
            }
            else {
                nodeColor = Constants::COLOR_ZINC_700;
                outlineColor = Constants::COLOR_ZINC_500;
            }
            
            float activeIntensity = 0.0f;
            if (node.getLayer() == Layer::INPUT) {
                int inputIndex = id;
                if (inputIndex >= 0 && inputIndex < static_cast<int>(m_inputValues.size())) {
                    activeIntensity = std::abs(m_inputValues[inputIndex]);
                }
            } else if (node.getLayer() == Layer::OUTPUT) {
                activeIntensity = std::abs(m_outputValue);
            }
            
            if (activeIntensity > 0.1f) {
                sf::CircleShape glow(NODE_RADIUS + 4);
                glow.setFillColor(sf::Color(outlineColor.r, outlineColor.g, outlineColor.b, 
                                           static_cast<uint8_t>(activeIntensity * 60)));
                glow.setOrigin({NODE_RADIUS + 4, NODE_RADIUS + 4});
                glow.setPosition(pos);
                target.draw(glow, states);
            }
            
            // Draw node circle
            sf::CircleShape circle(NODE_RADIUS);
            circle.setFillColor(nodeColor);
            circle.setOutlineColor(outlineColor);
            circle.setOutlineThickness(2.5f);
            circle.setOrigin({NODE_RADIUS, NODE_RADIUS});
            circle.setPosition(pos);
            target.draw(circle, states);
            
            if (!label.empty()) {
                sf::Text labelText(m_font, label, 11);
                labelText.setFillColor(Constants::COLOR_ZINC_400);
                
                float labelX = pos.x - labelText.getLocalBounds().size.x / 2.0f;
                float labelY = pos.y + NODE_RADIUS + 6;
                labelText.setPosition({labelX, labelY});
                target.draw(labelText, states);
            }
        }
        
        // Legend duh
        drawLegend(target, states);
    }
    
    void drawLegend(sf::RenderTarget& target, sf::RenderStates states) const {
        float legendX = m_bounds.position.x + m_bounds.size.x - 130;
        float legendY = m_bounds.position.y + 8;
        
        sf::CircleShape posCircle(7);
        posCircle.setFillColor(Constants::COLOR_EMERALD_500);
        posCircle.setOutlineColor(Constants::COLOR_EMERALD_400);
        posCircle.setOutlineThickness(1.5f);
        posCircle.setPosition({legendX, legendY});
        target.draw(posCircle, states);
        
        sf::Text posText(m_font, "Positive", 11);
        posText.setFillColor(Constants::COLOR_ZINC_400);
        posText.setPosition({legendX + 18, legendY});
        target.draw(posText, states);
        
        sf::CircleShape negCircle(7);
        negCircle.setFillColor(Constants::COLOR_ROSE_500);
        negCircle.setOutlineColor(Constants::COLOR_ROSE_400);
        negCircle.setOutlineThickness(1.5f);
        negCircle.setPosition({legendX + 70, legendY});
        target.draw(negCircle, states);
        
        sf::Text negText(m_font, "Negative", 11);
        negText.setFillColor(Constants::COLOR_ZINC_400);
        negText.setPosition({legendX + 88, legendY});
        target.draw(negText, states);
    }
    
    sf::FloatRect m_bounds;
    const sf::Font& m_font;
    sf::RectangleShape m_background;
    
    const Genome* m_genome = nullptr;
    std::unordered_map<int, sf::Vector2f> m_nodePositions;
    std::unordered_map<int, int> m_layers;  
    std::unordered_map<int, int> m_nodeDepths; 
    
    std::vector<float> m_inputValues;
    float m_outputValue = 0.0f;
    
    std::vector<std::string> m_inputLabels = {"CartX", "SinA", "CosA", "AngV", "CartV"};
    std::string m_outputLabel = "Force";
};
