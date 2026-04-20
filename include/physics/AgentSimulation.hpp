#pragma once

#include <vector>
#include <cmath>
#include "Constants.hpp"
#include "Physics.hpp"

// Represents the state of a single agent simulation (cart + pendulum)
struct AgentSimulation {
    // Cart state
    float cartX = 0.0f;         
    float cartVel = 0.0f;        
    
    // Single pendulum state
    float theta = 0.1f;    // Angle: 0 = down; pi = upright
    float thetaDot = 0.0f; // Angular velocity
    
    float pendulumLength = Constants::AGENT_PENDULUM_LENGTH;
    float cartMass = 5.0f;
    float cartDamping = Constants::AGENT_CART_DAMPING;
    float pendulumDamping = Constants::AGENT_PENDULUM_DAMPING;
    
    float leftBound = -750.0f;
    float rightBound = 750.0f;
    
    float fitness = 0.0f;
    float simulationTime = 0.0f;
    bool done = false;
    
    float lastControl = 0.0f;
    float controlChangeSum = 0.0f;
    float absControlSum = 0.0f;  
    
    // Timer-based reward tracking
    float uprightTimer = 0.0f; 
    int completedUprightSeconds = 0;  // seconds completed upright
    
    float lastControlOutput = 0.0f;
    
    static constexpr float MAX_SIMULATION_TIME = 100.0f;
    static constexpr float PI = 3.14159265f;
    static constexpr float UPRIGHT_THRESHOLD = -0.95f;
    static constexpr float REWARD_INTERVAL = 1.0f;  // Second interval for reward
    
    void reset() {
        cartX = 0.0f;
        cartVel = 0.0f;
        theta = 0.1f; // 0 down, pi up
        thetaDot = 0.0f;
        fitness = 0.0f;
        simulationTime = 0.0f;
        done = false;
        lastControl = 0.0f;
        controlChangeSum = 0.0f;
        absControlSum = 0.0f;
        lastControlOutput = 0.0f;
        uprightTimer = 0.0f;
        completedUprightSeconds = 0;
    }
    
    void step(float dt, float control, bool trackFitness = true) {
        if (done) return;
        
        // Store control output for visualization
        lastControlOutput = control;
        
        // Track control changes for fitness penalty
        float controlDelta = std::abs(control - lastControl);
        controlChangeSum += controlDelta;
        absControlSum += std::abs(control);
        lastControl = control;
        
        constexpr float MAX_ACCEL = 6000.0f;
        float xDDot = control * MAX_ACCEL;
        
        // Clamp acceleration at bounds
        if (cartX <= leftBound && xDDot < 0.0f) {
            xDDot = 0.0f;
        } else if (cartX >= rightBound && xDDot > 0.0f) {
            xDDot = 0.0f;
        }
        
        cartVel += xDDot * dt;
        cartVel *= cartDamping;
        
        float newX = cartX + cartVel * dt;
        float clampedX = std::clamp(newX, leftBound, rightBound);
        
        if (newX != clampedX) {
            cartVel = 0.0f;
        }
        cartX = clampedX;
        
        Physics::PendulumParams params = Physics::agentParams();
        params.pendulumLength = pendulumLength;
        params.pendulumDamping = pendulumDamping;
        
        Physics::singlePendulumRK4Step(theta, thetaDot, dt, xDDot, params);
        

        simulationTime += dt;
        
        if (trackFitness) {

            float cosTheta = std::cos(theta);
            

            if (cosTheta < UPRIGHT_THRESHOLD) {


                uprightTimer += dt;
                // Check if completed full second
                int newCompletedSeconds = static_cast<int>(uprightTimer / REWARD_INTERVAL);
                if (newCompletedSeconds > completedUprightSeconds) {
                    // Award points for each completed second
                    
                    int secondsToReward = newCompletedSeconds - completedUprightSeconds;
                    
                    float normalizedPos = cartX / rightBound;
                    float centerBonus = std::abs(1.0f - std::abs(normalizedPos));
                    
                    float stabilityBonus = 1.0f / (1.0f + std::abs(thetaDot) + std::abs(cartVel) * 0.001f);

                    float baseReward = 1.0f;
                    
                    fitness += baseReward * centerBonus * stabilityBonus;
                    // 1.0 / (1.0 + out_sum * 0.5) * dist_to_center_penalty;
                    completedUprightSeconds = newCompletedSeconds;
                }
            } else {
                // Pendulum fell below threshold
                uprightTimer = 0.0f;
                completedUprightSeconds = 0;
            }
        }
        
        // Check for simulation end
        if (simulationTime >= MAX_SIMULATION_TIME) {
            done = true;
        }
    }
    
    // Check if pendulum is close to upright
    bool isUpright() const {
        static constexpr float UPRIGHT_THRESHOLD_STRICT = 0.996f; 
        float cosTheta = std::cos(theta);
        return cosTheta < -UPRIGHT_THRESHOLD_STRICT;
    }
    
    bool hasFallen() const {
        static constexpr float FALL_THRESHOLD = 0.5f;
        float cosTheta = std::cos(theta);
        return cosTheta > FALL_THRESHOLD;
    }
    
    // Get world cart position (for rendering)
    float getWorldCartX() const {
        return Constants::WINDOW_WIDTH / 2.0f + cartX - 60.0f; // Cart half widt offset
    }
    
    sf::Vector2f getBobOffset() const {
        return {
            pendulumLength * std::sin(theta),
            pendulumLength * std::cos(theta)
        };
    }
};
