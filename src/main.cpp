#include <nadi/nadi.h>
#include <nadi/message_validation.hpp>
#include <nadi/unique_message.hpp>
#include <thread>
#include <mutex>
#include <optional>
#include <time.h>

class time_and_float_t{
    uint64_t nanoseconds_;
    double value_;
};


class signal_generator_t{
    nadi_receive_callback receive_;
    void* receive_ctx_;

    nadi_status handle_configure(nadi_message* message){
        return NADI_OK;
    }
    public:
    signal_generator_t(nadi_receive_callback cb, void* receive_ctx):receive_(cb),receive_ctx_(receive_ctx){}
    nadi_status send(nadi_unique_message msg, unsigned channel){
        return NADI_OK;
    }
    void handle_events(){

    }
    void free(nadi_message* message){
        delete[] message->meta;
        delete[] message->data;
        delete message;
    }
};



extern "C" {
    DLL_EXPORT nadi_status nadi_init(nadi_node_handle* node, nadi_receive_callback cb, void* cb_ctx){
        *node = new signal_generator_t(cb, cb_ctx);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_deinit(nadi_node_handle node){
        delete static_cast<signal_generator_t*>(node);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_send(nadi_message* message, nadi_node_handle node, unsigned int target_channel){
        static_cast<signal_generator_t*>(node)->send(nadi_unique_message(message), target_channel);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_handle_events(nadi_node_handle node){
        static_cast<signal_generator_t*>(node)->handle_events();
        return NADI_OK;
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