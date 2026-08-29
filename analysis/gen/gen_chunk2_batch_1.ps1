$h1 = @'
void handle_vm_cmdDraftVote(GameClient& cl, const uint8_t* body, size_t len)
{
    int stage = 0;
    int page = 0;
    std::string account;
    if (len >= 1) stage = body[0];
    if (len >= 3) page = rd16be(body, 1);
    if (len < 4) return;
    uint8_t slen = body[3];
    if (4 + slen > len) return;
    account.assign((const char*)body + 4, slen);
    applyLpc("cmd_draft_vote", {make_int(cl.clientNo), make_int(stage), make_int(page), make_string(account)});
}
'@

$h2 = @'
void handle_vm_cmdDrawCardRankBonus(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_draw_card_rank_bonus", {make_int(cl.clientNo), make_int(v)});
}
'@

$h3 = @'
void handle_vm_cmdDrop(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 5) return;
    uint8_t v1 = body[0];
    uint32_t v2 = rd32be(body, 1);
    applyLpc("cmd_drop", {make_int(cl.clientNo), make_int(v1), make_int(v2)});
}
'@

$h4 = @'
void handle_vm_cmdDropChild(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_drop_child", {make_int(cl.clientNo), make_int(v)});
}
'@

$h5 = @'
void handle_vm_cmdDropPet(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 4) return;
    uint32_t v = rd32be(body);
    applyLpc("cmd_drop_pet", {make_int(cl.clientNo), make_int(v)});
}
'@

$h6 = @'
void handle_vm_cmdDropRandomBoxAward(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_drop_random_box_award", {make_int(cl.clientNo)});
}
'@

$h7 = @'
void handle_vm_cmdDropTask(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    uint8_t slen = body[0];
    if (1 + slen > len) return;
    std::string name((const char*)body + 1, slen);
    applyLpc("cmd_drop_task", {make_int(cl.clientNo), make_string(name)});
}
'@

$h8 = @'
void handle_vm_cmdDungeonViewTasks(GameClient& cl, const uint8_t* body, size_t len)
{
    if (len < 1) return;
    applyLpc("cmd_dungeon_view_tasks", {make_int(cl.clientNo), make_int(body[0])});
}
'@

$h9 = @'
void handle_vm_cmdEmote(GameClient& cl, const uint8_t* body, size_t len)
{
    size_t off = 0;
    std::string verb, target, peer;
    auto readStr = [&](std::string& s) -> bool {
        if (off >= len) return false;
        uint8_t slen = body[off++];
        if (off + slen > len) return false;
        s.assign((const char*)body + off, slen);
        off += slen;
        return true;
    };
    if (!readStr(verb) || !readStr(target)) return;
    int channel = 0;
    if (off + 2 <= len) { channel = rd16be(body, off); off += 2; }
    if (!readStr(peer)) return;
    applyLpc("cmd_emote", {make_int(cl.clientNo), make_string(verb), make_string(target), make_int(channel), make_string(peer)});
}
'@

$h10 = @'
void handle_vm_cmdEndChallenge(GameClient& cl, const uint8_t* body, size_t len)
{
    (void)body; (void)len;
    applyLpc("cmd_end_challenge", {make_int(cl.clientNo)});
}
'@

