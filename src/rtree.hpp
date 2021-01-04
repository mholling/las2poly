#ifndef RTREE_HPP
#define RTREE_HPP

#include <vector>
#include <algorithm>

template <typename T>
class RTree {
	template<int axis>
	class Node {
		using Iterator = typename std::vector<T>::iterator;
		using Child = Node<1-axis>;
		friend Child;

		std::vector<Child> children;
		Iterator first, last;
		Bounds bounds;

	public:
		Node(Iterator first, Iterator last) : first(first), last(last) {
			const auto middle = first + (last - first) / 2;
			std::nth_element(first, last, middle, [](const auto &t1, const auto &t2) {
				return axis == 0 ? t1.bounds().x.min < t2.bounds().x.min : t1.bounds().y.min < t2.bounds().y.min;
			});
			switch (last - first) {
				case 1:
					bounds = first->bounds();
				case 0:
					break;
				default:
					children.push_back(Child(first, middle));
					children.push_back(Child(middle, last));
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
