#pragma once
#include <functional>

enum class Layer { INPUT, HIDDEN, OUTPUT };

using ActivationFn = std::function<double(double)>;

class Node {
public:
    Node(Layer layer, int id, ActivationFn activation, float bias = 0.0f)
        : layer(layer), id(id), activation(std::move(activation)), bias(bias) {
    }

    Node copy() const {
        return Node(layer, id, activation, bias);
    }

    double activate(double x) const {
        return activation(x + bias);
    }

    int getId() const { return id; }
    Layer getLayer() const { return layer; }
    float getBias() const { return bias; }
    const ActivationFn& getActivation() const { return activation; }

    void setBias(float val) { bias = val; }

private:
    Layer layer;
    int id;
    ActivationFn activation;
    float bias;
};