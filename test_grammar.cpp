#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <llama_context.h>
#include <llama_model.h>

using namespace godot;

int main() {
    std::cout << "=== Godot Llama Grammar Test ===" << std::endl;
    
    // Model path
    const char* model_path = "/home/deck/dev/ai-god/addons/godot_llama/models/gemma-2-2b-it.Q4_K_M.gguf";

	// Create model
	Ref<LlamaModel> model;
	
	model->load(model_path);

	if (!model.is_valid() || !model->is_loaded()) {
		std::cerr << "Failed to load model from path: " << model_path << std::endl;
		return 1;
	}

    std::cout << "✓ Done!" << std::endl;
    return 0;
}
