#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <llama.h>

int main() {
    std::cout << "=== Godot Llama Grammar Test (Pure C++) ===" << std::endl;
    
    // Use C API directly without Godot bindings to avoid GDExtension initialization
    const char* model_path = "/home/deck/dev/ai-god/addons/godot_llama/models/gemma-2-2b-it.Q4_K_M.gguf";
    const char* prompt = "<start_of_turn>user\n"
                         "Always answer truthfully and correct.\n"
                         "\n"
                         "Current state:\n"
                         "\n"
                         "\n"
                         "User Prompt state:\n"
                         "Is the color of a Bannana yellow?<start_of_turn>model";
    
    // Initialize backend
    llama_backend_init();
    
    // Load model
    std::cout << "\n[1/4] Loading model: " << model_path << std::endl;
    llama_model_params mparams = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(model_path, mparams);
    if (!model) {
        std::cerr << "ERROR: Failed to load model" << std::endl;
        llama_backend_free();
        return 1;
    }
    std::cout << "✓ Model loaded" << std::endl;
    
    // Create context
    std::cout << "\n[2/4] Creating context..." << std::endl;
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;
    ctx_params.n_batch = 512;
    ctx_params.n_ubatch = 512;
    ctx_params.n_threads = 8;
    ctx_params.n_threads_batch = 8;
    
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) {
        std::cerr << "ERROR: Failed to create context" << std::endl;
        llama_model_free(model);
        llama_backend_free();
        return 1;
    }
    std::cout << "✓ Context created" << std::endl;
    
    // Tokenize and decode prompt
    std::cout << "\n[3/4] Processing prompt..." << std::endl;
    const llama_vocab* vocab = llama_model_get_vocab(model);
    
    int32_t n_tokens = llama_tokenize(vocab, prompt, std::strlen(prompt), nullptr, 0, true, true);
    if (n_tokens < 0) n_tokens = -n_tokens;
    
    std::vector<llama_token> prompt_tokens(n_tokens);
    llama_tokenize(vocab, prompt, std::strlen(prompt), prompt_tokens.data(), n_tokens, true, true);
    std::cout << "✓ Tokenized " << prompt_tokens.size() << " prompt tokens" << std::endl;
    
    // Decode prompt
    for (size_t i = 0; i < prompt_tokens.size(); i += ctx_params.n_batch) {
        size_t batch_size = std::min(static_cast<size_t>(ctx_params.n_batch), prompt_tokens.size() - i);
        std::vector<llama_pos> positions(batch_size);
        std::vector<int32_t> n_seq_id(batch_size, 1);
        std::vector<llama_seq_id> seq_ids(batch_size, 0);
        std::vector<llama_seq_id*> seq_id_ptrs(batch_size);
        std::vector<int8_t> logits(batch_size, 0);
        
        for (size_t j = 0; j < batch_size; j++) {
            positions[j] = i + j;
            seq_id_ptrs[j] = &seq_ids[j];
        }
        logits[batch_size - 1] = 1;
        
        llama_batch batch = {};
        batch.n_tokens = batch_size;
        batch.token = prompt_tokens.data() + i;
        batch.pos = positions.data();
        batch.n_seq_id = n_seq_id.data();
        batch.seq_id = seq_id_ptrs.data();
        batch.logits = logits.data();
        
        if (llama_decode(ctx, batch) != 0) {
            std::cerr << "ERROR: Failed to decode prompt batch" << std::endl;
            llama_free(ctx);
            llama_model_free(model);
            llama_backend_free();
            return 1;
        }
    }
    std::cout << "✓ Prompt decoded" << std::endl;
    
    // Initialize samplers
    std::cout << "\n[4/4] Initializing samplers with grammar..." << std::endl;
    const char* grammar_str = "root ::= \"yes\" | \"no\"";
    llama_sampler* grammar_sampler = llama_sampler_init_grammar(vocab, grammar_str, "root");
    if (!grammar_sampler) {
        std::cerr << "ERROR: Failed to create grammar sampler" << std::endl;
        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return 1;
    }
    
    llama_sampler_chain_params chain_params = llama_sampler_chain_default_params();
    llama_sampler* chain = llama_sampler_chain_init(chain_params);
    llama_sampler_chain_add(chain, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(chain, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(chain, llama_sampler_init_penalties(64, 1.05f, 0.0f, 0.0f));
    llama_sampler_chain_add(chain, llama_sampler_init_temp(0.05f));
    llama_sampler_chain_add(chain, llama_sampler_init_dist(12345));
    
    std::cout << "✓ Samplers initialized" << std::endl;
    
    // Generate with grammar
    std::cout << "\n[GENERATION] Generating tokens with grammar constraint...\n" << std::endl;
    std::string output;
    int max_tokens = 50;
    
    for (int i = 0; i < max_tokens; i++) {
        float* logits = llama_get_logits(ctx);
        if (!logits) {
            std::cerr << "ERROR: Failed to get logits" << std::endl;
            break;
        }
        
        int32_t n_vocab = llama_vocab_n_tokens(vocab);
        std::vector<llama_token_data> candidates(n_vocab);
        for (int j = 0; j < n_vocab; j++) {
            candidates[j] = {static_cast<llama_token>(j), logits[j], 0.0f};
        }
        
        llama_token_data_array cur_p = {candidates.data(), static_cast<size_t>(n_vocab), -1, false};
        
        // Apply grammar first
        llama_sampler_apply(grammar_sampler, &cur_p);
        
        // Apply main sampler chain
        llama_sampler_apply(chain, &cur_p);
        
        if (cur_p.selected < 0 || cur_p.selected >= static_cast<int32_t>(cur_p.size)) {
            std::cerr << "ERROR: Invalid token selection" << std::endl;
            break;
        }
        
        llama_token token = cur_p.data[cur_p.selected].id;
        
        // Get token text
        std::vector<char> piece(128);
        int32_t n_chars = llama_token_to_piece(vocab, token, piece.data(), piece.size(), 0, true);
        if (n_chars < 0) n_chars = -n_chars;
        
        std::cout << (i + 1) << ": '" << std::string(piece.data(), n_chars) << "' (id=" << token << ")\n";
        output.append(piece.data(), n_chars);
        
        // Check for end of sequence
        if (llama_vocab_is_eog(vocab, token)) {
            std::cout << "\n(end of sequence)" << std::endl;
            break;
        }
        
        // Accept and decode
        llama_sampler_accept(chain, token);
        llama_sampler_accept(grammar_sampler, token);
        
        std::vector<llama_token> next_token = {token};
        std::vector<llama_pos> positions = {static_cast<llama_pos>(prompt_tokens.size() + i)};
        std::vector<int32_t> n_seq_id = {1};
        std::vector<llama_seq_id> seq_ids = {0};
        std::vector<llama_seq_id*> seq_id_ptrs = {&seq_ids[0]};
        std::vector<int8_t> logits_flags = {1};
        
        llama_batch decode_batch = {};
        decode_batch.n_tokens = 1;
        decode_batch.token = next_token.data();
        decode_batch.pos = positions.data();
        decode_batch.n_seq_id = n_seq_id.data();
        decode_batch.seq_id = seq_id_ptrs.data();
        decode_batch.logits = logits_flags.data();
        
        if (llama_decode(ctx, decode_batch) != 0) {
            std::cerr << "ERROR: Failed to decode token" << std::endl;
            break;
        }
    }
    
    std::cout << "\n=== RESULT ===\n" << output << std::endl;
    
    // Cleanup
    llama_sampler_free(grammar_sampler);
    llama_sampler_free(chain);
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();
    
    std::cout << "\n✓ Test completed successfully" << std::endl;
    return 0;
}