#include "formula.h"
#include "FormulaAST.h"

#include <sstream>
#include <variant>
#include <cmath>

using namespace std;

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(string expression)
        : ast_(ParseFormulaAST(expression)) {
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
    try {
        auto func = [&sheet](Position pos) -> double {
            const CellInterface* cell = sheet.GetCell(pos);
            if (!cell) {
                return 0.0;
            }

            auto value = cell->GetValue();
            if (holds_alternative<double>(value)) {
                return get<double>(value);
            } else if (holds_alternative<string>(value)) {
                const string& str = get<string>(value);
                if (str.empty()) {
                    return 0.0;
                }

                char* end = nullptr;
                double result = strtod(str.c_str(), &end);
                if (end != str.c_str() + str.size()) {
                    throw FormulaError(FormulaError::Category::Value);
                }
                return result;
            } else {
                throw get<FormulaError>(value);
            }
        };

        double result = ast_.Execute(func);

        if (!std::isfinite(result)) {
            return FormulaError(FormulaError::Category::Arithmetic);
        }

        return result;

    } catch (const FormulaError& fe) {
        return fe;
    } catch (...) {
        return FormulaError(FormulaError::Category::Value);
    }
}
    
    string GetExpression() const override {
        ostringstream oss;
        ast_.PrintFormula(oss);
        string result = oss.str();
        if (!result.empty() && result[0] == FORMULA_SIGN) {
            result = result.substr(1);
        }
        return result;
    }
    
    vector<Position> GetReferencedCells() const override {
        return ast_.GetReferencedCells();
    }
    
private:
    FormulaAST ast_;
};
}  // namespace

unique_ptr<FormulaInterface> ParseFormula(string expression) {
    return make_unique<Formula>(move(expression));
}

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Arithmetic:
            return "#ARITHM!";
        default:
            return "#ERROR!";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    switch (fe.GetCategory()) {
        case FormulaError::Category::Ref:
            return output << "#REF!";
        case FormulaError::Category::Value:
            return output << "#VALUE!";
        case FormulaError::Category::Arithmetic:
            return output << "#ARITHM!";
        default:
            return output << "#ERROR!";
    }
}