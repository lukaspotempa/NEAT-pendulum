#pragma once

#include <SFML/Graphics.hpp>
#include "Constants.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// SFML-based line graph renderer
class Graph : public sf::Drawable {
public:
    Graph(float x, float y, float width, float height)
        : m_position(x, y)
        , m_size(width, height)
    {
        // Background
        m_background.setSize(m_size);
        m_background.setPosition(m_position);
        m_background.setFillColor(Constants::COLOR_ZINC_800);
        m_background.setOutlineColor(Constants::COLOR_ZINC_600);
        m_background.setOutlineThickness(1.0f);
        
        m_gridLines.setPrimitiveType(sf::PrimitiveType::Lines);
        m_dataLine.setPrimitiveType(sf::PrimitiveType::LineStrip);
        m_dataLine2.setPrimitiveType(sf::PrimitiveType::LineStrip);
    }
    
    void setFont(const sf::Font& font) {
        m_font = &font;
    }
    
    void setTitle(const std::string& title) {
        m_title = title;
    }
    
    void setAxisLabels(const std::string& xLabel, const std::string& yLabel) {
        m_xLabel = xLabel;
        m_yLabel = yLabel;
    }
    
    void setLineColor(const sf::Color& color) {
        m_lineColor = color;
    }
    
    void setLine2Color(const sf::Color& color) {
        m_line2Color = color;
    }
    
    void setData(const std::vector<std::pair<int, float>>& data) {
        m_data = data;
        updateGeometry();
    }
    
    void setData2(const std::vector<std::pair<int, float>>& data) {
        m_data2 = data;
        updateGeometry();
    }
    
    void clearData() {
        m_data.clear();
        m_data2.clear();
        updateGeometry();
    }
    
    const std::vector<std::pair<int, float>>& getData() const { return m_data; }
    const std::vector<std::pair<int, float>>& getData2() const { return m_data2; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        target.draw(m_background, states);
        target.draw(m_gridLines, states);
        
        if (m_data.size() >= 2) {
            target.draw(m_dataLine, states);
        }
        if (m_data2.size() >= 2) {
            target.draw(m_dataLine2, states);
        }
        
        // Draw data points
        for (const auto& point : m_dataPoints) {
            target.draw(point, states);
        }
        
        // Draw labels
        for (const auto& label : m_labels) {
            target.draw(label, states);
        }
    }
    
