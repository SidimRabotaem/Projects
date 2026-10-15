#include "cell.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <memory>
#include <string>
#include <algorithm>
#include <variant>
#include <unordered_set>

using namespace std;

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue(const SheetInterface& sheet) const = 0;
    virtual string GetText() const = 0;
    virtual vector<Position> GetReferencedCells() const = 0;
    virtual bool IsEmpty() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue(const SheetInterface& /*sheet*/) const override {
        return string{};
    }

    string GetText() const override {
        return "";
    }

    vector<Position> GetReferencedCells() const override {
        return {};
    }

    bool IsEmpty() const override { return true; }
};

class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(string text) : text_(move(text)) {}

    Value GetValue(const SheetInterface& /*sheet*/) const override {
        if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    string GetText() const override {
        return text_;
    }

    vector<Position> GetReferencedCells() const override {
        return {};
    }

    bool IsEmpty() const override { return false; }

private:
    string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(unique_ptr<FormulaInterface> formula)
        : formula_(move(formula)) {}

    Value GetValue(const SheetInterface& sheet) const override {
        auto result = formula_->Evaluate(sheet);
        if (holds_alternative<double>(result)) {
            return get<double>(result);
        } else {
            return get<FormulaError>(result);
        }
    }

    string GetText() const override {
        return string(1, FORMULA_SIGN) + formula_->GetExpression();
    }

    vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    bool IsEmpty() const override { 
        return false;
    }

private:
    unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(Sheet& sheet, Position pos)
    : sheet_(sheet)
    , my_pos_(pos)
    , impl_(make_unique<EmptyImpl>()) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    auto old_impl = move(impl_);

    try {
        if (text.empty()) {
            impl_ = make_unique<EmptyImpl>();
        } else if (text[0] == FORMULA_SIGN && text.size() > 1) {
            auto formula = ParseFormula(text.substr(1));
            impl_ = make_unique<FormulaImpl>(move(formula));
        } else {
            impl_ = make_unique<TextImpl>(move(text));
        }
    } catch (const exception& e) {
        impl_ = move(old_impl);
        if (!text.empty() && text[0] == FORMULA_SIGN) {
            throw FormulaException(e.what());
        }
        throw;
    }

    SetDependencies(impl_->GetReferencedCells());
    InvalidateCache();

    unordered_set<Position, PositionHasher> visited;
    visited.insert(my_pos_);
    InvalidateDependentCaches(visited);
}

void Cell::Clear() {
    impl_ = make_unique<EmptyImpl>();
    SetDependencies({});
    InvalidateCache();

    unordered_set<Position, PositionHasher> visited;
    visited.insert(my_pos_);
    InvalidateDependentCaches(visited);
}

CellInterface::Value Cell::GetValue() const {
    if (!cache_valid_) {
        cached_value_ = ComputeValue();
        cache_valid_ = true;
    }
    return *cached_value_;
}

string Cell::GetText() const {
    return impl_->GetText();
}

vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::AddDependent(Position pos) {
    if (pos.IsValid() && find(dependents_.begin(), dependents_.end(), pos) == dependents_.end()) {
        dependents_.push_back(pos);
    }
}

void Cell::RemoveDependent(Position pos) {
    auto it = find(dependents_.begin(), dependents_.end(), pos);
    if (it != dependents_.end()) {
        dependents_.erase(it);
    }
}

const vector<Position>& Cell::GetDependents() const {
    return dependents_;
}

void Cell::InvalidateCache() {
    cache_valid_ = false;
    cached_value_ = nullopt;
}

void Cell::InvalidateDependentCaches(unordered_set<Position, PositionHasher>& visited) {
    for (const auto& dep : dependents_) {
        if (visited.count(dep)) continue;
        visited.insert(dep);

        auto* dep_cell = dynamic_cast<Cell*>(sheet_.GetCell(dep));
        if (dep_cell) {
            dep_cell->InvalidateCache();
            dep_cell->InvalidateDependentCaches(visited);
        }
    }
}

void Cell::SetDependencies(const vector<Position>& new_deps) {
    for (const auto& old_dep : dependencies_) {
        if (find(new_deps.begin(), new_deps.end(), old_dep) == new_deps.end()) {
            auto* old_cell = dynamic_cast<Cell*>(sheet_.GetCell(old_dep));
            if (old_cell) old_cell->RemoveDependent(my_pos_);
        }
    }

    for (const auto& new_dep : new_deps) {
        if (find(dependencies_.begin(), dependencies_.end(), new_dep) == dependencies_.end()) {
            auto* new_cell = dynamic_cast<Cell*>(sheet_.GetCell(new_dep));
            if (new_cell) new_cell->AddDependent(my_pos_);
        }
    }

    dependencies_ = new_deps;
}

bool Cell::IsEmpty() const {
    return impl_->IsEmpty();
}

CellInterface::Value Cell::ComputeValue() const {
    try {
        return impl_->GetValue(sheet_);
    } catch (const FormulaError& fe) {
        return fe;
    } catch (...) {
        return FormulaError(FormulaError::Category::Value);
    }
}

double Cell::GetCellValue(Position pos) const {
    auto* cell = sheet_.GetCell(pos);
    if (!cell) return 0.0;

    Value val = cell->GetValue();
    if (holds_alternative<double>(val)) {
        return get<double>(val);
    }
    return 0.0;
}





