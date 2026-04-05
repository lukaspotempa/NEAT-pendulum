#pragma once

#include <string>
#include <sstream>

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

	std::string toJson() const {
		std::ostringstream ss;
		ss << "{\"in\":" << nodeIn 
		   << ",\"out\":" << nodeOut 
		   << ",\"w\":" << weight 
		   << ",\"i\":" << innovation 
		   << ",\"e\":" << (enabled ? "true" : "false") << "}";
		return ss.str();
	}

	static ConnectionGene fromJson(const std::string& json) {
		int in = 0, out = 0, innov = 0;
		float w = 0.0f;
		bool en = true;
		
		size_t pos = json.find("\"in\":");
		if (pos != std::string::npos) in = std::stoi(json.substr(pos + 5));
		
		pos = json.find("\"out\":");
		if (pos != std::string::npos) out = std::stoi(json.substr(pos + 6));
		
		pos = json.find("\"w\":");
		if (pos != std::string::npos) w = std::stof(json.substr(pos + 4));
		
		pos = json.find("\"i\":");
		if (pos != std::string::npos) innov = std::stoi(json.substr(pos + 4));
		
		pos = json.find("\"e\":");
		if (pos != std::string::npos) en = (json.substr(pos + 4, 4) == "true");
		
		return ConnectionGene(in, out, w, innov, en);
	}

private:
	int nodeIn;
	int nodeOut;
	float weight;
	int innovation;
	bool enabled;
};