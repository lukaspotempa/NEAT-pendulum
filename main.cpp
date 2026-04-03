// Pendulum Simulator - Main Entry Point
// A physics simulation featuring single and double pendulums on a cart
// 
// Controls:
//   Mouse       - Move cart left/right
//   ESC         - Open/close menu
//   1           - Switch to single pendulum
//   2           - Switch to double pendulum
//   T           - Toggle trail visualization
//   R           - Reset simulation

#include "Application.hpp"
#include <iostream>
#include <unordered_map>


using namespace std;


int main() {
    Application app;
    app.run();
    return 0;
}
