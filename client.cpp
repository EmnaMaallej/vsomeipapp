#include <iomanip>
#include <iostream>
#include <sstream>
#include <condition_variable>
#include <thread>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_INSTANCE_ID 0x5678
#define SAMPLE_METHOD_ID 0x0421
#define SAMPLE_EVENT_ID 0x8778
#define SAMPLE_EVENTGROUP_ID 0x4465

class Client {
public:
    Client() : app_(vsomeip::runtime::get()->create_application("Client")) {}

    bool init() {
        if (!app_->init()) {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        app_->register_state_handler(std::bind(&Client::on_state, this, std::placeholders::_1));
        app_->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, 
            std::bind(&Client::on_response, this, std::placeholders::_1));
        app_->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID, 
            std::bind(&Client::on_notification, this, std::placeholders::_1));
        app_->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, 
            std::bind(&Client::on_availability, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        app_->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
        std::set<vsomeip::eventgroup_t> its_groups{SAMPLE_EVENTGROUP_ID};
        app_->request_event(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID, its_groups, 
            vsomeip::event_type_e::ET_EVENT);
        app_->subscribe(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENTGROUP_ID);
        std::thread sender(std::bind(&Client::send_request, this));
        app_->start();
        sender.join();
        return true;
    }

private:
    void on_state(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            std::cout << "Client registered" << std::endl;
        }
    }

    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "Service [" << std::hex << std::setw(4) << std::setfill('0') << _service << "." 
                  << std::setw(4) << std::setfill('0') << _instance << "] is "
                  << (_is_available ? "available" : "NOT available") << std::endl;
        if (_is_available) {
            condition_.notify_one();
        }
    }

    void on_response(const std::shared_ptr<vsomeip::message> &_response) {
        auto payload = _response->get_payload();
        std::string str(reinterpret_cast<const char*>(payload->get_data()), payload->get_length());
        std::cout << "Received response: " << str << " from Client/Session ["
                  << std::setw(4) << std::setfill('0') << std::hex << _response->get_client() << "/"
                  << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "]" 
                  << std::endl;
    }

    void on_notification(const std::shared_ptr<vsomeip::message> &_notification) {
        auto payload = _notification->get_payload();
        std::string str(reinterpret_cast<const char*>(payload->get_data()), payload->get_length());
        std::cout << "Received notification: " << str << " for event [" 
                  << std::setw(4) << std::setfill('0') << std::hex << _notification->get_method() << "]" 
                  << std::endl;
    }

    void send_request() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock);
        auto request = vsomeip::runtime::get()->create_request();
        request->set_service(SAMPLE_SERVICE_ID);
        request->set_instance(SAMPLE_INSTANCE_ID);
        request->set_method(SAMPLE_METHOD_ID);
        std::string str = "Hello, Server!";
        auto payload = vsomeip::runtime::get()->create_payload();
        payload->set_data(reinterpret_cast<const vsomeip::byte_t*>(str.data()), str.length());
        request->set_payload(payload);
        app_->send(request);
        std::cout << "Sent request: " << str << std::endl;
    }

    std::shared_ptr<vsomeip::application> app_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

int main() {
    Client client;
    if (!client.init()) {
        return 1;
    }
    return 0;
}
