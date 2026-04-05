#pragma once
#include <functional>
#include <string>
#include <sstream>
#include <cmath>

enum class Layer { INPUT, HIDDEN, OUTPUT };

// Activation function identifier
enum class ActivationType { IDENTITY, RELU, SIGMOID, TANH };

using ActivationFn = std::function<double(double)>;

//activation functions
inline double identity(double x) { return x; }
inline double relu(double x) { return x > 0.0 ? x : 0.0; }
inline double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }
inline double tanhActivation(double x) { return std::tanh(x); }

class Node {
public:
    Node(Layer layer, int id, ActivationFn activation, float bias = 0.0f, ActivationType actType = ActivationType::IDENTITY)
        : layer(layer), id(id), activation(std::move(activation)), bias(bias), activationType(actType) {
    }

    Node copy() const {
        return Node(layer, id, activation, bias, activationType);
    }

    double activate(double x) const {
        return activation(x + bias);
    }

    int getId() const { return id; }
    Layer getLayer() const { return layer; }
    float getBias() const { return bias; }
    const ActivationFn& getActivation() const { return activation; }
    ActivationFn getActivationFn() const { return activation; } 
    ActivationType getActivationType() const { return activationType; }

    void setBias(float val) { bias = val; }
    void setActivationType(ActivationType type) { activationType = type; }

    std::string toJson() const {
        std::ostringstream ss;
        ss << "{\"id\":" << id 
           << ",\"layer\":" << static_cast<int>(layer)
           << ",\"bias\":" << bias 
           << ",\"act\":" << static_cast<int>(activationType) << "}";
        return ss.str();
    }

    static Node fromJson(const std::string& json) {
        int nodeId = 0;
        int layerInt = 0;
        float nodeBias = 0.0f;
        int actInt = 0;
        
        size_t pos = json.find("\"id\":");
        if (pos != std::string::npos) nodeId = std::stoi(json.substr(pos + 5));
        
        pos = json.find("\"layer\":");
        if (pos != std::string::npos) layerInt = std::stoi(json.substr(pos + 8));
        
        pos = json.find("\"bias\":");
        if (pos != std::string::npos) nodeBias = std::stof(json.substr(pos + 7));
        
        pos = json.find("\"act\":");
        if (pos != std::string::npos) actInt = std::stoi(json.substr(pos + 6));
        
        Layer nodeLayer = static_cast<Layer>(layerInt);
        ActivationType actType = static_cast<ActivationType>(actInt);
        ActivationFn actFn = getActivationFunction(actType);
        
        return Node(nodeLayer, nodeId, actFn, nodeBias, actType);
    }

    static ActivationFn getActivationFunction(ActivationType type) {
        switch (type) {
            case ActivationType::RELU: return relu;
            case ActivationType::SIGMOID: return sigmoid;
            case ActivationType::TANH: return tanhActivation;
            case ActivationType::IDENTITY:
            default: return identity;
        }
    }

private:
    Layer layer;
    int id;
    ActivationFn activation;
    float bias;
    ActivationType activationType = ActivationType::IDENTITY;
};