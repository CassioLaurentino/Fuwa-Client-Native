#Requires -Version 5.1
$ErrorActionPreference = "Stop"

# Builda o Fuwa-Client-Native via Docker (container de build, nao runtime -
# ver docker/Dockerfile) e extrai o executavel para ./dist.
Set-Location (Join-Path $PSScriptRoot "..")

docker build --target export -o ./dist -f docker/Dockerfile .

Write-Host "Binario exportado em ./dist"
