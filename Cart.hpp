#pragma once

#include <SFML/Graphics.hpp>
#include "Constants.hpp"

// Ghosts are paralel agents
enum class RenderMode {
    Solid,  
    Ghost 
};

class Cart : public sf::Drawable, public sf::Transformable {
public:
    Cart();
    
    float update(float dt, float xDDot);
    void reset();
    
    float getMass() const { return mass; }
    float getVelocity() const { return velocity; }
    sf::Vector2f getPivot() const;
    sf::Vector2f getSize() const { return cartSize; }
    
    void setSimulationBounds(float centerX, float centerY, float trackWidth) {
        m_simCenterX = centerX;
        m_simCenterY = centerY;
        m_simTrackWidth = trackWidth;
    }
    
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
private:
    sf::RectangleShape cartRect;
    sf::Vector2f cartSize;
    float velocity = 0.f;
    float mass = 4.f;
    float damping = Constants::DEFAULT_DAMPING;
    RenderMode m_renderMode = RenderMode::Solid;
    
    // Simulation bounds
    float m_simCenterX = Constants::WINDOW_WIDTH / 2.0f;
    float m_simCenterY = Constants::WINDOW_HEIGHT * 0.35f;
    float m_simTrackWidth = 1000.0f;
    
    static constexpr uint8_t GHOST_ALPHA = 60;
};
