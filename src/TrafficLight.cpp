#include <iostream>
#include <random>
#include <future>

#include <thread>
#include "TrafficLight.h"
#include "TrafficObject.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> unique_lock(_mutex);
    _condition.wait(unique_lock);

    T message(std::move(_queue.back()));
    _queue.pop_back();
    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    _queue.clear();
    std::lock_guard<std::mutex> lock(_mutex);
    _condition.notify_one();
    _queue.emplace_back(std::move(msg));
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        if(_messageQueue->receive() == TrafficLightPhase::green)
        {
            return;
        }
    }
    
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. 
    // To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

// generate number between 4 and 6 as a random number, 
// you can use random library and mt19937 to generate random number also you can use srand with some calculations to get it

    // https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(4000, 6000);
    double cycleDuration = distrib(gen);

// getting the time difference between the actual time at this moment and the time at which we toggelled the traffic light at the beginning
// of the while loop after sleep 1ms would get you the "time between cycles accumlated".
// as we get a capture of the time when toggling and every time we pass again to the begining of the loop we calculate the time difference 
// between the capture and the time now if it exceeds the durationcycle generated will would toggle traffic light again and get another capture
// of time for the next calculation.

    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
        auto startT = std::chrono::system_clock::now();

        if (_currentPhase == TrafficLightPhase::red){
            _currentPhase = TrafficLightPhase::green;
        }else{
            _currentPhase = TrafficLightPhase::red;
        }

        std::future<void> snd = std::async(
            std::launch::async,
            &MessageQueue<TrafficLightPhase>::send,
            _messageQueue,
            std::move(_currentPhase)
        );
        snd.wait();

        auto endT = std::chrono::system_clock::now();

        double elapsed_time_ms = std::chrono::duration<double, std::milli>(startT-endT).count(); // ms

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }
}

//"why this time difference happens", because when we sleep for 1ms this sleep will result in leave the function body for at least 1ms depends
//  on the schedular and the running threads at this time, also "cycleThroughPhases" is running through threads as requried in 
//  simulate which means also it may leave the processor for another thread and from this appear the time difference.



