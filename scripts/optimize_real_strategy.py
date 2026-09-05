import os
import struct
import numpy as np

ticks = []
with open(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data', 'real_btc_ticks.bin'), 'rb') as f:
    while True:
        data = f.read(56)
        if not data: break
        ts, bid, ask, bsz, asz, last, vol = struct.unpack('Qdddddd', data)
        ticks.append((last, bid, ask, vol))

prices = np.array([t[0] for t in ticks])
bids = np.array([t[1] for t in ticks])
asks = np.array([t[2] for t in ticks])

print(f"Loaded {len(ticks):,} real Binance BTC/USDT ticks.")

def run_macro_breakout_sim(channel_window=3000, tp_bps=60.0, sl_bps=15.0, trail_bps=20.0, fee_bps=4.0):
    capital = 10000.0
    equity = 10000.0
    peak_eq = 10000.0
    max_dd = 0.0

    position = 0
    entry_price = 0.0
    peak_trade_price = 0.0
    trades = 0
    wins = 0
    gross_profit = 0.0
    gross_loss = 0.0
    fees_paid = 0.0
    returns = []
    prev_eq = capital

    fee_rate = fee_bps / 10000.0

    for i in range(len(prices)):
        p = prices[i]
        bid = bids[i]
        ask = asks[i]

        if i < channel_window:
            continue

        # Lookback channel high and low (over last channel_window ticks)
        lookback = prices[i - channel_window : i]
        chan_high = np.max(lookback)
        chan_low = np.min(lookback)

        if position == 0:
            # Donchian Breakout: New High
            if p > chan_high:
                position = 1
                entry_price = ask
                peak_trade_price = ask
                fee = capital * fee_rate
                fees_paid += fee
                equity -= fee
                trades += 1
            # Donchian Breakdown: New Low
            elif p < chan_low:
                position = -1
                entry_price = bid
                peak_trade_price = bid
                fee = capital * fee_rate
                fees_paid += fee
                equity -= fee
                trades += 1
        elif position == 1:
            peak_trade_price = max(peak_trade_price, bid)
            gain_bps = 10000.0 * (bid - entry_price) / entry_price
            drop_from_peak_bps = 10000.0 * (peak_trade_price - bid) / peak_trade_price

            # Exit on Take Profit, Stop Loss, or Trailing Stop
            if gain_bps >= tp_bps or gain_bps <= -sl_bps or (gain_bps > 20.0 and drop_from_peak_bps >= trail_bps):
                trade_pnl = (bid - entry_price) * (capital / entry_price)
                fee = capital * fee_rate
                fees_paid += fee
                net_pnl = trade_pnl - fee
                equity += net_pnl
                if net_pnl > 0:
                    wins += 1
                    gross_profit += net_pnl
                else:
                    gross_loss += abs(net_pnl)
                position = 0
        elif position == -1:
            peak_trade_price = min(peak_trade_price, ask)
            gain_bps = 10000.0 * (entry_price - ask) / entry_price
            rise_from_trough_bps = 10000.0 * (ask - peak_trade_price) / peak_trade_price

            if gain_bps >= tp_bps or gain_bps <= -sl_bps or (gain_bps > 20.0 and rise_from_trough_bps >= trail_bps):
                trade_pnl = (entry_price - ask) * (capital / entry_price)
                fee = capital * fee_rate
                fees_paid += fee
                net_pnl = trade_pnl - fee
                equity += net_pnl
                if net_pnl > 0:
                    wins += 1
                    gross_profit += net_pnl
                else:
                    gross_loss += abs(net_pnl)
                position = 0

        peak_eq = max(peak_eq, equity)
        dd = 100.0 * (peak_eq - equity) / peak_eq
        max_dd = max(max_dd, dd)

        if i % 1000 == 0:
            ret = (equity - prev_eq) / prev_eq
            returns.append(ret)
            prev_eq = equity

    ret_arr = np.array(returns)
    sharpe = (np.mean(ret_arr) / np.std(ret_arr) * np.sqrt(252 * 24 * 6)) if len(ret_arr) > 1 and np.std(ret_arr) > 0 else 0.0
    pf = gross_profit / max(1e-4, gross_loss)
    tot_ret = 100.0 * (equity - capital) / capital

    print(f"Window={channel_window} -> Return: {tot_ret:+.2f}%, Sharpe: {sharpe:.2f}, MaxDD: {max_dd:.2f}%, Trades: {trades}, WinRate: {100*wins/max(1,trades):.1f}%, ProfitFactor: {pf:.2f}, TotalFees: ${fees_paid:.2f}")

for w in [1500, 3000, 5000, 10000]:
    run_macro_breakout_sim(channel_window=w, tp_bps=50.0, sl_bps=12.0, trail_bps=15.0, fee_bps=4.0)
