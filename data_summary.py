import os


def read_output_files(path):
    events = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith("|"):
                continue

            cols = [x.strip() for x in line.split("|")[1:-1]]
            if len(cols) != 4:
                continue

            time, pid, old, new = cols

            if not time.isdigit():
                continue

            events.append({
                "time": int(time),
                "pid": int(pid),
                "old": old,
                "new": new
            })

    return events


def anaylze_data(blocks):

    first_pass = {}
    terminated = {}
    start_times = {}
    wait_time = {}
    last_ready = {}
    io_times = {}
    pids = set()

    for block in blocks:
        pid = block["pid"]
        time = block["time"]
        pids.add(pid)

        if pid not in wait_time:
            wait_time[pid] = 0
        if pid not in io_times:
            io_times[pid] = []

        if block["old"] == "NEW" and block["new"] == "READY":
            start_times[pid] = time
            last_ready[pid] = time

        if block["old"] == "READY" and block["new"] == "RUNNING":
            if pid not in first_pass:
                first_pass[pid] = time
            wait_time[pid] += time - last_ready[pid]
        if block["old"] == "RUNNING" and block["new"] == "WAITING":
            io_times[pid].append(time)

    
        if block["old"] == "WAITING" and block["new"] == "READY":
            last_ready[pid] = time
        if block["new"] == "TERMINATED":
            terminated[pid] = time

    pids = sorted(pids)
    N = len(pids)

    total_time = max(terminated.values())
    throughput = N / total_time
    avg_wait = sum(wait_time[p] for p in pids) / N
    avg_turn = sum(terminated[p] - start_times[p] for p in pids) / N

    # response times
    io_gaps = []
    for p in pids:
        for i in range(1, len(io_times[p])):
            io_gaps.append(io_times[p][i] - io_times[p][i-1])

    avg_resp = sum(io_gaps)/len(io_gaps) if io_gaps else 0

    return throughput, avg_wait, avg_turn, avg_resp

def average(values):
    if not values:
        return (0, 0, 0, 0)
    n = len(values)
    s = [sum(v[i] for v in values) / n for i in range(4)]
    return tuple(s)


def summarize(root):
    schedulers = ["interrupts_EP", "interrupts_RR", "interrupts_RR_EP"]
    categories = ["IO", "CPU", "Balanced"]

    results = {c: {s: [] for s in schedulers} for c in categories}

    for sched in schedulers:
        for cat in categories:
            folder = os.path.join(root, sched, cat)
            if not os.path.isdir(folder):
                continue

            for fname in os.listdir(folder):
                if not fname.endswith(".txt"):
                    continue

                blocks = read_output_files(os.path.join(folder, fname))
                r = anaylze_data(blocks)
                if r is not None:
                    results[cat][sched].append(r)

    for cat in categories:
        print(f"\n=== {cat} AVERAGES ===")
        for sched in schedulers:
            avg_vals = average(results[cat][sched])
            t, w, turn, resp = avg_vals
            print(f"{sched:15s}  Throughput={t:.4f}  Wait={w:.2f}  Turnaround={turn:.2f}  Response={resp:.2f}")


if __name__ == "__main__":
    import sys
    summarize(sys.argv[1])