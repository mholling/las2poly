////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RTREE_HPP
#define RTREE_HPP

#include "bounds.hpp"
#include "ring.hpp"
#include <memory>
#include <utility>
#include <variant>
#include <stack>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <vector>

template <typename Element>
class RTree {
	struct Node {
		using NodePtr = std::unique_ptr<Node>;
		using Children = std::pair<NodePtr, NodePtr>;
		using Value = std::variant<Element, Children>;

		Bounds bounds;
		Value value;

		Node(Element const &element) :
			bounds(element.bounds()),
			value(element)
		{ }

		Node(NodePtr &&node1, NodePtr &&node2) :
			bounds(node1->bounds + node2->bounds),
			value(std::in_place_type<Children>, std::move(node1), std::move(node2))
		{ }

		auto is_leaf() const {
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

		struct Search : std::stack<Node const *> {
			Bounds const &bounds;

			struct Iterator {
				using iterator_category = std::input_iterator_tag;
				using value_type        = Element;
				using reference         = Element &;
				using pointer           = void;
				using difference_type   = void;

				Search &search;
				std::size_t index;

				auto &next() {
					while (!search.empty()) {
						auto const &node = *search.top();
						if (!(node.bounds && search.bounds))
							search.pop();
						else if (node.is_leaf())
							break;
						else {
							auto const &[node1, node2] = node.children();
							search.pop();
							search.push(node1.get());
							search.push(node2.get());
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

			Search(Bounds const &bounds, Node const *root) : bounds(bounds) {
				this->push(root);
			}

			auto begin() { return Iterator(*this).next(); }
			auto   end() { return Iterator(*this); }
		};

		auto search(Bounds const &bounds) const {
			return Search(bounds, this);
		}

		auto erase(Element const &element, Bounds const &element_bounds) {
			if (is_leaf())
				return this->element() == element;
			if (!(bounds && element_bounds))
				return false;
			auto const &[node1, node2] = children();
			if (node1->erase(element, element_bounds))
				if (node1->is_leaf()) {
					auto &node = *node2;
					std::swap(node, *this);
					auto &[node1, node2] = node.children();
					node1.reset(), node2.reset();
				} else
					bounds = node1->bounds + node2->bounds;
			else if (node2->erase(element, element_bounds))
				if (node2->is_leaf()) {
					auto &node = *node1;
					std::swap(node, *this);
					auto &[node1, node2] = node.children();
					node2.reset(), node1.reset();
				} else
					bounds = node1->bounds + node2->bounds;
			else
				return false;
			return true;
		}

		auto update(Element const &element, Bounds const &old_bounds) {
			if (is_leaf()) {
				if (this->element() != element)
					return false;
				bounds = element.bounds();
				return true;
			}
			if (!(old_bounds && bounds))
				return false;
			auto const &[node1, node2] = children();
			if (node1->update(element, old_bounds))
				bounds = node1->bounds + node2->bounds;
			else if (node2->update(element, old_bounds))
				bounds = node1->bounds + node2->bounds;
			else
				return false;
			return true;
		}
	};

	using NodePtr = std::unique_ptr<Node>;

	NodePtr root;

	template <typename Iterator, bool horizontal = true>
	auto static partition(Iterator begin, Iterator end) {
		switch (end - begin) {
		case 0:
			throw std::runtime_error("not enough points");
		case 1:
			return std::make_unique<Node>(*begin);
		default:
			auto const middle = begin + (end - begin) / 2;
			std::nth_element(begin, middle, end, [](auto const &element1, auto const &element2) {
				if constexpr (horizontal)
					return element1.bounds().xmin < element2.bounds().xmin;
				else
					return element1.bounds().ymin < element2.bounds().ymin;
			});
			return std::make_unique<Node>(
				partition<Iterator, !horizontal>(begin, middle),
				partition<Iterator, !horizontal>(middle, end)
			);
		}
	}

public:
	RTree(std::vector<Element> &elements) : root(partition(elements.begin(), elements.end())) { }

	auto search(Bounds const &bounds) const {
		return root->search(bounds);
	}

	auto erase(Element const &element) {
		return root->erase(element, element.bounds());
	}

	auto update(Element const &element, Bounds const &old_bounds) const {
		return root->update(element, old_bounds);
	}
};

#endif