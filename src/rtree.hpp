////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RTREE_HPP
#define RTREE_HPP

#include "bounds.hpp"
#include <memory>
#include <utility>
#include <variant>
#include <stack>
#include <iterator>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <vector>

template <typename Element>
class RTree {
	using RTreePtr = std::unique_ptr<RTree>;
	using Children = std::pair<RTreePtr, RTreePtr>;
	using Value = std::variant<Children, Element>;

	Bounds bounds;
	Value value;

	auto leaf() const {
		return std::holds_alternative<Element>(value);
	}

	auto &children() const {
		return std::get<Children>(value);
	}

	auto &children() {
		return std::get<Children>(value);
	}

	auto &element() const {
		return std::get<Element>(value);
	}

	struct Search : std::stack<RTree const *> {
		Bounds const &bounds;

		struct Iterator {
			using iterator_category = std::input_iterator_tag;
			using value_type        = Element const;
			using reference         = Element const &;
			using pointer           = void;
			using difference_type   = void;

			Search &search;
			std::size_t index;

			auto &next() {
				while (!search.empty()) {
					auto const &rtree = *search.top();
					if (!(rtree.bounds && search.bounds))
						search.pop();
					else if (rtree.leaf())
						break;
					else {
						auto const &[rtree1, rtree2] = rtree.children();
						search.pop();
						search.push(rtree1.get());
						search.push(rtree2.get());
					}
				}
				index = search.empty() ? 0 : index + 1;
				return *this;
			}

			Iterator(Search &search) : search(search), index(0) { }

			auto &operator++() {
				search.pop();
				return next();
			}

			auto operator==(Iterator const &other) const {
				return other.index == index;
			}

			auto operator!=(Iterator const &other) const {
				return other.index != index;
			}

			auto &operator*() const {
				return search.top()->element();
			}
		};

		Search(Bounds const &bounds, RTree const *root) : bounds(bounds) {
			if (!root->bounds.empty())
				this->push(root);
		}

		auto begin() { return Iterator(*this).next(); }
		auto   end() { return Iterator(*this); }
	};

public:
	RTree(Element const &element) :
		bounds(element),
		value(element)
	{ }

	template <typename Iterator>
	RTree(Iterator begin, Iterator end, bool horizontal, int threads) {
		switch (end - begin) {
		case 0:
			break;
		case 1:
			bounds = Bounds(*begin);
			value = *begin;
			break;
		default:
			auto const middle = begin + (end - begin) / 2;
			std::nth_element(begin, middle, end, [=](auto const &element1, auto const &element2) {
				if (horizontal)
					return Bounds(element1).xmin < Bounds(element2).xmin;
				else
					return Bounds(element1).ymin < Bounds(element2).ymin;
			});
			auto rtree1 = RTreePtr();
			auto rtree2 = RTreePtr();
			if (1 == threads) {
				rtree1 = std::make_unique<RTree>(begin, middle, !horizontal, 1);
				rtree2 = std::make_unique<RTree>(middle,   end, !horizontal, 1);
			} else {
				auto thread1 = std::thread([&]() {
					rtree1 = std::make_unique<RTree>(begin, middle, !horizontal, threads/2);
				}), thread2 = std::thread([&]() {
					rtree2 = std::make_unique<RTree>(middle,   end, !horizontal, threads - threads/2);
				});
				thread1.join(), thread2.join();
			}
			bounds = rtree1->bounds + rtree2->bounds;
			value = Children(std::move(rtree1), std::move(rtree2));
		}
	}

	RTree(std::vector<Element> &elements, int threads) : RTree(elements.begin(), elements.end(), true, threads) { }

	auto search(Bounds const &bounds) const {
		return Search(bounds, this);
	}

	enum { found_none = 0, found_leaf, found_branch };

	auto erase(Element const &element, Bounds const &element_bounds) {
		if (leaf())
			return this->element() == element ? found_leaf : found_none;
		if (element_bounds <= bounds) {
			auto const &[rtree1, rtree2] = children();
			switch (rtree1->erase(element, element_bounds)) {
			case found_none: break;
			case found_branch:
				bounds = rtree1->bounds + rtree2->bounds;
				return found_branch;
			case found_leaf:
				auto &rtree = *rtree2;
				std::swap(rtree, *this);
				auto &[rtree1, rtree2] = rtree.children();
				rtree1.reset(), rtree2.reset();
				return found_branch;
			}
			switch (rtree2->erase(element, element_bounds)) {
			case found_none: break;
			case found_branch:
				bounds = rtree1->bounds + rtree2->bounds;
				return found_branch;
			case found_leaf:
				auto &rtree = *rtree1;
				std::swap(rtree, *this);
				auto &[rtree1, rtree2] = rtree.children();
				rtree2.reset(), rtree1.reset();
				return found_branch;
			}
		}
		return found_none;
	}

	auto replace(Element const &element, Bounds const &old_bounds, Element const &element1, Element const &element2) {
		if (leaf()) {
			if (this->element() == element) {
				auto rtree1 = std::make_unique<RTree>(element1);
				auto rtree2 = std::make_unique<RTree>(element2);
				bounds = rtree1->bounds + rtree2->bounds;
				value = Children(std::move(rtree1), std::move(rtree2));
				return true;
			}
		} else if (old_bounds <= bounds) {
			auto const &[rtree1, rtree2] = children();
			if (rtree1->replace(element, old_bounds, element1, element2) || rtree2->replace(element, old_bounds, element1, element2)) {
				bounds = rtree1->bounds + rtree2->bounds;
				return true;
			}
		}
		return false;
	}

	auto update(Element const &element, Bounds const &old_bounds) {
		if (leaf()) {
			if (this->element() == element) {
				bounds = Bounds(element);
				return true;
			}
		} else if (old_bounds <= bounds) {
			auto const &[rtree1, rtree2] = children();
			if (rtree1->update(element, old_bounds) || rtree2->update(element, old_bounds)) {
				bounds = rtree1->bounds + rtree2->bounds;
				return true;
			}
		}
		return false;
	}
};

#endif
