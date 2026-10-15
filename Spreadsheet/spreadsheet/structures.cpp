#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return "";
    }

    std::string result;
    result.reserve(MAX_POSITION_LENGTH);
    int c = col;
    while (c >= 0) {
        result.insert(result.begin(), 'A' + c % LETTERS);
        c = c / LETTERS - 1;
    }

    result += std::to_string(row + 1);

    return result;
}

Position Position::FromString(std::string_view str) {
    if (str.empty()) return Position::NONE;
    
    if (!std::isalpha(static_cast<unsigned char>(str[0]))) {
        return Position::NONE;
    }
    
    size_t i = 0;
    while (i < str.size() && std::isalpha(static_cast<unsigned char>(str[i]))) {
        if (!std::isupper(str[i])) {
            return Position::NONE; 
        }
        i++;
    }
    
    if (i == 0 || i == str.size()) {
        return Position::NONE;
    }
    
    int row = 0;
    for (size_t j = i; j < str.size(); j++) {
        if (!std::isdigit(static_cast<unsigned char>(str[j]))) {
            return Position::NONE;
        }
        row = row * 10 + (str[j] - '0');
    }
    
    if (row == 0) return Position::NONE;
    
    int col = 0;
    for (size_t j = 0; j < i; j++) {
        col = col * 26 + (str[j] - 'A' + 1);
    }
    
    return {row - 1, col - 1};
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}