#pragma once

#include "json.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

namespace json {

class Builder;

class DictContext;
class ArrayContext;
class DictKeyContext;
class ArrayValueContext;

class Builder {
public:
    Builder() = default;
    
    DictContext StartDict();
    ArrayContext StartArray();
    Node Build();

    DictKeyContext Key(std::string key);
    Builder& Value(Node::Value value);
    Builder& EndDict();
    Builder& EndArray();

private:
    friend class DictContext;
    friend class ArrayContext;
    friend class DictKeyContext;
    friend class ArrayValueContext;

    Node root_;
    std::vector<Node*> nodes_stack_;
    std::vector<std::string> keys_stack_;
    std::vector<bool> in_array_;
    bool built_ = false;
    
    void CheckNotBuilt();
    Node CreateNode(Node::Value value);
};

class DictContext {
public:
    DictContext(Builder& builder) : builder_(builder) {}
    
    DictKeyContext Key(std::string key);
    Builder& EndDict();
    
    Builder& Value(Node::Value value) = delete;
    DictContext StartDict() = delete;
    ArrayContext StartArray() = delete;
    Builder& EndArray() = delete;
    Node Build() = delete;

private:
    Builder& builder_;
};

class ArrayContext {
public:
    ArrayContext(Builder& builder) : builder_(builder) {}
    
    ArrayValueContext Value(Node::Value value);
    DictContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();
    
    DictKeyContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Node Build() = delete;

private:
    Builder& builder_;
};

class DictKeyContext {
public:
    DictKeyContext(Builder& builder) : builder_(builder) {}
    
    DictContext Value(Node::Value value);
    DictContext StartDict();
    ArrayContext StartArray();
    
    DictKeyContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Builder& EndArray() = delete;
    Node Build() = delete;

private:
    Builder& builder_;
};

class ArrayValueContext {
public:
    ArrayValueContext(Builder& builder) : builder_(builder) {}
    
    ArrayValueContext Value(Node::Value value);
    DictContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();
    
    DictKeyContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Node Build() = delete;

private:
    Builder& builder_;
};

} // namespace json