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
#include <cstddef>
#include <algorithm>
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
				Search &search;
				std::size_t index;

				auto &advance() {
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
					return advance();
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

			auto begin() { return Iterator(*this).advance(); }
			auto   end() { return Iterator(*this); }
		};

		auto search(Bounds const &bounds) const {
			return Search(bounds, this);
		}

		auto erase(Element const &element) {
			enum { found_none = 0, found_branch = 1, found_leaf = 2 };
			if (is_leaf())
				return this->element() != element ? found_none : found_leaf;
			if (!(element.bounds() && bounds)) // TODO: should be <= ?
				return found_none;
			auto const &[node1, node2] = children();
			if (auto const found = node1->erase(element); found != found_none) {
				if (found == found_leaf) {
					auto &node = *node2;
					std::swap(node, *this);
					auto &[node1, node2] = node.children();
					node1.reset(), node2.reset();
				} else
					bounds = node1->bounds + node2->bounds;
				return found_branch;
			}
			if (auto const found = node2->erase(element); found != found_none) {
				if (found == found_leaf) {
					auto &node = *node1;
					std::swap(node, *this);
					auto &[node1, node2] = node.children();
					node2.reset(), node1.reset();
				} else
					bounds = node1->bounds + node2->bounds;
				return found_branch;
			}
			return found_none;
		}

		auto update(Element const &element) {
			if (is_leaf()) {
				if (this->element() != element)
					return false;
				bounds = element.bounds();
				return true;
			}
			if (!(element.bounds() <= bounds))
				return false;
			auto const &[node1, node2] = children();
			if (node1->update(element))
				bounds = node1->bounds + node2->bounds;
			else if (node2->update(element))
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
		if (end - begin == 1)
			return std::make_unique<Node>(*begin);
		auto const middle = begin + (end - begin) / 2;
		std::nth_element(begin, middle, end, [](auto const &corner1, auto const &corner2) {
			if constexpr (horizontal)
				return corner1.bounds().xmin < corner2.bounds().xmin;
			else
				return corner1.bounds().ymin < corner2.bounds().ymin;
		});
		return std::make_unique<Node>(
			partition<Iterator, !horizontal>(begin, middle),
			partition<Iterator, !horizontal>(middle, end)
		);
	}

public:
	RTree(std::vector<Element> &elements) : root(partition(elements.begin(), elements.end())) {
		elements.clear();
	}

	auto search(Bounds const &bounds) const {
		return root->search(bounds);
	}

	void erase(Element const &element) {
		root->erase(element);
	}

	void update(Element const &element) const {
		root->update(element);
	}
};

#endif