    void updateGeometry() {
        m_gridLines.clear();
        m_dataLine.clear();
        m_dataLine2.clear();
        m_dataPoints.clear();
        m_labels.clear();
        
        if (!m_font) return;
        
        // Calculate margins for labels
        const float marginLeft = 50.0f;
        const float marginRight = 20.0f;
        const float marginTop = 30.0f;
        const float marginBottom = 30.0f;
        
        float plotX = m_position.x + marginLeft;
        float plotY = m_position.y + marginTop;
        float plotWidth = m_size.x - marginLeft - marginRight;
        float plotHeight = m_size.y - marginTop - marginBottom;
        
        // Find data ranges
        float minX = 0.0f, maxX = 1.0f;
        float minY = 0.0f, maxY = 1.0f;
        
        if (!m_data.empty()) {
            minX = static_cast<float>(m_data.front().first);
            maxX = static_cast<float>(m_data.back().first);
            
            for (const auto& point : m_data) {
                maxY = std::max(maxY, point.second);
            }
        }
        
        if (!m_data2.empty()) {
            for (const auto& point : m_data2) {
                maxY = std::max(maxY, point.second);
            }
        }
        
        maxY = std::ceil(maxY / 10.0f) * 10.0f;
        if (maxY < 10.0f) maxY = 10.0f;
        
        if (maxX <= minX) maxX = minX + 1.0f;
        
        // grid lines
        const int numHorizontalLines = 5;
        const int numVerticalLines = std::min(10, static_cast<int>(m_data.size()));
        
        sf::Color gridColor(Constants::COLOR_ZINC_700.r, Constants::COLOR_ZINC_700.g, 
                           Constants::COLOR_ZINC_700.b, 100);
        
        // Horizontal grid lines
        for (int i = 0; i <= numHorizontalLines; ++i) {
            float ratio = static_cast<float>(i) / numHorizontalLines;
            float y = plotY + plotHeight - (ratio * plotHeight);
            
            m_gridLines.append(sf::Vertex(sf::Vector2f(plotX, y), gridColor));
            m_gridLines.append(sf::Vertex(sf::Vector2f(plotX + plotWidth, y), gridColor));
            
            // y axis label
            float value = minY + ratio * (maxY - minY);
            sf::Text label(*m_font, std::to_string(static_cast<int>(value)), 11);
            label.setFillColor(Constants::COLOR_ZINC_500);
            label.setPosition({plotX - 35.0f, y - 7.0f});
            m_labels.push_back(label);
        }
        
        // Vertical grid lines
        if (numVerticalLines > 0 && !m_data.empty()) {
            int step = std::max(1, static_cast<int>(m_data.size()) / numVerticalLines);
            for (size_t i = 0; i < m_data.size(); i += step) {
                float ratio = (m_data[i].first - minX) / (maxX - minX);
                float x = plotX + ratio * plotWidth;
                
                m_gridLines.append(sf::Vertex(sf::Vector2f(x, plotY), gridColor));
                m_gridLines.append(sf::Vertex(sf::Vector2f(x, plotY + plotHeight), gridColor));
                
                // x axis label
                sf::Text label(*m_font, std::to_string(m_data[i].first), 11);
                label.setFillColor(Constants::COLOR_ZINC_500);
                label.setPosition({x - 8.0f, plotY + plotHeight + 5.0f});
                m_labels.push_back(label);
            }
        }
        
        // Title
        sf::Text title(*m_font, m_title, 14);
        title.setFillColor(Constants::COLOR_ZINC_300);
        title.setStyle(sf::Text::Bold);
        title.setPosition({m_position.x + m_size.x / 2.0f - title.getLocalBounds().size.x / 2.0f, m_position.y + 5.0f});
        m_labels.push_back(title);
        
        // Data line
        for (const auto& point : m_data) {
            float xRatio = (static_cast<float>(point.first) - minX) / (maxX - minX);
            float yRatio = (point.second - minY) / (maxY - minY);
            
            float x = plotX + xRatio * plotWidth;
            float y = plotY + plotHeight - (yRatio * plotHeight);
            
            m_dataLine.append(sf::Vertex(sf::Vector2f(x, y), m_lineColor));
            
            // Point marker
            sf::CircleShape marker(3.0f);
            marker.setFillColor(m_lineColor);
            marker.setPosition({x - 3.0f, y - 3.0f});
            m_dataPoints.push_back(marker);
        }
        
        // Secondary data line
        for (const auto& point : m_data2) {
            float xRatio = (static_cast<float>(point.first) - minX) / (maxX - minX);
            float yRatio = (point.second - minY) / (maxY - minY);
            
            float x = plotX + xRatio * plotWidth;
            float y = plotY + plotHeight - (yRatio * plotHeight);
            
            m_dataLine2.append(sf::Vertex(sf::Vector2f(x, y), m_line2Color));
        }
    }
    
    sf::Vector2f m_position;
    sf::Vector2f m_size;
    sf::RectangleShape m_background;
    
    const sf::Font* m_font = nullptr;
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    
    sf::Color m_lineColor = Constants::COLOR_EMERALD_400;    
    sf::Color m_line2Color{100, 150, 200};                   
    
    std::vector<std::pair<int, float>> m_data;
    std::vector<std::pair<int, float>> m_data2;
    
    sf::VertexArray m_gridLines;
    sf::VertexArray m_dataLine;
    sf::VertexArray m_dataLine2;
    mutable std::vector<sf::CircleShape> m_dataPoints;
    mutable std::vector<sf::Text> m_labels;
};
