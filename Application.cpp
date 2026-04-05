#include "Application.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

Application::Application() 
    : m_window(sf::VideoMode(), "Pendulum NEAT", sf::State::Fullscreen,
               sf::ContextSettings(0, 0, 8))
{
    m_window.setFramerateLimit(60);
    m_window.setMouseCursorVisible(false);
    
    m_windowCenter = sf::Vector2i(Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2);
    sf::Mouse::setPosition(m_windowCenter, m_window);
    
    float containerPadding = 20.0f;
    float topBarHeight = 50.0f;  // Space for top bar
    float bottomPadding = 30.0f;
    float nnContainerHeight = 280.0f;  // Neural network container height
    float simContainerHeight = Constants::WINDOW_HEIGHT - topBarHeight - nnContainerHeight - bottomPadding - 30.0f;
    
    // Store simulation bounds for physics
    float simContainerWidth = Constants::WINDOW_WIDTH - 2 * containerPadding;
    m_simCenterX = containerPadding + simContainerWidth / 2.0f;
    m_simCenterY = topBarHeight + simContainerHeight / 2.0f;
    m_simTrackWidth = simContainerWidth - 200.0f;  // Track is smaller than container
    
    m_simContainer = std::make_unique<RoundedRectangle>(
        containerPadding, topBarHeight, 
        simContainerWidth, simContainerHeight,
        12.0f,  
        Constants::COLOR_ZINC_800, 
        Constants::COLOR_ZINC_600, 
        2.0f
    );
    

    m_centerLine.setSize({m_simTrackWidth, 8.f});
    m_centerLine.setFillColor(Constants::COLOR_TRACK);
    m_centerLine.setOutlineThickness(2.f);
    m_centerLine.setOutlineColor(Constants::COLOR_TRACK_OUTLINE);
    sf::FloatRect bounds = m_centerLine.getLocalBounds();
    m_centerLine.setOrigin({bounds.size.x / 2, bounds.size.y / 2});
    m_centerLine.setPosition({m_simCenterX, m_simCenterY});
    

    float rulerY = m_simCenterY + 20.0f;
    
    float pixelsPerUnit = m_simTrackWidth / 1500.0f;
    
    for (int unit = -750; unit <= 750; unit += 100) {
        float tickX = m_simCenterX + unit * pixelsPerUnit;
        
        bool isMajor = (unit % 200 == 0);
        float tickHeight = isMajor ? 20.0f : 10.0f;
        float tickWidth = isMajor ? 2.0f : 1.0f;
        
        sf::RectangleShape tick;
        tick.setSize({tickWidth, tickHeight});
        tick.setFillColor(isMajor ? Constants::COLOR_ZINC_400 : Constants::COLOR_ZINC_500);
        tick.setPosition({tickX - tickWidth / 2.0f, rulerY});
        m_rulerTicks.push_back(tick);
    }
    
    m_infoBox = std::make_unique<RoundedRectangle>(
        containerPadding + 15.0f, topBarHeight + 15.0f,
        200.0f, 70.0f,
        8.0f,  // corner radius
        Constants::COLOR_ZINC_800,
        Constants::COLOR_EMERALD_600,
        2.0f
    );
    
    // Font
    if (m_font.openFromFile("C:/Windows/Fonts/segoeui.ttf")) {
        m_fontLoaded = true;
    } else if (m_font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        m_fontLoaded = true;
    }
    
    // Create ruler labels
    if (m_fontLoaded) {
        float rulerLabelY = rulerY + 22.0f;
        for (int unit = -800; unit <= 800; unit += 200) { 
            float tickX = m_simCenterX + unit * pixelsPerUnit;
            
            sf::Text label(m_font, std::to_string(unit / 100), 11);
            label.setFillColor(Constants::COLOR_ZINC_400);
            label.setPosition({tickX - label.getLocalBounds().size.x / 2.0f, rulerLabelY});
            m_rulerLabels.push_back(label);
        }
    }
    
    // Initialize pendulum
    m_pendulum = std::make_unique<SinglePendulum>();
    m_currentMode = PendulumMode::Single;
    
    // Set bounds on main cart
    m_cart.setSimulationBounds(m_simCenterX, m_simCenterY, m_simTrackWidth);
    m_cart.reset(); 

    m_neatController = std::make_unique<NEATController>(NEATController::DEFAULT_POPULATION_SIZE, 3.0f);
    
    // Set up batch completion callback
    m_neatController->setOnBatchComplete([this]() {
        onBatchComplete();
    });
    
    // Initialize ghost pendulums
    int popSize = m_neatController->getPopulationSize();
    m_ghostCarts.resize(popSize);
    for (int i = 0; i < popSize; ++i) {
        m_ghostCarts[i].setSimulationBounds(m_simCenterX, m_simCenterY, m_simTrackWidth);
        m_ghostCarts[i].setRenderMode(RenderMode::Ghost);
        m_ghostPendulums.push_back(std::make_unique<SinglePendulum>());
        m_ghostPendulums.back()->setRenderMode(RenderMode::Ghost);
        m_ghostPendulums.back()->setTrailEnabled(false);
    }
    
    // Setup menu
    if (m_fontLoaded) {
        setupMenu();
        setupDashboard();
        setupNetworkVisualizer();
       

        m_modeText = std::make_unique<sf::Text>(m_font, "Mode: Single Pendulum", 14);
        m_modeText->setFillColor(Constants::COLOR_ZINC_500);
        m_modeText->setPosition({containerPadding, Constants::WINDOW_HEIGHT - 25.f});
    }
}

