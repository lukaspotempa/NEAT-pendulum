#pragma once

#include <cmath>
#include "Constants.hpp"

namespace Physics {

    struct PendulumParams {
        float gravity = Constants::AGENT_GRAVITY;
        float pendulumDamping = Constants::AGENT_PENDULUM_DAMPING;
        float cartPendulumCoupling = Constants::AGENT_CART_PENDULUM_COUPLING;
        float pendulumLength = Constants::AGENT_PENDULUM_LENGTH;
    };

    // Default parameter sets
    inline PendulumParams agentParams() {
        return {
            Constants::AGENT_GRAVITY,
            Constants::AGENT_PENDULUM_DAMPING,
            Constants::AGENT_CART_PENDULUM_COUPLING,
            Constants::AGENT_PENDULUM_LENGTH
        };
    }

    inline PendulumParams manualParams() {
        return {
            Constants::MANUAL_GRAVITY,
            Constants::MANUAL_PENDULUM_DAMPING,
            Constants::MANUAL_CART_PENDULUM_COUPLING,
            200.0f 
        };
    }

    inline float singlePendulumAcceleration(float theta, float thetaDot, float aCart, const PendulumParams& params) {
        (void)thetaDot; 
        float s = std::sin(theta);
        float c = std::cos(theta);
        return -(params.gravity / params.pendulumLength) * s - (aCart / params.pendulumLength) * c;
    }

    inline void singlePendulumRK4Step(
        float& theta,
        float& thetaDot,
        float dt,
        float xDDot,
        const PendulumParams& params
    ) {
        float aCart = xDDot * params.cartPendulumCoupling;

        float k1_t, k1_w;
        float k2_t, k2_w;
        float k3_t, k3_w;
        float k4_t, k4_w;

        // k1
        k1_t = thetaDot;
        k1_w = singlePendulumAcceleration(theta, thetaDot, aCart, params);

        // k2
        k2_t = thetaDot + 0.5f * dt * k1_w;
        k2_w = singlePendulumAcceleration(theta + 0.5f * dt * k1_t, k2_t, aCart, params);

        // k3
        k3_t = thetaDot + 0.5f * dt * k2_w;
        k3_w = singlePendulumAcceleration(theta + 0.5f * dt * k2_t, k3_t, aCart, params);

        // k4
        k4_t = thetaDot + dt * k3_w;
        k4_w = singlePendulumAcceleration(theta + dt * k3_t, k4_t, aCart, params);

        // Update state
        theta += (dt / 6.0f) * (k1_t + 2.0f * k2_t + 2.0f * k3_t + k4_t);
        thetaDot += (dt / 6.0f) * (k1_w + 2.0f * k2_w + 2.0f * k3_w + k4_w);

        // Apply damping
        thetaDot *= params.pendulumDamping;
    }

    // Double Pendulum Physics
    struct DoublePendulumParams {
        float gravity = Constants::AGENT_GRAVITY;
        float pendulumDamping = Constants::AGENT_PENDULUM_DAMPING;
        float cartPendulumCoupling = Constants::AGENT_CART_PENDULUM_COUPLING;
        float L1 = 150.0f;  // Length of first pendulum
        float L2 = 125.0f;  // Length of second pendulum
        float m1 = 3.0f;    // Mass of first bob
        float m2 = 3.0f;    // Mass of second bob
    };

    inline DoublePendulumParams doublePendulumManualParams() {
        return {
            Constants::MANUAL_GRAVITY,
            Constants::MANUAL_PENDULUM_DAMPING,
            Constants::MANUAL_CART_PENDULUM_COUPLING,
            150.0f, 125.0f, 3.0f, 3.0f
        };
    }

