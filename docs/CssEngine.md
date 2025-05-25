# CSS Engine Documentation

## Overview

The CSS engine parses stylesheets, matches selectors to DOM elements, and computes final style values. It implements CSS cascade, inheritance, and specificity rules.

## Architecture

### Core Components

```cpp
namespace browser::css {
    class CSSParser {
        // Parses CSS text into rules
        std::shared_ptr<StyleSheet> parseStylesheet(const std::string& css);
    };
    
    class StyleResolver {
        // Matches rules to elements and computes styles
        ComputedStyle getComputedStyle(html::Element* element);
    };
    
    class ComputedStyle {
        // Final computed values for an element
        Value getProperty(const std::string& property);
    };
}
```

## CSS Parsing

### Tokenization

CSS is tokenized into meaningful units:

```cpp
enum class TokenType {
    IDENT,      // property names, keywords
    STRING,     // "quoted text"
    NUMBER,     // 123
    PERCENTAGE, // 50%
    DIMENSION,  // 10px, 2em
    HASH,       // #id
    DELIM,      // single characters
    // ... more token types
};
```

### Grammar Rules

The parser follows CSS grammar:

```
stylesheet  : rule*
rule        : selector_list '{' declaration_list '}'
selector    : simple_selector (combinator simple_selector)*
declaration : property ':' value
```

### Value Types

```cpp
enum class ValueType {
    KEYWORD,    // auto, none, block
    LENGTH,     // 10px, 2em, 50%
    COLOR,      // #fff, rgb(255,0,0)
    STRING,     // "Arial"
    URL,        // url(image.png)
    NUMBER,     // 1.5
};
```

## Selector System

### Selector Types

```cpp
enum class SelectorType {
    TYPE,              // div
    CLASS,             // .classname
    ID,                // #id
    UNIVERSAL,         // *
    ATTRIBUTE,         // [attr=value]
    PSEUDO_CLASS,      // :hover
    PSEUDO_ELEMENT,    // ::before
    DESCENDANT,        // ancestor descendant
    CHILD,             // parent > child
    ADJACENT_SIBLING,  // prev + next
    GENERAL_SIBLING    // prev ~ sibling
};
```

### Specificity Calculation

Specificity determines which rules win:

```cpp
int Selector::specificity() const {
    int a = 0; // ID selectors
    int b = 0; // Class, attribute, pseudo-class
    int c = 0; // Type selectors, pseudo-elements
    
    for (const auto& component : m_components) {
        switch (component.type) {
            case SelectorType::ID:
                a++;
                break;
            case SelectorType::CLASS:
            case SelectorType::ATTRIBUTE:
            case SelectorType::PSEUDO_CLASS:
                b++;
                break;
            case SelectorType::TYPE:
            case SelectorType::PSEUDO_ELEMENT:
                c++;
                break;
        }
    }
    
    return (a * 100) + (b * 10) + c;
}
```

### Selector Matching

```cpp
bool Selector::matches(html::Element* element) const {
    // Check each selector component
    for (const auto& component : m_components) {
        if (!matchComponent(element, component)) {
            return false;
        }
    }
    return true;
}
```

## Style Resolution

### Resolution Process

1. **Collect Matching Rules**: Find all rules that match element
2. **Sort by Specificity**: Higher specificity wins
3. **Apply Cascade**: Later rules override earlier ones
4. **Inheritance**: Inherit from parent for certain properties
5. **Initial Values**: Apply defaults for unset properties

### Computed Style

```cpp
class ComputedStyle {
private:
    std::map<std::string, Value> m_properties;
    
public:
    void inheritFrom(const ComputedStyle& parent) {
        // Inherit inheritable properties
        static const std::vector<std::string> inherited = {
            "color", "font-family", "font-size", "line-height"
        };
        
        for (const auto& prop : inherited) {
            if (!hasProperty(prop)) {
                setProperty(prop, parent.getProperty(prop));
            }
        }
    }
};
```

## Property Support

### Box Model Properties

