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


### 2026-03-14 — multithreaded, mutex queue, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.999 | 0.640 |
| User time | 24.448s | 0.602s |
| Sys time | 7.815s | 8.182s |
| Instructions | 302240630247 | 1113781092 |
| Cycles | 96520396613 | 1715621669 |
| IPC | 3.130 | 0.648 |
| Branch miss rate | 0.116% | 8.136% |
| Frontend idle | 27.976% | 64.858% |
| Page faults | 9946 | 134 |

**Throughput: 59940.600 orders/sec | Elapsed: 16.685s | Runs: 5 | Clients: 1**


### 2026-03-14 — multithreaded, mutex queue, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.998 | 1.730 |
| User time | 42.365s | 5.145s |
| Sys time | 21.916s | 48.521s |
| Instructions | 605233683098 | 5595907205 |
| Cycles | 200096038608 | 9347505379 |
| IPC | 3.024 | 0.600 |
| Branch miss rate | 0.088% | 8.270% |
| Frontend idle | 26.198% | 64.672% |
| Page faults | 42405 | 155 |

**Throughput: 131764.400 orders/sec | Elapsed: 38.016s | Runs: 5 | Clients: 5**



### 2026-03-15 — multithreaded, atomic check, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.999 | 0.667 |
| User time | 23.077s | 0.591s |
| Sys time | 7.220s | 8.016s |
| Instructions | 349898943849 | 1113649525 |
| Cycles | 94382503646 | 1697257972 |
| IPC | 3.706 | 0.658 |
| Branch miss rate | 0.110% | 8.036% |
| Frontend idle | 4.944% | 64.736% |
| Page faults | 10055 | 136 |

**Throughput: 63806.800 orders/sec | Elapsed: 15.674s | Runs: 5 | Clients: 1**


### 2026-03-15 — multithreaded, atomic check, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.997 | 1.823 |
| User time | 39.770s | 5.087s |
| Sys time | 19.933s | 48.081s |
| Instructions | 741767780433 | 5595407948 |
| Cycles | 201116572602 | 9302277784 |
| IPC | 3.688 | 0.600 |
| Branch miss rate | 0.054% | 8.262% |
| Frontend idle | 3.546% | 64.716% |
| Page faults | 42303 | 155 |

**Throughput: 140042.600 orders/sec | Elapsed: 35.797s | Runs: 5 | Clients: 5**

### 2026-03-15 — SPSC ring buffer, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 2.000 | 0.670 |
| User time | 23.622s | 0.574s |
| Sys time | 6.706s | 8.106s |
| Instructions | 385909550007 | 1114044313 |
| Cycles | 93621392877 | 1692520228 |
| IPC | 4.122 | 0.658 |
| Branch miss rate | 0.120% | 8.058% |
| Frontend idle | 4.546% | 64.632% |
| Page faults | 9952 | 136 |

**Throughput: 63,763 orders/sec | Elapsed: 15.685s | Runs: 5 | Clients: 1**

### 2026-03-15 — SPSC ring buffer, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.999 | 1.837 |
| User time | 39.861s | 5.171s |
| Sys time | 19.921s | 48.600s |
| Instructions | 827147427198 | 5595889086 |
| Cycles | 199892428569 | 9384306105 |
| IPC | 4.138 | 0.598 |
| Branch miss rate | 0.060% | 8.370% |
| Frontend idle | 2.738% | 64.912% |
| Page faults | 42618 | 155 |

**Throughput: 139,194 orders/sec | Elapsed: 35.952s | Runs: 5 | Clients: 5**


### 2026-03-15 — Pipelined client + read batching, 1 client

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.999 | 0.994 |
| User time | 9.952s | 1.077s |
| Sys time | 3.198s | 3.999s |
| Instructions | 162771150858 | 1083036619 |
| Cycles | 39955707227 | 1168585281 |
| IPC | 4.074 | 0.930 |
| Branch miss rate | 0.112% | 8.542% |
| Frontend idle | 4.722% | 60.976% |
| Page faults | 10371 | 135 |

**Throughput: 153,424 orders/sec | Elapsed: 6.520s | Runs: 5 | Clients: 1**

### 2026-03-15 — Pipelined client + read batching, 5 clients

| Metric | Server | Client |
|--------|--------|--------|
| CPU utilization | 1.992 | 4.606 |
| User time | 9.932s | 6.050s |
| Sys time | 4.363s | 17.923s |
| Instructions | 128355601644 | 5402834652 |
| Cycles | 40683601098 | 5890314671 |
| IPC | 3.154 | 0.916 |
| Branch miss rate | 0.210% | 8.460% |
| Frontend idle | 15.312% | 61.446% |
| Page faults | 44477 | 155 |

**Throughput: 749,967 orders/sec | Elapsed: 6.746s | Runs: 5 | Clients: 5**

