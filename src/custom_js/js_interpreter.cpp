#include "js_interpreter.h"
#include <iostream>
#include <sstream>

namespace browser {
namespace custom_js {

//-----------------------------------------------------------------------------
// Environment Implementation
//-----------------------------------------------------------------------------

Environment::Environment() : m_enclosing(nullptr) {
}

Environment::Environment(std::shared_ptr<Environment> enclosing) : m_enclosing(enclosing) {
}

void Environment::define(const std::string& name, const JSValue& value) {
    m_values[name] = value;
}

JSValue Environment::get(const std::string& name) {
    auto it = m_values.find(name);
    if (it != m_values.end()) {
        return it->second;
    }
    
    if (m_enclosing) {
        return m_enclosing->get(name);
    }
    
    std::stringstream ss;
    ss << "Undefined variable '" << name << "'.";
    throw RuntimeError(ss.str());
}

void Environment::assign(const std::string& name, const JSValue& value) {
    auto it = m_values.find(name);
    if (it != m_values.end()) {
        it->second = value;
        return;
    }
    
    if (m_enclosing) {
        m_enclosing->assign(name, value);
        return;
    }
    
    std::stringstream ss;
    ss << "Undefined variable '" << name << "'.";
    throw RuntimeError(ss.str());
}

bool Environment::exists(const std::string& name) const {
    if (m_values.find(name) != m_values.end()) {
        return true;
    }
    
    if (m_enclosing) {
        return m_enclosing->exists(name);
    }
    
    return false;
}

//-----------------------------------------------------------------------------
// JSInterpreter Implementation
//-----------------------------------------------------------------------------

JSInterpreter::JSInterpreter() : m_hasReturnValue(false) {
    m_globals = std::make_shared<Environment>();
    m_environment = m_globals;
    
    // Add built-in functions
    defineGlobalFunction("print", [](const std::vector<JSValue>& args, JSValue) {
        for (const auto& arg : args) {
            std::cout << arg.toString() << " ";
        }
        std::cout << std::endl;
        return JSValue();
    });
    
    defineGlobalFunction("console.log", [](const std::vector<JSValue>& args, JSValue) {
        for (const auto& arg : args) {
            std::cout << arg.toString() << " ";
        }
        std::cout << std::endl;
        return JSValue();
    });
    
    // Create global console object
    auto consoleObj = std::make_shared<JSObject>();
    auto logFunc = std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        for (const auto& arg : args) {
            std::cout << arg.toString() << " ";
        }
        std::cout << std::endl;
        return JSValue();
    });
    consoleObj->set("log", JSValue(logFunc));
    defineGlobalVariable("console", JSValue(consoleObj));
}

JSValue JSInterpreter::execute(std::shared_ptr<Program> program) {
    if (!program) return JSValue();
    
    try {
        for (const auto& statement : program->statements) {
            execute(statement);
            
            if (m_hasReturnValue) {
                // A return statement was encountered at the top level
                JSValue result = m_returnValue;
                m_hasReturnValue = false;
                return result;
            }
        }
    } catch (const RuntimeError& error) {
        std::cerr << "Runtime error: " << error.what() << std::endl;
        return JSValue();
    }
    
    return JSValue(); // undefined
}

JSValue JSInterpreter::execute(const std::string& source) {
    JSParser parser;
    std::shared_ptr<Program> program = parser.parse(source);
    return execute(program);
}

void JSInterpreter::defineGlobalFunction(const std::string& name, JSFunction::NativeFunction func) {
    m_globals->define(name, JSValue(std::make_shared<JSFunction>(func)));
}

void JSInterpreter::defineGlobalVariable(const std::string& name, const JSValue& value) {
    m_globals->define(name, value);
}

