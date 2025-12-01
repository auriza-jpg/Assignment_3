/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101287549_101302779.hpp>
enum timeout {
    PRE_TIMEOUT = 0,
    RR_TIMEOUT = 100,
    NO_TIMEOUT = UINT32_MAX
};
void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

int EP(std::vector<PCB> &ready_queue, PCB &running)
{
    if (ready_queue.empty())
        return NO_TIMEOUT;

    // Sort: smallest EP = highest priority
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &a, const PCB &b) {
            return a.EP < b.EP;
        }
    );

    // The highest priority READY process
    const PCB &best = ready_queue.front();

    // If CPU is idle we need to scheudle 
    if (running.PID == -1)
        return PRE_TIMEOUT; 

    // If a READY process has a lower EP:
    if (best.EP < running.EP)
        return PRE_TIMEOUT;   // preemption 

    //tie: round robin
    if (best.EP == running.EP)
        return RR_TIMEOUT;   

    return NO_TIMEOUT;       
 }

//manage the wait Q. Tick IO progress and move IO complete process's to READY 
std::string waitQ(std::vector<PCB> &ready_queue, std::vector<PCB> &wait_queue, std::vector<PCB> &job_list, int current_time, PCB & running)
{
    std::string execution_status;
    if(wait_queue.empty()!=true)
         {
            //loop through the wait_queue
             for(auto process  = wait_queue.begin(); process != wait_queue.end();) {
                if(process->processing_time >= process->io_duration)
                {
                    //on the last tick, the processed completed its IO
                    //it can still use this tick for CPU cycles
                    execution_status += print_exec_status(current_time,process->PID,process->state, READY);
                    process->state = READY; //change state to ready 
                    process->processing_time = 0; //has spent 0 time in CPU since being kicked 
                    PCB copy = *process;    //copy the process
                    ready_queue.push_back(copy); //add the copy to ready 
                    process = wait_queue.erase(process);  
                    sync_queue(job_list,copy); //sync
                }
                //the process is still doing IO, it will use this tick for IO
                else {
                    process->processing_time +=1; 
                     ++process; 
                }

            }
        }
    return execution_status;
}

//Handle case when timer expired on previous tick. free CPU and run a new process.  
std::string timer_experiry(std::vector<PCB> &ready_queue, std::vector<PCB> &wait_queue, std::vector<PCB> &job_list, int current_time, PCB & running, int timeout)
{
    std::string execution_status;
    if(running.PID != -1 && running.processing_time ==timeout) 
    {
         //is the process done?
        if(running.remaining_time <=0){
            terminate_process(running, job_list);
            execution_status += print_exec_status(current_time,running.PID, running.state, TERMINATED);//log
        }  
        else //not finished, kick to ready
           {
            running.state = READY; //state change
            running.processing_time = 0; //reset timer 
            ready_queue.push_back(running);//move to ready
            sync_queue(job_list, running); 
            execution_status += print_exec_status(current_time,running.PID, RUNNING, READY);//log
        }
    }
     return execution_status;
}


//EITHER: terminate completed process's OR  handle IO calls 
std::string update_running(std::vector<PCB> &ready_queue, std::vector<PCB> &wait_queue, std::vector<PCB> &job_list, int current_time, PCB & running)
{
    std::string execution_status;
    if(running.remaining_time <=0 && running.PID >= 0 ){
            //log termination, free the CPU and terminate the process
            execution_status += print_exec_status(current_time,running.PID,running.state, TERMINATED);
            terminate_process(running,job_list);
    }
        
    else if (running.io_freq > 0 && running.processing_time > 0 && running.processing_time % running.io_freq == 0)
    {
        //on the last tick, the x required seconds of CPU completed
        execution_status += print_exec_status(current_time,running.PID,running.state,WAITING);
        running.state = WAITING; //IO: move -> wait 

        running.processing_time = 1; //tracks IO completion. set to 1 as IO is completed in this tick
           
        wait_queue.push_back(running); //add the PCB to the wait Q
        sync_queue(job_list, running); 
        idle_CPU(running);// idle the CPU (Removes the process's PCB from running)
    }

    return execution_status;
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).
    std::vector<PCB> needs_memory; //stores PCB's that did not get memory on arrival                               
    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();
    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list)||job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        
        
        for(auto &process : list_processes) {

             if(process.arrival_time == current_time || waiting_for_memory(process,needs_memory)){
                //if so, assign memory and put the process into the ready queue
                if(assign_memory(process)){

                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue
                    job_list.push_back(process); //Add it to the list of processes
                    process.EP = process.PID + process.size;
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);

                }
               
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        execution_status += waitQ(ready_queue,wait_queue,job_list, current_time,running);
        //////////////////////////SCHEDULER//////////////////////////////
        //start IO, remove terminated process's 
         execution_status += update_running(ready_queue,wait_queue,job_list,current_time,running);  
        //EP returns a "timer expirey" value 

        //timeout of 0: preemption, timeout 100: running RR, Timeout UINT32 MAX: keep running EP 
        //if the current process is preempted: timeout = 0 
        int timer_exp = EP(ready_queue,running);
        execution_status += timer_experiry(ready_queue, wait_queue, job_list, current_time, running, timer_exp); //kicks running if running is complete      
        if(running.remaining_time<=0){
            //schedule the next process. 
            run_process(running,job_list,ready_queue,current_time); //get a new process from ready
            if(running.PID != -1)//don't log idle if no process can be scheduled 
            {
                execution_status +=print_exec_status(current_time, running.PID, READY, RUNNING);//log
            } 
            sync_queue(job_list,running);
        }                                                                          //or if if timer expires 
        /////////////////////////////////////////////////////////////////
        if(running.state == RUNNING)
        {
            //the running process uses this tick for CPU 
            if (running.remaining_time > 0)
                running.remaining_time -= 1; 
            running.processing_time +=1; 
            current_time +=1;
            sync_queue(job_list,running);
        }
        else current_time ++;

        if(all_process_terminated(job_list)){
            return make_tuple(execution_status);
        }
        
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line,"a"); //was getting issues with stoi... 
        auto new_process = add_process(input_tokens);
        if (input_tokens.size() != 6) continue;
        list_process.push_back(new_process);
    }
    input_file.close();
    printf("Closed file");

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
