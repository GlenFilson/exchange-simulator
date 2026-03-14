#pragma once
#include "order.hpp"
#include <vector>
#include <cstdint>
#include <random>


class OrderSimulator{
	public:
		OrderSimulator(double cp, uint64_t starting_id = 1);			
		Order generate_order();
		std::vector<Order> generate_n_orders(int n);

	private:
		uint64_t id_counter_;
		double center_price_;
		std::mt19937 rng_; 

};

