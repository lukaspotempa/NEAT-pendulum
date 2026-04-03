#include "Application.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

Application::Application() 
    : m_window(sf::VideoMode(Constants::WINDOW_SIZE), "Pendulum Simulator", sf::State::Windowed,
               sf::ContextSettings(0, 0, 8))
{
    m_window.setFramerateLimit(60);
    m_window.setMouseCursorVisible(false);
    
    m_windowCenter = sf::Vector2i(Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2);
    sf::Mouse::setPosition(m_windowCenter, m_window);
    
    // Setup center line
    m_centerLine.setSize({ Constants::WINDOW_WIDTH - Constants::CENTER_LINE_WIDTH, 8.f });
    m_centerLine.setFillColor(Constants::COLOR_TRACK);
    m_centerLine.setOutlineThickness(2.f);
    m_centerLine.setOutlineColor(Constants::COLOR_TRACK_OUTLINE);
    sf::FloatRect bounds = m_centerLine.getLocalBounds();
    m_centerLine.setOrigin({ bounds.size.x / 2, bounds.size.y / 2 });
    m_centerLine.setPosition({ static_cast<float>(m_windowCenter.x), static_cast<float>(m_windowCenter.y) });
    
    // Font
    if (m_font.openFromFile("C:/Windows/Fonts/segoeui.ttf")) {
        m_fontLoaded = true;
    } else if (m_font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        m_fontLoaded = true;
    }
    
    // Initialize pendulum (default single for NEAT compatibility)
    m_pendulum = std::make_unique<SinglePendulum>();
    m_currentMode = PendulumMode::Single;
    
    // Initialize NEAT controller
    m_neatController = std::make_unique<NEATController>(100, 3.0f);
    
    // Setup menu
    if (m_fontLoaded) {
        setupMenu();
        
        // Setup help text
        m_helpText = std::make_unique<sf::Text>(m_font, "ESC: menu | N: start NEAT / next gen | M: stop NEAT", 18);
        m_helpText->setFillColor(sf::Color(150, 150, 150));
        m_helpText->setPosition({ 10.f, 10.f });
        
        // Setup mode text
        m_modeText = std::make_unique<sf::Text>(m_font, "Mode: Single Pendulum", 18);
        m_modeText->setFillColor(sf::Color(150, 150, 150));
        m_modeText->setPosition({ 10.f, Constants::WINDOW_HEIGHT - 30.f });
        
        // Setup NEAT status text
        m_neatText = std::make_unique<sf::Text>(m_font, "NEAT: OFF", 18);
        m_neatText->setFillColor(sf::Color(150, 150, 150));
        m_neatText->setPosition({ 10.f, 35.f });
    }
}

void Application::setupMenu() {
    m_menu = std::make_unique<Menu>(m_font);
    
    m_menu->addItem("Single Pendulum", [this]() {
        switchPendulumMode(PendulumMode::Single);
        setMenuVisible(false);
    });
    
    m_menu->addItem("Double Pendulum", [this]() {
        switchPendulumMode(PendulumMode::Double);
        setMenuVisible(false);
    });
    
    m_menu->addToggleItem("Enable Trail", false, [this](bool enabled) {
        toggleTrail(enabled);
    });
    
    m_menu->addToggleItem("Enable NEAT AI", false, [this](bool enabled) {
        toggleNEAT(enabled);
    });
    
    m_menu->addItem("Reset Simulation", [this]() {
        resetSimulation();
        setMenuVisible(false);
    });
    
    m_menu->addItem("Close Menu", [this]() {
        setMenuVisible(false);
    });
}

void Application::setMenuVisible(bool visible) {
    if (m_menu) {
        if (visible) {
            m_menu->show();
        } else {
            m_menu->hide();
        }
        
        // Update mouse cursor visibility
        m_window.setMouseCursorVisible(visible);
        
        // Re-center and lock mouse when hiding menu
        if (!visible) {
            sf::Mouse::setPosition(m_windowCenter, m_window);
        }
    }
}

void Application::switchPendulumMode(PendulumMode mode) {
    if (mode == m_currentMode) return;
    
    m_currentMode = mode;
    
    if (mode == PendulumMode::Single) {
        m_pendulum = std::make_unique<SinglePendulum>();
        if (m_modeText) m_modeText->setString("Mode: Single Pendulum");
    } else {
        m_pendulum = std::make_unique<DoublePendulum>();
        if (m_modeText) m_modeText->setString("Mode: Double Pendulum");
    }
    
    // Apply trail state to new pendulum
    m_pendulum->setTrailEnabled(m_trailEnabled);
    
    // Reset cart position
    m_cart.reset();
}

void Application::toggleTrail(bool enabled) {
    m_trailEnabled = enabled;
    if (m_pendulum) {
        m_pendulum->setTrailEnabled(enabled);
    }
}

void Application::toggleNEAT(bool enabled) {
    m_neatEnabled = enabled;
    if (m_neatController) {
        m_neatController->setEnabled(enabled);
        
        // If enabling NEAT, switch to single pendulum
        if (enabled && m_currentMode != PendulumMode::Single) {
            switchPendulumMode(PendulumMode::Single);
        }
        
        // Reset simulation when toggling NEAT
        if (enabled) {
            resetSimulation();
            m_neatController->resetCurrentGenome();
        }
    }
    
    // Update cursor visibility - show cursor when NEAT is active (no manual control)
    if (!m_menu || !m_menu->isVisible()) {
        m_window.setMouseCursorVisible(enabled);
    }
}

