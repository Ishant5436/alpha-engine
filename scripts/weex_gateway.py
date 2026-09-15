#!/usr/bin/env python3
"""
WEEX AI Wars II Live Execution Gateway (alpha-engine)
Bridge between WEEX V3 Contract API and compiled C++20 AlphaEngine live_trader:
- Streams live public contract trade ticks (WebSocket: wss://ws-contract.weex.com/v3/ws/public)
- Pipes normalized ticks (<ts_ms> <price> <qty> <side>) to live_trader stdin
- Intercepts generated ORDER execution signals from live_trader stdout
- Signs and submits limit/market orders to WEEX V3 Private REST (POST /capi/v3/order)
- Enforces token-bucket rate limits (<= 40 reqs/min) and default paper-trading dry-run mode
"""
import argparse
import asyncio
import base64
import hashlib
import hmac
import json
import logging
import os
import sys
import time
import urllib.error
import urllib.request
from typing import Any, Dict, Optional

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
logger = logging.getLogger("weex_gateway")


class WeexSigner:
    """HMAC-SHA256 Base64 Authenticator for WEEX V3 API."""

    def __init__(
        self,
        api_key: Optional[str] = None,
        secret_key: Optional[str] = None,
        passphrase: Optional[str] = None
    ) -> None:
        self.api_key = api_key or os.environ.get("WEEX_API_KEY", "")
        self._secret_key = secret_key or os.environ.get("WEEX_SECRET_KEY", "")
        self._passphrase = passphrase or os.environ.get("WEEX_PASSPHRASE", "")

    def generate_signature(
        self,
        timestamp: str,
        method: str,
        request_path: str,
        query_string: str = "",
        body: str = ""
    ) -> str:
        """Calculate WEEX V3 signature: Base64(HMAC-SHA256(secret, ts + METHOD + path + [?query] + body))."""
        method_str = method.upper()
        if query_string:
            payload = f"{timestamp}{method_str}{request_path}?{query_string}{body}"
        else:
            payload = f"{timestamp}{method_str}{request_path}{body}"

        hmac_digest = hmac.new(
            self._secret_key.encode("utf-8"),
            payload.encode("utf-8"),
            hashlib.sha256
        ).digest()
        return base64.b64encode(hmac_digest).decode("utf-8")

    def get_headers(
        self,
        timestamp: str,
        method: str,
        request_path: str,
        query_string: str = "",
        body: str = ""
    ) -> Dict[str, str]:
        """Generate full authenticated HTTP headers."""
        signature = self.generate_signature(
            timestamp=timestamp,
            method=method,
            request_path=request_path,
            query_string=query_string,
            body=body
        )
        return {
            "ACCESS-KEY": self.api_key,
            "ACCESS-SIGN": signature,
            "ACCESS-TIMESTAMP": timestamp,
            "ACCESS-PASSPHRASE": self._passphrase,
            "Content-Type": "application/json"
        }

    def __repr__(self) -> str:
        masked_key = self.api_key[:4] + "***" if len(self.api_key) > 4 else "***"
        return f"<WeexSigner api_key='{masked_key}' secret='REDACTED' passphrase='***'>"

    def __str__(self) -> str:
        return self.__repr__()


class TokenBucketLimiter:
    """Non-blocking async Token Bucket rate limiter adhering to exchange API limits."""

    def __init__(self, capacity: int = 40, refill_rate_per_sec: float = 40.0 / 60.0) -> None:
        self.capacity = float(capacity)
        self.refill_rate = float(refill_rate_per_sec)
        self._tokens = float(capacity)
        self._last_refill = time.monotonic()
        self._lock = asyncio.Lock()

    def _refill(self) -> None:
        now = time.monotonic()
        elapsed = now - self._last_refill
        self._tokens = min(self.capacity, self._tokens + elapsed * self.refill_rate)
        self._last_refill = now

    def try_acquire(self, tokens: float = 1.0) -> bool:
        """Attempt immediate non-blocking token acquisition."""
        self._refill()
        if self._tokens >= tokens:
            self._tokens -= tokens
            return True
        return False

    async def acquire(self, tokens: float = 1.0) -> None:
        """Asynchronously wait until tokens become available."""
        while True:
            async with self._lock:
                self._refill()
                if self._tokens >= tokens:
                    self._tokens -= tokens
                    return
                deficit = tokens - self._tokens
                sleep_time = deficit / self.refill_rate
            await asyncio.sleep(min(1.0, max(0.01, sleep_time)))