JSValue JSInterpreter::visitExpressionNode(std::shared_ptr<ExpressionNode> node) {
    switch (node->getType()) {
        case ExpressionType::LITERAL:
            return visitLiteralExpr(std::dynamic_pointer_cast<LiteralExpr>(node));
        case ExpressionType::VARIABLE:
            return visitVariableExpr(std::dynamic_pointer_cast<VariableExpr>(node));
        case ExpressionType::UNARY:
            return visitUnaryExpr(std::dynamic_pointer_cast<UnaryExpr>(node));
        case ExpressionType::BINARY:
            return visitBinaryExpr(std::dynamic_pointer_cast<BinaryExpr>(node));
        case ExpressionType::CALL:
            return visitCallExpr(std::dynamic_pointer_cast<CallExpr>(node));
        case ExpressionType::ASSIGN:
            return visitAssignExpr(std::dynamic_pointer_cast<AssignExpr>(node));
        case ExpressionType::OBJECT:
            return visitObjectExpr(std::dynamic_pointer_cast<ObjectExpr>(node));
        case ExpressionType::ARRAY:
            return visitArrayExpr(std::dynamic_pointer_cast<ArrayExpr>(node));
        case ExpressionType::MEMBER:
            return visitMemberExpr(std::dynamic_pointer_cast<MemberExpr>(node));
        default:
            throw error("Unknown expression type.");
    }
}

JSValue JSInterpreter::visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) {
    switch (expr->literalType) {
        case LiteralExpr::LiteralType::NUMBER:
            return JSValue(std::stod(expr->value));
        case LiteralExpr::LiteralType::STRING:
            return JSValue(expr->value);
        case LiteralExpr::LiteralType::BOOLEAN:
            return JSValue(expr->value == "true");
        case LiteralExpr::LiteralType::NULL_TYPE:
            return JSValue(nullptr);
        case LiteralExpr::LiteralType::UNDEFINED:
            return JSValue(); // undefined
        default:
            return JSValue(); // undefined
    }
}

JSValue JSInterpreter::visitVariableExpr(std::shared_ptr<VariableExpr> expr) {
    return lookUpVariable(expr->name);
}

JSValue JSInterpreter::visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) {
    JSValue operand = evaluate(expr->operand);
    
    switch (expr->op) {
        case UnaryExpr::Operator::MINUS:
            return JSValue(-operand.toNumber());
        case UnaryExpr::Operator::NOT:
            return JSValue(!isTruthy(operand));
        case UnaryExpr::Operator::INCREMENT:
            if (expr->operand->getType() != ExpressionType::VARIABLE) {
                throw error("Invalid increment target.");
            }
            {
                auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr->operand);
                double value = operand.toNumber();
                JSValue newValue(value + 1.0);
                
                m_environment->assign(varExpr->name, newValue);
                
                return expr->isPrefix ? newValue : operand;
            }
        case UnaryExpr::Operator::DECREMENT:
            if (expr->operand->getType() != ExpressionType::VARIABLE) {
                throw error("Invalid decrement target.");
            }
            {
                auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr->operand);
                double value = operand.toNumber();
                JSValue newValue(value - 1.0);
                
                m_environment->assign(varExpr->name, newValue);
                
                return expr->isPrefix ? newValue : operand;
            }
        default:
            return JSValue(); // undefined
    }
}

