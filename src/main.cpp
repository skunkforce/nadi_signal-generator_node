#include <nadi/nadi.h>
#include <nadi/message_validation.hpp>
#include <thread>
#include <mutex>
#include <optional>
#include <time.h>
#include "scope_guard.hpp"

class time_and_float_t{
    uint64_t nanoseconds_;
    double value_;
};


class signal_generator_t{
    std::mutex lock_;
    std::optional<std::jthread> thread_;
    nadi_receive_callback receive_;
    std::map<std::pair<nadi_node_handle,unsigned>,unsigned> input_connections_;
    nadi_status handle_management(nadi_message* message){
        auto meta = nlohmann::json::parse(message->meta);
        if(meta.contains("type") && meta["type"] == "json"){
            auto data = nlohmann::json::parse(reinterpret_cast<const char*>(message->data));
            if(nadi::validation::validate_node_connect(data)){
                uint64_t source_node = data["source"][0];
                unsigned source_channel = data["source"][1];
                unsigned target_channel = data["target"];
                input_connections_[{source_node,source_channel}] = target_channel;

            }
            else if(nadi::validation::validate_node_disconnect(data)){
                uint64_t source_node = data["source"][0];
                unsigned source_channel = data["source"][1];
                unsigned target_channel = data["target"];
                size_t removed = input_connections_.erase({source_node,source_channel});
                if (removed > 0) {  

                }
                else{

                }     
            }
        }
        return NADI_OK;
    }
    nadi_status handle_configure(nadi_message* message){
        return NADI_OK;
    }
    public:
    signal_generator_t(nadi_receive_callback cb):receive_{cb}{
        thread_.emplace(std::jthread([p = this](std::stop_token stoken){
            while(!stoken.stop_requested()){
                auto pm = new nadi_message;
                pm->node = reinterpret_cast<uint64_t>(p);
                pm->channel = 0;
                pm->data = reinterpret_cast<char*>(new time_and_float_t[100]);
                pm->data_length = 0;
                pm->free = &nadi_free;
                pm->meta = 0;
                pm->meta_hash = 0;
                std::lock_guard<std::mutex> lock(p->lock_);
                p->receive_(pm);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }));
    }
    nadi_status send(nadi_message* msg){
        auto msg_guard = sg::make_scope_guard([msg](){
                msg->free(msg);
            });
        if(msg->node == 0 && msg->channel == 0xF100){
            return handle_management(msg);
        }
        if(input_connections_.contains({msg->node,msg->channel})){
            unsigned channel = input_connections_[{msg->node,msg->channel}];
            if(channel == 1){
                return handle_configure(msg);
            }
            return NADI_OK;
        }
        return NADI_INVALID_CHANNEL;
    }
    void free(nadi_message* message){
        delete[] message->meta;
        delete[] message->data;
        delete message;
    }
};



extern "C" {
    DLL_EXPORT nadi_status nadi_init(nadi_node_handle* node, nadi_receive_callback cb){
        *node = reinterpret_cast<uint64_t>(new signal_generator_t(cb));
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_deinit(nadi_node_handle node){
        delete reinterpret_cast<signal_generator_t*>(node);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_send(nadi_message* message, nadi_node_handle node){
        reinterpret_cast<signal_generator_t*>(node)->send(message);
        return NADI_OK;
    }

    DLL_EXPORT void nadi_free(nadi_message* message){
        reinterpret_cast<signal_generator_t*>(message->node)->free(message);
    }

    DLL_EXPORT nadi_status nadi_descriptor(char * descriptor, size_t* length){
        const char ret[] = R"({"name":"nadi-signal-generator"})";
        if(sizeof(ret) > *length){
            return NADI_BUFFER_TOO_SMALL;
        }
        *length = sizeof(ret);
        strcpy(descriptor,ret);
        return NADI_OK;
    }
}