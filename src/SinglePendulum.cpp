#include "SinglePendulum.hpp"
#include "Physics.hpp"
#include <cmath>

SinglePendulum::SinglePendulum() : trail(sf::Color(255, 200, 100)) {
    bob.setRadius(bobRadius);
    bob.setOrigin({ bobRadius, bobRadius });
    bob.setFillColor(Constants::COLOR_BOB);
    bob.setOutlineThickness(2.f);
    bob.setOutlineColor(Constants::COLOR_BOB_OUTLINE);

    hinge.setRadius(hingeRadius);
    hinge.setOrigin({ hingeRadius, hingeRadius });
    hinge.setFillColor(Constants::COLOR_HINGE);
    hinge.setOutlineThickness(1.f);
    hinge.setOutlineColor(Constants::COLOR_HINGE_OUTLINE);
}

void SinglePendulum::update(float dt, float xDDot, sf::Vector2f pivot) {
    pivotPos = pivot;
    
    Physics::PendulumParams params = Physics::manualParams();
    params.pendulumLength = length;
    params.pendulumDamping = m_damping;
    
    Physics::singlePendulumRK4Step(theta, thetaDot, dt, xDDot, params);
    
    // Update trail
    trail.addPoint(getBobPos());
    trail.update(dt);
}

void SinglePendulum::reset() {
    theta = initialTheta;
    thetaDot = initialThetaDot;
    trail.clear();
}

sf::Vector2f SinglePendulum::getBobPos() const {
    return {
        pivotPos.x + length * std::sin(theta),
        pivotPos.y + length * std::cos(theta)
    };
}

void SinglePendulum::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Dont draw trail in ghost mode
    if (m_renderMode == RenderMode::Solid) {
        target.draw(trail, states);
    }
    
    sf::Vector2f bobPos = getBobPos();

    sf::Color rodFill, rodOutline, bobFill, bobOutline, hingeFill, hingeOutline;
    
    if (m_renderMode == RenderMode::Ghost) {
        rodFill = sf::Color(120, 120, 120, GHOST_ALPHA);
        rodOutline = sf::Color(90, 90, 90, GHOST_ALPHA);
        bobFill = sf::Color(150, 150, 150, GHOST_ALPHA);
        bobOutline = sf::Color(110, 110, 110, GHOST_ALPHA);
        hingeFill = sf::Color(100, 100, 100, GHOST_ALPHA);
        hingeOutline = sf::Color(80, 80, 80, GHOST_ALPHA);
    } else {
        rodFill = Constants::COLOR_ROD;
        rodFill.a = m_alpha;
        rodOutline = Constants::COLOR_ROD_OUTLINE;
        rodOutline.a = m_alpha;
        bobFill = Constants::COLOR_BOB;
        bobFill.a = m_alpha;
        bobOutline = Constants::COLOR_BOB_OUTLINE;
        bobOutline.a = m_alpha;
        hingeFill = Constants::COLOR_HINGE;
        hingeFill.a = m_alpha;
        hingeOutline = Constants::COLOR_HINGE_OUTLINE;
        hingeOutline.a = m_alpha;
    }

    auto drawLine = [&](sf::Vector2f start, sf::Vector2f end, float width, sf::Color fill, sf::Color outline) {
        sf::Vector2f diff = { end.x - start.x, end.y - start.y };
        float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        lineShape.setSize({ len, width });
        lineShape.setOrigin({ 0.f, width / 2.f });
        lineShape.setPosition(start);
        lineShape.setRotation(sf::radians(std::atan2(diff.y, diff.x)));

        lineShape.setFillColor(fill);
        lineShape.setOutlineThickness(1.5f);
        lineShape.setOutlineColor(outline);

        target.draw(lineShape, states);
    };

    // Draw rod
    drawLine(pivotPos, bobPos, 8.f, rodFill, rodOutline);

    // Draw bob
    bob.setPosition(bobPos);
    bob.setFillColor(bobFill);
    bob.setOutlineColor(bobOutline);
    target.draw(bob, states);

    // Draw hinge
    hinge.setPosition(pivotPos);
    hinge.setFillColor(hingeFill);
    hinge.setOutlineColor(hingeOutline);
    target.draw(hinge, states);
}
