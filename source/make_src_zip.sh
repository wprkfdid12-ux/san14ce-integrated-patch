#!/bin/bash
# 소스 보관 ZIP: bash make_src_zip.sh R43 → /mnt/user-data/outputs/SAN14CE_통합패치_소스_<판>_1of1.zip (overlay_*·zip·임시 제외)
set -e; TAG=$1; [ -n "$TAG" ] || { echo "판 이름 필요"; exit 1; }
OUT=/mnt/user-data/outputs/SAN14CE_통합패치_소스_${TAG}_1of1.zip; rm -f "$OUT"
cd /home/claude && zip -qr "$OUT" r41 -x 'r41/overlay_*' 'r41/*.zip' 'r41/**/*.zip' 'r41/build_out/*.o' 'r41/build_tc/lib/*.bak*' 'r41/payload_r43/**/*.dll' 'r41/final_r43/*' 'r41/ai_compat/v1514/pkg/*' 'r41/ai_compat/v1514/dist/*' 'r41/ai_compat/v1514/work/*' 'r41/ai_compat/v1514/out/*.dll' 'r41/ai_compat/v1514/out/*.exe' 'r41/release/*'
sha256sum "$OUT"; python3 -c "import zipfile,sys;print(len(zipfile.ZipFile(sys.argv[1]).namelist()),'files')" "$OUT"
