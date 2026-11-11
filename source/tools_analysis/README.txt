분석 도구 (R42g)
- stubdec.py <SAN14CE_KO.exe> <dec.exe> : SteamStub .text 복호
- objdump -d -M intel dec.exe > game.asm ; python3 fn.py (주소 색인) ; python3 show.py 1400b1aa0,0,80 (함수/주변 보기)
- watch.sh : 장시간 데모 점검(적용확인.log·MinimapPerf·CrashLog·runtime.log 등, 경로는 스테이징 위치 기준)
