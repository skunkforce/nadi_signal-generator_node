#include "nadi.h"
#include <thread>
#include <mutex>
#include <optional>

class IotaProducer{
    std::mutex lock_;
    std::optional<std::jthread> thread_;
    nadi_receive_callback receive_;
    nadi_status handle_management(nadi_message* message){
        return 0;
    }
    public:
    IotaProducer(nadi_receive_callback cb):receive_{cb}{
        thread_.emplace(std::jthread([p = this](std::stop_token stoken){
            while(!stoken.stop_requested()){
                std::lock_guard<std::mutex> lock(p->lock_);
                auto pm = new nadi_message;
                pm->instance = p;
                pm->channel = 0;
                pm->data = 0;
                pm->data_length = 0;
                pm->free = &nadi_free;
                pm->meta = 0;
                pm->meta_hash = 0;
                p->receive_(pm);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }));
    }
    nadi_status send(nadi_message* message){
        switch (message->channel){
            case 0x8000:
            return handle_management(message);
            default:
            return 0;
        }
    }
    void free(nadi_message* message){
        delete[] message->meta;
        delete[] message->data;
        delete message;
    }
};



extern "C" {
    DLL_EXPORT nadi_status nadi_init(nadi_instance_handle* instance, nadi_receive_callback cb){
        *instance = new IotaProducer(cb);
        return 0;
    }

    DLL_EXPORT nadi_status nadi_deinit(nadi_instance_handle instance){
        delete instance;
        return 0;
    }

    DLL_EXPORT nadi_status nadi_send(nadi_message* message){
        static_cast<IotaProducer*>(message->instance)->send(message);
        return 0;
    }

    DLL_EXPORT void nadi_free(nadi_message* message){
        static_cast<IotaProducer*>(message->instance)->free(message);
    }
}