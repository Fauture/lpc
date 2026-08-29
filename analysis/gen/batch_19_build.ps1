$ErrorActionPreference = 'Stop'

$h_csetpos = @'
void handle_vm_cmdCSetPos(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_c_set_pos", {make_int(cl.clientNo), make_buffer(body, len)});
}
'@.Trim()

$h_csexchange = @'
void handle_vm_cmdCsExchange(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t itemId = rd16be(body);
    uint16_t count = rd16be(body + 2);
    applyLpc("cmd_cs_exchange", {make_int(cl.clientNo), make_int(itemId), make_int(count)});
}
'@.Trim()

$h_cusestun = @'
void handle_vm_cmdCUseStunChildbook(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_c_use_stun_child_book", {make_int(cl.clientNo)});
}
'@.Trim()

$h_deleteexisted = @'
void handle_vm_cmdDeleteExistedChar(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string name((const char*)body + off, nl);
    off += nl;
    if (off >= len) return;
    uint8_t na = body[off++];
    if (off + na > len) return;
    std::string accountId((const char*)body + off, na);
    applyLpc("cmd_delete_existed_char", {make_int(cl.clientNo), make_string(name), make_string(accountId)});
}
'@.Trim()

$h_deletemusic = @'
void handle_vm_cmdDeleteMusic(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t musicId = rd32be(body);
    applyLpc("cmd_delete_music", {make_int(cl.clientNo), make_int(musicId)});
}
'@.Trim()

$h_deposit = @'
void handle_vm_cmdDeposit(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t itemId = rd32be(body);
    uint32_t count = rd32be(body + 4);
    applyLpc("cmd_deposit", {make_int(cl.clientNo), make_int(itemId), make_int(count)});
}
'@.Trim()

$h_developskill = @'
void handle_vm_cmdDevelopSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint16_t v1 = rd16be(body);
    uint32_t v2 = rd32be(body + 2);
    uint16_t v3 = rd16be(body + 6);
    applyLpc("cmd_develop_skill", {make_int(cl.clientNo), make_int(v1), make_int(v2), make_int(v3)});
}
'@.Trim()

$h_digparty = @'
void handle_vm_cmdDigPartyFragment(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string partyId((const char*)body + off, nl);
    applyLpc("cmd_dig_party_fragment", {make_int(cl.clientNo), make_string(partyId)});
}
'@.Trim()

$h_dismiss = @'
void handle_vm_cmdDismissChatGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string group((const char*)body + off, nl);
    applyLpc("cmd_dismiss_chat_group", {make_int(cl.clientNo), make_string(group)});
}
'@.Trim()

$h_distribute = @'
void handle_vm_cmdDistributeRecruitment(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    size_t off = 0;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    std::string type((const char*)body + off, nl);
    off += nl;
    if (off + 2 > len) return;
    uint16_t minLevel = rd16be(body + off);
    off += 2;
    if (off + 2 > len) return;
    uint16_t maxLevel = rd16be(body + off);
    off += 2;
    if (off >= len) return;
    uint8_t nt = body[off++];
    if (off + nt > len) return;
    std::string tip((const char*)body + off, nt);
    applyLpc("cmd_distribute_recruitment", {make_int(cl.clientNo), make_string(type), make_int(minLevel), make_int(maxLevel), make_string(tip)});
}
'@.Trim()

