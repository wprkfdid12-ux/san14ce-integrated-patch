#!/bin/bash
# AI 모드 v15.14 통합패치 호환판 시험: ① 늦게 걸기(.aicfix)를 진짜 DLL 코드로 10가지 경우 ② 결과물 바이트·강제 끔·표·설치 관리자 항목 검사 ③ 설치 관리자 IL 전부 검사
#  결과물(ai_compat/v1514/out)이나 원본 꾸러미(pkg)가 없으면 건너뜀(정직하게 '건너뜀').
set -e
R=$(cd "$(dirname "$0")/../.." && pwd);A=$R/ai_compat/v1514
[ -f "$A/out/SAN14_CombatAI.dll" ] && [ -f "$A/pkg/AI_TacticsMod/build/input_package/SAN14_CombatAI.dll" ] || { echo "건너뜀: 호환판 결과물/원본 꾸러미 없음"; exit 0; }
T=$(mktemp -d);gcc -O1 -o "$T/sim" "$R/test/ai_defer/sim1514.c"
for s in ic noic ictimeout lateic rollback changed routechanged nothread raw noroute; do "$T/sim" "$A/out/SAN14_CombatAI.dll" $s; done
cd "$A" && python3 -I tools/check1514.py
cd "$R/ai_compat/v159" && python3 -I il_check.py "$A/pkg/AI모드_설치관리자.exe" "$A/out/AI모드_설치관리자.exe"
