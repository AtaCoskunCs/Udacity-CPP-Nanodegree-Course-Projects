#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <future>
#include <chrono>

// Writer : Ata Coskun
/* Implementation of class "MessageQueue" */

template<class T>
MessageQueue<T>::MessageQueue() {

}

template<class T>
MessageQueue<T>::~MessageQueue() {

}

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> unique_lock(mtx_queue);
    _condition_variable.wait(unique_lock, [this]{return !_queue.empty();});
    T message = std::move(_queue.back());
    _queue.clear();
    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock_guard(mtx_queue);
    _queue.push_back(std::move(msg));
    _condition_variable.notify_one();
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _current_phase = TrafficLightPhase::red;
    _message_queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

TrafficLight::~TrafficLight()
{

}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    std::lock_guard<std::mutex> lock(_mutex_traffic_light);
    while (true){
        TrafficLightPhase phase = _message_queue -> receive();
        if(phase == TrafficLightPhase::green){ break;}
    }
    _condition.notify_one();
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _current_phase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    std::random_device rd;
    std::mt19937 x(rd());
    std::uniform_int_distribution<int> dist(4000, 6000);

    int cycle_duration = dist(x);

    auto last_switch = std::chrono::system_clock::now();

    while(true){

         std::this_thread::sleep_for(std::chrono::milliseconds(1));
         auto elapsed =  std::chrono::duration_cast<std::chrono::milliseconds>
         (std::chrono::system_clock::now() - last_switch);

         if(elapsed.count() >= cycle_duration){
             if(_current_phase == TrafficLightPhase::red){ _current_phase = TrafficLightPhase::green;}
             else{_current_phase = TrafficLightPhase::red;}

             auto message = _current_phase;
             auto ftr_wait = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _message_queue,
                                            std::move(message));

            ftr_wait.wait();
            cycle_duration = dist(x);
            last_switch =  std::chrono::system_clock::now();

         }
    }
}
