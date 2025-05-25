# HTML Parser Documentation

## Overview

The HTML parser converts HTML text into a Document Object Model (DOM) tree. It implements the HTML5 parsing algorithm with tokenization and tree construction phases.

## Architecture

### Core Classes

```cpp
namespace browser::html {
    class HTMLParser {
        // Main parser class
        DOMTree parse(const std::string& html);
    };
    
    class Node {
        // Base class for all DOM nodes
    };
    
    class Element : public Node {
        // HTML elements
    };
    
    class Document : public Node {
        // Document root
    };
}
```

## Parsing Process

### 1. Tokenization

The parser converts raw HTML into tokens:

```cpp
enum class TokenType {
    DOCTYPE,
    START_TAG,
    END_TAG,
    COMMENT,
    TEXT,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string name;
    std::string data;
    std::map<std::string, std::string> attributes;
    bool selfClosing;
};
```

### 2. State Machine

The tokenizer uses a state machine with states like:

```cpp
enum class ParserState {
    DATA,
    TAG_OPEN,
    TAG_NAME,
    BEFORE_ATTRIBUTE_NAME,
    ATTRIBUTE_NAME,
    ATTRIBUTE_VALUE,
    // ... more states
};
```

### 3. Tree Construction

Tokens are processed to build the DOM tree:

```cpp
void HTMLParser::processToken(const Token& token) {
    switch (token.type) {
        case TokenType::START_TAG:
            insertElement(token);
            break;
        case TokenType::END_TAG:
            endTagToken(token);
            break;
        case TokenType::TEXT:
            insertText(token);
            break;
        // ...
    }
}
```

## DOM Tree Structure

### Node Hierarchy

```
Document
├── DocumentType (<!DOCTYPE html>)
└── Element (html)
    ├── Element (head)
    │   ├── Element (title)
    │   │   └── Text ("Page Title")
    │   └── Element (meta)
    └── Element (body)
        ├── Element (h1)
        │   └── Text ("Heading")
        └── Element (p)
            └── Text ("Paragraph")
```

### Node Types

1. **Document**: Root of the DOM tree
2. **Element**: HTML tags (`<div>`, `<p>`, etc.)
3. **Text**: Text content
4. **Comment**: HTML comments
5. **DocumentType**: DOCTYPE declaration

## Key Features

### Error Recovery

The parser handles malformed HTML gracefully:

```cpp
// Missing closing tag
<p>Paragraph <div>Nested</p>
// Parser auto-closes <div> before </p>

// Misnested tags
<b><i>text</b></i>
// Parser corrects to <b><i>text</i></b>
```

### Self-Closing Tags

```cpp
static const std::unordered_set<std::string> kSelfClosingTags = {
    "area", "base", "br", "col", "embed", "hr", "img", 
    "input", "link", "meta", "param", "source", "track", "wbr"
};
```

### Special Elements

Elements with special parsing rules:

```cpp
static const std::unordered_set<std::string> kSpecialElements = {
    "script", "style", "textarea", "title", "pre"
    // ... more elements
};
```

## API Usage

### Basic Parsing

```cpp
// Create parser
HTMLParser parser;
parser.initialize();

// Parse HTML
std::string html = "<html><body><h1>Hello</h1></body></html>";
DOMTree domTree = parser.parse(html);

// Access document
Document* doc = domTree.document();
Element* body = doc->querySelector("body");
```

### DOM Manipulation

```cpp
// Create elements
auto div = doc->createElement("div");
div->setAttribute("class", "container");

// Add text
auto text = doc->createTextNode("Hello World");
div->appendChild(text);

// Insert into document
body->appendChild(div);
```

### Querying

```cpp
// By ID
Element* elem = doc->getElementById("myId");

// By tag name
std::vector<Element*> divs = doc->getElementsByTagName("div");

// By class name
std::vector<Element*> items = doc->getElementsByClassName("item");

// CSS selectors (simplified)
Element* first = doc->querySelector(".container > p");
std::vector<Element*> all = doc->querySelectorAll("a[href]");
```

## Implementation Details

### Token Creation

```cpp
Token HTMLParser::createStartToken(const std::string& name) {
    Token token;
    token.type = TokenType::START_TAG;
    token.name = name;
    return token;
}
```

### Attribute Parsing

```cpp
// Parse: <div id="test" class="container">
// Result: attributes["id"] = "test"
//         attributes["class"] = "container"
```

### Text Handling

```cpp
void HTMLParser::insertText(const Token& token) {
    // Skip empty text nodes
    if (token.data.empty()) return;
    
    auto text = m_document.document()->createTextNode(token.data);
    if (m_currentNode) {
        m_currentNode->appendChild(text);
    }
}
```

## Error Handling

### Parse Errors

```cpp
void HTMLParser::parseError(const std::string& message) {
    m_errors.push_back("Error at position " + 
                      std::to_string(m_position) + 
                      ": " + message);
    std::cerr << "HTML Parser Error: " << message << std::endl;
}
```

### Common Errors

1. **Unclosed Tags**: `<div><p>text</div>`
2. **Invalid Nesting**: `<p><div>text</div></p>`
3. **Duplicate Attributes**: `<div id="a" id="b">`
4. **Invalid Characters**: `<div><<>></div>`

## Performance Considerations

### Memory Usage

- Nodes are reference counted with `std::shared_ptr`
- Text nodes are consolidated when possible
- Attributes stored in `std::map` for fast lookup

### Optimization Techniques

1. **String Interning**: Common tag names cached
2. **Lazy Parsing**: Parse only visible content first
3. **Incremental Parsing**: Parse in chunks for large documents

## Limitations

1. **Simplified Selectors**: Only basic CSS selectors supported
2. **No Namespace Support**: HTML namespaces not implemented
3. **Limited Error Recovery**: Some edge cases not handled
4. **No Fragment Parsing**: Can't parse HTML fragments directly

## Future Enhancements

1. **HTML5 Compliance**: Full spec compliance
2. **Streaming Parser**: Parse while downloading
3. **Worker Thread Parsing**: Offload to background thread
4. **Better Error Messages**: More descriptive errors
5. **Performance Profiling**: Identify bottlenecks