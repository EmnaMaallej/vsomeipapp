#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_INSTANCE_ID 0x5678
#define SAMPLE_METHOD_ID 0x0421
#define SAMPLE_EVENT_ID 0x8778
#define SAMPLE_EVENTGROUP_ID 0x4465

class Server {
public:
    Server() : app_(vsomeip::runtime::get()->create_application("Server")) {}

    bool init() {
        if (!app_->init()) {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        app_->register_state_handler(std::bind(&Server::on_state, this, std::placeholders::_1));
        app_->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, 
            std::bind(&Server::on_message, this, std::placeholders::_1));
        // Offer event for notifications
        std::set<vsomeip::eventgroup_t> its_groups{SAMPLE_EVENTGROUP_ID};
        app_->offer_event(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID, its_groups, 
            vsomeip::event_type_e::ET_EVENT, std::chrono::milliseconds(1000), false, true);
        // Start a thread for periodic notifications
        notification_thread_ = std::thread(&Server::send_periodic_notifications, this);
        app_->start();
        return true;
    }

private:
    void on_state(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            app_->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
            std::cout << "Service offered [" << std::hex << SAMPLE_SERVICE_ID << "." 
                      << SAMPLE_INSTANCE_ID << "]" << std::endl;
        }
    }

    void on_message(const std::shared_ptr<vsomeip::message> &_request) {
        // Print received payload
        std::shared_ptr<vsomeip::payload> its_payload = _request->get_payload();
        std::string received_data(reinterpret_cast<const char*>(its_payload->get_data()), 
                                 its_payload->get_length());
        std::cout << "Received request from Client/Session [" 
                  << std::setw(4) << std::setfill('0') << std::hex << _request->get_client() << "/"
                  << std::setw(4) << std::setfill('0') << std::hex << _request->get_session() << "] "
                  << "Data: " << received_data << std::endl;

        // Create response
        std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
        std::shared_ptr<vsomeip::payload> response_payload = vsomeip::runtime::get()->create_payload();
        std::string response_str = "Response from Server!";
        response_payload->set_data(reinterpret_cast<const vsomeip::byte_t*>(response_str.data()), 
                                  response_str.length());
        its_response->set_payload(response_payload);
        app_->send(its_response);
        std::cout << "Sent response: " << response_str << std::endl;

        // Trigger a notification
        send_notification("Notification triggered by request!");
    }

    void send_notification(const std::string& _message) {
        std::shared_ptr<vsomeip::payload> payload = vsomeip::runtime::get()->create_payload();
        payload->set_data(reinterpret_cast<const vsomeip::byte_t*>(_message.data()), _message.length());
        app_->notify(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID, payload);
        std::cout << "Sent notification: " << _message << std::endl;
    }

    void send_periodic_notifications() {
        while (true) {
            send_notification("Periodic notification from Server!");
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Notify every 5 seconds
        }
    }

    std::shared_ptr<vsomeip::application> app_;
    std::thread notification_thread_;
};

int main() {
    Server server;
    if (!server.init()) {
        return 1;
    }
    return 0;
}
