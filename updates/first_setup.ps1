# 통합패치 처음 설치(깃허브에서 받은 분용): updates 폴더의 기본판 조각 + 최신판 파일을 풀어 '통합패치' 폴더를 만들고 설치기를 연다.
# R43.31(사용자 11-11): 폴더 이름에 판 번호를 붙이지 않음 — 이후 업데이트는 이 폴더에 그대로 덮어씀.
# 모든 파일은 SHA-256 으로 확인한다. 게임 폴더는 여기서 건드리지 않는다(설치는 설치기에서).
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
try {
    $root = Split-Path -Parent $here
    $idxPath = Join-Path $here 'index.json'
    if (-not (Test-Path -LiteralPath $idxPath)) { throw ('판 목록이 없습니다: ' + $idxPath + "`r`nZIP 전체를 풀었는지 확인해 주세요.") }
    $idx = Get-Content -LiteralPath $idxPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $rel = @($idx.releases)[0]
    if ($null -eq $rel) { throw '판 목록에 판이 없습니다.' }
    function Check-Zip($file, $sha) {
        $p = Join-Path $root ($file -replace '/', '\')
        if (-not (Test-Path -LiteralPath $p)) { throw ('파일이 없습니다: ' + $p + "`r`n저장소 ZIP 을 끝까지 받았는지, 압축을 전부 풀었는지 확인해 주세요.") }
        $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($h -ne $sha) { throw ('파일이 손상됐습니다(지문 다름): ' + $p + "`r`n저장소 ZIP 을 다시 받아 주세요.") }
        return $p
    }
    $dest = Join-Path $root '통합패치'
    if (Test-Path -LiteralPath $dest) { $dest = $dest + ' (' + (Get-Date -Format 'yyyyMMdd_HHmmss') + ')' }
    New-Item -ItemType Directory -Path $dest | Out-Null
    Write-Host ('통합패치 ' + $rel.tag + ' 를 이 폴더에 만듭니다: ' + $dest)
    $parts = @($idx.base)
    $i = 0
    foreach ($b in $parts) {
        $i++
        Write-Host ('기본판 푸는 중 ' + $i + '/' + $parts.Count + ' ...')
        $p = Check-Zip $b.file $b.sha256
        Expand-Archive -LiteralPath $p -DestinationPath $dest -Force
    }
    Write-Host ('최신판(' + $rel.tag + ') 푸는 중 ...')
    $p = Check-Zip $rel.file $rel.sha256
    Expand-Archive -LiteralPath $p -DestinationPath $dest -Force
    Write-Host '파일 확인 중 ...'
    $bad = @()
    foreach ($line in [System.IO.File]::ReadAllLines((Join-Path $dest 'SHA256SUMS.txt'))) {
        if ($line.Length -lt 66) { continue }
        $sha = $line.Substring(0, 64).ToLowerInvariant(); $f = $line.Substring(64).Trim()
        $fp = Join-Path $dest ($f -replace '/', '\')
        if (-not (Test-Path -LiteralPath $fp) -or (Get-FileHash -LiteralPath $fp -Algorithm SHA256).Hash.ToLowerInvariant() -ne $sha) { $bad += $f }
    }
    if ($bad.Count -gt 0) { throw ('풀린 파일 ' + $bad.Count + '개가 목록과 다릅니다(예: ' + $bad[0] + '). 저장소 ZIP 을 다시 받아 주세요.') }
    Write-Host '완료. 설치기를 엽니다.'
    Start-Process -FilePath (Join-Path $dest '설치.cmd') -WorkingDirectory $dest
    exit 0
} catch {
    $msg = "통합패치 처음 설치를 끝내지 못했습니다. 게임 파일은 바꾸지 않았습니다.`r`n`r`n" + $_.Exception.Message + "`r`n`r`n이 글자를 그대로 복사해 댓글로 알려 주시면 원인을 확인합니다."
    Write-Host $msg
    try { Add-Type -AssemblyName System.Windows.Forms | Out-Null; [System.Windows.Forms.MessageBox]::Show($msg, '통합패치 - 처음 설치 실패') | Out-Null } catch {}
    exit 1
}
