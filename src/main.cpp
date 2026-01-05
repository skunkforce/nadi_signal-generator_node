#include <nadi/nadi.h>
#include <nadicpp/message.hpp>
#include <nadicpp/callback.hpp>
#include <nadicpp/pool.hpp>
#include <nlohmann/json.hpp>
#include <nadi/message_helpers.hpp>
#include <time.h>

class time_and_float_t{
    uint64_t nanoseconds_;
    double value_;
};

extern "C" inline void free_simple_msg_c(nadi_message* message);

using allocator_t = nadicpp::message_allocator<free_simple_msg_c,10*1024>;

extern "C" inline void free_simple_msg_c(nadi_message* message){
    allocator_t* alloc = static_cast<allocator_t*>(message->user);
    alloc->free(message);
}


class memory_management{
    allocator_t allocator_;

    public:
    nadicpp::message allocate_json_message(nadicpp::address a, const nlohmann::json& json){
        nadicpp::message m = allocator_.allocate();
        auto pm = m.get();
        const auto json_str = json.dump();
        pm->data_length = json_str.size();
        std::copy(json_str.begin(),json_str.end(),static_cast<char*>(pm->data));
        pm->meta_hash = 0;
        pm->meta = R"({"format":"json"})";
        m.set_address(a);
        return m;
    }
};


class signal_generator_t{
    nadicpp::callback out_;
    decltype(std::chrono::steady_clock::now()) last_sent_;
    memory_management mgmnt_;

    const nadicpp::address data_out_ = {this,1};

    nadi_status handle_configure(nadi_message* message){
        return NADI_OK;
    }
    public:
    signal_generator_t(nadicpp::callback cb):out_(cb),last_sent_(std::chrono::steady_clock::now()){}
    nadi_status send(nadicpp::message msg, unsigned channel){
        return NADI_OK;
    }
    void handle_events(){
        using namespace std::chrono_literals;
        if (std::chrono::steady_clock::now() > last_sent_ + 5s) {
            last_sent_ += 1s;
            nlohmann::json jmsg;
            jmsg["message"] = "i am the generator";
            auto m = mgmnt_.allocate_json_message(data_out_, std::move(jmsg));
            out_(std::move(m));
        }
    }
};



extern "C" {
    DLL_EXPORT nadi_status nadi_init(nadi_node_handle* node, nadi_receive_callback cb, void* cb_ctx){
        *node = new signal_generator_t(nadicpp::callback(cb, cb_ctx));
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_deinit(nadi_node_handle node){
        delete static_cast<signal_generator_t*>(node);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_send(nadi_message* message, nadi_node_handle node, unsigned int target_channel){
        static_cast<signal_generator_t*>(node)->send(nadicpp::message(message), target_channel);
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_handle_events(nadi_node_handle node){
        static_cast<signal_generator_t*>(node)->handle_events();
        return NADI_OK;
    }

    DLL_EXPORT nadi_status nadi_descriptor(char * descriptor, size_t* length){
        using namespace nlohmann;
        json j;
        j["name"] = "nadi-signal-generator";
        auto inputs = json::array();
        inputs[0] = json::object();
        inputs[0]["channel"] = 0xF200;
        inputs[0]["name"] = "configure";
        inputs[0]["formats"] = {"json"};
        j["inputs"] = inputs;
        auto outputs = json::array();
        outputs[0] = json::object();
        outputs[0]["channel"] = 1;
        outputs[0]["name"] = "generator_output";
        outputs[0]["formats"] = {"float-nanoseconds"};
        j["outputs"] = outputs;
        const std::string js = j.dump();
        if(js.size() > *length){
            return NADI_BUFFER_TOO_SMALL;
        }
        *length = js.size();
        std::copy(js.begin(),js.end(),descriptor);
        return NADI_OK;
    }
}