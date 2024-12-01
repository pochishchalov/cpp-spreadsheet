#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);

    // Устанавливает значение в ячейке
    void Set(std::string text);

    // Очищаяет значение ячейки
    void Clear();

    // Возвращает значение содержащаеся в ячейке
    Value GetValue() const override;
    // Возвращает тескт содержащийся в ячейке
    std::string GetText() const override;

    // Возвращает позиции ячеек на которые ссылается данная ячейка
    std::vector<Position> GetReferencedCells() const override;
    
    // Находит циклические зависимости, используется только при изменении
    // ячейки таблицы методом SetCell
    bool FindCircularDependency(Cell* cell);

    // Меняет содержимое одной ячейки на содержимое другой
    // при этом связь с ячейками которые ссылались на текущую ячейку остается,
    // а связи с ячейками на которые ссылались обе ячейки - обновляются,
    // после чего происходит инвалидация кэша
    void ResetContent(Cell* other);

    bool IsReferenced() const;

private:
    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual ~Impl() = default;
    };
    // Пустая ячейка
    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override { return std::string{}; };
        std::string GetText() const override { return {}; };
        std::vector<Position> GetReferencedCells() const override { return {}; }
    };
    // Текстовое представление ячейки
    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text, bool apostrophe = false)
            :text_value_(std::move(text))
            ,apostrophe_(apostrophe)
        {
        }
        Value GetValue() const override {
            try {
                double value = std::stod(text_value_);
                return value;
                
            }
            catch (...) {
                return text_value_;
            }
        };
        std::string GetText() const override {
            return (apostrophe_) ? ESCAPE_SIGN + text_value_ : text_value_;
        };
        std::vector<Position> GetReferencedCells() const override { return {}; }
        std::string text_value_;
        bool apostrophe_;
    };
    // Формульное представление ячейки
    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string expr, const SheetInterface& sheet)
            :formula_(ParseFormula(std::move(expr)))
            ,sheet_(sheet)
        {
        }
        Value GetValue() const override {
            auto value = formula_.get()->Evaluate(sheet_);
            if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            }
            else {
                return std::get<FormulaError>(value);
            }
        };
        std::string GetText() const override {
            return FORMULA_SIGN + formula_.get()->GetExpression();
        };
        std::vector<Position> GetReferencedCells() const override {
            return formula_.get()->GetReferencedCells();
        }
        std::unique_ptr<FormulaInterface> formula_;
        const SheetInterface& sheet_;
    };

    std::unique_ptr<Impl> impl_;

    // Ссылка на таблицу где хранится ячейка
    Sheet& sheet_;

    // Кэш значения, инвалидируется при изменении ячеки или изменении ячеек
    // на кторорые ссылается текущая ячейка
    mutable std::optional<Cell::Value> cache_value_;

    // Хранит связь с ячейками которые ссылаются на текущую ячейку
    std::unordered_set<Cell*> parents_;

    // Хранит связь с ячейками на которые ссылается данная ячейка
    std::vector<Cell*> childrens_;

    // Добавляет связь с ячейками задействованными в текущей ячейке
    void AddChildrens();

    // Добавляет связь с ячейкой которая ссылается на текущую
    void AddParent(Cell* parent);

    // Удаляет связь с ячейкой которая ссылалась на текущую
    void EraseParent(Cell* parent);

    // Инвалидация значения хранящегося в кэше
    void CacheInvalidation();
};