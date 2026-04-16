#pragma once

#include <SFML/Graphics.hpp>

class IPendulum : public sf::Drawable {
public:
    virtual ~IPendulum() = default;
    
    virtual void update(float dt, float xDDot, sf::Vector2f pivot) = 0;
    virtual void reset() = 0;
    
    virtual sf::Vector2f getEndBobPosition() const = 0;
    
    virtual void setTrailEnabled(bool enabled) = 0;
    virtual bool isTrailEnabled() const = 0;
    
    // New parameters for Demo
    virtual void setDamping(float damping) = 0;
    virtual float getDamping() const = 0;
    
    virtual void setAlpha(uint8_t alpha) = 0;
    virtual uint8_t getAlpha() const = 0;
};
