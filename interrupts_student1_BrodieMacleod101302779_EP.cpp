/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

void EP(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.EP < second.EP); 
                } 
            );
}

void calcWhenDoneIO(std::vector<PCB> &ready_queue){
    for(auto &process :ready_queue){

    }
}



std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue
        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses

        std::vector<PCB>used_tick_to_finish_IO;
        
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);
                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes
                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
         if(!wait_queue.empty())
         {
            //loop through the wait_queue
             for(auto process  = wait_queue.begin(); process != wait_queue.end();) {
                if(process->processing_time == process->io_duration)\
                {
                    //on the last tick, the processed completed its IO
                    //it can still use this tick on CPU cycles
                    print_exec_status(current_time,process->PID,process->state, READY);
                    ready_queue.push_back(*process); //add to ready 
                    ready_queue.erase(process);  //remove from wait 
                    process->state = READY;
                    
                }
                //the process is still doing IO, it will use this tick for IO
                else process->processing_time +=1;
            }
        }
        
        //////////////////////////SCHEDULER//////////////////////////////
        EP(ready_queue);// schedule process from the results of last tick 
    
        if(running.state == TERMINATED)
        {
            //log termination, free the CPU and terminate the process
            print_exec_status(current_time,running.PID,running.state,TERMINATED);
            terminate_process(running,job_list);
            idle_CPU(running); 
        }
        //on the last tick, the x required seconds of CPU completed, do IO for this tick
        else if (running.io_freq > 0 && running.processing_time > 0 
            && running.processing_time % running.io_freq == 0)
        {
            print_exec_status(current_time,running.PID,running.state,WAITING);
            running.state = WAITING; //IO: move -> wait 
            running.processing_time = 1; //using this varible to track IO completion, so set it to 1
            //this is set to 1 because the process will use the current tick to complete 1 second of IO
            
            wait_queue.push_back(running); //add the PCB to the wait Q
            sync_queue(job_list, running); 
            idle_CPU(running);// idle the CPU (Removes the process's PCB from running)
            
        }
        //add a process to running if the CPU is idle 
        if(running.state == NOT_ASSIGNED || running.state == TERMINATED|| running.state == WAITING)
        {
            run_process(running, job_list, ready_queue, current_time); 
            print_exec_status(current_time, running.PID, READY, RUNNING);
        }
        if(running.state == RUNNING)
        {
            //the running process uses this tick for CPU 
            running.remaining_time += 1;
            running.processing_time +=1; 
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
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}