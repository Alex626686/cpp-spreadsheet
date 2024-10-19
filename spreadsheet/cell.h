#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;
class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:

    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const {
            return {};
        }
        virtual bool CacheValid() const {
            return true;
        }
        virtual void ResetCache() {}

        virtual ~Impl() = default;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override {
            return "";
        }
        std::string GetText() const override {
            return "";
        }
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view text)
            : text_(std::move(text)) {}

        Value GetValue() const override {
            if (text_.length() > 0 && text_[0] == '\'') {
                return text_.substr(1);
            }
            return text_;
        }
        std::string GetText() const override {
            return text_;
        }
    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view expr, const SheetInterface& sheet)
            : formula_ptr_(ParseFormula(expr.substr(1).data())), sheet_(sheet) {}

        Value GetValue() const override {
            if (!cache_) {
                cache_ = formula_ptr_->Evaluate(sheet_);
            }
            if (!std::holds_alternative<double>(cache_.value())) {
                return std::get<FormulaError>(cache_.value());
            }
            return std::get<double>(cache_.value());
        }

        std::string GetText() const override {
            return FORMULA_SIGN + formula_ptr_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const {
            return formula_ptr_->GetReferencedCells();
        }

        bool CacheValid() const override {
            return cache_.has_value();
        }

        void ResetCache() override {
            cache_.reset();
        }


    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;
        const SheetInterface& sheet_;
        mutable std::optional<FormulaInterface::Value> cache_;
    };

    bool HasCyclic_Dependencies(Impl& impl) const;
    void InvalidateCache();

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> l_cells_;
    std::unordered_set<Cell*> r_cells_;
};