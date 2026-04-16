#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "Cart.hpp"
#include "IPendulum.hpp"
#include "SinglePendulum.hpp"
#include "DoublePendulum.hpp"
#include "Menu.hpp"
#include "Dashboard.hpp"
#include "Constants.hpp"
#include "NEATController.hpp"
#include "NetworkVisualizer.hpp"
#include "RoundedRectangle.hpp"

class Application {
public:
    Application();
    void run();
    
private:
    void processEvents();
    void update(float dt);
    void render();
    
    void setupMenu();
    void setupDashboard();
    void setupNetworkVisualizer();
    void switchPendulumMode(PendulumMode mode);
    void toggleTrail(bool enabled);
    void toggleNEAT(bool enabled);
    void toggleSeeAll(bool enabled);
    void toggleDemo(bool enabled);
    void updateDemo(float dt);
    void advanceDemoState();
    void resetSimulation();
    void setMenuVisible(bool visible);
    
    // Dashboard callbacks
    void onStartTraining(int generations);
    void onStopTraining();
    void onAgentSelected(int agentIndex);
    void onGenerationChanged(int genNumber);
    void onBatchComplete();
    
    // Update network visualizer with current agent state
    void updateNetworkVisualizer();
    
    void renderAgent(int agentIndex, RenderMode mode);
    void drawInfoBox();
    void drawRuler();
    void drawSimulationContainer();
    void drawLiveFitness();
    
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_fontLoaded = false;
    
    // Main cart
    Cart m_cart;
    std::unique_ptr<IPendulum> m_pendulum;
    PendulumMode m_currentMode = PendulumMode::Single;
    
    // Ghost carts/pendulums for "See All" mode
    std::vector<Cart> m_ghostCarts;
    std::vector<std::unique_ptr<SinglePendulum>> m_ghostPendulums;
    
    sf::RectangleShape m_centerLine;
    sf::Clock m_clock;
    sf::Vector2i m_windowCenter;
    
    std::unique_ptr<Menu> m_menu;
    std::unique_ptr<Dashboard> m_dashboard;
    std::unique_ptr<NetworkVisualizer> m_networkVisualizer;
    bool m_trailEnabled = false;
    
    // NEAT
    std::unique_ptr<NEATController> m_neatController;
    bool m_neatEnabled = false;
    bool m_seeAllEnabled = false;
    
    // Demo Mode
    bool m_demoEnabled = false;
    int m_demoState = 0;
    std::vector<Cart> m_demoCarts;
    std::vector<std::unique_ptr<IPendulum>> m_demoPendulums;
    
    // UI elements
    std::unique_ptr<RoundedRectangle> m_infoBox;
    std::unique_ptr<RoundedRectangle> m_simContainer;
    std::vector<sf::RectangleShape> m_rulerTicks;
    std::vector<sf::Text> m_rulerLabels;
    

    float m_simCenterX = 0.0f;
    float m_simCenterY = 0.0f;
    float m_simTrackWidth = 0.0f;
    
    float m_lastFitness = 0.0f;
    float m_recentFitnessGain = 0.0f;
    float m_timeSinceLastFitnessGain = 0.0f;
    
    std::unique_ptr<sf::Text> m_modeText;
};
