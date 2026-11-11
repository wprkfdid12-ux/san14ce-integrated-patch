R13 source and reproducible tests. Read CONTINUE_PROMPT_KO.txt and 검증결과.md.
Build: bash source/build.sh
Stage: python tools/package_r13.py --stage
Test: python tests/run_r13.py --with-package
Prior R12 and earlier audit/tests are historical, not a current release gate.