void Application::setupNetworkVisualizer() {
    // Layout constants 
    float containerPadding = 20.0f;
    float topBarHeight = 50.0f;
    float bottomPadding = 30.0f;
    float nnContainerHeight = 280.0f;
    float simContainerHeight = Constants::WINDOW_HEIGHT - topBarHeight - nnContainerHeight - bottomPadding - 30.0f;

    float vizX = containerPadding;
    float vizY = topBarHeight + simContainerHeight + 20.0f; 
    float vizWidth = Constants::WINDOW_WIDTH - 2 * containerPadding;  
    float vizHeight = nnContainerHeight;
    
    m_networkVisualizer = std::make_unique<NetworkVisualizer>(vizX, vizY, vizWidth, vizHeight, m_font);
    
    // Set input labels (4 inputs)
    m_networkVisualizer->setInputLabels({"CartX", "Sin\xCE\xB8", "Cos\xCE\xB8", "\xCE\xB8'"});
    m_networkVisualizer->setOutputLabel("Accel");
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
    
    m_menu->addToggleItem("See All Agents", false, [this](bool enabled) {
        toggleSeeAll(enabled);
    });
    
    m_menu->addItem("Reset Simulation", [this]() {
        resetSimulation();
        setMenuVisible(false);
    });
    
    m_menu->addItem("Close Menu", [this]() {
        setMenuVisible(false);
    });
}

void Application::setupDashboard() {
    // Dashboard on right side of the screen
    float dashboardX = Constants::WINDOW_WIDTH - Dashboard::DASHBOARD_WIDTH - 10.0f;
    float dashboardY = 30.0f;
    
    m_dashboard = std::make_unique<Dashboard>(dashboardX, dashboardY, m_font);
    
    m_dashboard->setOnStartTraining([this](int generations) {
        onStartTraining(generations);
    });
    
    m_dashboard->setOnStopTraining([this]() {
        onStopTraining();
    });
    
    m_dashboard->setOnAgentSelected([this](int agentIndex) {
        onAgentSelected(agentIndex);
    });
    
    m_dashboard->setOnGenerationChanged([this](int genNumber) {
        onGenerationChanged(genNumber);
    });
}

