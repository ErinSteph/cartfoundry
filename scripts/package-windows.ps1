param([string]$BuildDir = "build", [string]$Configuration = "Release", [ValidateSet("x86_64", "arm64")][string]$Architecture = "x86_64")
$ErrorActionPreference = "Stop"
Set-Location "$PSScriptRoot/.."
cmake --build $BuildDir --config $Configuration --target CartFoundry cf-build cf-update-helper
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
$Exe = Join-Path $BuildDir "$Configuration/CartFoundry.exe"
if (!(Test-Path $Exe)) { $Exe = Join-Path $BuildDir "CartFoundry.exe" }
if (Test-Path "dist/CartFoundry") { Remove-Item -Recurse -Force "dist/CartFoundry" }
New-Item -ItemType Directory -Force "dist/CartFoundry/resources/tools" | Out-Null
Copy-Item $Exe "dist/CartFoundry/CartFoundry.exe" -Force
foreach ($Tool in @("cf-build", "cf-update-helper")) {
    $ToolExe = Join-Path $BuildDir "$Configuration/$Tool.exe"
    if (!(Test-Path $ToolExe)) { $ToolExe = Join-Path $BuildDir "$Tool.exe" }
    Copy-Item $ToolExe "dist/CartFoundry/$Tool.exe" -Force
}
Copy-Item -Recurse -Force runtime, images, examples, docs "dist/CartFoundry/resources/"
New-Item -ItemType Directory -Force "dist/CartFoundry/resources/docs/versions" | Out-Null
Copy-Item -Recurse -Force "$BuildDir/documentation/*" "dist/CartFoundry/resources/docs/versions/"
Copy-Item -Recurse -Force tools/gbdk "dist/CartFoundry/resources/tools/"
Copy-Item -Recurse -Force licenses "dist/CartFoundry/resources/"
tar -czf "dist/CartFoundry/resources/licenses/mgba/source-0.10.5.tar.gz" -C third_party mgba
if ($LASTEXITCODE -ne 0) { throw "mGBA source archive failed" }
Copy-Item THIRD_PARTY.md "dist/CartFoundry/resources/"
windeployqt --release --no-translations "dist/CartFoundry/CartFoundry.exe"
if ($LASTEXITCODE -ne 0) { throw "Qt deployment failed" }
Write-Output "Packaged: dist/CartFoundry"

python scripts/make-update.py stamp --root dist/CartFoundry --platform "windows-$Architecture"
if ($LASTEXITCODE -ne 0) { throw "Metadata generation failed" }
python scripts/make-update.py archive --root dist/CartFoundry
if ($LASTEXITCODE -ne 0) { throw "Update archive generation failed" }
