param(
  [string]$Pattern = "chunk2_batch_*.json",
  [string]$SectionName = "chunk2",
  [string]$File = "src\game_service.cpp"
)
$cmds = @()
Get-ChildItem (Join-Path 'analysis\gen' $Pattern) | ForEach-Object {
  $cmds += (Get-Content -Raw $_.FullName | ConvertFrom-Json)
}
$ok = $cmds | Where-Object { $_.status -eq 'ok' }
$skip = $cmds | Where-Object { $_.status -ne 'ok' }
Write-Host ("parsed=$($cmds.Count) ok=$($ok.Count) skip=$($skip.Count)")

$h = @(); $h += "// ---- game commands (type=1) generated handlers, $SectionName ----"
foreach ($c in $ok) { $h += ''; $h += $c.handlerCode }
$k = @($ok | ForEach-Object { $_.kcmdEntry.TrimEnd(',') + ',' })
$d = @($ok | ForEach-Object {
  "    case 0x" + ('{0:X}' -f $_.cmdNo) + ": " + $_.handlerName + "(cl, body, len); break;"
})

$src = Get-Content -Raw $File

# kCmd entries before the }; that precedes kCmdCount
$marker = "constexpr size_t kCmdCount"
$mi = $src.IndexOf($marker)
$closeIdx = $src.LastIndexOf("};", $mi)
if ($closeIdx -lt 0) { Write-Host "K-ANCHOR FAIL"; exit 1 }
$src = $src.Substring(0, $closeIdx) + ($k -join "`n") + "`n" + $src.Substring($closeIdx)

# handlers before Dispatcher comment
$anchorH = "// Dispatcher: run the C handler for a parsed packet."
if (-not $src.Contains($anchorH)) { Write-Host "H-ANCHOR FAIL"; exit 1 }
$src = $src.Replace($anchorH, ($h -join "`n") + "`n`n" + $anchorH)

# dispatch before default: after the msg_get_counts case
$anchorD = "    case 0x4402: handleMsgGetCounts(cl, body, len); break;`n    default:"
if (-not $src.Contains("case 0x4402: handleMsgGetCounts")) { Write-Host "D-ANCHOR FAIL"; exit 1 }
$src = $src.Replace($anchorD, "    case 0x4402: handleMsgGetCounts(cl, body, len); break;`n" + ($d -join "`n") + "`n    default:")

[System.IO.File]::WriteAllText($File, $src, [System.Text.UTF8Encoding]::new($false))
Write-Host "integrated $SectionName. file len=$($src.Length)"
