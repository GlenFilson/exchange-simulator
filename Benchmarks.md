# Benchmarks

All benchmarks: 1M orders, localhost, printing disabled, Release mode.

**Machine:** Ryzen 7 7700x, 32GB DDR5 5600Mhz, Rocky Linux(Server)

## Results


### 2026-03-09 — TCP_NODELAY + single send() (Release)

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.590 | 0.573 |
| User time | 1.393s | 1.240s |
| Sys time | 8.482s | 8.170s |
| Instructions | 10297480589 | 6652682220 |
| Cycles | 7598847253 | 5451523931 |
| IPC | 1.356 | 1.220 |
| Branch miss rate | 1.070% | 1.744% |
| Frontend idle | 45.980% | 42.576% |
| Page faults | 123 | 129 |

**Throughput: 46728.200 orders/sec | Elapsed: 21.403s | Runs: 5**


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

### 2026-03-13 — Epoll, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.535 | 0.525 |
| User time | 1.257s | 0.903s |
| Sys time | 8.249s | 8.062s |
| Instructions | 1941850787 | 1113503882 |
| Cycles | 2467231037 | 1761550741 |
| IPC | 0.788 | 0.634 |
| Branch miss rate | 5.716% | 8.068% |
| Frontend idle | 65.122% | 64.854% |
| Page faults | 10004 | 134 |

**Throughput: 47801.000 orders/sec | Elapsed: 20.923s | Runs: 5 | Clients: 1**

### 2026-03-13 — Epoll, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.986 | 1.594 |
| User time | 5.606s | 4.853s |
| Sys time | 20.786s | 43.663s |
| Instructions | 9598259972 | 5595234642 |
| Cycles | 12407867353 | 9031295638 |
| IPC | 0.772 | 0.620 |
| Branch miss rate | 5.702% | 8.244% |
| Frontend idle | 51.164% | 64.530% |
| Page faults | 42464 | 157 |

**Throughput: 134735.600 orders/sec | Elapsed: 37.136s | Runs: 5 | Clients: 5**


### 2026-03-14 — try_send + flat array, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.491 | 0.554 |
| User time | 0.681s | 0.902s |
| Sys time | 7.300s | 8.001s |
| Instructions | 1792983692 | 1114016377 |
| Cycles | 1954223384 | 1730075394 |
| IPC | 0.918 | 0.644 |
| Branch miss rate | 4.230% | 8.088% |
| Frontend idle | 60.076% | 64.792% |
| Page faults | 10030 | 135 |

**Throughput: 50686.800 orders/sec | Elapsed: 19.731s | Runs: 5 | Clients: 1**

### 2026-03-14 — try_send + flat array, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.986 | 1.732 |
| User time | 3.518s | 4.958s |
| Sys time | 21.180s | 50.321s |
| Instructions | 9019057953 | 5595230311 |
| Cycles | 10429642058 | 9427010472 |
| IPC | 0.864 | 0.592 |
| Branch miss rate | 4.366% | 8.294% |
| Frontend idle | 44.612% | 65.102% |
| Page faults | 42386 | 155 |

**Throughput: 127900.800 orders/sec | Elapsed: 39.239s | Runs: 5 | Clients: 5**

### 2026-03-14 — try_send + flat array + write batching, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.490 | 0.554 |
| User time | 0.676s | 0.911s |
| Sys time | 7.320s | 8.027s |
| Instructions | 1789903496 | 1113659165 |
| Cycles | 1841972848 | 1731952203 |
| IPC | 0.970 | 0.642 |
| Branch miss rate | 3.734% | 8.084% |
| Frontend idle | 57.500% | 64.846% |
| Page faults | 10336 | 134 |

**Throughput: 50617.400 orders/sec | Elapsed: 19.758s | Runs: 5 | Clients: 1**


### 2026-03-14 — try_send + flat array + write batching, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 0.984 | 1.894 |
| User time | 3.229s | 4.785s |
| Sys time | 17.878s | 45.956s |
| Instructions | 8894051933 | 5595994548 |
| Cycles | 9666061448 | 9201098401 |
| IPC | 0.920 | 0.610 |
| Branch miss rate | 4.024% | 8.252% |
| Frontend idle | 39.678% | 64.708% |
| Page faults | 42284 | 155 |

**Throughput: 152448.800 orders/sec | Elapsed: 32.843s | Runs: 5 | Clients: 5**