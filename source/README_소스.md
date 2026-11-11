# 통합패치 소스

삼국지14 CE 통합패치를 만드는 소스 전부입니다.

- `src/` — 게임에 들어가는 DLL 소스(C/C++)
- `installer101/` — 설치기(C#, PC에서 .NET Framework csc 로 컴파일)
- `test/` — 오프라인 시험(`test/run_offline_all.sh`)
- `교훈_점검/` — 작업 기록·관문 검사
- `build_all.sh` — DLL 빌드, `build_overlay_r43.py` — 배포 꾸러미, `make_release_r43.py` — 업데이트 파일

게임 원본 자료와 빌드 결과물(DLL·EXE)은 넣지 않았습니다.
