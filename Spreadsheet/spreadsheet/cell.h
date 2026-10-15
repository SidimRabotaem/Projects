#pragma once

#include "common.h"
#include "formula.h"

#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <optional>
#include <unordered_set>

class Sheet;

struct PositionHasher {
    size_t operator()(const Position& pos) const {
        return static_cast<size_t>(pos.row) * 37 + static_cast<size_t>(pos.col);
    }
};

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet, Position pos);
    
    ~Cell();
 
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsEmpty() const;
   
    void Set(std::string text);
    void Clear();
    void AddDependent(Position pos);
    void RemoveDependent(Position pos);
    void SetDependencies(const std::vector<Position>& new_deps);
    
    const std::vector<Position>& GetDependents() const;
    
    void InvalidateCache();
    void InvalidateDependentCaches(std::unordered_set<Position, PositionHasher>& visited);
    
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    
    Sheet& sheet_;
    Position my_pos_;
    
    std::unique_ptr<Impl> impl_;
    std::vector<Position> dependencies_;
    std::vector<Position> dependents_;
    
    mutable std::optional<Value> cached_value_;
    mutable bool cache_valid_ = false;
    
    CellInterface::Value ComputeValue() const;
    double GetCellValue(Position pos) const;
};





