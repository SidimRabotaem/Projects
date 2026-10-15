#include "json_builder.h"
#include <stdexcept>

namespace json {

void Builder::CheckNotBuilt() {
    if (built_) {
        throw std::logic_error("Method call after Build");
    }
}

Node Builder::CreateNode(Node::Value value) {
    return std::visit([](auto&& arg) -> Node {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return Node(nullptr);
        } else if constexpr (std::is_same_v<T, bool>) {
            return Node(arg);
        } else if constexpr (std::is_same_v<T, int>) {
            return Node(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return Node(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return Node(std::move(arg));
        } else if constexpr (std::is_same_v<T, Array>) {
            return Node(std::move(arg));
        } else if constexpr (std::is_same_v<T, Dict>) {
            return Node(std::move(arg));
        }
    }, std::move(value));
}

DictContext Builder::StartDict() {
    CheckNotBuilt();
    
    if (nodes_stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Multiple root values");
        }
        root_ = Node(Dict{});
        nodes_stack_.push_back(&root_);
        in_array_.push_back(false);
    } else if (in_array_.back()) {
        auto& array = const_cast<Array&>(std::get<Array>(nodes_stack_.back()->GetValue()));
        array.push_back(Node(Dict{}));
        nodes_stack_.push_back(&array.back());
        in_array_.push_back(false);
    } else {
        if (keys_stack_.empty()) {
            throw std::logic_error("StartDict call in dictionary without key");
        }
        auto& dict = const_cast<Dict&>(std::get<Dict>(nodes_stack_.back()->GetValue()));
        dict[keys_stack_.back()] = Node(Dict{});
        nodes_stack_.push_back(&dict[keys_stack_.back()]);
        keys_stack_.pop_back();
        in_array_.push_back(false);
    }
    return DictContext(*this);
}

ArrayContext Builder::StartArray() {
    CheckNotBuilt();
    
    if (nodes_stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Multiple root values");
        }
        root_ = Node(Array{});
        nodes_stack_.push_back(&root_);
        in_array_.push_back(true);
    } else if (in_array_.back()) {
        auto& array = const_cast<Array&>(std::get<Array>(nodes_stack_.back()->GetValue()));
        array.push_back(Node(Array{}));
        nodes_stack_.push_back(&array.back());
        in_array_.push_back(true);
    } else {
        if (keys_stack_.empty()) {
            throw std::logic_error("StartArray call in dictionary without key");
        }
        auto& dict = const_cast<Dict&>(std::get<Dict>(nodes_stack_.back()->GetValue()));
        dict[keys_stack_.back()] = Node(Array{});
        nodes_stack_.push_back(&dict[keys_stack_.back()]);
        keys_stack_.pop_back();
        in_array_.push_back(true);
    }
    return ArrayContext(*this);
}

DictKeyContext Builder::Key(std::string key) {
    CheckNotBuilt();
    
    if (nodes_stack_.empty() || in_array_.back()) {
        throw std::logic_error("Key call outside dictionary");
    }
    if (!keys_stack_.empty()) {
        throw std::logic_error("Key call without value for previous key");
    }
    keys_stack_.push_back(std::move(key));
    return DictKeyContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    CheckNotBuilt();
    
    if (nodes_stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Multiple root values");
        }
        root_ = CreateNode(std::move(value));
    } else if (in_array_.back()) {
        auto& array = const_cast<Array&>(std::get<Array>(nodes_stack_.back()->GetValue()));
        array.push_back(CreateNode(std::move(value)));
    } else {
        if (keys_stack_.empty()) {
            throw std::logic_error("Value call in dictionary without key");
        }
        auto& dict = const_cast<Dict&>(std::get<Dict>(nodes_stack_.back()->GetValue()));
        dict[keys_stack_.back()] = CreateNode(std::move(value));
        keys_stack_.pop_back();
    }
    return *this;
}

Builder& Builder::EndDict() {
    CheckNotBuilt();
    
    if (nodes_stack_.empty() || in_array_.back()) {
        throw std::logic_error("EndDict call without StartDict");
    }
    nodes_stack_.pop_back();
    in_array_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    CheckNotBuilt();
    
    if (nodes_stack_.empty() || !in_array_.back()) {
        throw std::logic_error("EndArray call without StartArray");
    }
    nodes_stack_.pop_back();
    in_array_.pop_back();
    return *this;
}

Node Builder::Build() {
    if (built_) {
        throw std::logic_error("Build() called twice");
    }

    if (!nodes_stack_.empty()) {
        throw std::logic_error("Build() called with unfinished containers");
    }
    if (!keys_stack_.empty()) {
        throw std::logic_error("Build() called with unfinished key");
    }

    if (root_.IsNull()) {
        throw std::logic_error("Build() called without any value");
    }

    built_ = true;
    return std::move(root_);
}


DictKeyContext DictContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& DictContext::EndDict() {
    return builder_.EndDict();
}

ArrayValueContext ArrayContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayValueContext(builder_);
}

DictContext ArrayContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext ArrayContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayContext::EndArray() {
    return builder_.EndArray();
}

DictContext DictKeyContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictContext(builder_);
}

DictContext DictKeyContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext DictKeyContext::StartArray() {
    return builder_.StartArray();
}

ArrayValueContext ArrayValueContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayValueContext(builder_);
}

DictContext ArrayValueContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext ArrayValueContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayValueContext::EndArray() {
    return builder_.EndArray();
}

} // namespace json