$h1 = @'
void handle_vm_cmdStartTaotaotaoRun(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_start_taotaotao_run", {make_int(cl.clientNo)});
}
'@

$h2 = @'
void handle_vm_cmdStore(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 10) return;
    uint32_t v6 = rd32be(body, 0);
    uint16_t v7 = rd16be(body, 4);
    uint16_t v8 = rd16be(body, 6);
    uint16_t v9 = rd16be(body, 8);
    applyLpc("cmd_store", {make_int(cl.clientNo), make_int(v6), make_int(v7), make_int(v8), make_int(v9)});
}
'@

$h3 = @'
void handle_vm_cmdSubmitConfirmDlg(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t nl = body[off++];
    if (off + nl > len) return;
    const uint8_t* dialog_item = body + off; off += nl;
    if (off + 1 > len) return;
    uint8_t pl = body[off++];
    if (off + pl > len) return;
    const uint8_t* para = body + off; off += pl;
    applyLpc("cmd_submit_confirm_dlg", {make_int(cl.clientNo), make_string(dialog_item, nl), make_string(para, pl)});
}
'@

$h4 = @'
void handle_vm_cmdSubmitIcon(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    int oper_type = body[off++];
    if (off + 1 > len) return;
    uint8_t ml = body[off++];
    if (off + ml > len) return;
    const uint8_t* md5_value = body + off; off += ml;
    const uint8_t* buf = body + off;
    size_t bufLen = len - off;
    applyLpc("cmd_submit_icon", {make_int(cl.clientNo), make_int(oper_type), make_string(md5_value, ml), make_buffer(buf, bufLen)});
}
'@

$h5 = @'
void handle_vm_cmdSubmitImage(GameClient& cl, const uint8_t* body, size_t len)
{
    applyLpc("cmd_submit_individuality_image", {make_int(cl.clientNo), make_buffer(body, len)});
}
'@

$h6 = @'
void handle_vm_cmdSubmitPasspodOrder(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* passpod_iid = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* name = body + off; off += l2;
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* addres = body + off; off += l3;
    if (off + 1 > len) return;
    uint8_t l4 = body[off++];
    if (off + l4 > len) return;
    const uint8_t* phone = body + off; off += l4;
    if (off + 1 > len) return;
    uint8_t l5 = body[off++];
    if (off + l5 > len) return;
    const uint8_t* extra_phone = body + off; off += l5;
    if (off + 1 > len) return;
    uint8_t l6 = body[off++];
    if (off + l6 > len) return;
    const uint8_t* zip = body + off; off += l6;
    applyLpc("cmd_submit_passpod_order", {make_int(cl.clientNo), make_string(passpod_iid, l1), make_string(name, l2), make_string(addres, l3), make_string(phone, l4), make_string(extra_phone, l5), make_string(zip, l6)});
}
'@

$h7 = @'
void handle_vm_cmdSwitchBackEquip(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_switch_back_equip", {make_int(cl.clientNo), make_int(body[0])});
}
'@

$h8 = @'
void handle_vm_cmdSwitchChildItem(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 6) return;
    uint32_t v6 = rd32be(body, 0);
    uint8_t b4 = body[4];
    uint8_t b5 = body[5];
    applyLpc("cmd_switch_child_item", {make_int(cl.clientNo), make_int(v6), make_int(b4), make_int(b5)});
}
'@

$h9 = @'
void handle_vm_cmdSwitchGroup(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* name = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* fromGroup = body + off; off += l2;
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* toGroup = body + off; off += l3;
    applyLpc("cmd_switch_group", {make_int(cl.clientNo), make_string(name, l1), make_string(fromGroup, l2), make_string(toGroup, l3)});
}
'@

$h10 = @'
void handle_vm_cmdSwitchGroupEx(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    if (off + 1 > len) return;
    uint8_t l1 = body[off++];
    if (off + l1 > len) return;
    const uint8_t* name = body + off; off += l1;
    if (off + 1 > len) return;
    uint8_t l2 = body[off++];
    if (off + l2 > len) return;
    const uint8_t* group = body + off; off += l2;
    if (off + 1 > len) return;
    int friend_verify = body[off++];
    if (off + 1 > len) return;
    uint8_t l3 = body[off++];
    if (off + l3 > len) return;
    const uint8_t* to_group = body + off; off += l3;
    applyLpc("cmd_switch_group_ex", {make_int(cl.clientNo), make_string(name, l1), make_string(group, l2), make_int(friend_verify), make_string(to_group, l3)});
}
'@

