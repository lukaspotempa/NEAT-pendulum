#pragma once

#include <SFML/Graphics.hpp>
#include "IPendulum.hpp"
#include "Trail.hpp"
#include "Constants.hpp"

class DoublePendulum : public IPendulum {
public:
    DoublePendulum();
    
    void update(float dt, float xDDot, sf::Vector2f pivot) override;
    void reset() override;
    
    sf::Vector2f getEndBobPosition() const override { return getBob2Pos(); }
    
    void setTrailEnabled(bool enabled) override { trail.setEnabled(enabled); }
    bool isTrailEnabled() const override { return trail.isEnabled(); }
    
    // Getters
    float getTheta1() const { return theta1; }
    float getTheta2() const { return theta2; }
    float getTheta1Dot() const { return theta1Dot; }
    float getTheta2Dot() const { return theta2Dot; }
    float getMass1() const { return m1; }
    float getMass2() const { return m2; }
    float getLength1() const { return L1; }
    float getLength2() const { return L2; }
    
    sf::Vector2f getBob1Pos() const;
    sf::Vector2f getBob2Pos() const;
    
protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
private:
    // First pendulum
    float theta1 = 3.0f;
    float theta1Dot = 0.f;
    float L1 = 150.f;
    float m1 = 3.f;
    float bobRadius1 = 22.f;
    
    // Second pendulum
    float theta2 = 3.0f;
    float theta2Dot = 0.f;
    float L2 = 125.f;
    float m2 = 3.f;
    float bobRadius2 = 18.f;
    
    float hingeRadius = 6.f;
    
    // Initial values for reset
    float initialTheta1 = 3.0f;
    float initialTheta2 = 3.0f;
    float initialTheta1Dot = 0.f;
    float initialTheta2Dot = 0.f;
    
    sf::Vector2f pivotPos;
    mutable sf::CircleShape bob1;
    mutable sf::CircleShape bob2;
    mutable sf::CircleShape hinge;
    mutable sf::RectangleShape lineShape;
    
    Trail trail;
};
