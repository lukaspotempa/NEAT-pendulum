#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>

// Helper class to draw rounded rectangles
class RoundedRectangle : public sf::Drawable {
public:
    RoundedRectangle(float x, float y, float width, float height, float radius, 
                     sf::Color fillColor, sf::Color outlineColor = sf::Color::Transparent, 
                     float outlineThickness = 0.0f)
        : m_position(x, y), m_size(width, height), m_radius(radius), 
          m_fillColor(fillColor), m_outlineColor(outlineColor), m_outlineThickness(outlineThickness)
    {
        updateGeometry();
    }
    
    void setPosition(float x, float y) { m_position = {x, y}; updateGeometry(); }
    void setSize(float width, float height) { m_size = {width, height}; updateGeometry(); }
    void setFillColor(sf::Color color) { m_fillColor = color; updateGeometry(); }
    void setOutlineColor(sf::Color color) { m_outlineColor = color; updateGeometry(); }
    void setOutlineThickness(float thickness) { m_outlineThickness = thickness; updateGeometry(); }
    
    sf::Vector2f getPosition() const { return m_position; }
    sf::Vector2f getSize() const { return m_size; }
    
private:
    void updateGeometry() {
        m_vertices.clear();
        m_vertices.setPrimitiveType(sf::PrimitiveType::TriangleFan);
        
        const int quality = 8; // segments per corner
        const float pi = 3.14159265f;
        float r = std::min(m_radius, std::min(m_size.x / 2.0f, m_size.y / 2.0f));
        
        // Center vertex
        sf::Vector2f center = m_position + m_size / 2.0f;
        m_vertices.append(sf::Vertex(center, m_fillColor));
        
        // Top-left corner
        for (int i = 0; i <= quality; ++i) {
            float angle = pi + (pi / 2.0f) * (static_cast<float>(i) / quality);
            sf::Vector2f pos = m_position + sf::Vector2f(r, r) + sf::Vector2f(std::cos(angle), std::sin(angle)) * r;
            m_vertices.append(sf::Vertex(pos, m_fillColor));
        }
        
        // Top-right corner
        for (int i = 0; i <= quality; ++i) {
            float angle = 1.5f * pi + (pi / 2.0f) * (static_cast<float>(i) / quality);
            sf::Vector2f pos = m_position + sf::Vector2f(m_size.x - r, r) + sf::Vector2f(std::cos(angle), std::sin(angle)) * r;
            m_vertices.append(sf::Vertex(pos, m_fillColor));
        }
        
        // Bottom-right corner
        for (int i = 0; i <= quality; ++i) {
            float angle = 0.0f + (pi / 2.0f) * (static_cast<float>(i) / quality);
            sf::Vector2f pos = m_position + sf::Vector2f(m_size.x - r, m_size.y - r) + sf::Vector2f(std::cos(angle), std::sin(angle)) * r;
            m_vertices.append(sf::Vertex(pos, m_fillColor));
        }
        
        // Bottom-left corner
        for (int i = 0; i <= quality; ++i) {
            float angle = 0.5f * pi + (pi / 2.0f) * (static_cast<float>(i) / quality);
            sf::Vector2f pos = m_position + sf::Vector2f(r, m_size.y - r) + sf::Vector2f(std::cos(angle), std::sin(angle)) * r;
            m_vertices.append(sf::Vertex(pos, m_fillColor));
        }
        
        // Close the shape
        m_vertices.append(m_vertices[1]);
        
        // Build outline
        if (m_outlineThickness > 0.0f) {
            m_outline.clear();
            m_outline.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
            
            float outerR = r + m_outlineThickness;
            
            auto addCorner = [&](sf::Vector2f cornerCenter, float startAngle) {
                for (int i = 0; i <= quality; ++i) {
                    float angle = startAngle + (pi / 2.0f) * (static_cast<float>(i) / quality);
                    sf::Vector2f inner = cornerCenter + sf::Vector2f(std::cos(angle), std::sin(angle)) * r;
                    sf::Vector2f outer = cornerCenter + sf::Vector2f(std::cos(angle), std::sin(angle)) * outerR;
                    m_outline.append(sf::Vertex(inner, m_outlineColor));
                    m_outline.append(sf::Vertex(outer, m_outlineColor));
                }
            };
            
            addCorner(m_position + sf::Vector2f(r, r), pi);
            addCorner(m_position + sf::Vector2f(m_size.x - r, r), 1.5f * pi);
            addCorner(m_position + sf::Vector2f(m_size.x - r, m_size.y - r), 0.0f);
            addCorner(m_position + sf::Vector2f(r, m_size.y - r), 0.5f * pi);
            
            sf::Vector2f inner = m_position + sf::Vector2f(0, r);
            sf::Vector2f outer = m_position + sf::Vector2f(-m_outlineThickness, r);
            m_outline.append(sf::Vertex(inner, m_outlineColor));
            m_outline.append(sf::Vertex(outer, m_outlineColor));
        }
    }
    
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        target.draw(m_vertices, states);
        if (m_outlineThickness > 0.0f) {
            target.draw(m_outline, states);
        }
    }
    
    sf::Vector2f m_position;
    sf::Vector2f m_size;
    float m_radius;
    sf::Color m_fillColor;
    sf::Color m_outlineColor;
    float m_outlineThickness;
    sf::VertexArray m_vertices;
    sf::VertexArray m_outline;
};
