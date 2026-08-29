$results = @()

# 1. vm_cmdFishingBuyWeapon
$h1 = @'
void handle_vm_cmdFishingBuyWeapon(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint16_t weaponId = rd16be(body, 0);
    uint32_t val = rd32be(body, 2);
    applyLpc("cmd_fishing_buy_weapon", {make_int(cl.clientNo), make_int(weaponId), make_int(val)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 8998; fun = 'vm_cmdFishingBuyWeapon'; type = 1; addr = '0x8075594'
    lpcFun = 'cmd_fishing_buy_weapon'; nArgs = 3; typeSeq = @(88,88,88)
    bodyFields = @('u16be int','u32be int')
    handlerName = 'handle_vm_cmdFishingBuyWeapon'; handlerCode = $h1
    kcmdEntry = '{ 0x2326, ATTRIB_CLIENT_USER, "cmd_fishing_buy_weapon", 0 }'
    status = 'ok'; note = ''
}

# 2. vm_cmdFishingChangeWeapon
$h2 = @'
void handle_vm_cmdFishingChangeWeapon(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    uint16_t weaponId = rd16be(body, 0);
    applyLpc("cmd_fishing_change_weapon", {make_int(cl.clientNo), make_int(weaponId)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 16944; fun = 'vm_cmdFishingChangeWeapon'; type = 1; addr = '0x80754b0'
    lpcFun = 'cmd_fishing_change_weapon'; nArgs = 2; typeSeq = @(88,88)
    bodyFields = @('u16be int')
    handlerName = 'handle_vm_cmdFishingChangeWeapon'; handlerCode = $h2
    kcmdEntry = '{ 0x4230, ATTRIB_CLIENT_USER, "cmd_fishing_change_weapon", 0 }'
    status = 'ok'; note = ''
}

# 3. vm_cmdFishingFireBullet
$h3 = @'
void handle_vm_cmdFishingFireBullet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 8) return;
    uint32_t a = rd32be(body, 0);
    uint32_t b = rd32be(body, 4);
    applyLpc("cmd_fishing_fire_bullet", {make_int(cl.clientNo), make_int(a), make_int(b)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 9030; fun = 'vm_cmdFishingFireBullet'; type = 1; addr = '0x807552c'
    lpcFun = 'cmd_fishing_fire_bullet'; nArgs = 3; typeSeq = @(88,88,88)
    bodyFields = @('u32be int','u32be int')
    handlerName = 'handle_vm_cmdFishingFireBullet'; handlerCode = $h3
    kcmdEntry = '{ 0x2346, ATTRIB_CLIENT_USER, "cmd_fishing_fire_bullet", 0 }'
    status = 'ok'; note = ''
}

# 4. vm_cmdFishingOpenRemoteStore
$h4 = @'
void handle_vm_cmdFishingOpenRemoteStore(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_fishing_open_remote_store", {make_int(cl.clientNo)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 9036; fun = 'vm_cmdFishingOpenRemoteStore'; type = 1; addr = '0x8075710'
    lpcFun = 'cmd_fishing_open_remote_store'; nArgs = 1; typeSeq = @(88)
    bodyFields = @()
    handlerName = 'handle_vm_cmdFishingOpenRemoteStore'; handlerCode = $h4
    kcmdEntry = '{ 0x234C, ATTRIB_CLIENT_USER, "cmd_fishing_open_remote_store", 0 }'
    status = 'ok'; note = ''
}

# 5. vm_cmdFishingQuit
$h5 = @'
void handle_vm_cmdFishingQuit(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_fishing_quit", {make_int(cl.clientNo)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 4960; fun = 'vm_cmdFishingQuit'; type = 1; addr = '0x80754fc'
    lpcFun = 'cmd_fishing_quit'; nArgs = 1; typeSeq = @(88)
    bodyFields = @()
    handlerName = 'handle_vm_cmdFishingQuit'; handlerCode = $h5
    kcmdEntry = '{ 0x1360, ATTRIB_CLIENT_USER, "cmd_fishing_quit", 0 }'
    status = 'ok'; note = ''
}

# 6. vm_cmdFishSwallowDie
$h6 = @'
void handle_vm_cmdFishSwallowDie(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t val = rd32be(body, 0);
    applyLpc("cmd_fish_swallow_die", {make_int(cl.clientNo), make_int(val)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 4456; fun = 'vm_cmdFishSwallowDie'; type = 1; addr = '0x8053b08'
    lpcFun = 'cmd_fish_swallow_die'; nArgs = 2; typeSeq = @(88,88)
    bodyFields = @('u32be int')
    handlerName = 'handle_vm_cmdFishSwallowDie'; handlerCode = $h6
    kcmdEntry = '{ 0x1168, ATTRIB_CLIENT_USER, "cmd_fish_swallow_die", 0 }'
    status = 'ok'; note = ''
}

# 7. vm_cmdFishSwallowFinish
$h7 = @'
void handle_vm_cmdFishSwallowFinish(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint8_t die = body[0];
    uint16_t statLen = rd16be(body, 1);
    size_t off = 3;
    if (off + statLen > len) return;
    std::string stat((const char*)body + off, statLen);
    off += statLen;
    if (off >= len) return;
    uint8_t csLen = body[off++];
    if (off + csLen > len) return;
    std::string checksum((const char*)body + off, csLen);
    applyLpc("cmd_fish_swallow_finish", {make_int(cl.clientNo), make_int(die), make_string(stat), make_string(checksum)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 16594; fun = 'vm_cmdFishSwallowFinish'; type = 1; addr = '0x8053b58'
    lpcFun = 'cmd_fish_swallow_finish'; nArgs = 4; typeSeq = @(88,88,357,357)
    bodyFields = @('u8 int','u16be str len','str bytes','u8 str len','str bytes')
    handlerName = 'handle_vm_cmdFishSwallowFinish'; handlerCode = $h7
    kcmdEntry = '{ 0x40D2, ATTRIB_CLIENT_USER, "cmd_fish_swallow_finish", 0 }'
    status = 'ok'; note = ''
}

# 8. vm_cmdFriendDrinkInvite
$h8 = @'
void handle_vm_cmdFriendDrinkInvite(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t nameLen = body[0];
    if (1 + nameLen > len) return;
    std::string name((const char*)body + 1, nameLen);
    applyLpc("cmd_friend_drink_invite", {make_int(cl.clientNo), make_string(name)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 8588; fun = 'vm_cmdFriendDrinkInvite'; type = 1; addr = '0x8053c9c'
    lpcFun = 'cmd_friend_drink_invite'; nArgs = 2; typeSeq = @(88,357)
    bodyFields = @('u8 str len','str bytes')
    handlerName = 'handle_vm_cmdFriendDrinkInvite'; handlerCode = $h8
    kcmdEntry = '{ 0x218C, ATTRIB_CLIENT_USER, "cmd_friend_drink_invite", 0 }'
    status = 'ok'; note = ''
}

# 9. vm_cmdFriendDrinkRefresh
$h9 = @'
void handle_vm_cmdFriendDrinkRefresh(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_friend_drink_refresh", {make_int(cl.clientNo)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 4632; fun = 'vm_cmdFriendDrinkRefresh'; type = 1; addr = '0x8053d20'
    lpcFun = 'cmd_friend_drink_refresh'; nArgs = 1; typeSeq = @(88)
    bodyFields = @()
    handlerName = 'handle_vm_cmdFriendDrinkRefresh'; handlerCode = $h9
    kcmdEntry = '{ 0x1218, ATTRIB_CLIENT_USER, "cmd_friend_drink_refresh", 0 }'
    status = 'ok'; note = ''
}

# 10. vm_cmdFriendTell
$h10 = @'
void handle_vm_cmdFriendTell(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 3) return;
    uint16_t flag = rd16be(body, 0);
    size_t off = 2;
    uint8_t nameLen = body[off++];
    if (off + nameLen > len) return;
    std::string name((const char*)body + off, nameLen);
    off += nameLen;
    if (off >= len) return;
    uint8_t msgLen = body[off++];
    if (off + msgLen > len) return;
    std::string msg((const char*)body + off, msgLen);
    applyLpc("cmd_friend_tell", {make_int(cl.clientNo), make_int(flag), make_string(name), make_string(msg)});
}
'@
$results += [pscustomobject]@{
    cmdNo = 8966; fun = 'vm_cmdFriendTell'; type = 1; addr = '0x8053d50'
    lpcFun = 'cmd_friend_tell'; nArgs = 4; typeSeq = @(88,88,357,357)
    bodyFields = @('u16be int','u8 str len','str bytes','u8 str len','str bytes')
    handlerName = 'handle_vm_cmdFriendTell'; handlerCode = $h10
    kcmdEntry = '{ 0x2306, ATTRIB_CLIENT_USER, "cmd_friend_tell", 0 }'
    status = 'ok'; note = ''
}

$results | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk2_batch_5.json'
Write-Output ("wrote " + $results.Count + " results")
