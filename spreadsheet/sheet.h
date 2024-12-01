#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Cell;

class Sheet : public SheetInterface {
public:
    // ������������� �������� ������,
    // ����������� ������������� ��������� �������
    void SetCell(Position pos, std::string text) override;

    // ������� ����� ������ ������ �������
    Cell* NewCell(Position pos);

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    // ���������� ������ �������� ������� �������
    Size GetPrintableSize() const override;

    // ������� �������� ����� ������� � �����
    void PrintValues(std::ostream& output) const override;

    // ������� ����� ����� ������� � �����
    void PrintTexts(std::ostream& output) const override;
    
    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);
    
private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> data_;
    Size size_;

    // ��� ������������� ����������� �������� ������� �������,
    // ���������� ������ � ������ NewCell
    void MaybeIncreaseSizeToIncludePosition(Position pos);

    // ��� ������������� ��������� �������� ������� �������,
    // ���������� ������ � ������ ClearCell 
    void MaybeFitSizeToClearPosition(Position pos);

    std::string GetBoundary(int width) const;
    void PrintTableHeader(std::ostream& output) const;

};