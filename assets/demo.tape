# VHS Tape for AlphaEngine Demo
Output "assets/alpha_engine_demo.gif"
Output "assets/alpha_engine_demo.mp4"

Set FontSize 14
Set Width 1200
Set Height 700
Set Padding 20
Set Theme "TokyoNight"
Set TypingSpeed 25ms

Type "make clean && make test && make all"
Enter
Sleep 4s

Type "./bin/alpha_engine data/real_sol_ticks.bin"
Enter
Sleep 3s

Type "python3 scripts/audit_safety_invariants.py"
Enter
Sleep 3s

Sleep 2s
