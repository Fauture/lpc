$h1 = @'
void handle_vm_cmdSetPennantSkill(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t pennantSkill = rd16be(body, 0);
    applyLpc("cmd_set_pennant_skill", {make_int(cl.clientNo), make_int(pennantSkill)});
}
'@

$h2 = @'
void handle_vm_cmdSetPetName(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint8_t pos = body[0];
    uint8_t nameLen = body[1];
    if ((size_t)2 + nameLen > len) return;
    applyLpc("cmd_set_pet_name", {make_int(cl.clientNo), make_int(pos), make_string(body + 2, nameLen)});
}
'@

$h3 = @'
void handle_vm_cmdSetRecommendAttrib(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    uint32_t id = rd32be(body, 0);
    uint8_t con = body[4];
    uint8_t wiz = body[5];
    uint8_t str = body[6];
    uint8_t dex = body[7];
    uint8_t isAuto = body[8];
    applyLpc("cmd_set_recommend_attrib", {make_int(cl.clientNo), make_int(id), make_int(con), make_int(wiz), make_int(str), make_int(dex), make_int(isAuto)});
}
'@

$h4 = @'
void handle_vm_cmdSetRecPerson(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    const uint8_t* name = body + off;
    off += nameLen;
    if (off + 1 > len) return;
    uint8_t gidLen = body[off++];
    if (off + gidLen > len) return;
    applyLpc("cmd_set_rec_person", {make_int(cl.clientNo), make_string(name, nameLen), make_string(body + off, gidLen)});
}
'@

$h5 = @'
void handle_vm_cmdSetRegion(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t addrLen = body[0];
    if ((size_t)1 + addrLen > len) return;
    applyLpc("cmd_set_region", {make_int(cl.clientNo), make_string(body + 1, addrLen)});
}
'@

$h6 = @'
void handle_vm_cmdSetSafeLoginTime(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t safeLoginTime = rd16be(body, 0);
    applyLpc("cmd_set_safe_login_time", {make_int(cl.clientNo), make_int(safeLoginTime)});
}
'@

$h7 = @'
void handle_vm_cmdSetSafeMechanism(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 4 > len) return;
    uint16_t count = rd16be(body, off); off += 2;
    uint16_t num = rd16be(body, off); off += 2;
    if (off + 1 > len) return;
    uint8_t questionLen = body[off++];
    if (off + questionLen > len) return;
    const uint8_t* question = body + off;
    off += questionLen;
    if (off + 1 > len) return;
    uint8_t answerLen = body[off++];
    if (off + answerLen > len) return;
    applyLpc("cmd_set_safe_mechanism", {make_int(cl.clientNo), make_int(count), make_int(num), make_string(question, questionLen), make_string(body + off, answerLen)});
}
'@

$h8 = @'
void handle_vm_cmdSetSafeState(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t safeState = rd16be(body, 0);
    applyLpc("cmd_set_safe_state", {make_int(cl.clientNo), make_int(safeState)});
}
'@

$h9 = @'
void handle_vm_cmdSetSetting(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t settingLen = body[0];
    if ((size_t)1 + settingLen + 2 > len) return;
    const uint8_t* setting = body + 1;
    uint16_t param = rd16be(body, 1 + settingLen);
    applyLpc("cmd_set_setting", {make_int(cl.clientNo), make_string(setting, settingLen), make_int(param)});
}
'@

$h10 = @'
void handle_vm_cmdSetSignature(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t msgLen = body[0];
    if ((size_t)1 + msgLen > len) return;
    applyLpc("cmd_set_individuality_signature", {make_int(cl.clientNo), make_string(body + 1, msgLen)});
}
'@

