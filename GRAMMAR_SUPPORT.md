# Grammar Sampler Support - Implementation Guide

## Overview
Grammar support has been added to the `LlamaContext` class to constrain token generation according to GBNF (GGML Backus-Naur Form) grammar rules. This implementation mirrors the llama-cli approach and integrates seamlessly with the existing sampler chain.

## Changes Made

### Modified Files
- **`src/llama_context.cpp`** - Added grammar sampler initialization in `_generate_internal()` method

## Implementation Details

### Grammar Initialization
The grammar sampler is initialized in the `_generate_internal()` method when processing generation parameters:

```cpp
// Add grammar sampler first if grammar is provided
if (grammar != nullptr && model != nullptr) {
    llama_sampler * grammar_sampler = llama_sampler_init_grammar(
        model->get_vocab(),
        grammar,
        grammar_root != nullptr ? grammar_root : "root"
    );
    if (grammar_sampler != nullptr) {
        llama_sampler_chain_add(native_sampler, grammar_sampler);
    }
}
```

### Key Features
1. **Parameter Parsing**: Grammar and grammar_root are extracted from `p_params` Dictionary:
   ```gdscript
   params = {
       "grammar": "<GBNF grammar rules>",
       "grammar_root": "root",  # optional, defaults to "root"
       # ... other sampling parameters
   }
   ```

2. **Sampler Chain Position**: The grammar sampler is added **first** in the sampler chain, before other samplers (top-k, top-p, temperature, etc.). This ensures grammar constraints are applied before probabilistic sampling.

3. **Error Handling**: If grammar initialization fails, it gracefully continues without the grammar constraint.

4. **Vocabulary**: Uses the model's vocabulary for parsing the grammar rules.

## Usage

### From Godot GDScript
```gdscript
var model = LlamaModel.new()
model.load_model("path/to/model.gguf")

var context = LlamaContext.new()
context.create(model)
context.set_prompt("What is 2+2? Respond with JSON: ")

var params = {
    "max_tokens": 100,
    "temperature": 0.7,
    "grammar": """
        root   ::= object
        value  ::= object | array | string | number | ("true" | "false" | "null") ws
        object ::= "{" ws (string ":" ws value ("," ws string ":" ws value)*)? "}" ws
        array  ::= "[" ws (value ("," ws value)*)? "]" ws
        string ::= "\\"" ([^"\\\\] | "\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\\"" ws
        number ::= ("-"? ([0-9] | [1-9] [0-9]*)) ("." [0-9]+)? ([eE] [-+]? [0-9]+)? ws
        ws     ::= ([ \\t\\n] ws)?
    """,
    "grammar_root": "root"
}

var result = context.generate(100, params)
print(result)
```

## GBNF Grammar Reference

Grammar rules follow the GGML Backus-Naur Form (GBNF) syntax:

- `rule ::= item1 | item2 | ...` - Alternation (OR)
- `rule ::= item1 item2 ...` - Sequence (concatenation)
- `item ::= "literal"` - Literal string
- `item ::= [abc]` - Character class
- `item ::= [a-z]` - Character range
- `item ::= rule` - Reference to another rule
- `item ::= rule?` - Optional (0 or 1)
- `item ::= rule*` - Zero or more
- `item ::= rule+` - One or more

### Common Grammar Examples

#### JSON Output
```
root   ::= object
value  ::= object | array | string | number | ("true" | "false" | "null") ws
object ::= "{" ws (string ":" ws value ("," ws string ":" ws value)*)? "}" ws
array  ::= "[" ws (value ("," ws value)*)? "]" ws
string ::= "\\"" ([^"\\\\] | "\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\\"" ws
number ::= ("-"? ([0-9] | [1-9] [0-9]*)) ("." [0-9]+)? ([eE] [-+]? [0-9]+)? ws
ws     ::= ([ \\t\\n] ws)?
```

#### Yes/No Response
```
root ::= ("yes" | "no")
```

#### Number Output
```
root ::= [0-9]+
```

## Comparison with llama-cli Implementation

The implementation follows the same pattern as `common/sampling.cpp`:

| Aspect | llama-cli | godot-llama |
|--------|-----------|-------------|
| Grammar Initialization | `llama_sampler_init_grammar()` | `llama_sampler_init_grammar()` |
| Vocabulary Source | `llama_model_get_vocab()` | `model->get_vocab()` |
| Root Rule | Configurable, defaults to "root" | Configurable, defaults to "root" |
| Sampler Chain Position | First in chain | First in chain |
| Parameter Passing | Via struct | Via Dictionary |
| Error Handling | Logged and skipped | Gracefully skipped |

## Technical Notes

1. **Grammar Parsing**: The grammar string is parsed by the llama.cpp library itself during sampler initialization
2. **Token Filtering**: The grammar sampler filters the token candidates at each generation step
3. **Performance**: Grammar constraints may slightly reduce generation speed due to additional token validation
4. **Memory**: Grammar state is maintained within the sampler and is automatically freed when the sampler is destroyed

## Future Enhancements

Potential improvements for future versions:
- Lazy grammar evaluation with trigger patterns/tokens
- Multiple grammar support
- Grammar pre-compilation for better performance
- Grammar validation/testing utilities
- Advanced prefill support for complex grammars

## Troubleshooting

### Grammar Not Being Applied
- Ensure `grammar` parameter is non-empty
- Verify the grammar syntax is valid GBNF
- Check that `grammar_root` rule exists in the grammar

### Performance Issues
- Complex grammars may impact generation speed
- Consider using simpler grammar rules
- Test grammar performance independently

### Invalid Output
- Grammar syntax errors may cause silent failures
- Validate grammar with external GBNF validators
- Check llama.cpp documentation for supported grammar features
