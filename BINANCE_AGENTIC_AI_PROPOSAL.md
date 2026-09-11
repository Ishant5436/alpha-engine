# Beyond the Benchmark: Binance Agentic AI Challenge — Technical Proposal

**Team Name:** AlphaEngine Core  
**Lead Contributor:** Ishant Panchal (`Ishant5436` / `ishant.p@somaiya.edu`)  
**Institution:** Somaiya Vidyavihar University, Mumbai, India  
**Repository:** [https://github.com/Ishant5436/alpha-engine](https://github.com/Ishant5436/alpha-engine)  
**Track:** Autonomous Agentic AI, Quantitative Execution & Risk Governance  

---

## Form Field Responses (Ready for Direct Submission)

### General & Team Information
* **Team Name:** AlphaEngine Core
* **Team Size:** 1 (Solo Developer)
* **Name:** Ishant Panchal
* **Location:** Mumbai, India
* **Role:** Quantitative Systems Engineer / Systems Developer
* **University / Institution:** Somaiya Vidyavihar University
* **Highest Degree:** Bachelor's
* **Major:** Computer Engineering
* **Graduation Year:** 2026
* **Prior Experience:**
  Architected zero-heap C++20 algorithmic execution core (`alpha-engine`) benchmarked at >20,000,000 ticks/sec across Binance Spot tick histories; engineered multi-target orthogonal ensemble pipelines with linear feature neutralization for the Numerai tournament; built production FastMCP execution gateways with Power of 10 safety invariants and EIP-712 cryptographic permit signing.

---

### Question 37: Your Proposal
`AlphaEngine Core` is an autonomous, friction-aware agentic trading system engineered to bridge the gap between probabilistic LLM reasoning and deterministic financial execution in live Binance Web3 and spot markets. 

Most algorithmic agents fail in production because they treat trading as text completion: they ignore exchange taker fees (4.0 bps per fill), incur slippage in choppy market regimes, and bleed capital via high-frequency churning. `AlphaEngine Core` deploys a two-tier decoupled architecture:
1. **The Reasoning Layer (Cognitive Plane):** A reasoning agent that ingests Binance Web3 tick feeds, order book depth, and liquidity microstructure, synthesizing macro regime classification into probabilistic directional hypotheses.
2. **The Execution & Risk Core (Deterministic Plane):** A compiled, zero-heap C++20/Rust execution engine adhering to Gerard J. Holzmann's Power of 10 Safety Invariants. 

The agent operates under an asymmetric volatility deadband: in low-volatility consolidation regimes, the engine strictly forces a 100% `FLAT` state, eliminating 95%+ of spread friction. When Parkinson realized volatility indicates expansion, the reasoning layer initiates trades gated by a hard 3:1 reward-to-risk ratio, Kelly-derived fractional position sizing, and an immutable Keccak256 execution audit log. The system runs autonomously with zero human intervention and 100% deterministic reproducibility.

---

### Question 38: System Architecture (Summary of Uploaded PDF)
The architecture decouples non-deterministic perception and reasoning from deterministic execution:
* **Perception:** WebSocket tick streamer continuously ingests order book depth, trades, and funding rates into a fixed-capacity ring buffer (`MarketDataRingBuffer<2048>`), providing strictly $\mathcal{O}(1)$ push/pop and real-time VWAP calculation without dynamic heap allocations.
* **Reasoning:** A localized LLM agent evaluates multi-horizon EMA alignment across three discrete timescales (Fast 50-tick, Medium 250-tick, Slow 1,250-tick) alongside real-time Parkinson volatility estimates. Uncalibrated probabilities are calibrated against the empirical base rate via odds-ratio thresholding ($P \ge 1.25 \times \text{Base Rate}$).
* **Decisioning:** Signal deadbands prevent spread drag. The system operates in three discrete states: `LONG`, `FLAT`, and `SHORT`. Positions are entered only when directional conviction exceeds friction-adjusted hurdles ($>20\times$ fee drag).
* **Trading & Execution:** Orders are routed through authenticated Binance Web3 Open APIs with atomic nonce management, EIP-55 address validation, and a hard 4.0% portfolio circuit breaker. Every decision generates a deterministic JSON-RPC audit receipt signed with local keypairs.

---

### Question 39: Data / Tools Used
* **Market Data:** Binance Web3 Open APIs, Binance Spot WebSocket streams (Trade, Depth20, BookTicker), historical 1s/1m Kline distributions, and funding rate differentials.
* **Agent Harness:** FastMCP (Model Context Protocol) tool harness in Python 3.12, providing structured tool calling, input parameter validation, and sandbox telemetry.
* **Execution & Compute Core:** C++20 engine compiled with `-O3 -march=armv8.5-a` on Apple Silicon ARM64, integrated via zero-copy shared memory / IPC.
* **Verification & Testing:** Pytest, Hypothesis property-based fuzzing, AddressSanitizer, and Valgrind memory leak verification.

---

### Question 40: Position & Risk Management
1. **Expectancy-to-Friction Hurdle:** Trades are permitted only when expected excursion exceeds round-trip friction (taker fee 4 bps $\times 2$ + 2 bps half-spread = 10 bps) by at least $20\times$ (minimum 200 bps favorable excursion target).
2. **Fractional Kelly Sizing:** Base trade sizing scales dynamically with volatility: $\text{Size} = \min(0.05 \times \text{Portfolio}, \frac{\mu - r}{\sigma^2})$. Maximum capital per active position is capped at 5.0% of portfolio equity.
3. **Mandatory Inactivity Invariant:** The model spends $\ge 80\%$ of all market ticks in a 100% `FLAT` state, entering only during verified volatility regime breakouts.
4. **Hard Circuit Breaker:** An isolated, non-bypassable global kill-switch triggers immediate atomic position liquidation if daily cumulative drawdown breaches 4.0%, locking execution until manual post-mortem review.

---

### Question 41: Preventing Future Leakage & Overfitting
1. **Purged & Embargoed Cross-Validation:** Backtesting and evaluation employ Marcos López de Prado's Purged Walk-Forward Cross-Validation. A mandatory 5-day embargo and purge window between training and test folds eliminates auto-correlated serial leakage.
2. **Raw Historical Tick Data:** All backtests run against un-resampled historical trade ticks directly from public Binance archives; zero reliance on synthetic toy generators.
3. **Asymmetric Base-Rate Calibration:** Prevents high false-positive rates by calibrating output classification scores against historical base rates (~15% empirical breakout frequency).
4. **Deterministic Feature Neutralization:** Signal vectors are projected orthogonal to principal risk factors (market momentum, beta, sector concentration) via QR decomposition: $\mathbf{P}_{\text{neutral}} = \mathbf{P} - \alpha \mathbf{X}(\mathbf{X}^T \mathbf{X})^{-1}\mathbf{X}^T \mathbf{P}$.

---

### Question 42: Reproducibility Plan
1. **Single-Command Test & Verification:** The entire environment compiles and validates with a single command: `make clean && make test && make demo`.
2. **Deterministic Seed & Replay Engine:** Historical tick files are binary-encoded (`.bin`). The engine includes an offline deterministic replay runner that reads serialized tick logs and reproduces exact trade fills, latency metrics, and PnL down to the microsecond.
3. **Containerized Environment:** Packaged with multi-stage Dockerfiles pinning GCC 13, Python 3.12, and exact library lockfiles (`uv.lock`), ensuring identical behavior across Linux x86_64 and macOS ARM64.
4. **Read-Only Audit Credentials:** A dedicated read-only repository audit account and Binance Developer Portal testnet key are pre-configured for contest judges.

---

### Question 43: Team Capability & Intended Scope
As an independent quantitative engineer with end-to-end fluency across low-latency C++ algorithmic engines, Rust EVM proxies, and Python agent harnesses, I operate without organizational overhead or team coordination friction. 

**Intended Deliverables for the 4-Week Challenge:**
* **Weeks 1–2 (Build Phase):** Deploy containerized Binance Web3 Agentic Core with FastMCP tooling, live WebSocket telemetry, Parkinson volatility filter, and risk circuit breakers.
* **Weeks 3–4 (Live Scored Run):** Operate the agent 24/7 on designated bStocks / BNB Chain assets, publishing verifiable daily onchain audit logs, execution metrics, and risk-adjusted Sharpe/Sortino ratios.
