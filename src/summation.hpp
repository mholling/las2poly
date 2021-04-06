#ifndef SUMMATION_HPP
#define SUMMATION_HPP

class Summation {
	double &sum;
	double compensation;

public:
	Summation(double &sum) : sum(sum), compensation(0) { }

	auto &operator+=(double value) {
		auto compensated_value = value - compensation;
		auto new_sum = sum + compensated_value;
		compensation = (new_sum - sum) - compensated_value;
		sum = new_sum;
		return *this;
	}
};

#endif
