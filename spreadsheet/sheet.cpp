#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (!table_.count(pos)) {
        table_.emplace(pos, std::make_unique<Cell>(*this));
        ++rows_map[pos.row];
        ++cols_map[pos.col];
    }
    table_.at(pos)->Set(std::move(text));
}

const Cell* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (!table_.count(pos)) {
        return nullptr;
    }
    const Cell* cell = table_.at(pos).get();
    if (!cell || cell->GetText().size() == 0) {
        return nullptr;
    }
    return cell;
}
Cell* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (!table_.count(pos)) {
        return nullptr;
    }
    Cell* cell = table_.at(pos).get();
    return cell;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    const auto& table_it = table_.find(pos);
    size_t refs_count = 1;
    if (table_it != table_.end() && table_it->second != nullptr) {
        refs_count += table_it->second->GetReferencedCells().size();
        table_it->second->Clear();
        table_it->second.reset();
    }
    for (; refs_count > 0; --refs_count) {
        --rows_map[pos.row];
        --cols_map[pos.col];
    }
    if (rows_map[pos.row] < 1) {
        rows_map.erase(pos.row);
    }
    if (cols_map[pos.col] < 1) {
        cols_map.erase(pos.col);
    }
}

Size Sheet::GetPrintableSize() const {
    int row = 0, col = 0;
    if (!rows_map.empty()) {
        row = rows_map.rbegin()->first + 1;
    }
    if (!cols_map.empty()) {
        col = cols_map.rbegin()->first + 1;
    }
    return { row, col};
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
                if (table_.count({ row, col })) {
                const auto cell = GetCell({ row, col });
                if (cell != nullptr) {
                    auto value = cell->GetValue();
                    std::visit(
                        [&](const auto& x) {
                            output << x;
                        },
                        value);
                }
            }
        }
        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            if (table_.count({ row, col })) {
                const auto cell = GetCell({ row, col });
                if (cell != nullptr) {
                    output << cell->GetText();
                }
            }
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}