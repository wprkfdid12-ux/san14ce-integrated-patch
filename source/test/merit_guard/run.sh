#!/usr/bin/env bash
# 13번 함수 앞부분 검사 흉내 — 실제 merit_runtime.c 조각을 잘라 컴파일해 돌림
set -e;D=$(cd "$(dirname "$0")";pwd)
python3 - "$D" <<'P'
import sys;s=open(sys.argv[1]+'/../../mt/src/merit_runtime.c',encoding='utf-8').read()
a=s.index('static const char*const kFuncName');b=s.index('__declspec(dllexport) int SAN14_ModInitV1');open(sys.argv[1]+'/guard_part.inc','w').write(s[a:b])
P
clang -O1 -w -o /tmp/merit_guard_$$ "$D/test_merit_guard.c" && /tmp/merit_guard_$$;r=$?;rm -f /tmp/merit_guard_$$;exit $r
