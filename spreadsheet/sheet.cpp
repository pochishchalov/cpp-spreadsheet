#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos) {
    if (size_.rows < pos.row + 1) {
        size_.rows = pos.row + 1;
    }
    if (size_.cols < pos.col + 1) {
        size_.cols = pos.col + 1;
    }
}

void Sheet::MaybeFitSizeToClearPosition(Position pos) {
    if (pos.row + 1 == size_.rows || pos.col + 1 == size_.cols) {
        int row_size = 0, col_size = 0;
        for (const auto& [pos, cell] : data_) {
            row_size = std::max(pos.row + 1, row_size);
            col_size = std::max(pos.row + 1, col_size);
        }
        size_.rows = row_size;
        size_.cols = col_size;
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    Size old_size = size_;
    bool is_new_cell = false;
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range"s);
    }
    auto cell = GetConcreteCell(pos);
    if (cell == nullptr) {
        is_new_cell = true;
        cell = NewCell(pos);
    }
    else {
        // Если текст в ячейке не отличается, то ничего не происходит
        if (cell->GetText() == text) {
            return;
        }
    }
    // Временная ячейка, гарантирует неизменность значения при выбрасывании исключения
    std::unique_ptr<Cell> temp_cell(new Cell(*this));
    try {
        temp_cell->Set(text);
    }
    catch (InvalidPositionException& exc) {
        if (is_new_cell) {
            data_.erase(pos);
            size_ = old_size;
        }
        throw exc;
    }
    // Проверка на циклическую зависимость ячеек
    if (temp_cell->FindCircularDependency(cell)) {
        if (is_new_cell) {
            data_.erase(pos);
            size_ = old_size;
        }
        throw CircularDependencyException("circular dependency"s);
    }
    cell->ResetContent(temp_cell.get());
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range"s);
    }
    const auto ptr = data_.find(pos);
    return (ptr != data_.end()) ? data_.at(pos).get() : nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range"s);
    }
    const auto ptr = data_.find(pos);
    return (ptr != data_.end()) ? data_.at(pos).get() : nullptr;
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}
CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range"s);
    }
    auto cell = GetConcreteCell(pos);
    if (cell != nullptr) {
        // Если на ячейку сслылаются другие ячейки - ячейка не удаляется, а только очищяется ее значение
        if (cell->IsReferenced()) {
            cell->Clear();
        }
        else {
            cell->Clear();
            data_.erase(pos);
            MaybeFitSizeToClearPosition(pos);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int y = 0; y < size_.rows; ++y) {
        bool is_first = true;
        for (int x = 0; x < size_.cols; ++x) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            const auto cell = GetCell({ y, x });
            if (cell == nullptr) {
                continue;
            }
            const auto& value = cell->GetValue();
            if (std::holds_alternative<std::string>(value)) {
                output << std::get<std::string>(value);
            }
            else if (std::holds_alternative<double>(value)) {
                output << std::get<double>(value);
            }
            else {
                output << std::get<FormulaError>(value);
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int y = 0; y < size_.rows; ++y) {
        bool is_first = true;
        for (int x = 0; x < size_.cols; ++x) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            const auto cell = GetCell({ y, x });
            if (cell == nullptr) {
                continue;
            }
            output << cell->GetText();
        }
        output << '\n';
    }
}

Cell* Sheet::NewCell(Position pos) {
    MaybeIncreaseSizeToIncludePosition(pos);
    data_[pos].reset(new Cell(*this));
    return data_[pos].get();
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}