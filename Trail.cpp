#include "Trail.hpp"

Trail::Trail(sf::Color baseColor) : m_baseColor(baseColor) {
}

void Trail::addPoint(sf::Vector2f position) {
    if (!m_enabled) return;
    
    m_points.push_back({ position, 255.f });
    
    while (m_points.size() > m_maxPoints) {
        m_points.pop_front();
    }
}

void Trail::update(float dt) {
    if (!m_enabled) return;
    
    // Fade all points
    for (auto& point : m_points) {
        point.alpha -= m_fadeRate * dt * 60.f; 
    }
    
    // Remove fully faded points
    while (!m_points.empty() && m_points.front().alpha <= 0.f) {
        m_points.pop_front();
    }
}

void Trail::clear() {
    m_points.clear();
}

void Trail::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_enabled || m_points.size() < 2) return;
    
    for (size_t i = 1; i < m_points.size(); ++i) {
        const auto& p1 = m_points[i - 1];
        const auto& p2 = m_points[i];
        
        float avgAlpha = (p1.alpha + p2.alpha) / 2.f;
        if (avgAlpha <= 0.f) continue;
        
        sf::Color color = m_baseColor;
        color.a = static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, avgAlpha)));
        
        sf::Vector2f diff = p2.position - p1.position;
        float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        if (length < 0.1f) continue;
        
        float thickness = 2.f + (avgAlpha / 255.f) * 2.f;
        
        sf::RectangleShape line({ length, thickness });
        line.setOrigin({ 0.f, thickness / 2.f });
        line.setPosition(p1.position);
        line.setRotation(sf::radians(std::atan2(diff.y, diff.x)));
        line.setFillColor(color);
        
        target.draw(line, states);
    }
}
