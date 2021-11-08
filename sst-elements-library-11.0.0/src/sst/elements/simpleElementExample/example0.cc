// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


// This include is ***REQUIRED*** 
// for ALL SST implementation files
#include "sst_config.h"

#include "example0.h"
#include "basicEvent.h"


using namespace SST;
using namespace SST::simpleElementExample;
// int my_id = 0; //global variable

/* 
 * During construction the example component should prepare for simulation
 * - Read parameters
 * - Configure link
 * - Register its clock
 */
example0::example0(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    // Initialize with 
    // - no prefix ("")
    // - Verbose set to 1
    // - No mask
    // - Output to STDOUT (Output::STDOUT)
    out = new Output("", 1, 0, Output::STDOUT);
    std::cout << "This id is: " << id << std::endl;
    my_id = id;
    numChildren = 4;
    dim_row = 2;
    dim_col = 2;
    numLink = 2;
    link = new Link*[numLink];
    std::vector<int> temp;
    comm = this->communicator_table(3);
    temp = comm[0];
    std::cout << "map 0" << temp.at(0) << "," << temp.at(1) << "," << temp.at(2) << "," << temp.at(3) << std::endl;
    temp = comm[1];
    std::cout << "map 1" << temp.at(0) << "," << temp.at(1) << "," << temp.at(2) << "," << temp.at(3) << std::endl;
    temp = comm[2];
    std::cout << "map 2" << temp.at(0) << "," << temp.at(1) << "," << temp.at(2) << "," << temp.at(3) << std::endl;
    temp = comm[3];
    std::cout << "map 3" << temp.at(0) << "," << temp.at(1) << "," << temp.at(2) << "," << temp.at(3) << std::endl;

    for (int i=0; i < dim_row*dim_col; i++){
        sent.push_back(false);
    }
    
    for (int i=0; i < numChildren; i++){
        sum_local.push_back(0);
    }
    // eventsToSend = new int64_t[numLink];
    // eventSize = new int64_t[numLink];
    // lastEventReceived = new bool[numLink];
    // ComponentId_t is int64_t
    // if(id == 0){
    //     std::cout << "id is type 2" << id << std::endl;
    // }

    // if(id == 1){
    //     std::cout << "id is type 2" << id << std::endl;
    // }

    // Get parameter from the Python input
    bool found;
    int64_t eventsToSend_t, eventSize_t;
    eventsToSend = params.find<int64_t>("eventsToSend", 0, found);

    // If parameter wasn't found, end the simulation with exit code -1.
    // Tell the user how to fix the error (set 'eventsToSend' parameter in the input) 
    // and which component generated the error (getName())
    if (!found) {
        out->fatal(CALL_INFO, -1, "Error in %s: the input did not specify 'eventsToSend' parameter\n", getName().c_str());
    }

    // This parameter controls how big the messages are
    // If the user didn't set it, have the parameter default to 16 (bytes)
    eventSize = params.find<int64_t>("eventSize", 16);

    // Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // configure our link with a callback function that will be called whenever an event arrives
    // Callback function is optional, if not provided then component must poll the link
    link[0] = configureLink("port0", new Event::Handler<example0>(this, &example0::handleEvent));
    link[1] = configureLink("port1", new Event::Handler<example0>(this, &example0::handleEvent));

    // Make sure we successfully configured the links
    // Failure usually means the user didn't connect the port in the input file
    sst_assert(link[0], CALL_INFO, -1, "Error in %s: Link configuration failed\n", getName().c_str());
    sst_assert(link[1], CALL_INFO, -1, "Error in %s: Link configuration failed\n", getName().c_str());

    //set our clock. The simulator will call 'clockTic' at a 1GHz frequency
    registerClock("1GHz", new Clock::Handler<example0>(this, &example0::clockTic));

    // This simulation will end when we have sent 'eventsToSend' events and received a 'LAST' event
    // for (int i=0; i<numLink; i++){
    //     eventsToSend[i] = eventsToSend_t;
    // } 

    // for (int i=0; i<numLink; i++){
    //     eventSize[i] = eventSize_t;
    // }

    // for (int i=0; i<numLink; i++){
    //     lastEventReceived[i] = false;
    // }
}
/*
 * Destructor, clean up our output
 */
example0::~example0()
{
    delete out;
}

/* Event handler
 * Incoming events are scanned and deleted
 * Record if the event received is the last one our neighbor will send 
 */
//receive
void example0::handleEvent(SST::Event *ev)
{
    basicEvent *event = dynamic_cast<basicEvent*>(ev);
    int id_received;
    
    if (event) {
        
        // scan through each element in the payload and do something to it
        // volatile int sum = 0; // Don't let the compiler optimize this out
        for (std::vector<char>::iterator i = event->payload.begin();
             i != event->payload.end(); ++i) {
            // sum += *i;
            id_received = *i;
            std::cout << "id received is: " << id_received << std::endl;
            sum_local.at(id_received) += 1;
        }
        
        // This is the last event our neighbor will send us
        if (event->last) {
            lastEventReceived = true;
        }
        
        // Receiver has the responsiblity for deleting events
        delete event;

        for (int i=0; i<numChildren; i++){
            if (sum_local.at(i) == eventsToSend){
                std::cout << "Hey Im here!" << std::endl;
                sent.at(i) = true;
            }
        }

    } else {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s!\n", getName().c_str());
    }
}

