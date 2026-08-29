$ErrorActionPreference = 'Stop'

function New-Handler([string]$fun, [string]$lpc) {
  $h = @"
void handle_$fun(GameClient& cl, const uint8_t* body, size_t len)
{
  size_t off = 0;
  if (off + 1 > len) return;
  uint8_t cmdLen = body[off++];
  if (off + cmdLen > len) return;
  const uint8_t* cmd = body + off; off += cmdLen;
  if (off + 2 > len) return;
  uint16_t cnt = rd16be(body, off); off += 2;
  auto arr = std::make_shared<ArrayValue>();
  for (uint16_t i = 0; i < cnt; ++i) {
    if (off + 2 > len) return;
    uint16_t slen = rd16be(body, off); off += 2;
    if (off + slen > len) return;
    arr->items.push_back(make_string(body + off, slen));
    off += slen;
  }
  applyLpc("$lpc", {make_int(cl.clientNo), make_string(cmd, cmdLen), arr});
}
"@
  return $h.TrimEnd("`r","`n")
}

$specs = @(
  @{ cmdNo = 8537;  fun = 'vm_msgArenaOper';     addr = '0x806ea1c'; lpc = 'msg_arena_oper' },
  @{ cmdNo = 11729; fun = 'vm_cmdDraftOper';    addr = '0x8070680'; lpc = 'cmd_draft_oper' },
  @{ cmdNo = 64819; fun = 'vm_msgDraftOper';    addr = '0x8070890'; lpc = 'msg_draft_oper' },
  @{ cmdNo = 9044;  fun = 'vm_cmdSDDHOper';     addr = '0x806ec2c'; lpc = 'cmd_sddh_oper' },
  @{ cmdNo = 16645; fun = 'vm_msgSDDHOper';     addr = '0x806ee3c'; lpc = 'msg_sddh_oper' },
  @{ cmdNo = 4442;  fun = 'vm_cmdCelebrityOper';addr = '0x806f04c'; lpc = 'cmd_celebrity_oper' },
  @{ cmdNo = 4367;  fun = 'vm_msgCelebrityOper';addr = '0x806f25c'; lpc = 'msg_celebrity_oper' },
  @{ cmdNo = 8596;  fun = 'vm_cmdSamsungOper';  addr = '0x806f46c'; lpc = 'cmd_samsung_oper' },
  @{ cmdNo = 243;   fun = 'vm_msgSamsungOper';  addr = '0x806f67c'; lpc = 'msg_samsung_oper' }
)

$r = @()
foreach ($s in $specs) {
  $h = New-Handler $s.fun $s.lpc
  $hex = '0x{0:X}' -f $s.cmdNo
  $entry = "{ $hex, ATTRIB_INTERNAL_COMM, `"$($s.lpc)`", 0 }"
  $r += [pscustomobject]@{
    cmdNo        = $s.cmdNo
    fun          = $s.fun
    type         = 2
    addr         = $s.addr
    lpcFun       = $s.lpc
    nArgs        = 3
    typeSeq      = @(88, 357, 936)
    bodyFields   = @('u8len command', 'u16BE count', 'count*(u16BE slen+bytes) -> array<string>')
    handlerName  = "handle_$($s.fun)"
    handlerCode  = $h
    kcmdEntry    = $entry
    status       = 'ok'
    note         = ''
  }
}

$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk5_batch_13.json'
Write-Output "written $($r.Count) results"
