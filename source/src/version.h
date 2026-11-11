#pragma once
// 통합패치 판·기준 게임 판(한 곳에서만 고친다. 게임 업데이트 호환 때 upd_tools/apply_new.py 가 이 파일을 바꾼다)
#define S14_PATCH_VER  "R43"
#define S14_PATCH_VERW L"R43"
// 기준 게임 실행 파일(SAN14CE_KO.exe) PE 머리: 빌드 시각(TimeDateStamp)·이미지 크기(SizeOfImage)
// 옛 판 1.0.1: 0x6a983c25 / 0x29ff000 (2026-09-02 빌드) → R43 부터 2026-11-05 업데이트판 0x6ad06f53 / 0x2a02000
#define S14_GAME_STAMP 0x6ad06f53u
#define S14_GAME_IMAGE 0x2a02000u
#define S14_GAME_NAME  "2026-11-05 update(build 2026-10-15)"
