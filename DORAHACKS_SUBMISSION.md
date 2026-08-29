# WEEX AI Wars II Submission: AlphaEngine

**Project Name:** AlphaEngine  
**GitHub Repository:** https://github.com/Ishant5436/alpha-engine  
**Track:** AI Trading, Algorithmic Execution, Quantitative Risk Management  

---

### Project Description
AlphaEngine is an ultra-high-throughput (38M+ ticks/sec), zero-heap C++20 algorithmic trading engine designed for the WEEX AI Wars II Hackathon. Built under Gerard J. Holzmann's NASA / JPL Power of 10 safety invariants, it features real-time volatility regime switching, Parkinson volatility estimators, and a deterministic 4.0% maximum drawdown circuit breaker.

### Key Highlights:
1. **Zero-Heap Execution:** Zero dynamic memory allocation on the tick processing hot path.
2. **Sub-Microsecond Latency:** Processes over 38,000,000 ticks per second on standard ARM64 silicon.
3. **Deterministic Risk Protection:** Bounded execution loops and hard drawdown kill-switches protecting against flash-crash black swans.
