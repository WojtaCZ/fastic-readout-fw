#include "scheduler.hpp"

//Function to process and increment the scheduler counter
void scheduler::increment(){
    if((this->counter >= this->interval) && !(this->flags & scheduler::READY) && (this->flags & scheduler::ACTIVE)){
        this->flags |= READY;
        if(this->flags & scheduler::DISPATCH_ON_INCREMENT) this->dispatch();
    }else if(this->flags & scheduler::ACTIVE){
        this->counter++;
    }
}

//Function to take care of callbacks after scheduler finshed
void scheduler::dispatch(){
    if(this->flags & scheduler::READY){
        (*this->callback)();

        this->counter = 0;
        
        if(!(this->flags & scheduler::PERIODICAL)){
            this->flags &= ~scheduler::READY;
            this->flags |= scheduler::COMPLETED;
        }

        this->flags &= ~scheduler::READY;
    }
}

//Sets scheduler flags
uint16_t scheduler::setFlags(uint16_t flags){
    this->flags = 0 | flags;
    return this->flags;
}

//Enables scheduler flags
uint16_t scheduler::enableFlags(uint16_t flags){
    this->flags |= flags;
    return this->flags;
}

//Disables scheduler flags
uint16_t scheduler::disableFlags(uint16_t flags){
    this->flags &= ~flags;
    return this->flags;
}

//Resets the scheduler timer and READY and COMPLETED flags
void scheduler::reset(){
    this->disableFlags(scheduler::READY | scheduler::COMPLETED);
    this->counter = 0;
}

//Pauses the scheduler
void scheduler::pause(){
    if(!this->isActive()) return;
    this->disableFlags(scheduler::ACTIVE);
}

//Resumes the scheduler
void scheduler::resume(){
    if(this->isActive()) return;
    this->enableFlags(scheduler::ACTIVE);
}

//Indicates that the scheduler is ready
bool scheduler::isReady(){
    if(this->flags & scheduler::READY) return true;
    return false;
}

//Indicates that the scheduler is active
bool scheduler::isActive(){
    if(this->flags & scheduler::ACTIVE) return true;
    return false;
}

//Indicates that the scheduler is completed
bool scheduler::isCompleted(){
    if(this->flags & scheduler::COMPLETED) return true;
    return false;
}

//Sets the scheduler interval in ms
void scheduler::setInterval(int interval){
    this->interval = interval;
}