    inline void doublePendulumAccelerations(
        float t1, float t2, float w1, float w2, float aCart,
        const DoublePendulumParams& params,
        float& alpha1, float& alpha2
    ) {
        float dTheta = t1 - t2;
        float s1 = std::sin(t1);
        float s2 = std::sin(t2);
        float sd = std::sin(dTheta);
        float cd = std::cos(dTheta);

        float M = params.m1 + params.m2;
        float g = params.gravity;

        float denom1 = params.L1 * (M - params.m2 * cd * cd);

        alpha1 = (-params.m2 * cd * (params.L1 * w1 * w1 * sd - g * s2)
                  - params.m2 * params.L2 * w2 * w2 * sd
                  - M * g * s1
                  - M * aCart * std::cos(t1)) / denom1;

        float denom2 = params.L2 * (M - params.m2 * cd * cd);
        alpha2 = (params.m2 * cd * (params.L2 * w2 * w2 * sd + g * s1)
                  + M * params.L1 * w1 * w1 * sd
                  - M * g * s2
                  - params.m2 * aCart * std::cos(t2) * cd
                  + M * aCart * std::cos(t1) * cd
                  - M * aCart * std::cos(t2)) / denom2;
    }

    inline void doublePendulumRK4Step(
        float& theta1, float& theta2,
        float& theta1Dot, float& theta2Dot,
        float dt,
        float xDDot,
        const DoublePendulumParams& params
    ) {
        float aCart = xDDot * params.cartPendulumCoupling;

        float k1_t1, k1_t2, k1_w1, k1_w2;
        float k2_t1, k2_t2, k2_w1, k2_w2;
        float k3_t1, k3_t2, k3_w1, k3_w2;
        float k4_t1, k4_t2, k4_w1, k4_w2;
        float a1, a2;

        // k1
        k1_t1 = theta1Dot;
        k1_t2 = theta2Dot;
        doublePendulumAccelerations(theta1, theta2, theta1Dot, theta2Dot, aCart, params, a1, a2);
        k1_w1 = a1;
        k1_w2 = a2;

        // k2
        k2_t1 = theta1Dot + 0.5f * dt * k1_w1;
        k2_t2 = theta2Dot + 0.5f * dt * k1_w2;
        doublePendulumAccelerations(
            theta1 + 0.5f * dt * k1_t1, theta2 + 0.5f * dt * k1_t2,
            k2_t1, k2_t2, aCart, params, a1, a2);
        k2_w1 = a1;
        k2_w2 = a2;

        // k3
        k3_t1 = theta1Dot + 0.5f * dt * k2_w1;
        k3_t2 = theta2Dot + 0.5f * dt * k2_w2;
        doublePendulumAccelerations(
            theta1 + 0.5f * dt * k2_t1, theta2 + 0.5f * dt * k2_t2,
            k3_t1, k3_t2, aCart, params, a1, a2);
        k3_w1 = a1;
        k3_w2 = a2;

        // k4
        k4_t1 = theta1Dot + dt * k3_w1;
        k4_t2 = theta2Dot + dt * k3_w2;
        doublePendulumAccelerations(
            theta1 + dt * k3_t1, theta2 + dt * k3_t2,
            k4_t1, k4_t2, aCart, params, a1, a2);
        k4_w1 = a1;
        k4_w2 = a2;

        // Update state
        theta1 += (dt / 6.0f) * (k1_t1 + 2.0f * k2_t1 + 2.0f * k3_t1 + k4_t1);
        theta2 += (dt / 6.0f) * (k1_t2 + 2.0f * k2_t2 + 2.0f * k3_t2 + k4_t2);
        theta1Dot += (dt / 6.0f) * (k1_w1 + 2.0f * k2_w1 + 2.0f * k3_w1 + k4_w1);
        theta2Dot += (dt / 6.0f) * (k1_w2 + 2.0f * k2_w2 + 2.0f * k3_w2 + k4_w2);

        // Apply damping
        theta1Dot *= params.pendulumDamping;
        theta2Dot *= params.pendulumDamping;
    }

} // namespace Physics
