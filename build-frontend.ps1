#! /usr/bin/env pwsh
Push-Location $PSScriptRoot
Set-Location frontend
trunk build --release
Copy-Item -Recurse dist/* ../server/data/public
Pop-Location
