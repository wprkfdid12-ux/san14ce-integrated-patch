#!/bin/bash
# R40: build_out 의 새 모듈을 payload 로 (build_all.sh 뒤, build_pkg_r40.py 앞)
set -e; R=/home/claude/r41; B=$R/build_out; P=$R/payload
cp $B/SAN14_ModSettings.dll $P/base/
cp $B/SAN14_ICGuard.dll $P/base/
printf '; R40 1번 통합편의 장기 안정(ICGuard): 통합편의 본체 파일은 그대로 두고 실행 중에 덧댐(자동 탐색·창 고착·문전박대·처단불가·22번·26번)\r\n[Mod]\r\nId=SAN14CE.ICGuard\r\nFile=SAN14_ICGuard.dll\r\nEnabled=1\r\nCoreABI=65536\r\n' > $P/base/SAN14Mods/modules/ICGuard.ini
cp $B/SAN14_HiredFace.dll $P/f32/
cp $B/SAN14_StatusColor.dll $P/f29/
cp $B/SAN14_Range.dll $P/f33/
cp $B/SAN14_RegionOfficer.dll $P/f34/
cp /home/claude/r40ai/draggive/out/SAN14_DragGive.dll $P/f30/   # R40 AI 호환(83c6f86b)
cp $B/SAN14_ItemPicture.dll $P/f30/   # R41e A8 명품 목록 그림
printf '; R41e 30번 명품 목록 그림: 명품 창 선택 목록 왼쪽 그림 칸에 보유 무장 얼굴 대신 명품 그림\r\n[Mod]\r\nId=SAN14CE.ItemPicture\r\nFile=SAN14_ItemPicture.dll\r\nEnabled=1\r\nCoreABI=65536\r\n[ItemPicture]\r\nEnabled=1\r\n' > $P/f30/SAN14Mods/modules/ItemPicture.ini
mkdir -p $P/f04/SAN14Mods/modules
cp $B/SAN14_SceneSkip.dll $P/f04/   # R42 B1~B3 4번 연출 생략(재해·등용 결과·보물 발견)
printf '; R42 4번 연출 생략: 재해(역병·메뚜기·풍작) 연출·등용 결과 대사 장면·탐색 명품 발견 창. 결과는 원작 그대로\r\n[Mod]\r\nId=SAN14CE.SceneSkip\r\nFile=SAN14_SceneSkip.dll\r\nEnabled=1\r\nCoreABI=65536\r\n' > $P/f04/SAN14Mods/modules/SceneSkip.ini
mkdir -p $P/f26/SAN14Mods/modules
cp $B/SAN14_ExecuteLock.dll $P/f26/   # R42 B4
printf '; R42 26번 처단불가가 켜져 있으면 포로 처우 창·포로 회유 창의 처단 버튼을 회색으로\r\n[Mod]\r\nId=SAN14CE.ExecuteLock\r\nFile=SAN14_ExecuteLock.dll\r\nEnabled=1\r\nCoreABI=65536\r\n[ExecuteLock]\r\nEnabled=1\r\n' > $P/f26/SAN14Mods/modules/ExecuteLock.ini
cd $P && find . -name '*.dll' | sort | xargs sha256sum | cut -c1-16,65-
