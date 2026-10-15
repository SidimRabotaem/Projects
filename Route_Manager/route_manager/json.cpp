#include "json.h"

#include <cctype>
#include <sstream>
#include <locale>
#include <cmath>
#include <limits>

namespace json {

namespace {

Node LoadNode(std::istream& input);

void SkipWhitespace(std::istream& input) {
    while (std::isspace(input.peek())) {
        input.get();
    }
}

bool IsValidTokenEnd(char c) {
    return std::isspace(c) || c == ',' || c == ']' || c == '}' || c == EOF;
}

Node LoadArray(std::istream& input) {
    Array result;
    
    char c;
    input >> c; 
    if (c != '[') {
        throw ParsingError("Array should start with '['");
    }
    
    SkipWhitespace(input);
    if (input.peek() == ']') {
        input.get();
        return Node(std::move(result));
    }
    
    while (true) {
        SkipWhitespace(input);
        result.push_back(LoadNode(input));
        SkipWhitespace(input);
        
        if (!input.get(c)) {
            throw ParsingError("Unexpected end of array");
        }
        
        if (c == ']') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or ']' in array, got '" + std::string(1, c) + "'");
        }
    }
    
    return Node(std::move(result));
}

Node LoadString(std::istream& input) {
    std::string result;
    char c;
    
    input >> c; 
    if (c != '"') {
        throw ParsingError("String should start with '\"'");
    }
    
    while (input.get(c)) {
        if (c == '"') {
            break;
        } else if (c == '\\') {
            if (!input.get(c)) {
                throw ParsingError("Unexpected end of string after escape");
            }
            switch (c) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: 
                    throw ParsingError("Invalid escape sequence: \\" + std::string(1, c));
            }
        } else {
            result += c;
        }
    }
    
    if (c != '"') {
        throw ParsingError("String should end with '\"'");
    }
    
    return Node(std::move(result));
}

Node LoadDict(std::istream& input) {
    Dict result;
    
    char c;
    input >> c; 
    if (c != '{') {
        throw ParsingError("Dict should start with '{'");
    }
    
    SkipWhitespace(input);
    if (input.peek() == '}') {
        input.get();
        return Node(std::move(result));
    }
    
    while (true) {
        SkipWhitespace(input);
        Node key = LoadString(input);
        SkipWhitespace(input);
        
        if (!input.get(c) || c != ':') {
            throw ParsingError("Expected ':' after key in dict");
        }
        
        SkipWhitespace(input);
        Node value = LoadNode(input);
        result[key.AsString()] = std::move(value);
        SkipWhitespace(input);
        
        if (!input.get(c)) {
            throw ParsingError("Unexpected end of dict");
        }
        
        if (c == '}') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or '}' in dict, got '" + std::string(1, c) + "'");
        }
    }
    
    return Node(std::move(result));
}

Node LoadNumber(std::istream& input) {
    std::string number_str;
    char c = input.peek();
    
    while (std::isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
        number_str += input.get();
        c = input.peek();
    }
    
    if (number_str.empty()) {
        throw ParsingError("Failed to read number");
    }
    
  
    try {
        size_t pos = 0;
        int int_value = std::stoi(number_str, &pos);
        if (pos == number_str.size()) {
            return Node(int_value);
        }
    } catch (const std::exception&) {
       
    }
    
 
    try {
        size_t pos = 0;
        double double_value = std::stod(number_str, &pos);
        if (pos == number_str.size()) {
            return Node(double_value);
        }
    } catch (const std::exception&) {
     
    }
    
    throw ParsingError("Invalid number: " + number_str);
}

Node LoadNull(std::istream& input) {
    char expected[] = {'n', 'u', 'l', 'l'};
    for (int i = 0; i < 4; ++i) {
        char c;
        if (!input.get(c) || c != expected[i]) {
            throw ParsingError("Invalid null value");
        }
    }
    
    if (!IsValidTokenEnd(input.peek())) {
        throw ParsingError("Invalid null value");
    }
    
    return Node(nullptr);
}

Node LoadBool(std::istream& input) {
    char first_char;
    if (!input.get(first_char)) {
        throw ParsingError("Unexpected end while reading boolean");
    }
    
    if (first_char == 't') {
       
        char expected[] = {'r', 'u', 'e'};
        for (int i = 0; i < 3; ++i) {
            char c;
            if (!input.get(c) || c != expected[i]) {
                throw ParsingError("Invalid boolean value");
            }
        }
        
      
        if (!IsValidTokenEnd(input.peek())) {
            throw ParsingError("Invalid boolean value");
        }
        
        return Node(true);
    } else if (first_char == 'f') {
      
        char expected[] = {'a', 'l', 's', 'e'};
        for (int i = 0; i < 4; ++i) {
            char c;
            if (!input.get(c) || c != expected[i]) {
                throw ParsingError("Invalid boolean value");
            }
        }
        
        
        if (!IsValidTokenEnd(input.peek())) {
            throw ParsingError("Invalid boolean value");
        }
        
        return Node(false);
    } else {
        throw ParsingError("Invalid boolean value");
    }
}

Node LoadNode(std::istream& input) {
    SkipWhitespace(input);
    char c = input.peek();
    
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        return LoadBool(input);
    } else if (std::isdigit(c) || c == '-') {
        return LoadNumber(input);
    } else {
        throw ParsingError("Unexpected character: " + std::string(1, c));
    }
}

void PrintNode(const Node& node, std::ostream& output);

void PrintString(const std::string& str, std::ostream& output) {
    output << '"';
    for (char c : str) {
        switch (c) {
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            default: output << c;
        }
    }
    output << '"';
}

void PrintArray(const Array& array, std::ostream& output) {
    output << '[';
    bool first = true;
    for (const auto& item : array) {
        if (!first) {
            output << ',';
        }
        first = false;
        PrintNode(item, output);
    }
    output << ']';
}

void PrintDict(const Dict& dict, std::ostream& output) {
    output << '{';
    bool first = true;
    for (const auto& [key, value] : dict) {
        if (!first) {
            output << ',';
        }
        first = false;
        PrintString(key, output);
        output << ':';
        PrintNode(value, output);
    }
    output << '}';
}

void PrintNode(const Node& node, std::ostream& output) {
    if (node.IsNull()) {
        output << "null";
    } else if (node.IsBool()) {
        output << (node.AsBool() ? "true" : "false");
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsDouble()) {
        
        double value = node.AsDouble();
        if (std::isinf(value) || std::isnan(value)) {
            throw std::logic_error("Invalid double value");
        }
      
        if (value == std::floor(value) && value <= std::numeric_limits<int>::max() && value >= std::numeric_limits<int>::min()) {
            output << static_cast<int>(value);
        } else {
            output << value;
        }
    } else if (node.IsString()) {
        PrintString(node.AsString(), output);
    } else if (node.IsArray()) {
        PrintArray(node.AsArray(), output);
    } else if (node.IsMap()) {
        PrintDict(node.AsMap(), output);
    }
}

} // namespace

Document Load(std::istream& input) {
    return Document(LoadNode(input));
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

} // namespace json