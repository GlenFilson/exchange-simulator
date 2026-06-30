#pragma once
#include <cstdint>
#include "types.hpp"

//NOTE: Should probably rearrange this struct for better space. 
//I think i will also have to change where designated initialisers are used as they are definition order dependent
struct Trade {
	Price price;
	OrderId buyer_order_id;
	OrderId seller_order_id;
	Quantity quantity;
	Timestamp timestamp;
    
};



