$VERSION = "1.0.0"
$OUT_DIR = "release"
$PACKAGE = "undying-terminal-$VERSION-win64"

New-Item -ItemType Directory -Force -Path $OUT_DIR | Out-Null
New-Item -ItemType Directory -Force -Path "$OUT_DIR\$PACKAGE" | Out-Null

Copy-Item "build\undying-terminal.exe" "$OUT_DIR\$PACKAGE\"
Copy-Item "build\undying-terminal-terminal.exe" "$OUT_DIR\$PACKAGE\"
Copy-Item "build\undying-terminal-server.exe" "$OUT_DIR\$PACKAGE\"

Copy-Item "README.md" "$OUT_DIR\$PACKAGE\"
Copy-Item "LICENSE" "$OUT_DIR\$PACKAGE\"

Compress-Archive -Force -Path "$OUT_DIR\$PACKAGE\*" -DestinationPath "$OUT_DIR\$PACKAGE.zip"

Write-Host "Created: $OUT_DIR\$PACKAGE.zip"
Get-ChildItem "$OUT_DIR\$PACKAGE.zip" | Select-Object Name, Length
