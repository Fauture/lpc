$hFetchPics = @'
void handle_vm_cmdAdminFetchPics(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_admin_fetch_pics", {make_int(cl.clientNo)});
}
'@

$hKickoff = @'
void handle_vm_cmdAdminKickoff(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_kickoff", {make_int(cl.clientNo), make_string(name)});
}
'@

$hOperSysBroadcast = @'
void handle_vm_cmdAdminOperSysBroadcast(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t oper = rd16be(body);
    uint16_t index = rd16be(body + 2);
    applyLpc("cmd_admin_oper_sys_broadcast", {make_int(cl.clientNo), make_int(oper), make_int(index)});
}
'@

$hPrintScreen = @'
void handle_vm_cmdAdminPrintScreen(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_print_screen", {make_int(cl.clientNo), make_string(name)});
}
'@

$hQueryMap = @'
void handle_vm_cmdAdminQueryMap(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string mapName((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_map", {make_int(cl.clientNo), make_string(mapName)});
}
'@

$hQueryNetInfo = @'
void handle_vm_cmdAdminQueryNetInfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t type = body[0];
    uint8_t nl = body[1];
    if (2 + nl > len) return;
    std::string userName((const char*)body + 2, nl);
    applyLpc("cmd_admin_query_net_info", {make_int(cl.clientNo), make_int(type), make_string(userName)});
}
'@

$hQueryNpc = @'
void handle_vm_cmdAdminQueryNpc(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t nl = body[0];
    if (1 + nl + 2 > len) return;
    std::string name((const char*)body + 1, nl);
    uint16_t id = rd16be(body + 1 + nl);
    applyLpc("cmd_admin_query_npc", {make_int(cl.clientNo), make_string(name), make_int(id)});
}
'@

$hQueryPlayer = @'
void handle_vm_cmdAdminQueryPlayer(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string statement((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_player", {make_int(cl.clientNo), make_string(statement)});
}
'@

$hQuerySkill = @'
void handle_vm_cmdAdminQuerySkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nl = body[0];
    if (1 + nl > len) return;
    std::string name((const char*)body + 1, nl);
    applyLpc("cmd_admin_query_skill", {make_int(cl.clientNo), make_string(name)});
}
'@

$hReboot = @'
void handle_vm_cmdAdminReboot(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t delay = rd16be(body);
    applyLpc("cmd_admin_reboot", {make_int(cl.clientNo), make_int(delay)});
}
'@

$r = @(
  @{ cmdNo = 12014; fun = 'vm_cmdAdminFetchPics'; type = 1; addr = '0x806cc14'; lpcFun = 'cmd_admin_fetch_pics'; nArgs = 1; typeSeq = @(88); bodyFields = @(); handlerName = 'handle_vm_cmdAdminFetchPics'; handlerCode = $hFetchPics; kcmdEntry = '{ 0x2EEE, ATTRIB_CLIENT_USER, "cmd_admin_fetch_pics", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 52998; fun = 'vm_cmdAdminKickoff'; type = 1; addr = '0x8068e48'; lpcFun = 'cmd_admin_kickoff'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('u8len string name'); handlerName = 'handle_vm_cmdAdminKickoff'; handlerCode = $hKickoff; kcmdEntry = '{ 0xCF06, ATTRIB_CLIENT_USER, "cmd_admin_kickoff", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 8566; fun = 'vm_cmdAdminOperSysBroadcast'; type = 1; addr = '0x8068ecc'; lpcFun = 'cmd_admin_oper_sys_broadcast'; nArgs = 3; typeSeq = @(88,88,88); bodyFields = @('int u16 oper','int u16 index'); handlerName = 'handle_vm_cmdAdminOperSysBroadcast'; handlerCode = $hOperSysBroadcast; kcmdEntry = '{ 0x2176, ATTRIB_CLIENT_USER, "cmd_admin_oper_sys_broadcast", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 14866; fun = 'vm_cmdAdminPrintScreen'; type = 1; addr = '0x8077f40'; lpcFun = 'cmd_admin_print_screen'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('u8len string name'); handlerName = 'handle_vm_cmdAdminPrintScreen'; handlerCode = $hPrintScreen; kcmdEntry = '{ 0x3A12, ATTRIB_CLIENT_USER, "cmd_admin_print_screen", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 6908; fun = 'vm_cmdAdminQueryMap'; type = 1; addr = '0x8068f34'; lpcFun = 'cmd_admin_query_map'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('u8len string mapName'); handlerName = 'handle_vm_cmdAdminQueryMap'; handlerCode = $hQueryMap; kcmdEntry = '{ 0x1AFC, ATTRIB_CLIENT_USER, "cmd_admin_query_map", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 8438; fun = 'vm_cmdAdminQueryNetInfo'; type = 1; addr = '0x8068fb8'; lpcFun = 'cmd_admin_query_net_info'; nArgs = 3; typeSeq = @(88,88,357); bodyFields = @('int u8 type','u8len string userName'); handlerName = 'handle_vm_cmdAdminQueryNetInfo'; handlerCode = $hQueryNetInfo; kcmdEntry = '{ 0x20F6, ATTRIB_CLIENT_USER, "cmd_admin_query_net_info", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 12316; fun = 'vm_cmdAdminQueryNpc'; type = 1; addr = '0x8069058'; lpcFun = 'cmd_admin_query_npc'; nArgs = 3; typeSeq = @(88,357,88); bodyFields = @('u8len string name','int u16 id'); handlerName = 'handle_vm_cmdAdminQueryNpc'; handlerCode = $hQueryNpc; kcmdEntry = '{ 0x301C, ATTRIB_CLIENT_USER, "cmd_admin_query_npc", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 250; fun = 'vm_cmdAdminQueryPlayer'; type = 1; addr = '0x8069104'; lpcFun = 'cmd_admin_query_player'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('u8len string statement'); handlerName = 'handle_vm_cmdAdminQueryPlayer'; handlerCode = $hQueryPlayer; kcmdEntry = '{ 0xFA, ATTRIB_CLIENT_USER, "cmd_admin_query_player", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 10030; fun = 'vm_cmdAdminQuerySkill'; type = 1; addr = '0x8069188'; lpcFun = 'cmd_admin_query_skill'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('u8len string name'); handlerName = 'handle_vm_cmdAdminQuerySkill'; handlerCode = $hQuerySkill; kcmdEntry = '{ 0x272E, ATTRIB_CLIENT_USER, "cmd_admin_query_skill", 0 }'; status = 'ok'; note = '' },
  @{ cmdNo = 9282; fun = 'vm_cmdAdminReboot'; type = 1; addr = '0x806920c'; lpcFun = 'cmd_admin_reboot'; nArgs = 2; typeSeq = @(88,88); bodyFields = @('int u16 delay'); handlerName = 'handle_vm_cmdAdminReboot'; handlerCode = $hReboot; kcmdEntry = '{ 0x2442, ATTRIB_CLIENT_USER, "cmd_admin_reboot", 0 }'; status = 'ok'; note = '' }
)
New-Item -ItemType Directory -Force -Path 'analysis\gen' | Out-Null
$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis\gen\batch_2.json'
Write-Output "written $($r.Count) results"