def normalize_weex_trade(raw_msg: str) -> Optional[str]:
    """Parse WEEX V3 contract public trade message into space-delimited live_trader tick."""
    try:
        data = json.loads(raw_msg)
    except Exception:
        return None

    if not isinstance(data, dict):
        return None

    # Support both wrapped stream {"data": {...}} and flat payload
    trade_obj = data.get("data", data)
    if not isinstance(trade_obj, dict):
        return None

    # Ignore heartbeat/control messages
    if "method" in trade_obj or "event" in trade_obj:
        return None

    price_str = trade_obj.get("p")
    qty_str = trade_obj.get("q")
    ts = trade_obj.get("T") or trade_obj.get("E") or int(time.time() * 1000)
    is_maker_buyer = trade_obj.get("m")

    if price_str is None or qty_str is None:
        return None

    try:
        price = float(price_str)
        qty = float(qty_str)
        if price <= 0.0 or qty <= 0.0:
            return None
    except (ValueError, TypeError):
        return None

    # 'm' is True: Buyer was the maker -> Taker was a Seller -> SELL
    side = "SELL" if is_maker_buyer is True else "BUY"
    return f"{ts} {price:.2f} {qty:.4f} {side}"


def build_weex_order_payload(signal_line: str) -> Dict[str, Any]:
    """Convert C++ engine signal line (ORDER symbol side pos_side qty price conf) into WEEX payload."""
    parts = signal_line.strip().split()
    assert len(parts) >= 6, f"Malformed order signal line: {signal_line}"
    # Example: ORDER BTCUSDT BUY LONG 0.050000 60120.50 1.0000
    _, symbol, side, pos_side, qty_str, price_str = parts[:6]

    qty = float(qty_str)
    price = float(price_str)
    client_id = f"ALPHA-{int(time.time() * 1000)}-{os.urandom(2).hex()}"

    return {
        "symbol": symbol,
        "side": side,
        "type": "LIMIT",
        "quantity": f"{qty:.4f}",
        "price": f"{price:.2f}",
        "positionSide": pos_side,
        "timeInForce": "GTC",
        "newClientOrderId": client_id
    }


class WeexRestClient:
    """REST Client for WEEX Contract Futures execution."""

    def __init__(
        self,
        signer: Optional[WeexSigner] = None,
        base_url: str = "https://api-contract.weex.com",
        dry_run: bool = True,
        rate_limiter: Optional[TokenBucketLimiter] = None
    ) -> None:
        self.signer = signer or WeexSigner()
        self.base_url = base_url.rstrip("/")
        self.dry_run = dry_run
        self.rate_limiter = rate_limiter or TokenBucketLimiter()

    async def place_order(self, order_payload: Dict[str, Any]) -> Dict[str, Any]:
        """Dispatch limit order to WEEX V3 Contract API."""
        if self.dry_run:
            logger.info(
                "[DRY_RUN] Order Simulated: %s %s %s @ $%s (qty: %s)",
                order_payload.get("symbol"),
                order_payload.get("side"),
                order_payload.get("positionSide"),
                order_payload.get("price"),
                order_payload.get("quantity")
            )
            return {
                "status": "DRY_RUN_SIMULATED",
                "orderId": f"SIM-{int(time.time() * 1000)}",
                "symbol": order_payload.get("symbol"),
                "payload": order_payload
            }

        await self.rate_limiter.acquire()

        timestamp = str(int(time.time() * 1000))
        path = "/capi/v3/order"
        body_str = json.dumps(order_payload, separators=(",", ":"))
        headers = self.signer.get_headers(
            timestamp=timestamp,
            method="POST",
            request_path=path,
            body=body_str
        )

        url = f"{self.base_url}{path}"
        req = urllib.request.Request(
            url,
            data=body_str.encode("utf-8"),
            headers=headers,
            method="POST"
        )

        def _execute() -> Dict[str, Any]:
            try:
                with urllib.request.urlopen(req, timeout=5.0) as resp:
                    resp_body = resp.read().decode("utf-8")
                    return json.loads(resp_body)
            except urllib.error.HTTPError as e:
                err_msg = e.read().decode("utf-8")
                logger.error("WEEX HTTP %d error: %s", e.code, err_msg)
                return {"error": True, "status": e.code, "message": err_msg}
            except Exception as ex:
                logger.error("WEEX network error: %s", ex)
                return {"error": True, "message": str(ex)}

        return await asyncio.to_thread(_execute)


