SAN14CE MeritTraits R17 TurnTransitionPerf source
Base: delivered R16 Compatibility-TraitRules (audit/r16_release_baseline).

Build: MERIT_ZIG=/path/to/zig bash source/build.sh --main-only
Compiler: Zig 0.14.1, x86_64-windows-msvc, freestanding Win64.
Run: R17_PWSH=/path/to/pwsh python3 -u tests/run_r17.py --with-package
Benchmarks: tests/bench_r17_ai.py and tests/bench_r17_transitions.py.
Select R16 baseline with MERIT_BENCH_VERSION=R16 and MERIT_BENCH_DLL=<baseline DLL>.
AI benchmark also accepts MERIT_BENCH_OFFICERS=1 for a nonsaturated queue.
Package: python3 tools/package_r17.py --output <dir> --baseline <R16 source root>

Read 검증결과.md and audit/r17_suite_run.json. Historical suites retain their own
version assumptions: use the R17 suite, not every old package/performance gate.
Actual Win64 PE execution under Linux uses GCC/Python 3.11+ and substituted OS/
game boundaries. PowerShell filesystem fixtures stub Windows DLL loading only.
AI draws, rerolls, floating-point order, native hook addresses, supporting DLLs,
and save wire schema are unchanged from R16. Logs enqueue immutable snapshots.
No live Windows/DirectX/game/FPS claim. Do not overwrite arbitrary proxy DLLs.

R39 A1 (1.0.1 build, build_new.sh): merit_history.inc recruit_reset_* — recruited officer
(native 0x2717d0: rank 1/2, merit<0, counts 0) keeps native rank, removes only finalized
accepted gacha traits, compacts slots, wipes ledger+GState. User text: ../README_A1_항장.txt
Tests: /home/claude/r39/mt_test/tests/test_r39_recruit.py (R17 fixture, same merit_history.inc).