$r = @(
    [pscustomobject]@{ cmdNo = 4620; fun = 'vm_cmdCSetPos'; type = 1; addr = '0x8051b38'; lpcFun = 'cmd_c_set_pos'; nArgs = 2; typeSeq = @(88, 228); bodyFields = @('buffer(len=body)'); handlerName = 'handle_vm_cmdCSetPos'; handlerCode = $h_csetpos; kcmdEntry = '{ 0x120C, ATTRIB_CLIENT_USER, "cmd_c_set_pos", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 10028; fun = 'vm_cmdCsExchange'; type = 1; addr = '0x8052b98'; lpcFun = 'cmd_cs_exchange'; nArgs = 3; typeSeq = @(88, 88, 88); bodyFields = @('int u16 itemId', 'int u16 count'); handlerName = 'handle_vm_cmdCsExchange'; handlerCode = $h_csexchange; kcmdEntry = '{ 0x272C, ATTRIB_CLIENT_USER, "cmd_cs_exchange", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 8474; fun = 'vm_cmdCUseStunChildbook'; type = 1; addr = '0x8051b74'; lpcFun = 'cmd_c_use_stun_child_book'; nArgs = 1; typeSeq = @(88); bodyFields = @(); handlerName = 'handle_vm_cmdCUseStunChildbook'; handlerCode = $h_cusestun; kcmdEntry = '{ 0x211A, ATTRIB_CLIENT_USER, "cmd_c_use_stun_child_book", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 4698; fun = 'vm_cmdDeleteExistedChar'; type = 1; addr = '0x8052f50'; lpcFun = 'cmd_delete_existed_char'; nArgs = 3; typeSeq = @(88, 357, 357); bodyFields = @('string u8len name', 'string u8len accountId'); handlerName = 'handle_vm_cmdDeleteExistedChar'; handlerCode = $h_deleteexisted; kcmdEntry = '{ 0x125A, ATTRIB_CLIENT_USER, "cmd_delete_existed_char", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 10270; fun = 'vm_cmdDeleteMusic'; type = 1; addr = '0x8070ff0'; lpcFun = 'cmd_delete_music'; nArgs = 2; typeSeq = @(88, 88); bodyFields = @('int u32 musicId'); handlerName = 'handle_vm_cmdDeleteMusic'; handlerCode = $h_deletemusic; kcmdEntry = '{ 0x281E, ATTRIB_CLIENT_USER, "cmd_delete_music", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 8294; fun = 'vm_cmdDeposit'; type = 1; addr = '0x8052c00'; lpcFun = 'cmd_deposit'; nArgs = 3; typeSeq = @(88, 88, 88); bodyFields = @('int u32 itemId', 'int u32 count'); handlerName = 'handle_vm_cmdDeposit'; handlerCode = $h_deposit; kcmdEntry = '{ 0x2066, ATTRIB_CLIENT_USER, "cmd_deposit", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 64864; fun = 'vm_cmdDevelopSkill'; type = 1; addr = '0x8052c68'; lpcFun = 'cmd_develop_skill'; nArgs = 4; typeSeq = @(88, 88, 88, 88); bodyFields = @('int u16 v1', 'int u32 v2', 'int u16 v3'); handlerName = 'handle_vm_cmdDevelopSkill'; handlerCode = $h_developskill; kcmdEntry = '{ 0xFD60, ATTRIB_CLIENT_USER, "cmd_develop_skill", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 4380; fun = 'vm_cmdDigPartyFragment'; type = 1; addr = '0x8052e60'; lpcFun = 'cmd_dig_party_fragment'; nArgs = 2; typeSeq = @(88, 357); bodyFields = @('string u8len partyId'); handlerName = 'handle_vm_cmdDigPartyFragment'; handlerCode = $h_digparty; kcmdEntry = '{ 0x111C, ATTRIB_CLIENT_USER, "cmd_dig_party_fragment", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 11722; fun = 'vm_cmdDismissChatGroup'; type = 1; addr = '0x8071c04'; lpcFun = 'cmd_dismiss_chat_group'; nArgs = 2; typeSeq = @(88, 357); bodyFields = @('string u8len group'); handlerName = 'handle_vm_cmdDismissChatGroup'; handlerCode = $h_dismiss; kcmdEntry = '{ 0x2DCA, ATTRIB_CLIENT_USER, "cmd_dismiss_chat_group", 0 }'; status = 'ok'; note = '' },
    [pscustomobject]@{ cmdNo = 12356; fun = 'vm_cmdDistributeRecruitment'; type = 1; addr = '0x8052cf4'; lpcFun = 'cmd_distribute_recruitment'; nArgs = 5; typeSeq = @(88, 357, 88, 88, 357); bodyFields = @('string u8len type', 'int u16 minLevel', 'int u16 maxLevel', 'string u8len tip'); handlerName = 'handle_vm_cmdDistributeRecruitment'; handlerCode = $h_distribute; kcmdEntry = '{ 0x3044, ATTRIB_CLIENT_USER, "cmd_distribute_recruitment", 0 }'; status = 'ok'; note = '' }
)

$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/batch_19.json'
Write-Output "wrote $($r.Count) entries"
