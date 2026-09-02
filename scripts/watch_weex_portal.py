#!/usr/bin/env python3
"""
WEEX AI Wars II ($200,000 USDT) Portal Watcher Daemon
Monitors https://dorahacks.io/hackathon/weex-ai-wars2.
When the portal unlocks on September 2, 2026, it triggers submit_weex_hackathon.py.
"""

import subprocess
import datetime
import json
import time
import os

LOG_FILE = os.path.expanduser("~/.gemini/weex_watcher.log")
HACKATHON_URL = "https://dorahacks.io/hackathon/weex-ai-wars2"

def log(msg: str):
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{timestamp}] {msg}"
    print(line)
    with open(LOG_FILE, "a") as f:
        f.write(line + "\n")

def check_portal():
    js = """
    (function() {
        const text = document.body.innerText;
        const isUpcoming = text.includes("Submission starts in");
        const buttons = Array.from(document.querySelectorAll("button, a"))
            .map(b => b.innerText.trim());
        const hasSubmitBtn = buttons.some(t => t.toLowerCase().includes("submit buidl") || t.toLowerCase().includes("apply"));
        const isLive = hasSubmitBtn && !isUpcoming;
        return JSON.stringify({ isLive, isUpcoming, hasSubmitBtn, title: document.title });
    })()
    """
    cmd = f'''
    tell application "Brave Browser"
        set URL of active tab of front window to "{HACKATHON_URL}"
        delay 4
        set res to execute active tab of front window javascript {json.dumps(js)}
        return res
    end tell
    '''
    try:
        res = subprocess.check_output(["osascript", "-e", cmd]).decode("utf-8").strip()
        return json.loads(res)
    except Exception as e:
        return {"error": str(e)}

import sys

def main():
    loop_mode = "--loop" in sys.argv or "-l" in sys.argv
    log("[EXECUTE] WEEX AI Wars II Portal Watcher Daemon initialized.")
    log(f"Monitoring: {HACKATHON_URL} (Loop mode: {loop_mode})")

    while True:
        status = check_portal()
        log(f"Portal Status Check: {json.dumps(status)}")

        if status.get("isLive"):
            log("[COMPLETE] Submission Portal is LIVE and UNLOCKED! Triggering submission runner...")
            cmd = ["python3", os.path.expanduser("~/alpha-engine/scripts/submit_weex_hackathon.py")]
            subprocess.run(cmd)
            if loop_mode:
                log("[COMPLETE] Submission dispatched. Exiting daemon.")
                break
        else:
            log("[PENDING] Portal in Pre-Registration mode. Staged for September 2 unlock.")

        if not loop_mode:
            break

        # Adaptive sleep: sleep 300s (5m) overnight, then 30s as we approach 11:00 AM IST
        now = datetime.datetime.now()
        if now.day == 2 and now.hour >= 10:
            time.sleep(30)
        else:
            time.sleep(300)

if __name__ == "__main__":
    main()
