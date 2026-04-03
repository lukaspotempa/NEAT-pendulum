#pragma once

class ConnectionGene {
public:
	ConnectionGene(int nodeIn, int nodeOut, float weight, int innovation, bool enabled = true)
		: nodeIn(nodeIn), nodeOut(nodeOut), weight(weight), innovation(innovation), enabled(enabled) {}

	ConnectionGene copy() const {
		return ConnectionGene(nodeIn, nodeOut, weight, innovation, enabled);
	}

	bool isEnabled() const { return enabled; }
	int getInNodeId() const { return nodeIn; }
	int getOutNodeId() const { return nodeOut; }
	float getWeight() const { return weight; }
	int getInnovation() const { return innovation; }

	void setEnabled(bool val) { enabled = val; }
	void setWeight(float val) { weight = val; }

private:
	int nodeIn;
	int nodeOut;
	float weight;
	int innovation;
	bool enabled;
};