#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression))
    {
    }
    catch (const std::exception& exc)
    {
        std::throw_with_nested(FormulaException(exc.what()));
    }
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        }
        catch (FormulaError error) {
            return error;
        }
        
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells()  const override {
        const auto positions = ast_.GetCells();
        std::vector<Position> result{ positions.begin(), positions.end() };
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    };

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}