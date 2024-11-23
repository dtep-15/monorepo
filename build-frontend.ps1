#! /usr/bin/env pwsh
Push-Location $PSScriptRoot
Set-Location frontend
trunk build --release
Pop-Location