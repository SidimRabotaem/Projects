#pragma once

#include "cell.h"
#include "common.h"

#include <memory>
#include <vector>
#include <unordered_set>
#include <string>

struct PositionHash {
    size_t operator()(const Position& pos) const {
        return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);
    }
};

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet() override = default;

    void SetCell(Position pos, std::string text) override;
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    void ValidatePosition(Position pos) const;
    void EnsureGridSize(Position pos);
    Size CalculatePrintableSize() const;
    bool IsCellPrintable(const Cell* cell) const;
    Cell* GetCellRaw(Position pos);
    const Cell* GetCellRaw(Position pos) const;
    
    bool HasCircularDependency(Position pos, const std::vector<Position>& new_deps) const;
    bool CheckLeadsTo(Position from, Position to,std::unordered_set<Position, PositionHash>& visited) const;;
    void UpdateDependencies(Position pos, const std::vector<Position>& new_deps);

};
