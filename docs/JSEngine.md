# JavaScript Engine Documentation

## Overview

The custom JavaScript engine (`custom_js`) implements a subset of ECMAScript with lexing, parsing, AST generation, and interpretation. It provides basic JavaScript functionality for web pages.

## Architecture

### Core Components

```cpp
namespace browser::custom_js {
    class JSLexer {
        // Tokenizes JavaScript source code
        std::vector<Token> tokenize(const std::string& source);
    };
    
    class JSParser {
        // Parses tokens into Abstract Syntax Tree
        std::shared_ptr<Program> parse(const std::string& source);
    };
    
    class JSInterpreter {
        // Executes AST nodes
        JSValue execute(std::shared_ptr<Program> program);
    };
    
    class JSEngine {
        // High-level API for JavaScript execution
        bool executeScript(const std::string& script, 
                          std::string& result, 
                          std::string& error);
    };
}
```

## Lexical Analysis

### Token Types

```cpp
enum class TokenType {
    // Literals
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords
    VAR, LET, CONST, FUNCTION, RETURN,
    IF, ELSE, FOR, WHILE, DO,
    TRUE, FALSE, NULL_TOKEN, UNDEFINED,
    THIS, NEW,
    
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL, EQUAL_EQUAL, EQUAL_EQUAL_EQUAL,
    BANG, BANG_EQUAL, BANG_EQUAL_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AMPERSAND_AMPERSAND, PIPE_PIPE,
    
    // Delimiters
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, SEMICOLON, COLON,
    
    EOF_TOKEN
};
```

### Tokenization Example

```javascript
// Input
var x = 42;

// Tokens
VAR "var"
IDENTIFIER "x"
EQUAL "="
NUMBER "42"
SEMICOLON ";"
```

## Syntax Analysis

### AST Node Types

```cpp
// Expression nodes
class LiteralExpr : public ExpressionNode {
    LiteralType literalType;  // NUMBER, STRING, BOOLEAN, etc.
    std::string value;
};

class BinaryExpr : public ExpressionNode {
    Operator op;              // PLUS, MINUS, etc.
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
};

// Statement nodes
class VariableStmt : public StatementNode {
    DeclarationType declarationType;  // VAR, LET, CONST
    std::string name;
    std::shared_ptr<ExpressionNode> initializer;
};

class IfStmt : public StatementNode {
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> thenBranch;
    std::shared_ptr<StatementNode> elseBranch;
};
```

### Grammar Rules

```
program         → declaration* EOF
declaration     → varDecl | funDecl | statement
statement       → exprStmt | ifStmt | whileStmt | forStmt | 
                  returnStmt | block
expression      → assignment
assignment      → logicalOr ( "=" assignment )?
logicalOr       → logicalAnd ( "||" logicalAnd )*
logicalAnd      → equality ( "&&" equality )*
equality        → comparison ( ("==" | "!=") comparison )*
comparison      → term ( (">" | ">=" | "<" | "<=") term )*
term            → factor ( ("+" | "-") factor )*
factor          → unary ( ("*" | "/" | "%") unary )*
unary           → ("!" | "-" | "++" | "--") unary | call
call            → primary ( "(" arguments? ")" | "." IDENTIFIER )*
primary         → NUMBER | STRING | "true" | "false" | "null" | 
                  "undefined" | IDENTIFIER | "(" expression ")"
```

## Value System

### JSValue Types

```cpp
enum class JSValueType {
    UNDEFINED,
    NULL_TYPE,
    BOOLEAN,
    NUMBER,
    STRING,
    OBJECT,
    FUNCTION,
    ARRAY
};

class JSValue {
    // Type conversions
    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;
    std::shared_ptr<JSObject> toObject() const;
};
```

### Type Coercion

```cpp
// Truthiness
JSValue(undefined) → false
JSValue(null) → false
JSValue(0) → false
JSValue("") → false
JSValue(NaN) → false

// String concatenation
JSValue(5) + JSValue("5") → "55"

// Numeric operations
JSValue("5") - JSValue(2) → 3
```

## Execution Environment

### Variable Scoping

```cpp
class Environment {
    std::map<std::string, JSValue> m_values;
    std::shared_ptr<Environment> m_enclosing;
    
public:
    void define(const std::string& name, const JSValue& value);
    JSValue get(const std::string& name);
    void assign(const std::string& name, const JSValue& value);
};
```

