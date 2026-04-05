#include "DoublePendulum.hpp"
#include "Physics.hpp"
#include <cmath>

DoublePendulum::DoublePendulum() : trail(sf::Color(100, 200, 255)) {
    bob1.setRadius(bobRadius1);
    bob1.setOrigin({ bobRadius1, bobRadius1 });
    bob1.setFillColor(Constants::COLOR_BOB);
    bob1.setOutlineThickness(2.f);
    bob1.setOutlineColor(Constants::COLOR_BOB_OUTLINE);

    bob2.setRadius(bobRadius2);
    bob2.setOrigin({ bobRadius2, bobRadius2 });
    bob2.setFillColor(Constants::COLOR_BOB);
    bob2.setOutlineThickness(2.f);
    bob2.setOutlineColor(Constants::COLOR_BOB_OUTLINE);

    hinge.setRadius(hingeRadius);
    hinge.setOrigin({ hingeRadius, hingeRadius });
    hinge.setFillColor(Constants::COLOR_HINGE);
    hinge.setOutlineThickness(1.f);
    hinge.setOutlineColor(Constants::COLOR_HINGE_OUTLINE);
}

void DoublePendulum::update(float dt, float xDDot, sf::Vector2f pivot) {
    pivotPos = pivot;
    
    Physics::DoublePendulumParams params = Physics::doublePendulumManualParams();
    params.L1 = L1;
    params.L2 = L2;
    params.m1 = m1;
    params.m2 = m2;
    
    Physics::doublePendulumRK4Step(theta1, theta2, theta1Dot, theta2Dot, dt, xDDot, params);
    
    // Update trail
    trail.addPoint(getBob2Pos());
    trail.update(dt);
}

void DoublePendulum::reset() {
    theta1 = initialTheta1;
    theta2 = initialTheta2;
    theta1Dot = initialTheta1Dot;
    theta2Dot = initialTheta2Dot;
    trail.clear();
}

sf::Vector2f DoublePendulum::getBob1Pos() const {
    return {
        pivotPos.x + L1 * std::sin(theta1),
        pivotPos.y + L1 * std::cos(theta1)
    };
}

sf::Vector2f DoublePendulum::getBob2Pos() const {
    sf::Vector2f bob1Pos = getBob1Pos();
    return {
        bob1Pos.x + L2 * std::sin(theta2),
        bob1Pos.y + L2 * std::cos(theta2)
    };
}

void DoublePendulum::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Draw trail
    target.draw(trail, states);
    
    sf::Vector2f bob1Pos = getBob1Pos();
    sf::Vector2f bob2Pos = getBob2Pos();

    auto drawLine = [&](sf::Vector2f start, sf::Vector2f end, float width, sf::Color fill, sf::Color outline) {
        sf::Vector2f diff = { end.x - start.x, end.y - start.y };
        float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        lineShape.setSize({ length, width });
        lineShape.setOrigin({ 0.f, width / 2.f });
        lineShape.setPosition(start);
        lineShape.setRotation(sf::radians(std::atan2(diff.y, diff.x)));

        lineShape.setFillColor(fill);
        lineShape.setOutlineThickness(1.5f);
        lineShape.setOutlineColor(outline);

        target.draw(lineShape, states);
    };

    // Draw rods
    drawLine(pivotPos, bob1Pos, 8.f, Constants::COLOR_ROD, Constants::COLOR_ROD_OUTLINE);
    drawLine(bob1Pos, bob2Pos, 6.f, Constants::COLOR_ROD, Constants::COLOR_ROD_OUTLINE);

    // Draw bobs
    bob1.setPosition(bob1Pos);
    target.draw(bob1, states);

    bob2.setPosition(bob2Pos);
    target.draw(bob2, states);

    // Draw hinge
    hinge.setPosition(pivotPos);
    target.draw(hinge, states);
}
