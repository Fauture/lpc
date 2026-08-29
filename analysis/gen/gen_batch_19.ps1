$r = @(
@{
  cmdNo = 4316; fun = 'vm_cmdOperLongtengShengshi'; type = 1; addr = '0x8073984';
  lpcFun = 'cmd_oper_longteng_shengshi'; nArgs = 3; typeSeq = @(88,88,357);
  bodyFields = @('int u32 id (BE)','string u8len');
  handlerName = 'handle_vm_cmdOperLongtengShengshi';
  handlerCode = @"
void handle_vm_cmdOperLongtengShengshi(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    int32_t id = rd32be(body, 0);
    uint8_t slen = body[4];
    if (5 + slen > len) return;
    std::string s((const char*)body + 5, slen);
    applyLpc("cmd_oper_longteng_shengshi", {make_int(cl.clientNo), make_int(id), make_string(s)});
}
"@;
  kcmdEntry = '{ 0x10DC, ATTRIB_CLIENT_USER, "cmd_oper_longteng_shengshi", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 10022; fun = 'vm_cmdOperLotteryTicket'; type = 1; addr = '0x805651c';
  lpcFun = 'cmd_oper_lottery_ticket'; nArgs = 3; typeSeq = @(88,88,88);
  bodyFields = @('int u8','int u8');
  handlerName = 'handle_vm_cmdOperLotteryTicket';
  handlerCode = @"
void handle_vm_cmdOperLotteryTicket(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 2) return;
    applyLpc("cmd_oper_lottery_ticket", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1])});
}
"@;
  kcmdEntry = '{ 0x2726, ATTRIB_CLIENT_USER, "cmd_oper_lottery_ticket", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 64790; fun = 'vm_cmdOperMagicPicture'; type = 1; addr = '0x807152c';
  lpcFun = 'cmd_oper_magic_picture'; nArgs = 7; typeSeq = @(88,88,88,88,88,88);
  bodyFields = @('int u32 (BE)','int u8','int u8','int u16 (BE)','int u8','int u8');
  handlerName = 'handle_vm_cmdOperMagicPicture';
  handlerCode = @"
void handle_vm_cmdOperMagicPicture(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    int32_t id = rd32be(body, 0);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    uint16_t b6 = rd16be(body, 6);
    uint8_t b8 = body[8];
    uint8_t b9 = body[9];
    applyLpc("cmd_oper_magic_picture", {make_int(cl.clientNo), make_int(id), make_int(b4), make_int(b5), make_int(b6), make_int(b8), make_int(b9)});
}
"@;
  kcmdEntry = '{ 0xFD16, ATTRIB_CLIENT_USER, "cmd_oper_magic_picture", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 9284; fun = 'vm_cmdOperMaster'; type = 1; addr = '0x8056568';
  lpcFun = 'cmd_oper_master'; nArgs = 4; typeSeq = @(88,88,88,357);
  bodyFields = @('int u32 id (BE)','int u16 flag (BE)','string u8len signature');
  handlerName = 'handle_vm_cmdOperMaster';
  handlerCode = @"
void handle_vm_cmdOperMaster(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    int32_t id = rd32be(body, 0);
    uint16_t flag = rd16be(body, 4);
    uint8_t slen = body[6];
    if (7 + slen > len) return;
    std::string signature((const char*)body + 7, slen);
    applyLpc("cmd_oper_master", {make_int(cl.clientNo), make_int(id), make_int(flag), make_string(signature)});
}
"@;
  kcmdEntry = '{ 0x2444, ATTRIB_CLIENT_USER, "cmd_oper_master", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 16394; fun = 'vm_cmdoperpackeditem'; type = 1; addr = '0x80594ac';
  lpcFun = 'cmd_oper_packed_item'; nArgs = 6; typeSeq = @(88,88,88,88,88);
  bodyFields = @('int u8','int u8','int u8','int u8','int u8');
  handlerName = 'handle_vm_cmdoperpackeditem';
  handlerCode = @"
void handle_vm_cmdoperpackeditem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    applyLpc("cmd_oper_packed_item", {make_int(cl.clientNo), make_int(body[0]), make_int(body[1]), make_int(body[2]), make_int(body[3]), make_int(body[4])});
}
"@;
  kcmdEntry = '{ 0x400A, ATTRIB_CLIENT_USER, "cmd_oper_packed_item", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 15910; fun = 'vm_cmdOperSecurity'; type = 1; addr = '0x806bc2c';
  lpcFun = 'cmd_oper_security'; nArgs = 4; typeSeq = @(88,88,88);
  bodyFields = @('int u16 (BE)','int u32 (BE)','int u8');
  handlerName = 'handle_vm_cmdOperSecurity';
  handlerCode = @"
