#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
        
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock); // pass unique lock to condition variable

    // remove last TrafficLight from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++

}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform queue modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add new TrafficLight to queue
    // std::cout << "TrafficLight #" << msg << " will be added to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    _condition.notify_one(); // notify client after pushing new TrafficLight into vector
}

/* Function of getting random time*/
int getCycleTime()
{
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<int> dis(0, 1);
    return 4+dis(g)*2;
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        std::cout << "Current TrafficLight #" << _id << "'s light is " << getCurrentPhase() << "." << std::endl;
        auto msg = _msgQueue.receive();
        std::cout << "Received message is " << msg << std::endl;
        if (msg == TrafficLightPhase::green)
        {
            std::cout << "Current TrafficLight #" << _id << "'s light is " << getCurrentPhase() << "." << std::endl;
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

    int cycle_time = getCycleTime();

    // define t1 and t2
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point t2;

    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute the duration 
        t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(t2-t1).count();
        
        if (duration >= cycle_time)
        {
            std::lock_guard<std::mutex> uLock(_mutex);

            // setup new cycle time
            cycle_time = getCycleTime();
            t1 = std::chrono::high_resolution_clock::now();
            
            // toggle the current phase of the traffice light
            auto nextPhase = (getCurrentPhase() == TrafficLightPhase::red)? TrafficLightPhase::green : TrafficLightPhase::red;
            std::cout << "TrafficLight #" << _id << " will be set as " << nextPhase << std::endl;
            _currentPhase = nextPhase;

            // send an update method to the message queue.
            _msgQueue.send(std::move(nextPhase));
        }
    }

}

