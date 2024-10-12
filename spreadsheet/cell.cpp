#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(Sheet& sheet)
    :impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet)
{
}

void Cell::Set(std::string text) {
    auto size = text.size();
    if (size == 0) {
        return;
    }
    auto iter = text.begin();
    if (*iter == FORMULA_SIGN && size > 1) {
        ++iter;
        impl_.reset(new FormulaImpl(std::string{ std::make_move_iterator(iter),
            std::make_move_iterator(text.end()) }, sheet_));
        AddChildrens();
        return;
    }
    if (*iter == ESCAPE_SIGN) {
        ++iter;
        impl_.reset(new TextImpl(std::string{ std::make_move_iterator(iter),
        std::make_move_iterator(text.end()) }, true));
        return;
    }
    else {
        impl_.reset(new TextImpl(std::string{ std::make_move_iterator(iter),
            std::make_move_iterator(text.end()) }));
    }
}

void Cell::Clear() {
    for (auto child : childrens_) {
        child->EraseParent(this);
    }
    impl_.reset(new EmptyImpl());
}

Cell::Value Cell::GetValue() const {
    if (!cache_value_.has_value()) {
        cache_value_.emplace(impl_.get()->GetValue());
    }
    return cache_value_.value();
}
std::string Cell::GetText() const {
    return impl_.get()->GetText();
}

void Cell::AddChildrens() {
    const auto& ref_cells = GetReferencedCells();
    std::vector<Position> new_cells(ref_cells.size());
    try {
        for (auto pos : ref_cells) {
            auto cell = sheet_.GetConcreteCell(pos);
            if (cell == nullptr) {
                cell = sheet_.NewCell(pos);
                new_cells.push_back(pos);
            }
            childrens_.push_back(cell);
        }
    }
    catch (InvalidPositionException& exc) {
        for (auto new_cell : new_cells) {
            sheet_.ClearCell(new_cell);
        }
        throw exc;
    }
}

bool Cell::FindCircularDependency(Cell* cell) {
    for (auto child : childrens_) {
        if (child == cell) {
            return true;
        }
        if (child->FindCircularDependency(cell)) {
            return true;
        }
    }
    return false;
}

void Cell::AddParent(Cell* parent) {
    parents_.insert(parent);
}

void Cell::EraseParent(Cell* parent) {
    auto ptr = parents_.find(parent);
    if (ptr != parents_.end()) {
        parents_.erase(ptr);
    }
}

void Cell::CacheInvalidation() {
    cache_value_.reset();
    for (auto& parent : parents_) {

        parent->CacheInvalidation();
    }
}

void Cell::ResetContent(Cell* other) {
    for (auto child : childrens_) {
        child->EraseParent(this);
    }
    std::swap(childrens_, other->childrens_);
    std::swap(impl_, other->impl_);
    for (auto child : childrens_) {
        child->AddParent(this);
    }
    CacheInvalidation();
}

bool Cell::IsReferenced() const {
    return !parents_.empty();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_.get()->GetReferencedCells();
}