void Application::resetSimulation() {
    m_cart.reset();
    if (m_pendulum) {
        m_pendulum->reset();
    }
}

void Application::run() {
    while (m_window.isOpen()) {
        processEvents();
        
        float dt = m_clock.restart().asSeconds();
        dt = std::min(dt, 0.05f);
        
        update(dt);
        render();
    }
}

void Application::processEvents() {
    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }
        
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                if (m_menu) {
                    setMenuVisible(!m_menu->isVisible());
                }
            }
            else if (m_menu && m_menu->isVisible()) {
                m_menu->handleKeyPressed(keyPressed->code);
            }
            else if (keyPressed->code == sf::Keyboard::Key::R) {
                resetSimulation();
                if (m_neatController && m_neatEnabled) {
                    m_neatController->resetCurrentGenome();
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::T) {
                m_trailEnabled = !m_trailEnabled;
                toggleTrail(m_trailEnabled);
                if (m_menu) {
                    m_menu->setToggleState(2, m_trailEnabled); // Index 2 is the trail toggle
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::N) {
                if (m_neatEnabled && m_neatController) {
                    // If NEAT is running, N skips to next generation
                    m_neatController->skipToNextGeneration();
                    resetSimulation();
                } else {
                    // If NEAT is off, N toggles it on
                    m_neatEnabled = true;
                    toggleNEAT(m_neatEnabled);
                    if (m_menu) {
                        m_menu->setToggleState(3, m_neatEnabled); // Index 3 is the NEAT toggle
                    }
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::M) {
                // M toggles NEAT off
                if (m_neatEnabled) {
                    m_neatEnabled = false;
                    toggleNEAT(m_neatEnabled);
                    if (m_menu) {
                        m_menu->setToggleState(3, m_neatEnabled);
                    }
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::Num1) {
                switchPendulumMode(PendulumMode::Single);
            }
            else if (keyPressed->code == sf::Keyboard::Key::Num2) {
                switchPendulumMode(PendulumMode::Double);
            }
        }
    }
}

void Application::update(float dt) {
    if (m_menu && m_menu->isVisible()) {
        m_menu->update();
        return; // Pause simulation while menu is open
    }
    
    float xDDot = 0.0f;
    
    if (m_neatEnabled && m_neatController && m_currentMode == PendulumMode::Single) {
        // NEAT controls the cart
        auto* singlePendulum = dynamic_cast<SinglePendulum*>(m_pendulum.get());
        if (singlePendulum) {
            float theta = singlePendulum->getTheta();
            float thetaDot = singlePendulum->getThetaDot();
            sf::Vector2f cartPos = m_cart.getPivot();
            float cartX = cartPos.x - Constants::WINDOW_WIDTH / 2.0f; // Relative to center
            float cartVel = m_cart.getVelocity();
            
            // Get control from NEAT (-1 to 1)
            float control = m_neatController->getControl(theta, thetaDot, cartX, cartVel);
            
            // Convert to acceleration (scale factor for responsiveness)
            xDDot = control * 2000.0f;
            
            // Update fitness - returns true if genome evaluation ended
            bool shouldReset = m_neatController->updateFitness(dt, theta);
            
            if (shouldReset) {
                // Reset simulation for next genome
                resetSimulation();
            }
        }
    }
    else {
        // Manual mouse control
        sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
        float delta = static_cast<float>(mousePos.x - m_windowCenter.x);
        sf::Mouse::setPosition(m_windowCenter, m_window);
        
        float F = delta * Constants::SENSITIVITY;
        float M = m_cart.getMass();
        xDDot = F / M;
    }
    
    float effectiveXDDot = m_cart.update(dt, xDDot);
    
    if (m_pendulum) {
        m_pendulum->update(dt, effectiveXDDot, m_cart.getPivot());
    }
    
    // Update NEAT status text
    if (m_neatText && m_neatController) {
        std::ostringstream ss;
        if (m_neatEnabled) {
            ss << "NEAT: ON | Gen: " << m_neatController->getGeneration()
               << " | Time: " << std::fixed << std::setprecision(0) << m_neatController->getGenerationTime() << "/40s"
               << " | Genome: " << (m_neatController->getCurrentGenomeIndex() + 1) 
               << "/" << m_neatController->getPopulationSize()
               << " | Fitness: " << std::fixed << std::setprecision(1) << m_neatController->getCurrentFitness()
               << " | Best: " << std::fixed << std::setprecision(1) << m_neatController->getBestFitness()
               << " (N=next gen)";
        }
        else {
            ss << "NEAT: OFF (Press N to enable)";
        }
        m_neatText->setString(ss.str());
    }
}

void Application::render() {
    m_window.clear(Constants::COLOR_BACKGROUND);
    
    m_window.draw(m_centerLine);
    m_window.draw(m_cart);
    
    if (m_pendulum) {
        m_window.draw(*m_pendulum);
    }
    
    // Draw UI
    if (m_fontLoaded) {
        if (m_helpText) m_window.draw(*m_helpText);
        if (m_modeText) m_window.draw(*m_modeText);
        if (m_neatText) m_window.draw(*m_neatText);
        
        if (m_menu) {
            m_window.draw(*m_menu);
        }
    }
    
    m_window.display();
}