$r = @(
    @{ cmdNo = 28700; fun = 'vm_cmdDraftVote'; type = 1; addr = '0x8070bd8'; lpcFun = 'cmd_draft_vote'; nArgs = 4; typeSeq = @(88,88,88,357); bodyFields = @('stage:u8','page:u16be','account:u8len-string'); handlerName = 'handle_vm_cmdDraftVote'; handlerCode = $h1; kcmdEntry = '{ 0x701C, ATTRIB_CLIENT_USER, "cmd_draft_vote", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 8772; fun = 'vm_cmdDrawCardRankBonus'; type = 1; addr = '0x8076bb8'; lpcFun = 'cmd_draw_card_rank_bonus'; nArgs = 2; typeSeq = @(88,88); bodyFields = @('val:u32be'); handlerName = 'handle_vm_cmdDrawCardRankBonus'; handlerCode = $h2; kcmdEntry = '{ 0x2244, ATTRIB_CLIENT_USER, "cmd_draw_card_rank_bonus", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 8758; fun = 'vm_cmdDrop'; type = 1; addr = '0x80530d0'; lpcFun = 'cmd_drop'; nArgs = 3; typeSeq = @(88,88,88); bodyFields = @('v1:u8','v2:u32be'); handlerName = 'handle_vm_cmdDrop'; handlerCode = $h3; kcmdEntry = '{ 0x2236, ATTRIB_CLIENT_USER, "cmd_drop", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 16390; fun = 'vm_cmdDropChild'; type = 1; addr = '0x806ccf8'; lpcFun = 'cmd_drop_child'; nArgs = 2; typeSeq = @(88,88); bodyFields = @('val:u32be'); handlerName = 'handle_vm_cmdDropChild'; handlerCode = $h4; kcmdEntry = '{ 0x4006, ATTRIB_CLIENT_USER, "cmd_drop_child", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 13056; fun = 'vm_cmdDropPet'; type = 1; addr = '0x8053128'; lpcFun = 'cmd_drop_pet'; nArgs = 2; typeSeq = @(88,88); bodyFields = @('val:u32be'); handlerName = 'handle_vm_cmdDropPet'; handlerCode = $h5; kcmdEntry = '{ 0x3300, ATTRIB_CLIENT_USER, "cmd_drop_pet", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 64820; fun = 'vm_cmdDropRandomBoxAward'; type = 1; addr = '0x80777d8'; lpcFun = 'cmd_drop_random_box_award'; nArgs = 1; typeSeq = @(88); bodyFields = @(); handlerName = 'handle_vm_cmdDropRandomBoxAward'; handlerCode = $h6; kcmdEntry = '{ 0xFD34, ATTRIB_CLIENT_USER, "cmd_drop_random_box_award", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 16504; fun = 'vm_cmdDropTask'; type = 1; addr = '0x806e0e4'; lpcFun = 'cmd_drop_task'; nArgs = 2; typeSeq = @(88,357); bodyFields = @('name:u8len-string'); handlerName = 'handle_vm_cmdDropTask'; handlerCode = $h7; kcmdEntry = '{ 0x4078, ATTRIB_CLIENT_USER, "cmd_drop_task", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 8483; fun = 'vm_cmdDungeonViewTasks'; type = 1; addr = '0x805d568'; lpcFun = 'cmd_dungeon_view_tasks'; nArgs = 2; typeSeq = @(88,88); bodyFields = @('val:u8'); handlerName = 'handle_vm_cmdDungeonViewTasks'; handlerCode = $h8; kcmdEntry = '{ 0x2123, ATTRIB_CLIENT_USER, "cmd_dungeon_view_tasks", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 7176; fun = 'vm_cmdEmote'; type = 1; addr = '0x80532d8'; lpcFun = 'cmd_emote'; nArgs = 5; typeSeq = @(88,357,357,88,357); bodyFields = @('verb:u8len-string','target:u8len-string','channel:u16be','peer:u8len-string'); handlerName = 'handle_vm_cmdEmote'; handlerCode = $h9; kcmdEntry = '{ 0x1C08, ATTRIB_CLIENT_USER, "cmd_emote", 0 }'; status = 'ok'; note = '' },
    @{ cmdNo = 52996; fun = 'vm_cmdEndChallenge'; type = 1; addr = '0x806d22c'; lpcFun = 'cmd_end_challenge'; nArgs = 1; typeSeq = @(88); bodyFields = @(); handlerName = 'handle_vm_cmdEndChallenge'; handlerCode = $h10; kcmdEntry = '{ 0xCF04, ATTRIB_CLIENT_USER, "cmd_end_challenge", 0 }'; status = 'ok'; note = '' }
)

$r | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 'analysis/gen/chunk2_batch_1.json'
Write-Output ('wrote ' + $r.Count + ' results')
