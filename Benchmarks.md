# Benchmarks

All benchmarks: 1M orders, localhost, printing disabled, Release mode.

**Machine:** Ryzen 7 7700x, 32GB DDR5 5600Mhz, Rocky Linux(Server)

## Results


### 2026-03-09 — TCP_NODELAY + single send()

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.591 | 0.573 |
| User time | 1.357s | 1.269s |
| Sys time | 8.589s | 8.226s |
| Instructions | 10298410592 | 6652817977 |
| Cycles | 7585726006 | 5550182514 |
| IPC | 1.354 | 1.198 |
| Branch miss rate | 1.150% | 1.752% |
| Frontend idle | 46.878% | 42.714% |
| Page faults | 122 | 129 |

**Throughput: 46152.800 orders/sec | Elapsed: 21.669s | Runs: 5**

### 2026-03-11 — Pre-allocated buffers, reused Message

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.582 | 0.580 |
| User time | 0.653s | 0.781s |
| Sys time | 8.102s | 7.836s |
| Instructions | 1712744011 | 1055144325 |
| Cycles | 1810490887 | 1441953392 |
| IPC | 0.946 | 0.732 |
| Branch miss rate | 3.804% | 5.970% |
| Frontend idle | 58.146% | 64.064% |
| Page faults | 124 | 128 |

**Throughput: 50643.400 orders/sec | Elapsed: 19.748s | Runs: 5**