void Application::onStartTraining(int generations) {
    if (!m_neatController) return;
    
    // Enable NEAT
    if (!m_neatEnabled) {
        m_neatEnabled = true;
        m_neatController->setEnabled(true);
        
        if (m_currentMode != PendulumMode::Single) {
            switchPendulumMode(PendulumMode::Single);
        }
        
        if (m_menu) {
            m_menu->setToggleState(3, m_neatEnabled);
        }
    }
    
    // Show dashboard in fullscreen mode during training
    if (m_dashboard) {
        m_dashboard->setVisible(true);
        m_dashboard->setFullscreen(true);  
        m_dashboard->setTrainingState(true);
        m_window.setMouseCursorVisible(true);
    }
    
    // Start training batch
    m_neatController->startBatchTraining(generations);
}

void Application::onStopTraining() {
    if (m_neatController) {
        m_neatController->stopBatchTraining();
    }
    
    if (m_dashboard) {
        m_dashboard->setFullscreen(false);  // Exit fullscreen when training stops
        m_dashboard->setTrainingState(false);
        m_dashboard->updateFromHistory(m_neatController->getHistory());
        
        if (!m_neatController->getHistory().isEmpty()) {
            m_dashboard->setSelectedGeneration(m_neatController->getHistory().getLastGenerationNumber());
            const auto* snapshot = m_neatController->getHistory().getLatestGeneration();
            m_dashboard->updateAgentList(snapshot, m_neatController->getSelectedAgentIndex());
        }
    }
    
    // Reset fitness tracking for replay
    m_lastFitness = 0.0f;
    m_recentFitnessGain = 0.0f;
    m_timeSinceLastFitnessGain = 0.0f;
}

void Application::onAgentSelected(int agentIndex) {
    if (m_neatController) {
        m_neatController->selectAgent(agentIndex);
        
        // Reset fitness tracking for new agent replay
        m_lastFitness = 0.0f;
        m_recentFitnessGain = 0.0f;
        m_timeSinceLastFitnessGain = 0.0f;
        
        const auto* snapshot = m_neatController->getCurrentSnapshot();
        if (!snapshot) {
            snapshot = m_neatController->getHistory().getLatestGeneration();
        }
        m_dashboard->updateAgentList(snapshot, agentIndex);
    }
}

void Application::onGenerationChanged(int genNumber) {
    if (m_neatController) {
        if (m_neatController->loadGenerationForReplay(genNumber)) {
            const auto* snapshot = m_neatController->getHistory().getGeneration(genNumber);
            m_dashboard->updateAgentList(snapshot, m_neatController->getSelectedAgentIndex());
        }
    }
}

void Application::onBatchComplete() {
    // Exit fullscreen and update dashboard state when batch training completes
    if (m_dashboard) {
        m_dashboard->setFullscreen(false);
        m_dashboard->setTrainingState(false);
        
        if (m_neatController) {
            m_dashboard->updateFromHistory(m_neatController->getHistory());
            
            if (!m_neatController->getHistory().isEmpty()) {
                m_dashboard->setSelectedGeneration(m_neatController->getHistory().getLastGenerationNumber());
                const auto* snapshot = m_neatController->getHistory().getLatestGeneration();
                m_dashboard->updateAgentList(snapshot, m_neatController->getSelectedAgentIndex());
            }
        }
    }
    
    m_lastFitness = 0.0f;
    m_recentFitnessGain = 0.0f;
    m_timeSinceLastFitnessGain = 0.0f;
}

