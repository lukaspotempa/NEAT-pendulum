#pragma once

#include <SFML/Graphics.hpp>
#include "IPendulum.hpp"
#include "Trail.hpp"
#include "Constants.hpp"
#include "Cart.hpp" 

class SinglePendulum : public IPendulum {
public:
    SinglePendulum();
    
    void update(float dt, float xDDot, sf::Vector2f pivot) override;
    void reset() override;
    
    sf::Vector2f getEndBobPosition() const override { return getBobPos(); }
    
    void setTrailEnabled(bool enabled) override { trail.setEnabled(enabled); }
    bool isTrailEnabled() const override { return trail.isEnabled(); }
    
    float getTheta() const { return theta; }
    float getThetaDot() const { return thetaDot; }
    float getMass() const { return mass; } 
    float getLength() const { return length; }
    sf::Vector2f getBobPos() const;
    
    void setInitialTheta(float angle) { initialTheta = angle; }
    
    void setTheta(float angle) { theta = angle; }
    void setThetaDot(float vel) { thetaDot = vel; }
    void setPivot(sf::Vector2f pivot) { pivotPos = pivot; }
    
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    
    void setDamping(float damping) override { m_damping = damping; }
    float getDamping() const override { return m_damping; }
    
    void setAlpha(uint8_t alpha) override { m_alpha = alpha; }
    uint8_t getAlpha() const override { return m_alpha; }
    
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
private:

    float theta = 0.f;      
    float thetaDot = 0.f;
    float length = 200.f;
    float mass = 2.f;
    float bobRadius = 25.f;
    float hingeRadius = 6.f;
    
    // Initial values for reset
    float initialTheta = 0.f;   
    float initialThetaDot = 0.f;
    
    sf::Vector2f pivotPos;
    mutable sf::CircleShape bob;
    mutable sf::CircleShape hinge;
    mutable sf::RectangleShape lineShape;
    
    Trail trail;
    RenderMode m_renderMode = RenderMode::Solid;
    
    float m_damping = Constants::MANUAL_PENDULUM_DAMPING;
    uint8_t m_alpha = 255;
    static constexpr uint8_t GHOST_ALPHA = 60;
};
