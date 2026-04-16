#pragma once

#include <SFML/Graphics.hpp>

namespace Constants {

    // Window settings
    constexpr unsigned int WINDOW_WIDTH = 1920;
    constexpr unsigned int WINDOW_HEIGHT = 1080;
    const sf::Vector2u WINDOW_SIZE(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Agent Physics
    constexpr float AGENT_GRAVITY = 1000.0f;
    constexpr float AGENT_CART_DAMPING = 0.99f;
    constexpr float AGENT_PENDULUM_DAMPING = 0.99f;
    constexpr float AGENT_CART_PENDULUM_COUPLING = 0.70f;
    constexpr float AGENT_PENDULUM_LENGTH = 200.0f;
    
    // Manual Physics (when using pendulum manually)
    constexpr float MANUAL_GRAVITY = 1000.0f;
    constexpr float MANUAL_CART_DAMPING = 0.99f;
    constexpr float MANUAL_PENDULUM_DAMPING = 0.99f;
    constexpr float MANUAL_CART_PENDULUM_COUPLING = 0.70f;
    

    // TODO: replace old variables with new constants
    constexpr float GRAVITY = AGENT_GRAVITY;
    constexpr float DEFAULT_DAMPING = AGENT_CART_DAMPING;
    constexpr float PENDULUM_DAMPING = AGENT_PENDULUM_DAMPING;
    constexpr float CART_PENDULUM_COUPLING = AGENT_CART_PENDULUM_COUPLING;
    

    constexpr float SENSITIVITY = 700.0f;
    constexpr float CENTER_LINE_WIDTH = 200.f;
    
    // theme (definetly not stolen from tailwind)
    const sf::Color COLOR_ZINC_950(9, 9, 11);       // Darkest
    const sf::Color COLOR_ZINC_900(24, 24, 27);     // Main background
    const sf::Color COLOR_ZINC_800(39, 39, 42);     // Container background
    const sf::Color COLOR_ZINC_700(63, 63, 70);     // Borders, secondary
    const sf::Color COLOR_ZINC_600(82, 82, 91);     // Lighter borders
    const sf::Color COLOR_ZINC_500(113, 113, 122);  // Muted text
    const sf::Color COLOR_ZINC_400(161, 161, 170);  // Secondary text
    const sf::Color COLOR_ZINC_300(212, 212, 216);  // Primary text
    
    // Emerald accent colors (primary)
    const sf::Color COLOR_EMERALD_500(16, 185, 129);   // Primary accent
    const sf::Color COLOR_EMERALD_400(52, 211, 153);   // Lighter accent
    const sf::Color COLOR_EMERALD_600(5, 150, 105);    // Darker accent
    const sf::Color COLOR_EMERALD_300(110, 231, 183);  // Highlight
    
    // Rose colors
    const sf::Color COLOR_ROSE_500(244, 63, 94);    // Primary negative
    const sf::Color COLOR_ROSE_400(251, 113, 133);  // Lighter negative
    
    // TODO: replace old variables with new constnats
    const sf::Color COLOR_BACKGROUND = COLOR_ZINC_900;
    const sf::Color COLOR_GREY = COLOR_ZINC_700;
    const sf::Color COLOR_TRACK = COLOR_ZINC_500;
    const sf::Color COLOR_TRACK_OUTLINE = COLOR_ZINC_600;
    const sf::Color COLOR_CART = COLOR_EMERALD_500;
    const sf::Color COLOR_CART_OUTLINE = COLOR_EMERALD_600;
    const sf::Color COLOR_BOB(255, 255, 255);
    const sf::Color COLOR_BOB_OUTLINE = COLOR_ZINC_400;
    const sf::Color COLOR_ROD = COLOR_ZINC_400;
    const sf::Color COLOR_ROD_OUTLINE = COLOR_ZINC_500;
    const sf::Color COLOR_HINGE = COLOR_ZINC_400;
    const sf::Color COLOR_HINGE_OUTLINE = COLOR_ZINC_500;
    
    // Trail settings
    constexpr size_t MAX_TRAIL_POINTS = 500;
    constexpr float TRAIL_FADE_RATE = 0.5f;
    
    // Ruler settings
    constexpr float RULER_MINOR_TICK_INTERVAL = 100.f;
    constexpr float RULER_MAJOR_TICK_INTERVAL = 200.f;
}
