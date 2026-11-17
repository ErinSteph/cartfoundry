param([string]$BuildDir = "build", [string]$Configuration = "Release")
$ErrorActionPreference = "Stop"
Set-Location "$PSScriptRoot/.."
cmake --build $BuildDir --config $Configuration --target CartFoundry
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
$Exe = Join-Path $BuildDir "$Configuration/CartFoundry.exe"
if (!(Test-Path $Exe)) { $Exe = Join-Path $BuildDir "CartFoundry.exe" }
New-Item -ItemType Directory -Force "dist/CartFoundry/resources/tools" | Out-Null
Copy-Item $Exe "dist/CartFoundry/CartFoundry.exe" -Force
Copy-Item -Recurse -Force runtime, images, examples, docs "dist/CartFoundry/resources/"
Copy-Item -Recurse -Force tools/gbdk "dist/CartFoundry/resources/tools/"
Copy-Item -Recurse -Force licenses "dist/CartFoundry/resources/"
tar -czf "dist/CartFoundry/resources/licenses/mgba/source-0.10.5.tar.gz" -C third_party mgba
Copy-Item THIRD_PARTY.md "dist/CartFoundry/resources/"
windeployqt --release --no-translations "dist/CartFoundry/CartFoundry.exe"
if ($LASTEXITCODE -ne 0) { throw "Qt deployment failed" }
Write-Output "Packaged: dist/CartFoundry"
