#!/bin/bash
# 32번 고용무장 얼굴: 최적화 전 소스(인자 1)와 지금 소스(src/hired_face.cpp)의 얼굴 결과가 400단계 모두 같은지
set -e
cd "$(dirname "$0")/.."
OLD=${1:?최적화 전 hired_face.cpp 경로}
clang++ -std=c++17 -O1 -w -I test/shim -I src -DHF_SRC="\"$OLD\"" test/test_hired_face_native.cpp -o /tmp/hf_old
clang++ -std=c++17 -O1 -w -I test/shim -I src -DHF_NEW -DHF_SRC="\"$(pwd)/src/hired_face.cpp\"" test/test_hired_face_native.cpp -o /tmp/hf_new
/tmp/hf_old > /tmp/hf_old.txt; /tmp/hf_new > /tmp/hf_new.txt
if cmp -s /tmp/hf_old.txt /tmp/hf_new.txt; then echo "같음: $(tail -1 /tmp/hf_new.txt) (400단계 얼굴 전부 동일)"; else echo "다름:"; diff /tmp/hf_old.txt /tmp/hf_new.txt | head; exit 1; fi
