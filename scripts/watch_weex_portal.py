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
        const buttons = Array.from(document.querySelectorAll("button, a"))
            .map(b => b.innerText.trim());
        const hasSubmit = buttons.some(t => t.toLowerCase().includes("submit buidl") || t.toLowerCase().includes("apply"));
        const isPreReg = buttons.some(t => t.toLowerCase().includes("pre-registration") || t.toLowerCase().includes("registered"));
        return JSON.stringify({ hasSubmit, isPreReg, buttonCount: buttons.length, title: document.title });
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

def main():
    log("🚀 WEEX AI Wars II Portal Watcher Daemon initialized.")
    log(f"Monitoring: {HACKATHON_URL}")
    
    # Run an initial check
    status = check_portal()
    log(f"Initial Portal Status: {json.dumps(status)}")
    
    if status.get("hasSubmit"):
        log("🎉 Submission Portal is UNLOCKED! Triggering submission runner...")
        subprocess.run(["python3", "/Users/ishantpanchal/alpha-engine/scripts/submit_weex_hackathon.py"])
    else:
        log("⏳ Portal in Pre-Registration mode. Staged for September 2 unlock.")

if __name__ == "__main__":
    main()
