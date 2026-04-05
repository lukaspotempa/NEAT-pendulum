#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>

const sf::Color COLOR_GREY(65, 65, 65, 255);

const sf::Vector2u windowSize(1580, 760);
const float g = 500.f;          
float damping = 0.95f;         
float sensitivity = 800.00f;
const float centerLineWidth = 200.f;


class Cart : public sf::Drawable, public sf::Transformable {
public:
    Cart() {
        cartSize = { 120.f, 30.f };
        cartRect.setSize(cartSize);
        cartRect.setFillColor(sf::Color(41, 128, 185));
        cartRect.setOutlineColor(sf::Color(31, 97, 141));
        cartRect.setOutlineThickness(3.f);

        sf::Vector2f cartPos = {
            ((float)windowSize.x - cartSize.x) / 2,
            ((float)windowSize.y - cartSize.y) / 2
        };
        setPosition(cartPos);
    }

    float update(float dt, float xDDot) {
        if (dt <= 0.f) return xDDot;

        sf::Vector2f pos = getPosition();
        float leftBound = centerLineWidth / 2.f - 5.f;
        float rightBound = windowSize.x - cartSize.x - centerLineWidth / 2.f;

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

    float getMass() const { return mass; }
    float getVelocity() const { return velocity; }

    sf::Vector2f getPivot() const {
        sf::Vector2f pos = getPosition();
        return { pos.x + cartSize.x / 2.f, pos.y + cartSize.y / 2.f };
    }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        states.transform *= getTransform();
        target.draw(cartRect, states);
    }

private:
    sf::RectangleShape cartRect;
    sf::Vector2f cartSize;
    float velocity = 0.f;
    float mass = 5.f;
};

class DoublePendulum : public sf::Drawable {
public:
    DoublePendulum() {
        bob1.setRadius(bobRadius1);
        bob1.setOrigin({ bobRadius1, bobRadius1 });
        bob1.setFillColor(sf::Color::White);
        bob1.setOutlineThickness(2.f);
        bob1.setOutlineColor(sf::Color(185, 185, 185));

        bob2.setRadius(bobRadius2);
        bob2.setOrigin({ bobRadius2, bobRadius2 });
        bob2.setFillColor(sf::Color::White);
        bob2.setOutlineThickness(2.f);
        bob2.setOutlineColor(sf::Color(185, 185, 185));

        hinge.setRadius(hingeRadius);
        hinge.setOrigin({ hingeRadius, hingeRadius });
        hinge.setFillColor(sf::Color(149, 165, 166));
        hinge.setOutlineThickness(1.f);
        hinge.setOutlineColor(sf::Color(127, 140, 141)); 
    }

    void computeAccelerations(float t1, float t2, float w1, float w2, float aCart,
                              float& alpha1, float& alpha2) {
        float dTheta = t1 - t2;
        float s1 = std::sin(t1);
        float s2 = std::sin(t2);
        float sd = std::sin(dTheta);
        float cd = std::cos(dTheta);
        
        float M = m1 + m2;
        
        float denom = L1 * (M - m2 * cd * cd);

        // Negative sign on aCart terms for correct inertial effect
        alpha1 = (-m2 * cd * (L1 * w1 * w1 * sd - g * s2)
                  - m2 * L2 * w2 * w2 * sd
                  - M * g * s1
                  - M * aCart * std::cos(t1)) / denom;
        
        float denom2 = L2 * (M - m2 * cd * cd);
        alpha2 = (m2 * cd * (L2 * w2 * w2 * sd + g * s1)
                  + M * L1 * w1 * w1 * sd
                  - M * g * s2
                  - m2 * aCart * std::cos(t2) * cd
                  + M * aCart * std::cos(t1) * cd
                  - M * aCart * std::cos(t2)) / denom2;
    }