void handle_vm_cmdOperSecurity(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 7) return;
    uint16_t v0 = rd16be(body, 0);
    int32_t v2 = rd32be(body, 2);
    uint8_t v6 = body[6];
    applyLpc("cmd_oper_security", {make_int(cl.clientNo), make_int(v0), make_int(v2), make_int(v6)});
}
"@;
  kcmdEntry = '{ 0x3E26, ATTRIB_CLIENT_USER, "cmd_oper_security", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 16412; fun = 'vm_cmdopersixanniinfo'; type = 1; addr = '0x8059590';
  lpcFun = 'cmd_oper_six_anni_info'; nArgs = 3; typeSeq = @(88,88);
  bodyFields = @('int u8','int u32 (BE)');
  handlerName = 'handle_vm_cmdopersixanniinfo';
  handlerCode = @"
void handle_vm_cmdopersixanniinfo(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t b0 = body[0];
    int32_t b1 = rd32be(body, 1);
    applyLpc("cmd_oper_six_anni_info", {make_int(cl.clientNo), make_int(b0), make_int(b1)});
}
"@;
  kcmdEntry = '{ 0x401C, ATTRIB_CLIENT_USER, "cmd_oper_six_anni_info", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 8788; fun = 'vm_cmdOperSortTask'; type = 1; addr = '0x806d2b8';
  lpcFun = 'cmd_oper_sort_task'; nArgs = 4; typeSeq = @(88,88,357);
  bodyFields = @('int u16 task_type (BE)','int u8 oper_type','string u8len para');
  handlerName = 'handle_vm_cmdOperSortTask';
  handlerCode = @"
void handle_vm_cmdOperSortTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint16_t task_type = rd16be(body, 0);
    uint8_t oper_type = body[2];
    uint8_t slen = body[3];
    if (4 + slen > len) return;
    std::string para((const char*)body + 4, slen);
    applyLpc("cmd_oper_sort_task", {make_int(cl.clientNo), make_int(task_type), make_int(oper_type), make_string(para)});
}
"@;
  kcmdEntry = '{ 0x2254, ATTRIB_CLIENT_USER, "cmd_oper_sort_task", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 8562; fun = 'vm_cmdoperstrategywar'; type = 1; addr = '0x80595e8';
  lpcFun = 'cmd_oper_strategy_war'; nArgs = 3; typeSeq = @(357,357);
  bodyFields = @('string u8len oper_type','string u8len para');
  handlerName = 'handle_vm_cmdoperstrategywar';
  handlerCode = @"
void handle_vm_cmdoperstrategywar(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off >= len) return;
    uint8_t len1 = body[off++];
    if (off + len1 > len) return;
    std::string oper_type((const char*)body + off, len1);
    off += len1;
    if (off >= len) return;
    uint8_t len2 = body[off++];
    if (off + len2 > len) return;
    std::string para((const char*)body + off, len2);
    applyLpc("cmd_oper_strategy_war", {make_int(cl.clientNo), make_string(oper_type), make_string(para)});
}
"@;
  kcmdEntry = '{ 0x2172, ATTRIB_CLIENT_USER, "cmd_oper_strategy_war", 0 }';
  status = 'ok'; note = ''
},
@{
  cmdNo = 4450; fun = 'vm_cmdOperTeleportItem'; type = 1; addr = '0x8056688';
  lpcFun = 'cmd_oper_teleport_item'; nArgs = 5; typeSeq = @(88,88,88,357);
  bodyFields = @('int u32 item_id (BE)','int u16 oper (BE)','int u16 para1 (BE)','string u8len para2');
  handlerName = 'handle_vm_cmdOperTeleportItem';
  handlerCode = @"
void handle_vm_cmdOperTeleportItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 9) return;
    int32_t item_id = rd32be(body, 0);
    uint16_t oper = rd16be(body, 4);
    int16_t para1 = (int16_t)rd16be(body, 6);
    uint8_t slen = body[8];
    if (9 + slen > len) return;
    std::string para2((const char*)body + 9, slen);
    applyLpc("cmd_oper_teleport_item", {make_int(cl.clientNo), make_int(item_id), make_int(oper), make_int(para1), make_string(para2)});
}
"@;
  kcmdEntry = '{ 0x1162, ATTRIB_CLIENT_USER, "cmd_oper_teleport_item", 0 }';
  status = 'ok'; note = ''
}
);
$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk2_batch_19.json'
Write-Output "written $($r.Count) entries"
