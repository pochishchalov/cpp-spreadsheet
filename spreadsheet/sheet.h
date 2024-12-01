#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Cell;

class Sheet : public SheetInterface {
public:
    // Устанавливает значение ячейки,
    // гарантирует консистентное состояние таблицы
    void SetCell(Position pos, std::string text) override;

    // Создает новую пустую ячейку таблицы
    Cell* NewCell(Position pos);

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    // Возвращает размер печатной области таблицы
    Size GetPrintableSize() const override;

    // Выводит значения ячеек таблицы в поток
    void PrintValues(std::ostream& output) const override;

    // Выводит текст ячеек таблицы в поток
    void PrintTexts(std::ostream& output) const override;
    
    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);
    
private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> data_;
    Size size_;

    // При необходимости увеличивает печатную область таблицы,
    // вызывается только в методе NewCell
    void MaybeIncreaseSizeToIncludePosition(Position pos);

    // При необходимости уменьшает печатную область таблицы,
    // вызывается только в методе ClearCell 
    void MaybeFitSizeToClearPosition(Position pos);

    std::string GetBoundary(int width) const;
    void PrintTableHeader(std::ostream& output) const;

};