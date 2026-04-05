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
};
