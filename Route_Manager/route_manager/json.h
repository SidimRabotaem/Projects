#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <cctype>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;
    
    Node() : value_(nullptr) {}
    Node(Array array) : value_(std::move(array)) {}
    Node(Dict map) : value_(std::move(map)) {}
    Node(int value) : value_(value) {}
    Node(double value) : value_(value) {}
    Node(bool value) : value_(value) {}
    Node(std::string value) : value_(std::move(value)) {}
    Node(std::nullptr_t) : value_(nullptr) {}
    Node(const char* value) : value_(std::string(value)) {}

    bool IsInt() const { return std::holds_alternative<int>(value_); }
    bool IsDouble() const { return std::holds_alternative<int>(value_) || std::holds_alternative<double>(value_); }
    bool IsPureDouble() const { return std::holds_alternative<double>(value_); }
    bool IsBool() const { return std::holds_alternative<bool>(value_); }
    bool IsString() const { return std::holds_alternative<std::string>(value_); }
    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool IsArray() const { return std::holds_alternative<Array>(value_); }
    bool IsMap() const { return std::holds_alternative<Dict>(value_); }

    int AsInt() const {
        if(!IsInt()) {
            throw std::logic_error("Not an int");
        }
        return std::get<int>(value_);
    }

    bool AsBool() const  {
        if(!IsBool()) {
            throw std::logic_error("Not a bool");
        }
        return std::get<bool>(value_);
    }

    double AsDouble() const {
    if (IsInt()) {
        return static_cast<double>(std::get<int>(value_));
    } else if (IsPureDouble()) {
        return std::get<double>(value_);
    }
    throw std::logic_error("Not a number");
}
   
    bool IsNumber() const {
    return IsInt() || IsPureDouble();
}

    const std::string& AsString() const {
        if(!IsString()) {
            throw std::logic_error("Not a string");
        }
        return std::get<std::string>(value_);
    }

    const Array& AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Not an array");
        }
        return std::get<Array>(value_);
    }

    const Dict& AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Not a map");
        }
        return std::get<Dict>(value_);
    }

    bool operator==(const Node& other) const {
        return value_ == other.value_;
    }

    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

    const Value& GetValue() const { return value_; }

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root)
        : root_(std::move(root)) {
    }

    const Node& GetRoot() const {
        return root_;
    }

    bool operator==(const Document& other) const {
        return root_ == other.root_;
    }

    bool operator!=(const Document& other) const {
        return !(*this == other);
    }

private:
    Node root_;
};

Document Load(std::istream& input);
void Print(const Document& doc, std::ostream& output);

}  // namespace json