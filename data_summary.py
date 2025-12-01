
import os 
import csv
from tracemalloc import start


def read_output_files(path):
    blocks = []
    with open(path) as f:
        for line in f:
            if line.startswith("|"):
                cols = [x.strip() for x in line.split("|")[1:-1]]
                time, pid, last_state, new_state = cols
                blocks.append({
                    "time": int(time),
                    "pid": int(pid),
                    "old": last_state,
                    "new": new_state
                })
    return blocks

def anaylze_data(blocks):

    first_pass = {}
    termintated = {}
    start_times = {}
    wait_time = {}
    last_ready = {}
    io_times = {}
    pids = set()

    for block in blocks:
        pid = block["pid"]
        time = block["time"]
        pids.add(pid)


         #running to waiting: started IO at the time 
        if block["old"] == "RUNNING" and block["new"] == "WAITING":
            io_times[pid].append(time)
        if block["new"] == "NEW":
            start_times[pid] = time
        # ready to running: time spent in ready Q
        if block["old"] == "READY" and block["new"] == "RUNNING":
            if pid not in first_pass:
                first_pass[pid] = time
            if pid in last_ready:
                wait_time[pid] += time - last_ready[pid]
        # waiting to ready
        if block["old"] == "WAITING" and block["new"] == "READY":
            last_ready[pid] = time
 # new to ready
        if block["old"] == "NEW" and block["new"] == "READY":
            block[pid] = time
            last_ready[pid] = time
            wait_time[pid] = 0
            io_times[pid] = []
        # terminated 
        if block["new"] == "TERMINATED":
            termintated[pid] = time

    #now use the data to find wait time etc 
    pids = sorted(pids)
    N = len(pids)

    total_time = max(termintated.values()) # how long the whole thing took = largest endtime
    throughput = N / total_time
    avg_wait = sum(wait_time[p] for p in pids) / N
    avg_turn = sum(termintated[p] - start_times[p] for p in pids) / N

    io_wait_time = []
    for p in pids:
        ios = io_times[p]
        for i in range(1, len(ios)): #all test cases will need at least two IO calls
            try:
                io_wait_time.append(ios[i] - ios[i - 1])
            except: io_wait_time.append(0)

    
    try:
        avg_response = sum(io_wait_time) / len(io_wait_time + 0.01 ) 
    except: avg_response = 0

    return throughput, avg_wait, avg_turn, avg_response

def main():
    import sys
    blocks = read_output_files(sys.argv[1])
    throughput, avg_wait, avg_turn, avg_response = anaylze_data(blocks)

    print(f"Throughput:            {throughput:.3f}")
    print(f"Average Wait Time:     {avg_wait:.3f}")
    print(f"Average Turnaround:    {avg_turn:.3f}")
    print(f"Average Response Time: {avg_response:.3f}")
   
def csv_gen(root_dir, out_csv):

    simulators = ["interrupts_EP", "interrupts_RR", "interrupts_RR_EP"]
    categories = ["IO", "CPU", "Balanced"]

    results = {}
    

    #gather data from output files using the above functions 
    for sim in simulators:
        sim_path = os.path.join(root_dir, sim)
        if not os.path.isdir(sim_path):
            continue

        for cat in categories:
            cat_path = os.path.join(sim_path, cat)
            if not os.path.isdir(cat_path):
                continue

            for fname in os.listdir(cat_path):
                if not fname.endswith(".txt"):
                    continue

                fullpath = os.path.join(cat_path, fname)
                blocks = read_output_files(fullpath)
                output = anaylze_data(blocks)
                key = (cat, fname)
                if key not in results:
                    results[key] = {}
                results[key][sim] = output 


    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)

        w.writerow([
            "Category",
            "Testcase",

            "EP Throughput",
            "EP Avg Wait",
            "EP Avg Turnaround",
            "EP Avg Response",

            "RR Throughput",
            "RR Avg Wait",
            "RR Avg Turnaround",
            "RR Avg Response",

            "RR_EP Throughput",
            "RR_EP Avg Wait",
            "RR_EP Avg Turnaround",
            "RR_EP Avg Response"
        ])

        for (cat, fname), sims in sorted(results.items()):

            
            def get(sim, index):
                return f"{sims[sim][index]:.4f}" if sim in sims else ""

            w.writerow([
                cat,
                fname,

                get("interrupts_EP", 0),
                get("interrupts_EP", 1),
                get("interrupts_EP", 2),
                get("interrupts_EP", 3),

                get("interrupts_RR", 0),
                get("interrupts_RR", 1),
                get("interrupts_RR", 2),
                get("interrupts_RR", 3),

                get("interrupts_RR_EP", 0),
                get("interrupts_RR_EP", 1),
                get("interrupts_RR_EP", 2),
                get("interrupts_RR_EP", 3)
            ])

if __name__ == "__main__":
    import sys
    csv_gen(sys.argv[1], sys.argv[2])

