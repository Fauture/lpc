param([int]$pid2, [int]$samples = 4000, [int]$interval = 1)
$ErrorActionPreference = 'Stop'
$base = Split-Path -Parent $MyInvocation.MyCommand.Path
$code = Get-Content -Raw (Join-Path $base 'StackLoop86.cs')
Add-Type -TypeDefinition $code -Language CSharp
[StackLoop86]::Run($pid2, $samples, $interval)