### Function Execution

```cpp
// JavaScript function
function add(a, b) {
    return a + b;
}

// Internal representation
auto function = std::make_shared<JSFunction>(
    [](const std::vector<JSValue>& args, JSValue thisValue) {
        if (args.size() >= 2) {
            return JSValue(args[0].toNumber() + args[1].toNumber());
        }
        return JSValue(); // undefined
    }
);
```

## Built-in Objects

### Global Functions

```javascript
// Type conversion
parseInt("42")        // 42
parseFloat("3.14")    // 3.14

// Type checking
isNaN(NaN)           // true
isFinite(Infinity)   // false

// Output
console.log("Hello") // Prints to console
```

### Math Object

```javascript
Math.PI              // 3.14159...
Math.E               // 2.71828...
Math.abs(-5)         // 5
Math.sqrt(16)        // 4
Math.min(1, 2, 3)    // 1
Math.max(1, 2, 3)    // 3
Math.random()        // 0.0 to 1.0
```

### Array Methods

```javascript
var arr = [1, 2, 3];
arr.length           // 3
arr.push(4)          // [1, 2, 3, 4]
arr.pop()            // 4
```

## DOM Integration

### Window Object

```cpp
// Create window object
auto windowObj = std::make_shared<JSObject>();

// Add to global scope
m_interpreter->defineGlobalVariable("window", JSValue(windowObj));

// Self-references
windowObj->set("window", JSValue(windowObj));
windowObj->set("self", JSValue(windowObj));
```

### Document Object

```javascript
// Accessible from JavaScript
document.getElementById("myId")
document.getElementsByTagName("div")
document.createElement("p")
```

## Usage Examples

### Basic Script Execution

```cpp
// Create engine
JSEngine engine;
engine.initialize();

// Execute script
std::string script = R"(
    var x = 10;
    var y = 20;
    x + y
)";

std::string result, error;
if (engine.executeScript(script, result, error)) {
    std::cout << "Result: " << result << std::endl; // "30"
} else {
    std::cout << "Error: " << error << std::endl;
}
```

### Adding Native Functions

```cpp
// Add custom function
engine.addGlobalFunction("alert", 
    [](const std::vector<JSValue>& args, JSValue thisValue) {
        if (!args.empty()) {
            std::cout << "Alert: " << args[0].toString() << std::endl;
        }
        return JSValue(); // undefined
    }
);
```

### Binding DOM Elements

```cpp
// Create DOM element wrapper
auto elementWrapper = std::make_shared<JSObject>();

// Add properties
elementWrapper->set("tagName", JSValue("DIV"));
elementWrapper->set("id", JSValue("myDiv"));

// Add methods
elementWrapper->set("getAttribute", JSValue(
    std::make_shared<JSFunction>(
        [element](const std::vector<JSValue>& args, JSValue) {
            if (!args.empty()) {
                std::string attrName = args[0].toString();
                return JSValue(element->getAttribute(attrName));
            }
            return JSValue();
        }
    )
));
```

## Error Handling

### Runtime Errors

```cpp
class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const std::string& message) 
        : std::runtime_error(message) {}
};

// Example errors
"Undefined variable 'x'"
"Cannot call non-function"
"Division by zero"
```

### Error Recovery

```cpp
try {
    JSValue result = m_interpreter->execute(script);
} catch (const RuntimeError& e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    // Continue execution or abort
}
```

## Limitations

1. **No Prototypes**: Prototype chain not implemented
2. **Limited Closures**: Basic closure support only
3. **No Async**: No promises, async/await
4. **No Modules**: No import/export
5. **Basic Error Handling**: No try/catch/finally
6. **No Regular Expressions**: RegExp not supported

## Performance Considerations

### Optimization Techniques

1. **Constant Folding**: Evaluate constant expressions at parse time
2. **Variable Caching**: Cache frequently accessed variables
3. **Inline Caching**: Cache property lookups

### Memory Management

- Reference counting for objects
- Garbage collection not implemented
- Manual cleanup of circular references

## Future Enhancements

1. **JIT Compilation**: Compile hot functions to native code
2. **Better Type System**: Add TypedArrays, WeakMap, etc.
3. **ES6+ Features**: Arrow functions, destructuring, classes
4. **Debugger Support**: Breakpoints, step execution
5. **Performance Profiler**: Identify bottlenecks