$r = @(
    @{ cmdNo = 4306;  fun = "vm_cmdSetPennantSkill";     type = 1; addr = "0x8058348"; lpcFun = "cmd_set_pennant_skill";             nArgs = 2; typeSeq = @(88,88);            bodyFields = @("pennantSkill:u16be");                      handlerName = "handle_vm_cmdSetPennantSkill";     handlerCode = $h1;  kcmdEntry = '{ 0x10D2, ATTRIB_CLIENT_USER, "cmd_set_pennant_skill", 0 }';             status = "ok"; note = "" },
    @{ cmdNo = 64854; fun = "vm_cmdSetPetName";           type = 1; addr = "0x8058268"; lpcFun = "cmd_set_pet_name";                 nArgs = 3; typeSeq = @(88,88,357);         bodyFields = @("pos:u8","nameLen:u8","name:string");          handlerName = "handle_vm_cmdSetPetName";           handlerCode = $h2;  kcmdEntry = '{ 0xFD56, ATTRIB_CLIENT_USER, "cmd_set_pet_name", 0 }';                 status = "ok"; note = "" },
    @{ cmdNo = 28692; fun = "vm_cmdSetRecommendAttrib";   type = 1; addr = "0x8058394"; lpcFun = "cmd_set_recommend_attrib";         nArgs = 7; typeSeq = @(88,88,88,88,88,88,88); bodyFields = @("id:u32be","con:u8","wiz:u8","str:u8","dex:u8","isAuto:u8"); handlerName = "handle_vm_cmdSetRecommendAttrib";   handlerCode = $h3;  kcmdEntry = '{ 0x7014, ATTRIB_CLIENT_USER, "cmd_set_recommend_attrib", 0 }';         status = "ok"; note = "" },
    @{ cmdNo = 61659; fun = "vm_cmdSetRecPerson";         type = 1; addr = "0x805d3e0"; lpcFun = "cmd_set_rec_person";              nArgs = 3; typeSeq = @(88,357,357);         bodyFields = @("nameLen:u8","name:string","gidLen:u8","gid:string"); handlerName = "handle_vm_cmdSetRecPerson";         handlerCode = $h4;  kcmdEntry = '{ 0xF0DB, ATTRIB_CLIENT_USER, "cmd_set_rec_person", 0 }';              status = "ok"; note = "" },
    @{ cmdNo = 64940; fun = "vm_cmdSetRegion";            type = 1; addr = "0x805dbcc"; lpcFun = "cmd_set_region";                  nArgs = 2; typeSeq = @(88,357);             bodyFields = @("addrLen:u8","address:string");                 handlerName = "handle_vm_cmdSetRegion";            handlerCode = $h5;  kcmdEntry = '{ 0xFDAC, ATTRIB_CLIENT_USER, "cmd_set_region", 0 }';                  status = "ok"; note = "" },
    @{ cmdNo = 8540;  fun = "vm_cmdSetSafeLoginTime";     type = 1; addr = "0x806af94"; lpcFun = "cmd_set_safe_login_time";          nArgs = 2; typeSeq = @(88,88);            bodyFields = @("safeLoginTime:u16be");                      handlerName = "handle_vm_cmdSetSafeLoginTime";     handlerCode = $h6;  kcmdEntry = '{ 0x215C, ATTRIB_CLIENT_USER, "cmd_set_safe_login_time", 0 }';          status = "ok"; note = "" },
    @{ cmdNo = 20746; fun = "vm_cmdSetSafeMechanism";     type = 1; addr = "0x806b040"; lpcFun = "cmd_set_safe_mechanism";           nArgs = 5; typeSeq = @(88,88,88,357,357);   bodyFields = @("count:u16be","num:u16be","questionLen:u8","question:string","answerLen:u8","answer:string"); handlerName = "handle_vm_cmdSetSafeMechanism";     handlerCode = $h7;  kcmdEntry = '{ 0x510A, ATTRIB_CLIENT_USER, "cmd_set_safe_mechanism", 0 }';           status = "ok"; note = "" },
    @{ cmdNo = 19186; fun = "vm_cmdSetSafeState";         type = 1; addr = "0x806b17c"; lpcFun = "cmd_set_safe_state";              nArgs = 2; typeSeq = @(88,88);            bodyFields = @("safeState:u16be");                        handlerName = "handle_vm_cmdSetSafeState";         handlerCode = $h8;  kcmdEntry = '{ 0x4AF2, ATTRIB_CLIENT_USER, "cmd_set_safe_state", 0 }';              status = "ok"; note = "" },
    @{ cmdNo = 9032;  fun = "vm_cmdSetSetting";           type = 1; addr = "0x8058430"; lpcFun = "cmd_set_setting";                 nArgs = 3; typeSeq = @(88,357,88);          bodyFields = @("settingLen:u8","setting:string","param:u16be"); handlerName = "handle_vm_cmdSetSetting";           handlerCode = $h9;  kcmdEntry = '{ 0x2348, ATTRIB_CLIENT_USER, "cmd_set_setting", 0 }';                 status = "ok"; note = "" },
    @{ cmdNo = 64253; fun = "vm_cmdSetSignature";         type = 1; addr = "0x805dcd4"; lpcFun = "cmd_set_individuality_signature";  nArgs = 2; typeSeq = @(88,357);             bodyFields = @("msgLen:u8","msg:string");                    handlerName = "handle_vm_cmdSetSignature";         handlerCode = $h10; kcmdEntry = '{ 0xFAFD, ATTRIB_CLIENT_USER, "cmd_set_individuality_signature", 0 }';  status = "ok"; note = "" }
)

$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk3_batch_17.json'
Write-Output "written"
