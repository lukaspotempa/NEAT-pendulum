enum layer {
	HIDDEN,
	INPUT,
	OUTPUT
};

class Node {
	public:
		Node(layer layer, int id, double activation, float bias) {
			this->layer = layer;
			this->id = id;
			this->activation = activation;
			this->bias = bias;
		}

	private:
		layer layer;
		int id;
		double activation;
		float bias;
};

