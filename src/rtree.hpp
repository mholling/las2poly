#ifndef RTREE_HPP
#define RTREE_HPP

#include <vector>
#include <algorithm>

template <typename T>
class RTree {
	template<int axis>
	class Node {
		using I = typename std::vector<T>::iterator;
		using child_type = Node<1-axis>;
		friend child_type;

		std::vector<child_type> children;
		const I first, last;
		Bounds bounds;

	public:
		Node(const I first, const I last) : first(first), last(last) {
			std::sort(first, last, [](const auto &t1, const auto &t2) {
				return t1.bounds()[axis][0] < t2.bounds()[axis][0];
			});
			switch (last - first) {
				case 1:
					bounds = first->bounds();
				case 0:
					break;
				default:
					const auto middle = first + (last - first) / 2;
					children.push_back(child_type(first, middle));
					children.push_back(child_type(middle, last));
					bounds = children[0].bounds + children[1].bounds;
			}
		}

		template<typename F>
		void search(const T &element, F function) const {
			if (last == first)
				return;
			if (!(bounds && element.bounds()))
				return;
			if (last == first + 1)
				function(*first);
			for (const auto &node: children)
				node.search(element, function);
		}
	};

	std::vector<T> elements;
	Node<0> node;

public:
	template <typename C>
	RTree(const C &container) : elements(container.begin(), container.end()), node(elements.begin(), elements.end()) { }

	template <typename F>
	void search(const T &element, F function) const {
		node.search(element, function);
	}
};

#endif