JSValue JSInterpreter::visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) {
    JSValue left = evaluate(expr->left);
    JSValue right = evaluate(expr->right);
    
    switch (expr->op) {
        case BinaryExpr::Operator::PLUS:
            if (left.isString() || right.isString()) {
                // String concatenation
                return JSValue(left.toString() + right.toString());
            }
            return JSValue(left.toNumber() + right.toNumber());
        case BinaryExpr::Operator::MINUS:
            return JSValue(left.toNumber() - right.toNumber());
        case BinaryExpr::Operator::MULTIPLY:
            return JSValue(left.toNumber() * right.toNumber());
        case BinaryExpr::Operator::DIVIDE:
            if (right.toNumber() == 0.0) {
                throw error("Division by zero.");
            }
            return JSValue(left.toNumber() / right.toNumber());
        case BinaryExpr::Operator::MODULO:
            if (right.toNumber() == 0.0) {
                throw error("Modulo by zero.");
            }
            return JSValue(std::fmod(left.toNumber(), right.toNumber()));
        case BinaryExpr::Operator::POWER:
            return JSValue(std::pow(left.toNumber(), right.toNumber()));
        case BinaryExpr::Operator::EQUAL:
            return JSValue(isEqual(left, right));
        case BinaryExpr::Operator::NOT_EQUAL:
            return JSValue(!isEqual(left, right));
        case BinaryExpr::Operator::STRICT_EQUAL:
            return JSValue(left == right);
        case BinaryExpr::Operator::STRICT_NOT_EQUAL:
            return JSValue(left != right);
        case BinaryExpr::Operator::LESS:
            return JSValue(left.toNumber() < right.toNumber());
        case BinaryExpr::Operator::LESS_EQUAL:
            return JSValue(left.toNumber() <= right.toNumber());
        case BinaryExpr::Operator::GREATER:
            return JSValue(left.toNumber() > right.toNumber());
        case BinaryExpr::Operator::GREATER_EQUAL:
            return JSValue(left.toNumber() >= right.toNumber());
        case BinaryExpr::Operator::AND:
            return JSValue(isTruthy(left) && isTruthy(right));
        case BinaryExpr::Operator::OR:
            return JSValue(isTruthy(left) || isTruthy(right));
        default:
            return JSValue(); // undefined
    }
}

JSValue JSInterpreter::visitCallExpr(std::shared_ptr<CallExpr> expr) {
    JSValue callee = evaluate(expr->callee);
    
    if (!callee.isFunction()) {
        throw error("Can only call functions.");
    }
    
    // Evaluate arguments
    std::vector<JSValue> arguments;
    for (const auto& arg : expr->arguments) {
        arguments.push_back(evaluate(arg));
    }
    
    // Get function
    std::shared_ptr<JSFunction> function = callee.toFunction();
    if (!function) {
        throw error("Invalid function call.");
    }
    
    // Call function with this=undefined
    return function->call(arguments, JSValue());
}

JSValue JSInterpreter::visitAssignExpr(std::shared_ptr<AssignExpr> expr) {
    JSValue value = evaluate(expr->value);
    
    if (expr->target->getType() == ExpressionType::VARIABLE) {
        auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr->target);
        m_environment->assign(varExpr->name, value);
    } else if (expr->target->getType() == ExpressionType::MEMBER) {
        auto memberExpr = std::dynamic_pointer_cast<MemberExpr>(expr->target);
        JSValue object = evaluate(memberExpr->object);
        
        if (!object.isObject()) {
            throw error("Cannot set property on non-object.");
        }
        
        std::shared_ptr<JSObject> obj = object.toObject();
        if (!obj) {
            throw error("Invalid object for property assignment.");
        }
        
        if (memberExpr->computed) {
            // obj[prop]
            JSValue property = evaluate(memberExpr->property);
            obj->set(property.toString(), value);
        } else {
            // obj.prop
            auto propExpr = std::dynamic_pointer_cast<LiteralExpr>(memberExpr->property);
            if (!propExpr) {
                throw error("Invalid property access.");
            }
            obj->set(propExpr->value, value);
        }
    } else {
        throw error("Invalid assignment target.");
    }
    
    return value;
}

JSValue JSInterpreter::visitObjectExpr(std::shared_ptr<ObjectExpr> expr) {
    auto object = std::make_shared<JSObject>();
    
    for (const auto& prop : expr->properties) {
        JSValue value = evaluate(prop.value);
        object->set(prop.key, value);
    }
    
    return JSValue(object);
}