void Application::setMenuVisible(bool visible) {
    if (m_menu) {
        if (visible) {
            m_menu->show();
        } else {
            m_menu->hide();
        }
        
        // Update cursor visibility
        bool showCursor = visible || (m_dashboard && m_dashboard->isVisible());
        m_window.setMouseCursorVisible(showCursor);
        
        if (!visible && (!m_dashboard || !m_dashboard->isVisible())) {
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
        
        if (enabled && m_currentMode != PendulumMode::Single) {
            switchPendulumMode(PendulumMode::Single);
        }
        
        if (enabled) {
            m_neatController->runGeneration();
        }
    }
    
    if (!m_menu || !m_menu->isVisible()) {
        m_window.setMouseCursorVisible(enabled);
    }
}

void Application::toggleSeeAll(bool enabled) {
    m_seeAllEnabled = enabled;
    if (m_neatController) {
        m_neatController->showAllAgents = enabled;
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
        
        // Handle mouse move for dashboard hover
        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            if (m_dashboard && m_dashboard->isVisible()) {
                m_dashboard->handleMouseMove(static_cast<float>(mouseMoved->position.x), 
                                             static_cast<float>(mouseMoved->position.y));
            }
        }
        
        // Handle mouse click
        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                if (m_dashboard && m_dashboard->isVisible()) {
                    if (m_dashboard->handleClick(static_cast<float>(mousePressed->position.x),
                                                  static_cast<float>(mousePressed->position.y))) {
                        continue; 
                    }
                }
            }
        }
        
        // Handle mouse scroll for dashboard
        if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (m_dashboard && m_dashboard->isVisible()) {
                m_dashboard->handleScroll(mouseScrolled->delta);
            }
        }
        
        // Handle text input for dashboard
        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
            if (m_dashboard && m_dashboard->isVisible()) {
                if (textEntered->unicode < 128) {
                    if (m_dashboard->handleTextInput(static_cast<char>(textEntered->unicode))) {
                        continue; 
                    }
                }
            }
        }
        
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            // Tab dashboard
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
                if (m_dashboard) {
                    m_dashboard->toggleVisible();
                    m_window.setMouseCursorVisible(m_dashboard->isVisible() || (m_menu && m_menu->isVisible()));
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::Escape) {

                if (m_dashboard && m_dashboard->isVisible()) {
                    m_dashboard->setVisible(false);
                    m_window.setMouseCursorVisible(m_menu && m_menu->isVisible());
                }
                else if (m_menu) {
                    setMenuVisible(!m_menu->isVisible());
                }
            }

            else if (m_dashboard && m_dashboard->isVisible()) {
                if (m_dashboard->handleKey(keyPressed->code)) {
                    continue; 
                }
            }
            else if (m_menu && m_menu->isVisible()) {
                m_menu->handleKeyPressed(keyPressed->code);
            }
            else if (keyPressed->code == sf::Keyboard::Key::R) {
                resetSimulation();
            }
            else if (keyPressed->code == sf::Keyboard::Key::T) {
                m_trailEnabled = !m_trailEnabled;
                toggleTrail(m_trailEnabled);
                if (m_menu) {
                    m_menu->setToggleState(2, m_trailEnabled);
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::Space) {
                // Space bar START NEAT algoruthm
                if (!m_neatEnabled) {
                    m_neatEnabled = true;
                    toggleNEAT(m_neatEnabled);
                    if (m_menu) {
                        m_menu->setToggleState(3, m_neatEnabled);
                    }
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::N) {
                // N next generation
                if (m_neatEnabled && m_neatController && !m_neatController->isBatchTraining()) {
                    m_neatController->skipToNextGeneration();
                    
                    // Update dashboard if visible
                    if (m_dashboard) {
                        m_dashboard->updateFromHistory(m_neatController->getHistory());
                        const auto* snapshot = m_neatController->getHistory().getLatestGeneration();
                        m_dashboard->updateAgentList(snapshot, m_neatController->getSelectedAgentIndex());
                        m_dashboard->setSelectedGeneration(m_neatController->getGeneration());
                    }
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::M) {
                // M toggle NEAT off
                if (m_neatEnabled) {
                    m_neatEnabled = false;
                    toggleNEAT(m_neatEnabled);
                    if (m_menu) {
                        m_menu->setToggleState(3, m_neatEnabled);
                    }
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::S) {
                // S "See all" mode
                m_seeAllEnabled = !m_seeAllEnabled;
                toggleSeeAll(m_seeAllEnabled);
                if (m_menu) {
                    m_menu->setToggleState(4, m_seeAllEnabled);
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::Left) {
                // Select previous agent
                if (m_neatEnabled && m_neatController) {
                    m_neatController->selectPrevAgent();
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::Right) {
                // Select next agent
                if (m_neatEnabled && m_neatController) {
                    m_neatController->selectNextAgent();
                }
            }
            else if (keyPressed->code == sf::Keyboard::Key::B) {
                // B select best agent
                if (m_neatEnabled && m_neatController) {
                    m_neatController->selectBestAgent();
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
    
    if (m_neatController && m_neatController->getState() == NEATState::Simulating) {
        m_neatController->updateSimulation(dt);
        

        if (m_dashboard && m_dashboard->isVisible()) {
            // TODO: Simulation progress
        }
        return;
    }
    
    if (m_neatController && m_neatController->isBatchTraining()) {
        // Check if ready to advance next generation
        if (m_neatController->shouldContinueBatchTraining()) {
            // Update dashboard with latest data
            if (m_dashboard) {
                m_dashboard->updateFromHistory(m_neatController->getHistory());
                const auto* snapshot = m_neatController->getHistory().getLatestGeneration();
                if (snapshot) {
                    m_dashboard->updateAgentList(snapshot, m_neatController->getSelectedAgentIndex());
                }
                m_dashboard->setSelectedGeneration(m_neatController->getGeneration());
            }
            
            // Advance to next generation
            m_neatController->advanceBatchTraining();
        }
        return;
    }
    
    if (m_neatEnabled && m_neatController && m_currentMode == PendulumMode::Single) {
        // Parallel mode
        auto* singlePendulum = dynamic_cast<SinglePendulum*>(m_pendulum.get());
        if (singlePendulum && m_neatController->isReplaying()) {
            m_neatController->updateReplay(dt);
            
 
            const auto& replayAgent = m_neatController->getReplayAgent();
            float currentFitness = replayAgent.fitness;
            
  
            if (currentFitness > m_lastFitness) {
                m_recentFitnessGain = currentFitness - m_lastFitness;
                m_timeSinceLastFitnessGain = 0.0f;
                m_lastFitness = currentFitness;
            } else {
                m_timeSinceLastFitnessGain += dt;
            }
            
            float cartX = m_neatController->getReplayCartX();
            float theta = m_neatController->getReplayTheta();
            
            float pixelsPerUnit = m_simTrackWidth / 1500.0f;  // 1500 = range from -750 to +750
            float screenX = m_simCenterX + cartX * pixelsPerUnit - m_cart.getSize().x / 2.0f;
            float cartY = m_simCenterY - m_cart.getSize().y / 2.0f;
            m_cart.setPosition({ screenX, cartY });
            
            singlePendulum->setTheta(theta);
            singlePendulum->setPivot(m_cart.getPivot());
            
            updateNetworkVisualizer();
        }
        
        if (m_seeAllEnabled && m_neatController->getState() == NEATState::Simulating) {
            int popSize = m_neatController->getPopulationSize();
            int selectedIdx = m_neatController->getSelectedAgentIndex();
            float pixelsPerUnit = m_simTrackWidth / 1500.0f;
            
            for (int i = 0; i < popSize && i < static_cast<int>(m_ghostCarts.size()); ++i) {
                if (i == selectedIdx) continue; 
                
                const auto& agent = m_neatController->getAgent(i);
                
                float ghostCartX = agent.cartX;
                float ghostTheta = agent.theta;
                
                // Set ghost cart position
                float ghostScreenX = m_simCenterX + ghostCartX * pixelsPerUnit - m_ghostCarts[i].getSize().x / 2.0f;
                float ghostCartY = m_simCenterY - m_ghostCarts[i].getSize().y / 2.0f;
                m_ghostCarts[i].setPosition({ ghostScreenX, ghostCartY });
                
                // Set ghost pendulum
                if (m_ghostPendulums[i]) {
                    m_ghostPendulums[i]->setTheta(ghostTheta);
                    m_ghostPendulums[i]->setPivot(m_ghostCarts[i].getPivot());
                }
            }
        }
    }
    else if (!m_dashboard || !m_dashboard->isVisible()) {
        // Manual mouse control
        sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
        float delta = static_cast<float>(mousePos.x - m_windowCenter.x);
        sf::Mouse::setPosition(m_windowCenter, m_window);
        
        float F = delta * Constants::SENSITIVITY;
        float M = m_cart.getMass();
        float xDDot = F / M;
        
        float effectiveXDDot = m_cart.update(dt, xDDot);
        
        if (m_pendulum) {
            m_pendulum->update(dt, effectiveXDDot, m_cart.getPivot());

        }
    }
}

void Application::updateNetworkVisualizer() {
    if (!m_networkVisualizer || !m_neatController) return;
    if (!m_neatController->isReplaying()) return;
    
    // Get current snapshot or latest generation
    const GenerationSnapshot* snapshot = m_neatController->getCurrentSnapshot();
    if (!snapshot) {
        snapshot = m_neatController->getHistory().getLatestGeneration();
    }
    
    if (!snapshot) return;
    
    int selectedIdx = m_neatController->getSelectedAgentIndex();
    if (selectedIdx < 0 || selectedIdx >= static_cast<int>(snapshot->genomes.size())) return;
    
    // Set the genome for visualization
    m_networkVisualizer->setGenome(&snapshot->genomes[selectedIdx]);
    
    // Get current state from live replay simulation
    const auto& replayAgent = m_neatController->getReplayAgent();
    
    float cartX = replayAgent.cartX;
    float theta = replayAgent.theta;
    float controlOutput = replayAgent.lastControlOutput;
    float thetaDot = replayAgent.thetaDot;
    
    float normalizedCartX = std::clamp(cartX / replayAgent.rightBound, -1.0f, 1.0f);
    float sinTheta = std::sin(theta);
    float cosTheta = std::cos(theta);
    constexpr float SIM_DT = 0.016f;
    float angularDisplacement = thetaDot * SIM_DT;
    
    std::vector<float> inputs = {
        normalizedCartX,
        sinTheta,
        cosTheta,
        angularDisplacement
    };
    
    m_networkVisualizer->updateValues(inputs, controlOutput);
}

void Application::drawInfoBox() {
    if (!m_infoBox) return;
    
    // info box container
    m_window.draw(*m_infoBox);
    
    if (!m_fontLoaded) return;
    
    sf::Vector2f boxPos = m_infoBox->getPosition();
    float boxX = boxPos.x + 15.0f;
    float boxY = boxPos.y + 12.0f;
    
    // Get generation info
    int totalGens = m_neatController ? m_neatController->getHistory().getGenerationCount() : 0;
    int currentGen = m_neatController ? m_neatController->getGeneration() : 0;
    
    // Title
    sf::Text titleText(m_font, "Generation Info", 13);
    titleText.setFillColor(Constants::COLOR_EMERALD_400);
    titleText.setStyle(sf::Text::Bold);
    titleText.setPosition({boxX, boxY});
    m_window.draw(titleText);
    
    // Total generations
    sf::Text totalLabel(m_font, "Total Generations: " + std::to_string(totalGens), 12);
    totalLabel.setFillColor(Constants::COLOR_ZINC_400);
    totalLabel.setPosition({boxX, boxY + 20.0f});
    m_window.draw(totalLabel);
    
    // Current generation
    sf::Text currentLabel(m_font, "Current Generation: " + std::to_string(currentGen - 1), 12);
    currentLabel.setFillColor(Constants::COLOR_ZINC_400);
    currentLabel.setPosition({boxX, boxY + 38.0f});
    m_window.draw(currentLabel);
}

// Draws the ruler duh
void Application::drawRuler() {
    for (const auto& tick : m_rulerTicks) {
        m_window.draw(tick);
    }
    
    for (const auto& label : m_rulerLabels) {
        m_window.draw(label);
    }
}

void Application::drawSimulationContainer() {
    if (!m_simContainer) return;
    
    m_window.draw(*m_simContainer);
}

void Application::drawLiveFitness() {
    if (!m_fontLoaded || !m_neatController || !m_neatController->isReplaying()) return;
    
    const auto& replayAgent = m_neatController->getReplayAgent();
    float currentFitness = replayAgent.fitness;
    
    sf::Vector2f containerPos = m_simContainer->getPosition();
    float centerX = containerPos.x + m_simContainer->getSize().x / 2.0f;
    float topY = containerPos.y + 35.0f;
    
    std::ostringstream ss;
    ss << "Fitness: " << std::fixed << std::setprecision(2) << currentFitness;
    
    sf::Text fitnessText(m_font, ss.str(), 16);
    fitnessText.setFillColor(Constants::COLOR_EMERALD_400);
    fitnessText.setStyle(sf::Text::Bold);
    
    sf::FloatRect textBounds = fitnessText.getLocalBounds();
    fitnessText.setPosition({centerX - textBounds.size.x / 2.0f, topY});
    
    m_window.draw(fitnessText);
    
    // Draw recent fitness gain with fade out
    if (m_recentFitnessGain > 0.01f && m_timeSinceLastFitnessGain < 2.5f) {
        float alpha = 255.0f;
        if (m_timeSinceLastFitnessGain > 2.0f) {
            float fadeProgress = (m_timeSinceLastFitnessGain - 2.0f) / 0.5f;
            alpha = 255.0f * (1.0f - fadeProgress);
        }
        
        std::ostringstream gainSs;
        gainSs << " +" << std::fixed << std::setprecision(2) << m_recentFitnessGain;
        
        sf::Text gainText(m_font, gainSs.str(), 14);
        sf::Color gainColor = Constants::COLOR_EMERALD_300;
        gainColor.a = static_cast<uint8_t>(alpha);
        gainText.setFillColor(gainColor);
        gainText.setStyle(sf::Text::Bold);
        

        float gainX = centerX + textBounds.size.x / 2.0f + 5.0f;
        gainText.setPosition({gainX, topY + 2.0f});
        
        m_window.draw(gainText);
    }
}

void Application::render() {
    m_window.clear(Constants::COLOR_BACKGROUND);
    

    bool isTraining = m_neatController && 
                      (m_neatController->getState() == NEATState::Simulating ||
                       m_neatController->isBatchTraining());
    
    if (!isTraining) {
        drawSimulationContainer();
        
        // "Track" or whatever I call the thing in the middle
        m_window.draw(m_centerLine);
        
        // Ruler in view
        drawRuler();
        
        if (m_neatEnabled && m_seeAllEnabled && m_neatController) {
            int selectedIdx = m_neatController->getSelectedAgentIndex();
            int popSize = m_neatController->getPopulationSize();
            
            for (int i = 0; i < popSize && i < static_cast<int>(m_ghostCarts.size()); ++i) {
                if (i == selectedIdx) continue;
                
                m_window.draw(m_ghostCarts[i]);
                if (m_ghostPendulums[i]) {
                    m_window.draw(*m_ghostPendulums[i]);
                }
            }
        }
        
        // Show selected agent
        m_window.draw(m_cart);
        if (m_pendulum) {
            m_window.draw(*m_pendulum);
        }
        
        if (m_networkVisualizer && m_neatEnabled && m_neatController && 
            m_neatController->isReplaying() && !m_dashboard->isFullscreen()) {
            m_window.draw(*m_networkVisualizer);
        }
        
        if (m_neatEnabled) {
            drawInfoBox();
        }
        
        // Live fitnes display during replay
        drawLiveFitness();
    }
    
    // UI
    if (m_fontLoaded) {

        if (!isTraining && m_modeText) {
            // m_window.draw(*m_modeText);
        }
        
        // dashboard
        if (m_dashboard && m_dashboard->isVisible()) {
            m_window.draw(*m_dashboard);
        }
        
        if (m_menu && m_menu->isVisible()) {
            m_window.draw(*m_menu);
        }
    }
    
    m_window.display();
}
