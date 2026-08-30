#!/usr/bin/env python3
"""
WEEX AI Wars II ($200,000 USDT) Automated Submission Runner
Executes via local Brave Browser automation on September 2, 2026 when submission portal unlocks.
"""

import subprocess
import json
import time

HACKATHON_URL = "https://dorahacks.io/hackathon/weex-ai-wars-2"

def run_brave_js(js_code: str) -> str:
    cmd = f'tell application "Brave Browser"\nexecute active tab of front window javascript {json.dumps(js_code)}\nend tell'
    res = subprocess.check_output(["osascript", "-e", cmd]).decode("utf-8").strip()
    return res

def main():
    print("🚀 Launching WEEX AI Wars II Automated Submission Engine...")
    subprocess.run(["open", "-a", "Brave Browser", HACKATHON_URL])
    time.sleep(4)

    print("Checking hackathon portal state...")
    status = run_brave_js("""
    (function() {
        return JSON.stringify({
            url: window.location.href,
            title: document.title,
            hasSubmit: Array.from(document.querySelectorAll("button")).some(b => b.innerText.includes("Submit"))
        });
    })()
    """)
    print("Portal status:", status)

if __name__ == "__main__":
    main()
