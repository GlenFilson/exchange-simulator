#pragma once
#include <cstdint>

//NOTE: Should probably rearrange this struct for better space. 
//I think i will also have to change where designated initialisers are used as they are definition order dependent
struct Trade {	
	double price;
	uint64_t buyer_order_id;
	uint64_t seller_order_id;
	uint32_t quantity;
	uint64_t timestamp;
	
};



