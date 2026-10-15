#include "sheet.h"
#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <string>

using namespace std;

static string FormulaErrorToString(const FormulaError& fe) {
    switch (fe.GetCategory()) {
        case FormulaError::Category::Ref:        return "#REF!";
        case FormulaError::Category::Value:      return "#VALUE!";
        case FormulaError::Category::Arithmetic: return "#ARITHM!";
    }
    return "#ERROR!";
}

static bool HasCycle(Position from, Position target, const SheetInterface& sheet, set<Position>& visited) {
    if (from == target) return true;
    if (!visited.insert(from).second) return false;

    const CellInterface* cell = sheet.GetCell(from);
    if (!cell) return false;

    for (const Position& ref : cell->GetReferencedCells()) {
        if (HasCycle(ref, target, sheet, visited)) return true;
    }
    return false;
}

Sheet::Sheet() = default;

void Sheet::ValidatePosition(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
}

void Sheet::EnsureGridSize(Position pos) {
    if (pos.row >= static_cast<int>(cells_.size())) cells_.resize(pos.row + 1);
    for (auto& row : cells_) {
        if (pos.col >= static_cast<int>(row.size())) row.resize(pos.col + 1);
    }
}

void Sheet::SetCell(Position pos, string text) {
    ValidatePosition(pos);

    if (text.empty()) {
        ClearCell(pos);
        return;
    }

    EnsureGridSize(pos);

    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = make_unique<Cell>(*this, pos);
    }

    Cell& cell = *cells_[pos.row][pos.col];
    string old_text = cell.GetText();

    try {
        cell.Set(move(text));

        set<Position> visited;
        for (const Position& ref : cell.GetReferencedCells()) {
            if (HasCycle(ref, pos, *this, visited)) {
                throw CircularDependencyException("Circular dependency");
            }
        }
    } catch (...) {
        cell.Set(old_text);
        throw;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    ValidatePosition(pos);
    if (pos.row >= static_cast<int>(cells_.size()) || pos.col >= static_cast<int>(cells_[pos.row].size()))
    return nullptr;
    return cells_[pos.row][pos.col].get();
}

const CellInterface* Sheet::GetCell(Position pos) const {
    ValidatePosition(pos);
    if (pos.row >= static_cast<int>(cells_.size()) || pos.col >= static_cast<int>(cells_[pos.row].size()))
    return nullptr;
    return cells_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) {
    ValidatePosition(pos);
    if (pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size()))
        cells_[pos.row][pos.col].reset();
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0, max_col = 0;
    for (int r = 0; r < static_cast<int>(cells_.size()); ++r) {
        for (int c = 0; c < static_cast<int>(cells_[r].size()); ++c) {
            if (cells_[r][c] && !cells_[r][c]->GetText().empty()) {
                max_row = max(max_row, r + 1);
                max_col = max(max_col, c + 1);
            }
        }
    }
    return {max_row, max_col};
}

void Sheet::PrintValues(std::ostream& out) const {
    Size sz = GetPrintableSize();
    for (int r = 0; r < sz.rows; ++r) {
        for (int c = 0; c < sz.cols; ++c) {
            if (c > 0) out << '\t';  
            const CellInterface* cell = nullptr;
            if (r < static_cast<int>(cells_.size()) && 
                c < static_cast<int>(cells_[r].size())) {
                cell = cells_[r][c].get();
            }
            
            if (cell) {
                auto val = cell->GetValue();
                if (holds_alternative<double>(val)) {
                    out << get<double>(val);
                } else if (holds_alternative<string>(val)) {
                    out << get<string>(val);
                } else if (holds_alternative<FormulaError>(val)) {
                    out << FormulaErrorToString(get<FormulaError>(val));
                }
            }
        }
        out << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& out) const {
    Size sz = GetPrintableSize();
    for (int r = 0; r < sz.rows; ++r) {
        for (int c = 0; c < sz.cols; ++c) {
            if (c > 0) out << '\t';
            
            const CellInterface* cell = nullptr;
            if (r < static_cast<int>(cells_.size()) && 
                c < static_cast<int>(cells_[r].size())) {
                cell = cells_[r][c].get();
            }
            
            if (cell) {
                out << cell->GetText();
            }
        }
        out << '\n';
    }
}

unique_ptr<SheetInterface> CreateSheet() {
    return make_unique<Sheet>();
}

















