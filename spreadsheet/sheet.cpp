#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <iomanip>

using namespace std::literals;

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos) {
    if (size_.rows < pos.row + 1 ) {
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
        // ≈сли текст в €чейке не отличаетс€, то ничего не происходит
        if (cell->GetText() == text) {
            return;
        }
    }
    // ¬ременна€ €чейка, гарантирует неизменность значени€ при выбрасывании исключени€
    std::unique_ptr<Cell> temp_cell (new Cell(*this));
    try {
        temp_cell->Set(text);
    }
    catch (InvalidPositionException &exc) {
        if (is_new_cell) {
            data_.erase(pos);
            size_ = old_size;
        }
        throw exc;
    }
    // ѕроверка на циклическую зависимость €чеек
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
        // ≈сли на €чейку сслылаютс€ другие €чейки - €чейка не удал€етс€, а только очищ€етс€ ее значение
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

int GetRowsHeaderSize(int rows) {
    int result = 0;
    while (rows > 0) {
        rows /= 10;
        ++result;
    }
    return result;
}

std::string Sheet::GetBoundary(int width) const {
    std::string result;
    auto size = GetPrintableSize();
    int rows_header_size = GetRowsHeaderSize(size.rows);
    for (size_t i = 0; i < rows_header_size; ++i) {
        result += '-';
    }
    result += '|';
    for (int i = 0; i < size.cols; ++i)
    {
        for (int j = 0; j < width; ++j) {
            result += '-';
        }
        result += '|';
    }
    
    return result;
}

void Sheet::PrintTableHeader(std::ostream& output) const {
    int rows_header_size = GetRowsHeaderSize(size_.rows);
    for (size_t i = 0; i < rows_header_size; ++i) {
        output << ' ';
    }
    output << '|';
    for (size_t i = 0; i < size_.cols; ++i)
    {
        int c = i;
        std::string result;
        result.reserve(17);
        while (c >= 0) {
            result.insert(result.begin(), 'A' + c % 26);
            c = c / 26 - 1;
        }
        output << std::setw(12) << result << '|';
    }
    output << '\n';
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintTableHeader(output);
    int rows_header_size = GetRowsHeaderSize(size_.rows);
    output << GetBoundary(12) << '\n';
    for (int y = 0; y < size_.rows; ++y) {
        bool is_first = true;
        for (int x = 0; x < size_.cols; ++x) {
            if (is_first) {
                output << std::setw(rows_header_size) << y + 1 << '|';
            }
            else {
                output << '|';
            }
            is_first = false;
            const auto cell = GetCell({ y, x });
            if (cell == nullptr) {
                output << "            "s;
                continue;
            }
            const auto& value = cell->GetValue();
            if (std::holds_alternative<double>(value)) {
                output << std::setw(12) << std::get<double>(value);
            }
            else if (std::holds_alternative<std::string>(value)) {
                std::string text = std::get<std::string>(value);
                if (text.size() <= 12) {
                    output << std::setw(12) << text;
                }
                else {
                    output << std::string_view(text.data(), 9) << "..."s;
                }
            }
            else {
                output << std::setw(12) << std::get<FormulaError>(value);
            }
        }
        output << '|' << '\n' << GetBoundary(12) << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintTableHeader(output);
    int rows_header_size = GetRowsHeaderSize(size_.rows);
    output << GetBoundary(12) << '\n';
    for (int y = 0; y < size_.rows; ++y) {
        bool is_first = true;
        for (int x = 0; x < size_.cols; ++x) {
            if (is_first) {
                output << std::setw(rows_header_size) << y + 1 << '|';
            }
            else {
                output << '|';
            }
            is_first = false;
            const auto cell = GetCell({ y, x });
            if (cell == nullptr) {
                output << "            "s;
                continue;
            }
            std::string  text = cell->GetText();
            if (text.size() <= 12) {
                output << std::setw(12) << text;
            }
            else {
                output << std::string_view(text.data(), 9) << "..."s;
            }
        }
        output << '|' << '\n' << GetBoundary(12) << '\n';
    }
}

/*

* old versions

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
*/

Cell* Sheet::NewCell(Position pos) {
    MaybeIncreaseSizeToIncludePosition(pos);
    data_[pos].reset(new Cell(*this));
    return data_[pos].get();
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}