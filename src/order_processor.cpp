#include "order_processor.hpp"
#include <cstring>
void OrderProcessor::run(){
    
    while(true){
        std::optional<InboundMessage> result = inbound_queue_.try_pop();
        if(result.has_value()){
            InboundMessage message = std::move(*result);
            process(message);
        }
    }
}


void OrderProcessor::process(InboundMessage& i_message){
    std::vector<uint8_t> buffer;
    switch(i_message.type){
        case MessageType::NEW_ORDER: {
            uint64_t raw_id;
            std::memcpy(&raw_id, i_message.payload.data(), sizeof(uint64_t));
            try{
                Message message{
                    .type = MessageType::NEW_ORDER,
                    .payload = std::move(i_message.payload)
                };
                Order order = serializer_.deserialize_order(message);
                std::vector<Trade> trades = matching_engine_.match(order);
                order_to_client_fd_[order.id()] = i_message.fd;
                for(const Trade& trade : trades){
                    //batch client (taker) messages
                    serializer_.serialize_trade(trade, buffer);
                    
                    std::vector<uint8_t> cp_buffer;
                    serializer_.serialize_trade(trade, cp_buffer);
                    //send counterparty straight away
                    outbound_queue_.push(OutboundMessage{
                        .fd = order_to_client_fd_[order.side() == Side::ASK ? trade.buyer_order_id : trade.seller_order_id],
                        .payload = std::move(cp_buffer)
                    });
                }
                Acknowledgement ack{
                    .id = order.id()
                };
                serializer_.serialize_acknowledgement(ack, buffer);
            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                serializer_.serialize_rejection(rejection, buffer);
                break;
            }
            break;
        }


        case MessageType::CANCEL_ORDER: {

            uint64_t raw_id;
            std::memcpy(&raw_id, i_message.payload.data(), sizeof(uint64_t));
            try {
                Message message{
                    .type = MessageType::CANCEL_ORDER,
                    .payload = std::move(i_message.payload)
                };
                uint64_t id = serializer_.deserialize_cancel(message);
                Acknowledgement cancel_ack{
                    .id = id
                };
                serializer_.serialize_cancel_ack(cancel_ack, buffer);
                orderbook_.cancel_order(id);
                order_to_client_fd_.erase(id);
            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                serializer_.serialize_rejection(rejection, buffer);
                break;
            }
            break;
        }
        default:
            throw std::runtime_error("unknown message type");
            return;
    }

    outbound_queue_.push(OutboundMessage{
        .fd = i_message.fd, 
        .payload = std::move(buffer)
    });

  

    


}