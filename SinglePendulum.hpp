#pragma once

#include <SFML/Graphics.hpp>
#include "IPendulum.hpp"
#include "Trail.hpp"
#include "Constants.hpp"

class SinglePendulum : public IPendulum {
public:
    SinglePendulum();
    
    void update(float dt, float xDDot, sf::Vector2f pivot) override;
    void reset() override;
    
    sf::Vector2f getEndBobPosition() const override { return getBobPos(); }
    
    void setTrailEnabled(bool enabled) override { trail.setEnabled(enabled); }
    bool isTrailEnabled() const override { return trail.isEnabled(); }
    
    // Getters
    float getTheta() const { return theta; }
    float getThetaDot() const { return thetaDot; }
    float getMass() const { return mass; }
    float getLength() const { return length; }
    sf::Vector2f getBobPos() const;
    
    // Set initial angle for reset (radians, 0 = hanging down, PI = upright)
    void setInitialTheta(float angle) { initialTheta = angle; }
    
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
private:
    void computeAcceleration(float t, float w, float aCart, float& alpha);
    
    // Pendulum state
    float theta = 3.14159265f + 0.1f;      // Start near upright for NEAT training (PI + small offset)
    float thetaDot = 0.f;
    float length = 200.f;
    float mass = 3.f;
    float bobRadius = 25.f;
    float hingeRadius = 6.f;
    
    // Initial values for reset
    float initialTheta = 3.14159265f + 0.1f;   // Near upright (PI = upright, 0 = hanging down)
    float initialThetaDot = 0.f;
    
    sf::Vector2f pivotPos;
    mutable sf::CircleShape bob;
    mutable sf::CircleShape hinge;
    mutable sf::RectangleShape lineShape;
    
    Trail trail;
};
