# Grammar Support Examples

This file contains practical examples of using grammar-constrained generation with the godot-llama extension.

## Basic Example: JSON Output

```gdscript
extends Node

var model: LlamaModel
var context: LlamaContext

func _ready():
	model = LlamaModel.new()
	model.load_model("res://models/mistral-7b.gguf")
	
	context = LlamaContext.new()
	context.create(model)

func generate_json_response(user_query: String):
	var json_grammar = """
		root   ::= object
		value  ::= object | array | string | number | ("true" | "false" | "null") ws
		object ::= "{" ws (string ":" ws value ("," ws string ":" ws value)*)? "}" ws
		array  ::= "[" ws (value ("," ws value)*)? "]" ws
		string ::= "\\"" ([^"\\\\] | "\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\\"" ws
		number ::= ("-"? ([0-9] | [1-9] [0-9]*)) ("." [0-9]+)? ([eE] [-+]? [0-9]+)? ws
		ws     ::= ([ \\t\\n] ws)?
	"""
	
	context.set_prompt("User: %s\n\nRespond with JSON: " % user_query)
	
	var params = {
		"max_tokens": 200,
		"temperature": 0.7,
		"grammar": json_grammar,
		"grammar_root": "root"
	}
	
	var response = context.generate(200, params)
	print("Generated JSON:", response)
	return response
```

## Structured Data: Yes/No Responses

```gdscript
func generate_yes_no(question: String) -> String:
	var grammar = """
		root ::= ("yes" | "no") ws
		ws   ::= [ \\t\\n]*
	"""
	
	context.set_prompt("Question: %s\n\nAnswer: " % question)
	
	var params = {
		"max_tokens": 10,
		"temperature": 0.1,
		"grammar": grammar
	}
	
	return context.generate(10, params)
```

## List Generation

```gdscript
func generate_list(topic: String) -> PackedStringArray:
	var grammar = """
		root        ::= item ("," ws item)* ws
		item        ::= "- " [^,\\n]+
		ws          ::= [ \\t\\n]*
	"""
	
	context.set_prompt("List of %s:\n" % topic)
	
	var params = {
		"max_tokens": 150,
		"temperature": 0.8,
		"grammar": grammar
	}
	
	var response = context.generate(150, params)
	var items = response.split("\\n")
	return PackedStringArray(items)
```

## Math Expressions

```gdscript
func generate_math_expression() -> String:
	var grammar = """
		root       ::= expr
		expr       ::= term ((add | sub) term)*
		term       ::= factor ((mul | div) factor)*
		factor     ::= number | "(" expr ")"
		
		add        ::= ws "+" ws
		sub        ::= ws "-" ws
		mul        ::= ws "*" ws
		div        ::= ws "/" ws
		
		number     ::= [0-9]+ ("." [0-9]+)?
		ws         ::= ([ \\t])*
	"""
	
	context.set_prompt("Solve: ")
	
	var params = {
		"max_tokens": 50,
		"grammar": grammar
	}
	
	return context.generate(50, params)
```

## Email Format

```gdscript
func generate_email() -> String:
	var grammar = """
		root    ::= "To: " email "\\n" "Subject: " subject "\\n\\n" body
		email   ::= [a-zA-Z0-9._-]+ "@" [a-zA-Z0-9.-]+ "\\n"
		subject ::= [^\\n]+ "\\n"
		body    ::= [^\\x00]+
	"""
	
	context.set_prompt("Generate an email response. ")
	
	var params = {
		"max_tokens": 300,
		"temperature": 0.7,
		"grammar": grammar
	}
	
	return context.generate(300, params)
```

## Code Generation: Python Function

```gdscript
func generate_python_function(description: String) -> String:
	var grammar = """
		root       ::= "def " name "(" params "):" body
		name       ::= [a-z_][a-zA-Z0-9_]*
		params     ::= param ("," ws param)* | ""
		param      ::= [a-z_][a-zA-Z0-9_]*
		body       ::= "\\n" indent statement+
		statement  ::= indent [^\\n]* "\\n"
		indent     ::= "    "
		ws         ::= [ \\t]*
	"""
	
	context.set_prompt("Generate Python function: %s\\n" % description)
	
	var params = {
		"max_tokens": 200,
		"temperature": 0.5,
		"grammar": grammar
	}
	
	return context.generate(200, params)
```