```css
/* Margin */
margin: 10px;
margin-top/right/bottom/left: 10px;

/* Padding */
padding: 10px;
padding-top/right/bottom/left: 10px;

/* Border */
border: 1px solid black;
border-width/style/color: ...;

/* Dimensions */
width: 100px;
height: auto;
```

### Display Properties

```css
display: block | inline | inline-block | none;
position: static | relative | absolute | fixed;
float: none | left | right;
```

### Text Properties

```css
color: #333;
font-family: Arial, sans-serif;
font-size: 16px;
font-weight: normal | bold;
text-align: left | center | right;
line-height: 1.5;
```

### Background Properties

```css
background-color: #fff;
background-image: url(image.png);
background-position: center;
background-repeat: no-repeat;
```

## Usage Examples

### Parsing CSS

```cpp
// Create parser
CSSParser parser;

// Parse stylesheet
std::string css = R"(
    body {
        margin: 0;
        font-family: Arial;
    }
    
    .container {
        width: 960px;
        margin: 0 auto;
    }
    
    #header {
        background-color: #333;
        color: white;
    }
)";

auto stylesheet = parser.parseStylesheet(css);
```

### Resolving Styles

```cpp
// Create style resolver
StyleResolver resolver;
resolver.setDocument(document);
resolver.addStyleSheet(*stylesheet);

// Resolve styles for entire document
resolver.resolveStyles();

// Get computed style for element
Element* element = document->getElementById("header");
ComputedStyle style = resolver.getComputedStyle(element);

// Access properties
Value bgColor = style.getProperty("background-color");
Value color = style.getProperty("color");
```

### Working with Values

```cpp
// Parse values
Value width("100px");
Value color("#ff0000");
Value percentage("50%");

// Check value type
if (width.type() == ValueType::LENGTH) {
    float pixels = width.numericValue();
    Unit unit = width.unit();
}

// Convert to string
std::string widthStr = width.toString(); // "100px"
```

## Advanced Features

### Media Queries (Planned)

```css
@media screen and (max-width: 768px) {
    .container {
        width: 100%;
    }
}
```

### Pseudo-classes (Partial Support)

```css
a:hover { color: blue; }
li:first-child { font-weight: bold; }
input:focus { border-color: blue; }
```

### CSS Variables (Planned)

```css
:root {
    --primary-color: #333;
}

.header {
    color: var(--primary-color);
}
```

## Performance Optimizations

### Rule Matching

1. **Right-to-Left Matching**: Start with rightmost selector
2. **Bloom Filter**: Quick rejection of non-matching elements
3. **Rule Buckets**: Group rules by selector type

### Caching

```cpp
class StyleResolver {
private:
    // Cache computed styles
    std::map<html::Element*, ComputedStyle> m_elementStyles;
    
    // Invalidate on DOM changes
    void invalidateCache(html::Element* element);
};
```

## Limitations

1. **Limited Selector Support**: Complex selectors not implemented
2. **No Animations**: CSS animations not supported
3. **No Transforms**: 2D/3D transforms not implemented
4. **Basic Layout**: Only simple box model layout

## Testing

### Unit Tests

```cpp
TEST(CSSParser, ParseSimpleRule) {
    CSSParser parser;
    auto sheet = parser.parseStylesheet("p { color: red; }");
    
    EXPECT_EQ(sheet->rules().size(), 1);
    EXPECT_EQ(sheet->rules()[0].selectors()[0].toString(), "p");
    EXPECT_EQ(sheet->rules()[0].declarations()[0].property(), "color");
}
```

### Integration Tests

```cpp
TEST(StyleResolver, ComputeInheritance) {
    // Create DOM
    auto parent = document->createElement("div");
    parent->setAttribute("style", "color: blue; font-size: 16px;");
    
    auto child = document->createElement("span");
    parent->appendChild(child);
    
    // Resolve styles
    resolver.resolveStyles();
    
    // Check inheritance
    auto childStyle = resolver.getComputedStyle(child.get());
    EXPECT_EQ(childStyle.getProperty("color").toString(), "blue");
}
```