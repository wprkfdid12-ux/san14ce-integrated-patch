#!/bin/bash
# 사용자 11-11: 통합패치 소스를 깃허브 저장소 source/ 폴더에 공개.
#   bash publish_source.sh [저장소 폴더]   → 저장소/source 를 지금 r41 로 통째로 바꿈(커밋·올리기는 따로)
# 빼는 것: 남의 AI 모드 호환 자료(ai_compat·aiinst — 개인용), 게임 원본에서 나온 자료(.s14·.rdb 등),
#          빌드 결과물·실행 파일(dll·exe·o·obj·a·pyc·리눅스 시험 실행 파일), 배포 꾸러미(overlay_*·final_r43·release·newpkg 꾸러미)
set -e
REPO=${1:-/home/claude/san14ce-integrated-patch}; R=/home/claude/r41
[ -d "$REPO/.git" ] || { echo "저장소 폴더가 아님: $REPO"; exit 1; }
rm -rf "$REPO/source"; mkdir -p "$REPO/source"
cd "$R"
find . -type f \
 -not -path './ai_compat/*' -not -path './aiinst/*' -not -path './newpkg/payload_*' -not -path './field_out_*' -not -path './build_out/*' \
 -not -path './overlay_*' -not -path './final_r43/*' -not -path './release/*' -not -path './payload_r43/*' -not -path '*/__pycache__/*' \
 -not -path './test/th_sim/th_sim' -not -path './test/ai_defer/sim' -not -path './test/ai_defer/sim1514' \
 -not -regex '.*\.\(dll\|exe\|o\|obj\|a\|pyd\|pyc\|bin\|rdb\|merged\|s14\|zip\|lib\|bak.*\)$' -print0 \
 | tar --null -cf - -T - | (cd "$REPO/source" && tar xf -)
# 남은 파일 중 바이너리가 있으면 멈춤(새로 생긴 실행 파일이 섞이지 않게)
BIN=$(cd "$REPO/source" && find . -type f -print0 | xargs -0 file -b --mime-encoding -- 2>/dev/null | grep -c '^binary$' || true)
[ "$BIN" = 0 ] || { echo "바이너리 $BIN 개 섞임 — 빼는 목록에 더할 것"; (cd "$REPO/source" && find . -type f -exec sh -c '[ "$(file -b --mime-encoding "$1")" = binary ] && echo "$1"' _ {} \;); exit 1; }
# 개인 정보 검사(전자우편)
if grep -rIl -E '[A-Za-z0-9._%+-]+@(naver|gmail|daum|hanmail|kakao)\.' "$REPO/source" >/dev/null; then echo "전자우편 주소가 섞임"; grep -rIl -E '[A-Za-z0-9._%+-]+@(naver|gmail|daum|hanmail|kakao)\.' "$REPO/source"; exit 1; fi
cat > "$REPO/source/README_소스.md" <<'EOF'
# 통합패치 소스

삼국지14 CE 통합패치를 만드는 소스 전부입니다.

- `src/` — 게임에 들어가는 DLL 소스(C/C++)
- `installer101/` — 설치기(C#, PC에서 .NET Framework csc 로 컴파일)
- `test/` — 오프라인 시험(`test/run_offline_all.sh`)
- `교훈_점검/` — 작업 기록·관문 검사
- `build_all.sh` — DLL 빌드, `build_overlay_r43.py` — 배포 꾸러미, `make_release_r43.py` — 업데이트 파일

게임 원본 자료와 빌드 결과물(DLL·EXE)은 넣지 않았습니다.
EOF
echo "source/ $(cd "$REPO/source" && find . -type f | wc -l) 개, $(du -sh "$REPO/source" | cut -f1)"