JSValue JSInterpreter::visitArrayExpr(std::shared_ptr<ArrayExpr> expr) {
    std::vector<JSValue> elements;
    for (const auto& element : expr->elements) {
        elements.push_back(evaluate(element));
    }
    
    return JSValue(std::make_shared<JSArray>(elements));
}

JSValue JSInterpreter::visitMemberExpr(std::shared_ptr<MemberExpr> expr) {
    JSValue object = evaluate(expr->object);
    
    if (!object.isObject()) {
        throw error("Cannot access property of non-object.");
    }
    
    std::shared_ptr<JSObject> obj = object.toObject();
    if (!obj) {
        throw error("Invalid object for property access.");
    }
    
    if (expr->computed) {
        // obj[prop]
        JSValue property = evaluate(expr->property);
        return obj->get(property.toString());
    } else {
        // obj.prop
        auto propExpr = std::dynamic_pointer_cast<LiteralExpr>(expr->property);
        if (!propExpr) {
            throw error("Invalid property access.");
        }
        return obj->get(propExpr->value);
    }
}

void JSInterpreter::executeStatementNode(std::shared_ptr<StatementNode> node) {
    switch (node->getType()) {
        case StatementType::EXPRESSION:
            visitExpressionStmt(std::dynamic_pointer_cast<ExpressionStmt>(node));
            break;
        case StatementType::VARIABLE:
            visitVariableStmt(std::dynamic_pointer_cast<VariableStmt>(node));
            break;
        case StatementType::BLOCK:
            visitBlockStmt(std::dynamic_pointer_cast<BlockStmt>(node));
            break;
        case StatementType::IF:
            visitIfStmt(std::dynamic_pointer_cast<IfStmt>(node));
            break;
        case StatementType::WHILE:
            visitWhileStmt(std::dynamic_pointer_cast<WhileStmt>(node));
            break;
        case StatementType::FOR:
            visitForStmt(std::dynamic_pointer_cast<ForStmt>(node));
            break;
        case StatementType::FUNCTION:
            visitFunctionStmt(std::dynamic_pointer_cast<FunctionStmt>(node));
            break;
        case StatementType::RETURN:
            visitReturnStmt(std::dynamic_pointer_cast<ReturnStmt>(node));
            break;
        default:
            throw error("Unknown statement type.");
    }
}

void JSInterpreter::visitExpressionStmt(std::shared_ptr<ExpressionStmt> stmt) {
    evaluate(stmt->expression);
}

void JSInterpreter::visitVariableStmt(std::shared_ptr<VariableStmt> stmt) {
    JSValue value;
    
    if (stmt->initializer) {
        value = evaluate(stmt->initializer);
    }
    
    m_environment->define(stmt->name, value);
}

void JSInterpreter::visitBlockStmt(std::shared_ptr<BlockStmt> stmt) {
    auto environment = std::make_shared<Environment>(m_environment);
    executeBlock(stmt, environment);
}

void JSInterpreter::visitIfStmt(std::shared_ptr<IfStmt> stmt) {
    if (isTruthy(evaluate(stmt->condition))) {
        execute(stmt->thenBranch);
    } else if (stmt->elseBranch) {
        execute(stmt->elseBranch);
    }
}

void JSInterpreter::visitWhileStmt(std::shared_ptr<WhileStmt> stmt) {
    while (isTruthy(evaluate(stmt->condition))) {
        execute(stmt->body);
        
        if (m_hasReturnValue) {
            break;
        }
    }
}

void JSInterpreter::visitForStmt(std::shared_ptr<ForStmt> stmt) {
    // Create a new scope for the loop
    auto environment = std::make_shared<Environment>(m_environment);
    std::shared_ptr<Environment> previousEnvironment = m_environment;
    m_environment = environment;
    
    try {
        // Initializer
        if (stmt->initializer) {
            execute(stmt->initializer);
        }
        
        // Loop
        while (true) {
            // Check condition
            if (stmt->condition && !isTruthy(evaluate(stmt->condition))) {
                break;
            }
            
            // Execute body
            execute(stmt->body);
            
            // Check for return
            if (m_hasReturnValue) {
                break;
            }
            
            // Increment
            if (stmt->increment) {
                evaluate(stmt->increment);
            }
        }
    } catch (const RuntimeError& error) {
        // Restore environment
        m_environment = previousEnvironment;
        throw error;
    }
    
    // Restore environment
    m_environment = previousEnvironment;
}