    void update(float dt, float xDDot, sf::Vector2f pivot) {
        pivotPos = pivot;
        
        float aCart = xDDot * Constants::CART_PENDULUM_COUPLING;
        
        float k1_t1, k1_t2, k1_w1, k1_w2;
        float k2_t1, k2_t2, k2_w1, k2_w2;
        float k3_t1, k3_t2, k3_w1, k3_w2;
        float k4_t1, k4_t2, k4_w1, k4_w2;
        float a1, a2;
        
        // k1
        k1_t1 = theta1Dot;
        k1_t2 = theta2Dot;
        computeAccelerations(theta1, theta2, theta1Dot, theta2Dot, aCart, a1, a2);
        k1_w1 = a1;
        k1_w2 = a2;
        
        // k2
        k2_t1 = theta1Dot + 0.5f * dt * k1_w1;
        k2_t2 = theta2Dot + 0.5f * dt * k1_w2;
        computeAccelerations(theta1 + 0.5f * dt * k1_t1, theta2 + 0.5f * dt * k1_t2,
                            k2_t1, k2_t2, aCart, a1, a2);
        k2_w1 = a1;
        k2_w2 = a2;
        
        // k3
        k3_t1 = theta1Dot + 0.5f * dt * k2_w1;
        k3_t2 = theta2Dot + 0.5f * dt * k2_w2;
        computeAccelerations(theta1 + 0.5f * dt * k2_t1, theta2 + 0.5f * dt * k2_t2,
                            k3_t1, k3_t2, aCart, a1, a2);
        k3_w1 = a1;
        k3_w2 = a2;
        
        // k4
        k4_t1 = theta1Dot + dt * k3_w1;
        k4_t2 = theta2Dot + dt * k3_w2;
        computeAccelerations(theta1 + dt * k3_t1, theta2 + dt * k3_t2,
                            k4_t1, k4_t2, aCart, a1, a2);
        k4_w1 = a1;
        k4_w2 = a2;
        
        // Update state
        theta1 += (dt / 6.0f) * (k1_t1 + 2.0f * k2_t1 + 2.0f * k3_t1 + k4_t1);
        theta2 += (dt / 6.0f) * (k1_t2 + 2.0f * k2_t2 + 2.0f * k3_t2 + k4_t2);
        theta1Dot += (dt / 6.0f) * (k1_w1 + 2.0f * k2_w1 + 2.0f * k3_w1 + k4_w1);
        theta2Dot += (dt / 6.0f) * (k1_w2 + 2.0f * k2_w2 + 2.0f * k3_w2 + k4_w2);
        
        const float pDamp = 0.998f;
        theta1Dot *= pDamp;
        theta2Dot *= pDamp;
    }

    float getTheta1() const { return theta1; }
    float getTheta2() const { return theta2; }
    float getTheta1Dot() const { return theta1Dot; }
    float getTheta2Dot() const { return theta2Dot; }
    float getMass1() const { return m1; }
    float getMass2() const { return m2; }
    float getLength1() const { return L1; }
    float getLength2() const { return L2; }
    
    sf::Vector2f getBob1Pos() const {
        return {
            pivotPos.x + L1 * std::sin(theta1),
            pivotPos.y + L1 * std::cos(theta1)
        };
    }
    
    sf::Vector2f getBob2Pos() const {
        sf::Vector2f bob1Pos = getBob1Pos();
        return {
            bob1Pos.x + L2 * std::sin(theta2),
            bob1Pos.y + L2 * std::cos(theta2)
        };
    }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
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

        sf::Color rodFill(189, 195, 199);
        sf::Color rodOutline(127, 140, 141);

        drawLine(pivotPos, bob1Pos, 8.f, rodFill, rodOutline);
        drawLine(bob1Pos, bob2Pos, 6.f, rodFill, rodOutline);

        bob1.setPosition(bob1Pos);
        target.draw(bob1, states);

        bob2.setPosition(bob2Pos);
        target.draw(bob2, states);

        hinge.setPosition(pivotPos);
        target.draw(hinge, states);
    }

private:
    // first pendulum
    float theta1 = 3.0f; 
    float theta1Dot = 0.f;
    float L1 = 150.f;     
    float m1 = 3.f;      
    float bobRadius1 = 22.f;
    // second pendulum
    float theta2 = 3.0f;  
    float theta2Dot = 0.f;
    float L2 = 125.f;      
    float m2 = 3.f;      
    float bobRadius2 = 18.f;
    float hingeRadius = 6.f;

    sf::Vector2f pivotPos;
    mutable sf::CircleShape bob1;
    mutable sf::CircleShape bob2;
    mutable sf::CircleShape hinge;
    mutable sf::RectangleShape lineShape;
};

int main() {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(
        sf::VideoMode(windowSize),
        "Double Inverted Pendulum",
        sf::State::Windowed,
        settings
    );
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);

    sf::Vector2i windowCenter(windowSize.x / 2, windowSize.y / 2);
    sf::Mouse::setPosition(windowCenter, window);

    
    sf::RectangleShape centerLine({ windowSize.x - centerLineWidth, 8.f });
    centerLine.setFillColor(sf::Color(149, 165, 166));
    centerLine.setOutlineThickness(2.f);
    centerLine.setOutlineColor(sf::Color(127, 140, 141));
    sf::FloatRect bounds = centerLine.getLocalBounds();
    centerLine.setOrigin({ bounds.size.x / 2, bounds.size.y / 2});
    centerLine.setPosition({(float)windowCenter.x, (float)windowCenter.y});


    Cart cart;
    DoublePendulum pendulum;
    sf::Clock clock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
     
        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.05f); 

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float delta = static_cast<float>(mousePos.x - windowCenter.x);
        sf::Mouse::setPosition(windowCenter, window);

        float F = delta * sensitivity;
        float M = cart.getMass();

        float xDDot = F / M;

        float effectiveXDDot = cart.update(dt, xDDot);
        pendulum.update(dt, effectiveXDDot, cart.getPivot());

        window.clear(sf::Color(30, 39, 46));
        window.draw(centerLine);
        window.draw(cart);
        window.draw(pendulum);   
        window.display();
    }

    return 0;
}