class WeexGatewayDaemon:
    """Orchestrates the live WebSocket connection, C++ engine process, and order execution."""

    def __init__(
        self,
        symbol: str = "BTCUSDT",
        capital: float = 10000.0,
        dry_run: bool = True,
        mock_stream: bool = False
    ) -> None:
        self.symbol = symbol
        self.capital = capital
        self.dry_run = dry_run
        self.mock_stream = mock_stream
        self.signer = WeexSigner()
        self.rest_client = WeexRestClient(signer=self.signer, dry_run=dry_run)
        self._proc: Optional[asyncio.subprocess.Process] = None

    async def _handle_engine_stdout(self) -> None:
        """Read and process structured signals from live_trader stdout."""
        assert self._proc is not None and self._proc.stdout is not None
        while True:
            raw_line = await self._proc.stdout.readline()
            if not raw_line:
                break
            line = raw_line.decode("utf-8").strip()
            if line.startswith("ORDER "):
                try:
                    payload = build_weex_order_payload(line)
                    await self.rest_client.place_order(payload)
                except Exception as e:
                    logger.error("Failed to process order signal '%s': %s", line, e)
            elif line.startswith("SUMMARY "):
                logger.info("Engine completed session: %s", line)

    async def _stream_mock_ticks(self) -> None:
        """Stream synthetic ticks for end-to-end testing without network dependencies."""
        assert self._proc is not None and self._proc.stdin is not None
        base_price = 60000.0
        ts = int(time.time() * 1000)
        logger.info("Streaming 25 mock ticks through live_trader pipe...")

        for i in range(25):
            # Create a synthetic breakout to trigger confidence threshold
            price = base_price + (i * 25.0)
            side = "BUY" if i % 2 == 0 else "SELL"
            tick_line = f"{ts + i * 1000} {price:.2f} 0.5000 {side}\n"
            self._proc.stdin.write(tick_line.encode("utf-8"))
            await self._proc.stdin.drain()
            await asyncio.sleep(0.02)

        self._proc.stdin.close()

    async def run(self) -> None:
        """Start subprocess and run market data loop."""
        bin_path = os.path.join(os.path.dirname(__file__), "..", "bin", "live_trader")
        if not os.path.exists(bin_path):
            raise FileNotFoundError(f"live_trader binary not found at {bin_path}. Run 'make all' first.")

        cmd = [bin_path, self.symbol, str(self.capital), "--pipe"]
        logger.info("Spawning AlphaEngine subprocess: %s", " ".join(cmd))

        self._proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )

        stdout_task = asyncio.create_task(self._handle_engine_stdout())

        if self.mock_stream:
            await self._stream_mock_ticks()
        else:
            logger.info("WEEX live WebSocket stream mode enabled for %s", self.symbol)
            # Live WebSocket connection loop
            import websockets
            ws_url = "wss://ws-contract.weex.com/v3/ws/public"
            sub_msg = json.dumps({
                "method": "SUBSCRIBE",
                "params": [f"{self.symbol.upper()}@trade"],
                "id": 1
            })

            try:
                async with websockets.connect(ws_url, ping_interval=20, ping_timeout=10) as ws:
                    await ws.send(sub_msg)
                    logger.info("Subscribed to WEEX %s@trade", self.symbol)
                    while True:
                        raw_msg = await ws.recv()
                        tick = normalize_weex_trade(str(raw_msg))
                        if tick and self._proc.stdin:
                            self._proc.stdin.write(f"{tick}\n".encode("utf-8"))
                            await self._proc.stdin.drain()
            except asyncio.CancelledError:
                pass
            except Exception as e:
                logger.error("WebSocket stream error: %s", e)
            finally:
                if self._proc.stdin:
                    self._proc.stdin.close()

        await self._proc.wait()
        await stdout_task
        logger.info("Gateway daemon stopped cleanly.")


def main() -> None:
    parser = argparse.ArgumentParser(description="WEEX Live Execution Gateway for AlphaEngine")
    parser.add_argument("--symbol", default="BTCUSDT", help="Trading symbol (e.g. BTCUSDT)")
    parser.add_argument("--capital", type=float, default=10000.0, help="Initial allocated capital in USD")
    parser.add_argument("--live", action="store_true", help="Enable REAL order execution (default: dry run)")
    parser.add_argument("--mock-stream", action="store_true", help="Stream mock ticks for offline verification")
    args = parser.parse_args()

    is_dry_run = not args.live
    if not is_dry_run and os.environ.get("WEEX_LIVE_CONFIRMED") != "1":
        logger.error("Live order execution requires WEEX_LIVE_CONFIRMED=1 environment variable guardrail.")
        sys.exit(1)

    daemon = WeexGatewayDaemon(
        symbol=args.symbol,
        capital=args.capital,
        dry_run=is_dry_run,
        mock_stream=args.mock_stream
    )
    asyncio.run(daemon.run())


if __name__ == "__main__":
    main()