$r = @(
  @{ cmdNo = 20492; fun = "vm_cmdStartTaotaotaoRun"; type = 1; addr = "0x806c960"; lpcFun = "cmd_start_taotaotao_run"; nArgs = 1; typeSeq = @(88); bodyFields = @(); handlerName = "handle_vm_cmdStartTaotaotaoRun"; handlerCode = $h1; kcmdEntry = '{ 0x500C, ATTRIB_CLIENT_USER, "cmd_start_taotaotao_run", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 62326; fun = "vm_cmdStore"; type = 1; addr = "0x80587b8"; lpcFun = "cmd_store"; nArgs = 5; typeSeq = @(88,88,88,88,88); bodyFields = @("u32BE value","u16BE v7","u16BE v8","u16BE v9"); handlerName = "handle_vm_cmdStore"; handlerCode = $h2; kcmdEntry = '{ 0xF376, ATTRIB_CLIENT_USER, "cmd_store", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 16662; fun = "vm_cmdSubmitConfirmDlg"; type = 1; addr = "0x8058864"; lpcFun = "cmd_submit_confirm_dlg"; nArgs = 3; typeSeq = @(88,357,357); bodyFields = @("u8len dialog_item","u8len para"); handlerName = "handle_vm_cmdSubmitConfirmDlg"; handlerCode = $h3; kcmdEntry = '{ 0x4116, ATTRIB_CLIENT_USER, "cmd_submit_confirm_dlg", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 63937; fun = "vm_cmdSubmitIcon"; type = 1; addr = "0x8061048"; lpcFun = "cmd_submit_icon"; nArgs = 4; typeSeq = @(88,88,357,228); bodyFields = @("u8 oper_type","u8len md5_value","buffer rest"); handlerName = "handle_vm_cmdSubmitIcon"; handlerCode = $h4; kcmdEntry = '{ 0xF9C1, ATTRIB_CLIENT_USER, "cmd_submit_icon", 0 }'; status = "ok"; note = "trailing vm_apply arg v6=md5len is register artifact beyond nArgs, ignored" },
  @{ cmdNo = 4967; fun = "vm_cmdSubmitImage"; type = 1; addr = "0x805de38"; lpcFun = "cmd_submit_individuality_image"; nArgs = 2; typeSeq = @(88,228); bodyFields = @("buffer whole body"); handlerName = "handle_vm_cmdSubmitImage"; handlerCode = $h5; kcmdEntry = '{ 0x1367, ATTRIB_CLIENT_USER, "cmd_submit_individuality_image", 0 }'; status = "ok"; note = "trailing vm_apply arg a1@ecx is register artifact beyond nArgs, ignored" },
  @{ cmdNo = 8802; fun = "vm_cmdSubmitPasspodOrder"; type = 1; addr = "0x8058938"; lpcFun = "cmd_submit_passpod_order"; nArgs = 7; typeSeq = @(88,357,357,357,357,357,357); bodyFields = @("u8len passpod_iid","u8len name","u8len addres","u8len phone","u8len extra_phone","u8len zip"); handlerName = "handle_vm_cmdSubmitPasspodOrder"; handlerCode = $h6; kcmdEntry = '{ 0x2262, ATTRIB_CLIENT_USER, "cmd_submit_passpod_order", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 8464; fun = "vm_cmdSwitchBackEquip"; type = 1; addr = "0x8058b48"; lpcFun = "cmd_switch_back_equip"; nArgs = 2; typeSeq = @(88,88); bodyFields = @("u8 equip"); handlerName = "handle_vm_cmdSwitchBackEquip"; handlerCode = $h7; kcmdEntry = '{ 0x2110, ATTRIB_CLIENT_USER, "cmd_switch_back_equip", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 12370; fun = "vm_cmdSwitchChildItem"; type = 1; addr = "0x806d13c"; lpcFun = "cmd_switch_child_item"; nArgs = 4; typeSeq = @(88,88,88,88); bodyFields = @("u32BE item","u8","u8"); handlerName = "handle_vm_cmdSwitchChildItem"; handlerCode = $h8; kcmdEntry = '{ 0x3052, ATTRIB_CLIENT_USER, "cmd_switch_child_item", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 158; fun = "vm_cmdSwitchGroup"; type = 1; addr = "0x8058b88"; lpcFun = "cmd_switch_group"; nArgs = 4; typeSeq = @(88,357,357,357); bodyFields = @("u8len name","u8len fromGroup","u8len toGroup"); handlerName = "handle_vm_cmdSwitchGroup"; handlerCode = $h9; kcmdEntry = '{ 0x9E, ATTRIB_CLIENT_USER, "cmd_switch_group", 0 }'; status = "ok"; note = "" },
  @{ cmdNo = 16936; fun = "vm_cmdSwitchGroupEx"; type = 1; addr = "0x806c9c0"; lpcFun = "cmd_switch_group_ex"; nArgs = 5; typeSeq = @(88,357,357,88,357); bodyFields = @("u8len name","u8len group","u8 friend_verify","u8len to_group"); handlerName = "handle_vm_cmdSwitchGroupEx"; handlerCode = $h10; kcmdEntry = '{ 0x4228, ATTRIB_CLIENT_USER, "cmd_switch_group_ex", 0 }'; status = "ok"; note = "" }
)

$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk4_batch_3.json'
Write-Output "written $($r.Count) results"

