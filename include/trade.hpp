#pragma once
#include <cstdint>


struct Trade {	
	double price;
	uint64_t buyer_order_id;
	uint64_t seller_order_id;
	uint32_t quantity;
	uint64_t timestamp;
	
};