/* 
 * On each clock cycle we will send an event to our neighbor until we've sent our last event
 * Then we will check for the exit condition and notify the simulator when the simulation is done
 */
//send
bool example0::clockTic( Cycle_t cycleCount)
{
    // TODO: you cannot share variables between different components as they have their own variables and objects; /
    // instead you shoud send a signal via send_data with payload that lets say you have finished sending. 
    // Send an event if we need to 
    if (eventsToSend > 0) {
        // Only send when all of your predecessors are done or you are leave node (you have -1 as all of your children).
        // if (this->check_sent(my_id)){
            // sent.at(my_id) = true;
            std::cout << "My id is:" << my_id << std::endl;
            std::vector<int> children_t;
            children_t = this->predecessors(my_id);
            std::cout << "Predecessors:" << children_t.at(0) << "," << children_t.at(1) << "," << children_t.at(2) << "," << children_t.at(3) << std::endl;
            basicEvent *event1 = new basicEvent();
            basicEvent *event2 = new basicEvent();
            
            // Create a dummy payload with eventSize bytes
            for (int i = 0; i < eventSize; i++) {
                event1->payload.push_back(my_id);
                event2->payload.push_back(my_id);
            }
    
            // This is the last event we'll send
            if (eventsToSend == 1) {
                event1->last = true;
                event2->last = true;
            }
    
            // Send the event if all of the predecessors are done sending.
            link[0]->send(event1);
            link[1]->send(event2);
            eventsToSend--;
        // }
    }

    // Check if the exit conditions are met
    if (eventsToSend == 0 && lastEventReceived == true) {
        
        // Tell SST that it's OK to end the simulation (once all primary components agree, simulation will end)
        primaryComponentOKToEndSim(); 

        // Retrun true to indicate that this clock handler can be disabled
        return true;
    }

    // Return false to indicate the clock handler should not be disabled
    return false;
}

std::vector<int> example0::predecessors(int id){
    // This is a lambda experssion (anonymous functor) to make it easy work with a function with different operations (callback!)
    // -1 is invalid
    // auto comp_dir = [](const int data_id, const std::string op){
    //     if (op=="left") {(data_id % dim_col) == 0 ? return -1 : return data_id - 1;}
    //     else if (op=="up") {(data_id - dim_col) > 0 ? return data_id - dim_col : return -1;}
    //     else if (op=="right") {((data_id + 1) % dim_col == 0) ? return -1 : return data_id + 1;};
    //     else if (op=="down") {((data_id + dim_col) < (dim_row*dim_col)) ? return data_id + dim_col : return -1;}
    //     else  return -1;
    // }

    std::vector<int> children;
    children.push_back(this->comp_dir(id, "left"));
    children.push_back(this->comp_dir(id, "up"));
    children.push_back(this->comp_dir(id, "right"));
    children.push_back(this->comp_dir(id, "down"));

    return children;
}

int example0::comp_dir(const int data_id, const std::string op){
    // it is assumed that routers are numbered like below:
    // 0 1 2 3
    // 4 5 6 7 
    // 8 9 10 11 
    // 12 13 14 15

    if (op=="left") {return (data_id % dim_col) == 0 ? -1 : data_id - 1;}
    else if (op=="up") {return (data_id - dim_col) >= 0 ? data_id - dim_col : -1;}
    else if (op=="right") {return ((data_id + 1) % dim_col == 0) ? -1 : data_id + 1;}
    else if (op=="down") {return ((data_id + dim_col) < (dim_row*dim_col)) ? data_id + dim_col : -1;}
    else  return -1;
}

bool example0::check_sent(int id){
    // if all of your children is -1 then it is leave; you can send data or if all of your predecessor have sent data or a mix of two approaches
    bool can_send;
    int count = 0;
    std::vector<int> my_children;
    my_children = comm[id];
    for (int i=0; i<numChildren; i++){ 
        if (my_children[i] == -1 or sent.at(my_children[i]) == true){
            count++;
        }
    }
    if(count == 4){ 
        can_send = true;
    }
    else{
        can_send = false;
    }
    return can_send;
}

std::map<int, std::vector<int>> example0::communicator_table(int root){

    std::map<int, std::vector<int>> comm_table;
    std::string direction[4] = {"left", "up", "right", "down"};
    std::vector<bool> visited;
    std::vector<int> real_children;
    for (int i=0; i < dim_row*dim_col; i++){ // initialize vivited for all of the nodes
        visited.push_back(false);
    }
    std::vector<int> next_node;
    next_node.push_back(root); //initialize next_node
    int curr_node; // current node
    int child;

    while(next_node.size()!=0){
        curr_node = next_node.back();
        next_node.pop_back();
        for (auto dir : direction){
            child = this->comp_dir(curr_node, dir);
            if (child == -1){
                real_children.push_back(-1);
            }
            else{
                if (visited.at(child) == false){
                    real_children.push_back(child);
                    next_node.push_back(child);
                    visited.at(child) = true;
                }
                else{
                    real_children.push_back(-1);
                }
            }
            
        }
        comm_table.insert(std::pair<int, std::vector<int>>(curr_node, real_children)); 
        real_children.clear();
        visited.at(curr_node) = true;
    }
    return comm_table;
}
