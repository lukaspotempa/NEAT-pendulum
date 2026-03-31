#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>

const sf::Vector2u windowSize(1580, 760);
const float g = 500.f;          
float damping = 0.95f;         
float sensitivity = 800.00f;

std::string cartImg = "assets/cart-wheel.png";


class Cart : public sf::Drawable, public sf::Transformable {
public:
    Cart(const sf::Texture& wheelTexture) 
        : wheel1Sprite(wheelTexture), wheel2Sprite(wheelTexture) 
    {
        cartSize = { 100.f, 15.f };
        cartRect.setSize(cartSize);
        cartRect.setFillColor(sf::Color::White);


        sf::Vector2f cartPos = {
            ((float)windowSize.x - cartSize.x) / 2,
            ((float)windowSize.y - cartSize.y) / 2
        };
        setPosition(cartPos);

        sf::FloatRect bounds1 = wheel1Sprite.getLocalBounds();
        sf::FloatRect bounds2 = wheel2Sprite.getLocalBounds();

        wheel1Sprite.setOrigin({ bounds1.size.x / 2.f, bounds1.size.y / 2.f });
        wheel2Sprite.setOrigin({ bounds2.size.x / 2.f, bounds2.size.y / 2.f });

        wheel1Sprite.setScale({ 0.6f , 0.6f});
        wheel2Sprite.setScale({ 0.6f , 0.6f });

        float fWheelY = cartSize.y - 50.f;

        wheel1Sprite.setPosition({ 0.f, fWheelY });
        wheel2Sprite.setPosition({ cartSize.x, fWheelY });
    }

    void update(float dt, float xDDot) {
        velocity += xDDot * dt;
        velocity *= damping;

        sf::Vector2f pos = getPosition();
        pos.x = std::clamp(pos.x + velocity * dt, 0.f, windowSize.x - cartSize.x);
        setPosition(pos);
    }

    float getMass() const { return mass; }
    float getVelocity() const { return velocity; }

    sf::Vector2f getPivot() const {
        sf::Vector2f pos = getPosition();
        return { pos.x + cartSize.x / 2.f, pos.y };
    }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        states.transform *= getTransform();
        target.draw(cartRect, states);

        sf::Vector2f cartCenter(cartSize.x / 2.f, cartSize.y / 2.f);
        float fLineOffsetX = 45.f;

        auto drawLine = [&](sf::Vector2f start, sf::Vector2f end, float width) {
            sf::Vector2f diff = { end.x - start.x, end.y - start.y };
            float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            sf::RectangleShape line({ length, width });

            line.setOrigin({ 0.f, width / 2.f });
            line.setPosition(start);

            line.setRotation(sf::radians(std::atan2(diff.y, diff.x)));
            line.setFillColor(sf::Color::White);

            target.draw(line, states);
        };

        float lineWidth = 4.0f;
        drawLine({ cartCenter.x + fLineOffsetX, cartCenter.y }, wheel2Sprite.getPosition(), lineWidth);
        drawLine({ cartCenter.x - fLineOffsetX, cartCenter.y }, wheel1Sprite.getPosition(), lineWidth);

        // Draw the wheels on top
        target.draw(wheel1Sprite, states);
        target.draw(wheel2Sprite, states);
    }

private:
    sf::RectangleShape cartRect;
    sf::Vector2f cartSize;
    sf::Sprite wheel1Sprite;
    sf::Sprite wheel2Sprite;
    float velocity = 0.f;
    float mass = 5.f;
};


class DoublePendulum : public sf::Drawable {
public:
    DoublePendulum() {
        bob1.setRadius(bobRadius1);
        bob1.setOrigin({ bobRadius1, bobRadius1 });
        bob1.setFillColor(sf::Color::Red);

        bob2.setRadius(bobRadius2);
        bob2.setOrigin({ bobRadius2, bobRadius2 });
        bob2.setFillColor(sf::Color::Cyan);
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

        alpha1 = (-m2 * cd * (L1 * w1 * w1 * sd - g * s2)
                  - m2 * L2 * w2 * w2 * sd
                  - M * g * s1
                  + M * aCart * std::cos(t1)) / denom;
        
        float denom2 = L2 * (M - m2 * cd * cd);
        alpha2 = (m2 * cd * (L2 * w2 * w2 * sd + g * s1)
                  + M * L1 * w1 * w1 * sd
                  - M * g * s2
                  + m2 * aCart * std::cos(t2) * cd
                  - M * aCart * std::cos(t1) * cd
                  + M * aCart * std::cos(t2)) / denom2;
    }

    void update(float dt, float xDDot, sf::Vector2f pivot) {
        pivotPos = pivot;
        
        float aCart = xDDot * 0.15f;
        
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

        // Draw first rod
        sf::Vertex line1[2] = {
            sf::Vertex{ pivotPos, sf::Color::White },
            sf::Vertex{ bob1Pos,  sf::Color::White }
        };
        target.draw(line1, 2, sf::PrimitiveType::Lines);
        
        // Draw second ro
        sf::Vertex line2[2] = {
            sf::Vertex{ bob1Pos, sf::Color::White },
            sf::Vertex{ bob2Pos, sf::Color::White }
        };
        target.draw(line2, 2, sf::PrimitiveType::Lines);

        bob1.setPosition(bob1Pos);
        target.draw(bob1, states);
        
        bob2.setPosition(bob2Pos);
        target.draw(bob2, states);
    }

private:
    // first pendulum
    float theta1 = 3.0f; 
    float theta1Dot = 0.f;
    float L1 = 150.f;     
    float m1 = 3.f;      
    float bobRadius1 = 18.f;
    // second pendulum
    float theta2 = 3.0f;  
    float theta2Dot = 0.f;
    float L2 = 125.f;      
    float m2 = 3.f;      
    float bobRadius2 = 12.f;

    sf::Vector2f pivotPos;
    mutable sf::CircleShape bob1;
    mutable sf::CircleShape bob2;
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

    /*
     sf::FloatRect bounds1 = wheel1Sprite.getLocalBounds();
        sf::FloatRect bounds2 = wheel2Sprite.getLocalBounds();

        wheel1Sprite.setOrigin({ bounds1.size.x / 2.f, bounds1.size.y / 2.f });
        wheel2Sprite.setOrigin({ bounds2.size.x / 2.f, bounds2.size.y / 2.f });
        */
    sf::RectangleShape centerLine({ windowSize.x - 200.f, 5.f });
    sf::FloatRect bounds = centerLine.getLocalBounds();
    centerLine.setOrigin({ bounds.size.x / 2, bounds.size.y / 2});
    centerLine.setPosition({(float)windowCenter.x, (float)windowCenter.y});

    sf::Texture wheelTexture;
    if (!wheelTexture.loadFromFile(cartImg)) {
        // Handle error
    }

    Cart cart(wheelTexture);
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

        cart.update(dt, xDDot);
        pendulum.update(dt, xDDot, cart.getPivot());

        window.clear(sf::Color::Black);
        window.draw(centerLine);
        window.draw(cart);
        window.draw(pendulum);   
        window.display();
    }

    return 0;
}