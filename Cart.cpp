#include "Cart.hpp"
#include <algorithm>

Cart::Cart() {
    cartSize = { 120.f, 30.f };
    cartRect.setSize(cartSize);
    cartRect.setFillColor(Constants::COLOR_CART);
    cartRect.setOutlineColor(Constants::COLOR_CART_OUTLINE);
    cartRect.setOutlineThickness(3.f);

    // Position cart at center
    sf::Vector2f cartPos = {
        m_simCenterX - cartSize.x / 2.0f,
        m_simCenterY - cartSize.y / 2.0f
    };
    setPosition(cartPos);
}

float Cart::update(float dt, float xDDot) {
    if (dt <= 0.f) return xDDot;

    sf::Vector2f pos = getPosition();
    
    float leftBound = m_simCenterX - m_simTrackWidth / 2.0f;
    float rightBound = m_simCenterX + m_simTrackWidth / 2.0f - cartSize.x;

    if (pos.x <= leftBound && xDDot < 0.f) {
        xDDot = 0.f;
    } else if (pos.x >= rightBound && xDDot > 0.f) {
        xDDot = 0.f;
    }

    velocity += xDDot * dt;
    velocity *= damping;

    float newX = pos.x + velocity * dt;
    float clampedX = std::clamp(newX, leftBound, rightBound);

    if (newX != clampedX) {
        velocity = 0.f;
    }

    setPosition({ clampedX, pos.y });

    return xDDot;
}

void Cart::reset() {
    velocity = 0.f;
    // Reset center position
    sf::Vector2f cartPos = {
        m_simCenterX - cartSize.x / 2.0f,
        m_simCenterY - cartSize.y / 2.0f
    };
    setPosition(cartPos);
}

sf::Vector2f Cart::getPivot() const {
    sf::Vector2f pos = getPosition();
    return { pos.x + cartSize.x / 2.f, pos.y + cartSize.y / 2.f };
}

void Cart::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    
    // Apply ghost rendering 
    if (m_renderMode == RenderMode::Ghost) {
        sf::RectangleShape ghostRect = cartRect;
        sf::Color ghostColor(100, 100, 100, GHOST_ALPHA);
        sf::Color ghostOutline(70, 70, 70, GHOST_ALPHA);
        ghostRect.setFillColor(ghostColor);
        ghostRect.setOutlineColor(ghostOutline);
        target.draw(ghostRect, states);
    } else {
        target.draw(cartRect, states);
    }
}
