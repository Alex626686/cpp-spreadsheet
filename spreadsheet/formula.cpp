#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::FormulaError(Category category)
    : category_(category) {}

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
    }
    return "";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) {
    }
    catch (const std::exception& exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        const auto func = [&sheet](const Position pos)->double {
            const auto* cell = sheet.GetCell(pos);
            if (!cell) {
                return 0;
            }
            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }
            if (std::holds_alternative<std::string>(cell->GetValue())) {
                std::string value = std::get<std::string>(cell->GetValue());                
                if (!value.empty()) {
                    size_t nan_pos = 0;
                    try {
                        double res = std::stod(value, &nan_pos);
                        if (value.size() > nan_pos) {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                        return res;
                    }
                    catch (...) {}                    
                }
            }
            throw FormulaError(FormulaError::Category::Value);
        };

        try {
            return ast_.Execute(func);
        }
        catch(const FormulaError& error){
            return error;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        const auto& cells = ast_.GetCells();
        std::vector<Position> res = std::vector<Position>(cells.begin(), cells.end());
        std::sort(res.begin(), res.end());
        res.resize(std::unique(res.begin(), res.end()) - res.begin());
        return res;
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}