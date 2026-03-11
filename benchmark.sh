#!/bin/bash
# benchmark.sh — run from the build/ directory
# Usage: ./benchmark.sh [num_runs] [label]
# Example: ./benchmark.sh 5 "TCP_NODELAY + single send(), 1M orders"
# Requires: perf, bc

RUNS=${1:-5}
LABEL=${2:-"unlabelled"}
DATE=$(date +%Y-%m-%d)

echo "=== Benchmark: $RUNS iterations ==="
echo ""

# Accumulator arrays
declare -a throughput_arr elapsed_arr
declare -a c_cpu_arr c_ipc_arr c_branch_arr c_user_arr c_sys_arr c_instructions_arr c_cycles_arr c_frontend_arr c_pagefaults_arr
declare -a s_cpu_arr s_ipc_arr s_branch_arr s_user_arr s_sys_arr s_instructions_arr s_cycles_arr s_frontend_arr s_pagefaults_arr

for ((i=1; i<=RUNS; i++)); do
    echo "Run $i/$RUNS"

    # Start server under perf stat
    perf stat -o /tmp/bench_server.txt ./exchange &
    SERVER_PID=$!
    sleep 0.5

    # Run client under perf stat, capture stdout for throughput
    perf stat -o /tmp/bench_client.txt ./client > /tmp/bench_client_out.txt 2>&1

    # Kill server
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

    # Extract throughput
    tput=$(grep "orders/sec" /tmp/bench_client_out.txt | grep -oP '^\d+' || echo "0")
    throughput_arr+=($tput)
    echo "  -> $tput orders/sec"

    # Parse client perf stat
    c_cpu_arr+=($(grep "CPUs utilized" /tmp/bench_client.txt | grep -oP '[\d.]+(?=\s+CPUs)' || echo "0"))
    c_ipc_arr+=($(grep "insn per cycle" /tmp/bench_client.txt | grep -oP '[\d.]+(?=\s+insn)' || echo "0"))
    c_branch_arr+=($(grep "branch-misses" /tmp/bench_client.txt | grep -oP '[\d.]+(?=%)' || echo "0"))
    c_frontend_arr+=($(grep "frontend cycles idle" /tmp/bench_client.txt | grep -oP '[\d.]+(?=%)' || echo "0"))
    c_user_arr+=($(grep "seconds user" /tmp/bench_client.txt | awk '{print $1}' || echo "0"))
    c_sys_arr+=($(grep "seconds sys" /tmp/bench_client.txt | awk '{print $1}' || echo "0"))
    c_instructions_arr+=($(grep "instructions" /tmp/bench_client.txt | awk '{print $1}' | tr -d ',' || echo "0"))
    c_cycles_arr+=($(grep -m1 "cycles:u" /tmp/bench_client.txt | awk '{print $1}' | tr -d ',' || echo "0"))
    c_pagefaults_arr+=($(grep "page-faults" /tmp/bench_client.txt | awk '{print $1}' | tr -d ',' || echo "0"))
    elapsed_arr+=($(grep "seconds time elapsed" /tmp/bench_client.txt | awk '{print $1}' || echo "0"))

    # Parse server perf stat
    s_cpu_arr+=($(grep "CPUs utilized" /tmp/bench_server.txt | grep -oP '[\d.]+(?=\s+CPUs)' || echo "0"))
    s_ipc_arr+=($(grep "insn per cycle" /tmp/bench_server.txt | grep -oP '[\d.]+(?=\s+insn)' || echo "0"))
    s_branch_arr+=($(grep "branch-misses" /tmp/bench_server.txt | grep -oP '[\d.]+(?=%)' || echo "0"))
    s_frontend_arr+=($(grep "frontend cycles idle" /tmp/bench_server.txt | grep -oP '[\d.]+(?=%)' || echo "0"))
    s_user_arr+=($(grep "seconds user" /tmp/bench_server.txt | awk '{print $1}' || echo "0"))
    s_sys_arr+=($(grep "seconds sys" /tmp/bench_server.txt | awk '{print $1}' || echo "0"))
    s_instructions_arr+=($(grep "instructions" /tmp/bench_server.txt | awk '{print $1}' | tr -d ',' || echo "0"))
    s_cycles_arr+=($(grep -m1 "cycles:u" /tmp/bench_server.txt | awk '{print $1}' | tr -d ',' || echo "0"))
    s_pagefaults_arr+=($(grep "page-faults" /tmp/bench_server.txt | awk '{print $1}' | tr -d ',' || echo "0"))
done

# Averaging functions
avg() {
    local arr=("$@")
    local count=${#arr[@]}
    local sum=0
    for val in "${arr[@]}"; do
        sum=$(echo "$sum + $val" | bc -l)
    done
    printf "%.3f" $(echo "$sum / $count" | bc -l)
}

avg_int() {
    local arr=("$@")
    local count=${#arr[@]}
    local sum=0
    for val in "${arr[@]}"; do
        sum=$(echo "$sum + $val" | bc -l)
    done
    printf "%.0f" $(echo "$sum / $count" | bc -l)
}

echo ""
echo "========================================="
echo "RESULTS (averaged over $RUNS runs)"
echo "========================================="
echo ""
echo "Throughput: $(avg "${throughput_arr[@]}") orders/sec"
echo "Elapsed:    $(avg "${elapsed_arr[@]}")s"
echo ""
echo "==========================================="
echo "COPY BELOW INTO BENCHMARKS.md"
echo "==========================================="
echo ""
echo "### $DATE — $LABEL"
echo ""
echo "| Metric | Server | Client |"
echo "|--------|--------|--------|"
echo "| CPU utilization | $(avg "${s_cpu_arr[@]}") | $(avg "${c_cpu_arr[@]}") |"
echo "| User time | $(avg "${s_user_arr[@]}")s | $(avg "${c_user_arr[@]}")s |"
echo "| Sys time | $(avg "${s_sys_arr[@]}")s | $(avg "${c_sys_arr[@]}")s |"
echo "| Instructions | $(avg_int "${s_instructions_arr[@]}") | $(avg_int "${c_instructions_arr[@]}") |"
echo "| Cycles | $(avg_int "${s_cycles_arr[@]}") | $(avg_int "${c_cycles_arr[@]}") |"
echo "| IPC | $(avg "${s_ipc_arr[@]}") | $(avg "${c_ipc_arr[@]}") |"
echo "| Branch miss rate | $(avg "${s_branch_arr[@]}")% | $(avg "${c_branch_arr[@]}")% |"
echo "| Frontend idle | $(avg "${s_frontend_arr[@]}")% | $(avg "${c_frontend_arr[@]}")% |"
echo "| Page faults | $(avg_int "${s_pagefaults_arr[@]}") | $(avg_int "${c_pagefaults_arr[@]}") |"
echo ""
echo "**Throughput: $(avg "${throughput_arr[@]}") orders/sec | Elapsed: $(avg "${elapsed_arr[@]}")s | Runs: $RUNS**"

# Cleanup
rm -f /tmp/bench_server.txt /tmp/bench_client.txt /tmp/bench_client_out.txt
