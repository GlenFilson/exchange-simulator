#pragma once
#include "order.hpp"
#include "types.hpp"
#include <vector>
#include <random>


class OrderSimulator{
	public:
		OrderSimulator(Price cp, OrderId starting_id = 1);
		Order generate_order();
		std::vector<Order> generate_n_orders(int n);

	private:
		OrderId id_counter_;
		Price center_price_;
		std::mt19937 rng_; 

};

