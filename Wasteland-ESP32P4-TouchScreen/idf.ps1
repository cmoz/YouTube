param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$IdfArgs
)

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$idfRoot = 'C:\Espressif\v5.5.3'

Set-Location $projectRoot
$env:IDF_PATH = $idfRoot
. "$idfRoot\export.ps1"

if (-not $IdfArgs -or $IdfArgs.Count -eq 0) {
    idf.py build
} else {
    idf.py @IdfArgs
}