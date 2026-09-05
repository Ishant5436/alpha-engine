#!/usr/bin/env python3
"""
Binance Spot Live WebSocket Streamer to AlphaEngine C++ Core
Connects to public WebSocket feed (wss://stream.binance.com:9443/ws/<symbol>@trade)
and streams real-time market trade ticks into the zero-heap C++ paper trading engine.
"""

import sys
import os
import json
import time
import subprocess
import urllib.request

try:
    import websockets
    import asyncio
    HAS_WEBSOCKETS = True
except ImportError:
    HAS_WEBSOCKETS = False

def run_sync_stream(symbol="BTCUSDT", capital=10000.0, max_ticks=None):
    """Fallback streaming via fast polling aggTrades if websockets library is not installed."""
    print(f"Connecting to Binance Spot feed for {symbol} (Capital: ${capital:,.2f})...")
    bin_path = os.path.join(os.path.dirname(__file__), "..", "bin", "live_trader")
    if not os.path.exists(bin_path):
        subprocess.run(["make", "-C", os.path.join(os.path.dirname(__file__), ".."), "live"], check=True)

    proc = subprocess.Popen([bin_path, symbol, str(capital)], stdin=subprocess.PIPE, text=True, bufsize=1)

    url = f"https://api.binance.com/api/v3/trades?symbol={symbol}&limit=50"
    last_id = 0
    ticks_sent = 0

    try:
        while proc.poll() is None:
            try:
                req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
                with urllib.request.urlopen(req, timeout=3) as resp:
                    trades = json.loads(resp.read().decode('utf-8'))
                    for t in trades:
                        tid = t['id']
                        if tid > last_id:
                            last_id = tid
                            ts = t['time']
                            p = float(t['price'])
                            q = float(t['qty'])
                            side = "BUY" if not t['isBuyerMaker'] else "SELL"
                            proc.stdin.write(f"{ts} {p} {q} {side}\n")
                            proc.stdin.flush()
                            ticks_sent += 1
                            if max_ticks is not None and ticks_sent >= max_ticks:
                                print(f"\n[STREAM] Completed target stream of {max_ticks} live market ticks.")
                                return
                time.sleep(0.2)
            except Exception:
                time.sleep(0.5)
    except KeyboardInterrupt:
        print("\nStopping live paper trader...")
    finally:
        if proc.stdin and not proc.stdin.closed:
            try:
                proc.stdin.close()
            except Exception:
                pass
        try:
            proc.wait(timeout=3)
        except Exception:
            if proc.poll() is None:
                proc.terminate()

async def run_ws_stream(symbol="BTCUSDT", capital=10000.0, max_ticks=None):
    bin_path = os.path.join(os.path.dirname(__file__), "..", "bin", "live_trader")
    if not os.path.exists(bin_path):
        subprocess.run(["make", "-C", os.path.join(os.path.dirname(__file__), ".."), "live"], check=True)

    proc = subprocess.Popen([bin_path, symbol, str(capital)], stdin=subprocess.PIPE, text=True, bufsize=1)
    ws_url = f"wss://stream.binance.com:9443/ws/{symbol.lower()}@trade"
    ticks_sent = 0

    reconnect_attempts = 0
    max_reconnects = 10
    base_reconnect_delay = 1.0

    try:
        while proc.poll() is None:
            if max_ticks is not None and ticks_sent >= max_ticks:
                break
            try:
                print(f"[STREAM] Connecting live WebSocket (Attempt {reconnect_attempts + 1}): {ws_url}")
                async with websockets.connect(ws_url, ping_interval=20, ping_timeout=20) as ws:
                    reconnect_attempts = 0  # Reset upon successful connection
                    while proc.poll() is None:
                        msg = await ws.recv()
                        data = json.loads(msg)
                        ts = data['T']
                        p = float(data['p'])
                        q = float(data['q'])
                        side = "SELL" if data['m'] else "BUY"
                        proc.stdin.write(f"{ts} {p} {q} {side}\n")
                        proc.stdin.flush()
                        ticks_sent += 1
                        if max_ticks is not None and ticks_sent >= max_ticks:
                            print(f"\n[STREAM] Completed target stream of {max_ticks} live market ticks.")
                            return
            except (asyncio.CancelledError, KeyboardInterrupt):
                print("\nStopping live paper trader...")
                break
            except Exception as e:
                reconnect_attempts += 1
                if reconnect_attempts > max_reconnects:
                    print(f"[WARN] WebSocket reconnect limit ({max_reconnects}) reached. Falling back to REST stream: {e}")
                    break
                delay = min(30.0, base_reconnect_delay * (2 ** (reconnect_attempts - 1)))
                print(f"[WARN] WebSocket disconnected ({e}). Reconnecting in {delay:.1f}s (Attempt {reconnect_attempts}/{max_reconnects})...")
                await asyncio.sleep(delay)
    finally:
        if proc.stdin and not proc.stdin.closed:
            try:
                proc.stdin.close()
            except Exception:
                pass
        try:
            proc.wait(timeout=3)
        except Exception:
            if proc.poll() is None:
                proc.terminate()

if __name__ == "__main__":
    symbol = sys.argv[1] if len(sys.argv) > 1 else "BTCUSDT"
    capital = float(sys.argv[2]) if len(sys.argv) > 2 else 10000.0
    max_ticks = int(sys.argv[3]) if len(sys.argv) > 3 else None

    if HAS_WEBSOCKETS:
        try:
            asyncio.run(run_ws_stream(symbol, capital, max_ticks=max_ticks))
        except Exception:
            run_sync_stream(symbol, capital, max_ticks=max_ticks)
    else:
        run_sync_stream(symbol, capital, max_ticks=max_ticks)
