#include "cell.h"
#include "sheet.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet) {}

Cell::~Cell(){}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> tmp_impl;
	if (text.size() == 0) {
		tmp_impl = std::make_unique<EmptyImpl>();
	}
	else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		tmp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
	}
	else {
		tmp_impl = std::make_unique<TextImpl>(std::move(text));
	}

	if (HasCyclic_Dependencies(*tmp_impl)) {
		throw CircularDependencyException("");
	}
	impl_ = std::move(tmp_impl);
	InvalidateCache();
	ProcessingDependentCells();
}

void Cell::Clear() {
	Set("");
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const{
	return impl_->GetReferencedCells();
}

bool Cell::HasCyclic_Dependencies(Impl& impl) const{
	std::vector<Position> ref_cells_vec = impl.GetReferencedCells();
	if (ref_cells_vec.empty()) {
		return false;
	}
	std::unordered_set<const Cell*> ref_cells;
	for (const auto& pos : impl.GetReferencedCells()) {
		ref_cells.insert(sheet_.GetCell(pos));
	}
	std::unordered_set<const Cell*> passed_cells;
	std::stack<const Cell*> unpassed_cells;
	unpassed_cells.push(this);

	while (!unpassed_cells.empty()) {
		passed_cells.insert(unpassed_cells.top());
		
		if (ref_cells.count(unpassed_cells.top())) {
			return true;
		}

		const Cell* tmp_cell = unpassed_cells.top();
		unpassed_cells.pop();
		for (const Cell* incoming : tmp_cell->l_cells_) {
			if (passed_cells.find(incoming) == passed_cells.end()) unpassed_cells.push(incoming);
		}
	}
	return false;
}

void Cell::InvalidateCache(){
	impl_->ResetCache();
	if (impl_->CacheValid()) {
		for (Cell* incoming : l_cells_) {
			incoming->InvalidateCache();
		}
	}	
}

void Cell::ProcessingDependentCells(){
	for (Cell* r_cell : r_cells_) {
		r_cell->l_cells_.erase(this);
	}
	r_cells_.clear();

	for (const auto& pos : impl_->GetReferencedCells()) {
		Cell* r_cell = sheet_.GetCell(pos);
		if (!r_cell) {
			sheet_.SetCell(pos, "");
			r_cell = sheet_.GetCell(pos);
		}
		r_cells_.insert(r_cell);
		r_cell->l_cells_.insert(this);
	}
}