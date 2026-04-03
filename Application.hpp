#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "Cart.hpp"
#include "IPendulum.hpp"
#include "SinglePendulum.hpp"
#include "DoublePendulum.hpp"
#include "Menu.hpp"
#include "Constants.hpp"
#include "NEATController.hpp"

class Application {
public:
    Application();
    void run();
    
private:
    void processEvents();
    void update(float dt);
    void render();
    
    void setupMenu();
    void switchPendulumMode(PendulumMode mode);
    void toggleTrail(bool enabled);
    void toggleNEAT(bool enabled);
    void resetSimulation();
    void setMenuVisible(bool visible);
    
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_fontLoaded = false;
    
    Cart m_cart;
    std::unique_ptr<IPendulum> m_pendulum;
    PendulumMode m_currentMode = PendulumMode::Single; // Default to single for NEAT
    
    sf::RectangleShape m_centerLine;
    sf::Clock m_clock;
    sf::Vector2i m_windowCenter;
    
    std::unique_ptr<Menu> m_menu;
    bool m_trailEnabled = false;
    
    // NEAT
    std::unique_ptr<NEATController> m_neatController;
    bool m_neatEnabled = false;
    
    // UI text
    std::unique_ptr<sf::Text> m_helpText;
    std::unique_ptr<sf::Text> m_modeText;
    std::unique_ptr<sf::Text> m_neatText;
};
