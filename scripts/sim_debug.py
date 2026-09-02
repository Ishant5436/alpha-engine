import struct

ticks = []
with open(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data', 'ticks_500k.bin'), 'rb') as f:
    while True:
        data = f.read(56)
        if not data: break
        ts, bid, ask, bsz, asz, last, vol = struct.unpack('Qdddddd', data)
        ticks.append((last, bid, ask))

prices = [t[0] for t in ticks]
print(f"Total ticks: {len(ticks):,}")
print(f"Price range: {min(prices):.2f} -> {max(prices):.2f} (Start: {prices[0]:.2f}, End: {prices[-1]:.2f})")

# Test momentum / trend follower
fast_ema = prices[0]
slow_ema = prices[0]
position = 0
entry_price = 0.0
realized_pnl = 0.0
trades = 0
wins = 0

for i, (p, bid, ask) in enumerate(ticks):
    fast_ema = 0.005 * p + 0.995 * fast_ema
    slow_ema = 0.0005 * p + 0.9995 * slow_ema
    signal = (fast_ema - slow_ema) / p

    if position == 0:
        if signal > 0.0002:
            position = 1
            entry_price = ask
            trades += 1
        elif signal < -0.0002:
            position = -1
            entry_price = bid
            trades += 1
    elif position == 1:
        if signal < 0.0 or (p - entry_price) < -30.0:
            pnl = (bid - entry_price) * (10000.0 / entry_price) # 1x leverage $10k
            realized_pnl += pnl
            if pnl > 0: wins += 1
            position = 0
    elif position == -1:
        if signal > 0.0 or (entry_price - p) < -30.0:
            pnl = (entry_price - ask) * (10000.0 / entry_price)
            realized_pnl += pnl
            if pnl > 0: wins += 1
            position = 0

print(f"Results: Trades={trades}, Wins={wins} ({100*wins/max(1,trades):.1f}%), Realized PnL=${realized_pnl:.2f}")
