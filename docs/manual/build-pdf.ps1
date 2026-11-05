# Renders guide.html to "docs\StickBend User Guide.pdf" with Edge (already on every Windows 10/11 PC).
# Run from anywhere:  powershell -ExecutionPolicy Bypass -File docs\manual\build-pdf.ps1
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$html = Join-Path $here 'guide.html'
$pdf  = Join-Path (Split-Path -Parent $here) 'StickBend User Guide.pdf'
$edge = "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
# Fresh profile each run: a leftover headless Edge on a reused profile silently skips the print.
$prof = Join-Path $env:TEMP ("stickbend-pdf-" + [guid]::NewGuid())

if (Test-Path $pdf) { Remove-Item $pdf }
$uri = 'file:///' + ($html -replace '\\', '/')
$p = Start-Process $edge -PassThru -Wait -ArgumentList @('--headless=new', '--disable-gpu', '--no-pdf-header-footer',
    "--user-data-dir=`"$prof`"", '--allow-file-access-from-files', "--print-to-pdf=`"$pdf`"", "`"$uri`"")
Remove-Item -Recurse -Force $prof -ErrorAction SilentlyContinue
if (-not (Test-Path $pdf)) { throw "PDF was not written" }
"Wrote $pdf"
