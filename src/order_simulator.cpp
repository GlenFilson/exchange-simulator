#include "order_simulator.hpp"
#include <cstdint>
#include <chrono>


OrderSimulator::OrderSimulator(double cp, uint64_t starting_id)
	/*
	initialiser list instead of explicit member assignment
	member assignment initialises members with a default value, then reassigns it
	initialiser list initialises members with the correct value instantly
	*/
	: id_counter_(starting_id), center_price_(cp), rng_(std::random_device{}())
{

}


Order OrderSimulator::generate_order(){

	//create the distribution object
	std::uniform_int_distribution<int> side_dist(0, 1);
	//use the distribution object with the rng_, random_device
	std::uniform_int_distribution<int> order_type_dist(0, 1);
	std::normal_distribution<double> price_dist(center_price_, 5.0);
	std::uniform_int_distribution<uint32_t> quantity_dist(1, 1000);

	Side side = side_dist(rng_) == 0 ? Side::BID : Side::ASK;
	OrderType orderType = order_type_dist(rng_) == 0 ? OrderType::LIMIT : OrderType::MARKET; 
	double price = (orderType == OrderType::LIMIT ? price_dist(rng_) : 0);
	uint32_t quantity = quantity_dist(rng_);
	
	//assign the counter then increment it (post)
	Order order(id_counter_++, price, quantity, side, orderType);
	return order;
}

std::vector<Order> OrderSimulator::generate_n_orders(int n){
	/*
	we know we are going to assign n orders
	we should reserve the space required for n orders, rather than letting the vector resize itself

	*/
	std::vector<Order> orders;
	orders.reserve(n);
	for (int i = 0; i < n; i++){
		Order order = generate_order();
		orders.push_back(order);
	}
	return orders;
}