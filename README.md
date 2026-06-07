# Week 1

## System Hardware Config

* Architecture: x86_64
* CPU Model: AMD EPYC 7763 64-Core Processor
* Caches: L1d 32 KiB, L1i 32 KiB, L2 512 KiB, L3 32 MiB
* Memory: 3.8 GiB total

## Latency (on local)

* p50 Latency: 16 nanoseconds
* p90 Latency: 32 nanoseconds
* p99 Latency: 64 nanoseconds

On Leaderboard Judge:
* p50 Latency: 24 nanoseconds   
* p99 Latency: 72 nanoseconds   
* p999 Latency: 104 nanoseconds   
* Throughput: 17.73 M

## Optimization Highlights

1) Optimized asset state lookups by comparing memory addresses directly instead of executing string comparisons.
2) Used Sliding Windows to eliminate nested loops when tracking rolling indicators.
3) Replaces conditional if/else control blocks with flat arithmetic expressions. (Branchless Logic)