## Streaming with Grammar

```gdscript
func generate_json_stream(user_query: String):
	var json_grammar = """
		root   ::= object
		value  ::= object | array | string | number | ("true" | "false" | "null") ws
		object ::= "{" ws (string ":" ws value ("," ws string ":" ws value)*)? "}" ws
		array  ::= "[" ws (value ("," ws value)*)? "]" ws
		string ::= "\\"" ([^"\\\\] | "\\\\" (["\\\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\\"" ws
		number ::= ("-"? ([0-9] | [1-9] [0-9]*)) ("." [0-9]+)? ([eE] [-+]? [0-9]+)? ws
		ws     ::= ([ \\t\\n] ws)?
	"""
	
	context.set_prompt("User: %s\n\nRespond with JSON: " % user_query)
	
	# Connect to signals
	context.token_generated.connect(_on_token_generated)
	context.generation_finished.connect(_on_generation_finished)
	
	var params = {
		"max_tokens": 200,
		"grammar": json_grammar
	}
	
	context.generate_stream(200, params)

func _on_token_generated(token_text: String, token_id: int):
	print("Token: ", token_text)

func _on_generation_finished(full_text: String):
	print("Complete response: ", full_text)
```

## Complex Nested Structure

```gdscript
func generate_config_file() -> String:
	var grammar = """
		root       ::= entry+
		entry      ::= key_value | section
		section    ::= "[" [a-zA-Z_]+ "]" "\\n" entry*
		key_value  ::= key "=" value "\\n"
		key        ::= [a-zA-Z_][a-zA-Z0-9_]*
		value      ::= string | number | boolean
		string     ::= "\\"" [^\\"]* "\\""
		number     ::= "-"? [0-9]+ ("." [0-9]+)?
		boolean    ::= "true" | "false"
	"""
	
	context.set_prompt("Generate configuration file content:\n")
	
	var params = {
		"max_tokens": 500,
		"temperature": 0.3,
		"grammar": grammar
	}
	
	return context.generate(500, params)
```

## CSV Format

```gdscript
func generate_csv(columns: PackedStringArray, rows: int) -> String:
	var grammar = """
		root    ::= header row+
		header  ::= field ("," field)* "\\n"
		row     ::= field ("," field)* "\\n"
		field   ::= "\\"" [^\\"]* "\\"" | [^,\\n]+
	"""
	
	var prompt = "Generate CSV with columns: %s (%d rows)\\n" % [", ".join(columns), rows]
	context.set_prompt(prompt)
	
	var params = {
		"max_tokens": 200 * rows,
		"temperature": 0.7,
		"grammar": grammar
	}
	
	return context.generate(200 * rows, params)
```

## Tips for Writing Effective Grammars

1. **Start Simple**: Begin with a basic grammar and gradually add complexity
2. **Test Thoroughly**: Validate grammar against expected outputs
3. **Use Whitespace Carefully**: Define `ws` (whitespace) rules explicitly
4. **Escape Strings**: Remember to escape quotes and special characters
5. **Performance**: Simpler grammars perform better; avoid overly complex nested rules
6. **Default Root**: Always ensure your grammar has a `root` rule or specify `grammar_root`

## Debugging Tips

```gdscript
# Enable error signals
context.generation_error.connect(_on_generation_error)

func _on_generation_error(message: String):
	print("Generation error: ", message)

# Check if context is ready
if context.is_initialized():
	print("Context is ready for generation")
else:
	print("Context not initialized")

# Verify model is loaded
if context.get_model().is_loaded():
	print("Model loaded successfully")
else:
	print("Model failed to load")
```

## Performance Notes

- Grammar constraints add computational overhead to each token generation
- Simpler grammars execute faster
- Consider token budget vs. generation quality
- Use `reuse_kv` parameter to speed up multiple generations with same context
- Profile with different grammar complexities to find optimal balance