void JSInterpreter::visitFunctionStmt(std::shared_ptr<FunctionStmt> stmt) {
    // Create a function object that captures the current environment
    auto function = std::make_shared<JSFunction>([this, stmt](const std::vector<JSValue>& args, JSValue thisValue) {
        // Create a new environment with the closure
        auto environment = std::make_shared<Environment>(m_environment);
        std::shared_ptr<Environment> previousEnvironment = m_environment;
        m_environment = environment;
        
        // Bind parameters to arguments
        for (size_t i = 0; i < stmt->parameters.size(); i++) {
            if (i < args.size()) {
                environment->define(stmt->parameters[i], args[i]);
            } else {
                // Default to undefined
                environment->define(stmt->parameters[i], JSValue());
            }
        }
        
        // Execute function body
        try {
            executeBlock(stmt->body, environment);
        } catch (const RuntimeError& error) {
            // Restore environment
            m_environment = previousEnvironment;
            throw error;
        }
        
        // Restore environment
        m_environment = previousEnvironment;
        
        // Check for return value
        if (m_hasReturnValue) {
            JSValue returnValue = m_returnValue;
            m_hasReturnValue = false;
            return returnValue;
        }
        
        return JSValue(); // undefined
    });
    
    // Define the function in the current environment
    m_environment->define(stmt->name, JSValue(function));
}

void JSInterpreter::visitReturnStmt(std::shared_ptr<ReturnStmt> stmt) {
    JSValue value;
    
    if (stmt->value) {
        value = evaluate(stmt->value);
    }
    
    // Set return value
    m_hasReturnValue = true;
    m_returnValue = value;
}

JSValue JSInterpreter::evaluate(std::shared_ptr<ExpressionNode> expr) {
    return visitExpressionNode(expr);
}

void JSInterpreter::execute(std::shared_ptr<StatementNode> stmt) {
    executeStatementNode(stmt);
}

void JSInterpreter::executeBlock(std::shared_ptr<BlockStmt> block, std::shared_ptr<Environment> environment) {
    std::shared_ptr<Environment> previousEnvironment = m_environment;
    m_environment = environment;
    
    try {
        for (const auto& statement : block->statements) {
            execute(statement);
            
            if (m_hasReturnValue) {
                break;
            }
        }
    } catch (const RuntimeError& error) {
        // Restore environment
        m_environment = previousEnvironment;
        throw error;
    }
    
    // Restore environment
    m_environment = previousEnvironment;
}

bool JSInterpreter::isTruthy(const JSValue& value) {
    return value.toBoolean();
}

bool JSInterpreter::isEqual(const JSValue& a, const JSValue& b) {
    // Perform type conversion for non-strict equality
    if (a.isNumber() && b.isString()) {
        return a.toNumber() == b.toNumber();
    }
    
    if (a.isString() && b.isNumber()) {
        return a.toNumber() == b.toNumber();
    }
    
    // Otherwise fall back to strict equality
    return a == b;
}

RuntimeError JSInterpreter::error(const std::string& message) {
    return RuntimeError(message);
}

JSValue JSInterpreter::lookUpVariable(const std::string& name) {
    // Check if variable exists in the environment
    if (m_environment->exists(name)) {
        return m_environment->get(name);
    }
    
    // Check global environment
    if (m_globals->exists(name)) {
        return m_globals->get(name);
    }
    
    std::stringstream ss;
    ss << "Undefined variable '" << name << "'.";
    throw error(ss.str());
}

} // namespace custom_js
} // namespace browser