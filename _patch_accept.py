# -*- coding: utf-8 -*-
"""Wait for the RIGHT registry when a new client is accepted.

The old loop waited for find_agent_by_client(cno) only.  client_agents[] is
filled only by feature/common/agent.c::comm_set_client_no, which is reached
from comm::set_client_no - in the GS that happens for INTERNAL comm agents
(configd's config_agent) but NEVER for a real client connection, whose login
object registers itself through logind::new_user -> comm::set_client ->
set_client_user -> client_users[cno].  So the loop always ran all 200
iterations (200 x 5ms sleep + ~10ms per LPC round trip) = ~3.1s added to
EVERY new connection before the first packet was even read - the client's
"first packet takes ~3.6s after connecting" bug.

Wait for either registry (user first: it is what a player connection gets),
and log how long it took.
"""
import io

P = 'src/game_service.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


before = d.count(b'\r\n')
print('CRLF', before, 'bareLF', d.count(b'\n') - before)

OLD = B("""                {
                    auto root = lpc::masterSearchRoot();
                    for (int tries = 0; tries < 200; ++tries) {
                        Value agent = callMasterProgramFunction("find_agent_by_client",
                                                               {make_int(clientNo)}, root);
                        if (agent.index() == 7) break;   // object registered
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[game] waiting agent client=" << clientNo
                                      << " try=" << tries << "\\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                }
                {
                    auto root = lpc::masterSearchRoot();
                    Value agent = callMasterProgramFunction("find_agent_by_client",
                                                           {make_int(clientNo)}, root);
                    std::string peer = "?";
                    sockaddr_in sin{};
                    int slen = static_cast<int>(sizeof(sin));
                    if (getpeername(c, reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
                        char pbuf[64];
                        snprintf(pbuf, sizeof(pbuf), "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
                        peer = pbuf;
                    }
                    std::cerr << "[accept] t=" << lpc::msSinceStart() << "ms client=" << clientNo
                              << " agent-type=" << static_cast<int>(agent.index())
                              << " peer=" << peer << "\\n";
                }
""")

NEW = B("""                int waitMs = 0;
                int agentType = 0;
                int userType = 0;
                {
                    // The original driver is single threaded: the listen
                    // callback (cmd_new_connection -> logind::new_user ->
                    // comm::set_client -> set_client_user) completes BEFORE the
                    // first packet is parsed.  Here cmd_new_connection only
                    // QUEUES the dispatch on the scheduler thread, so the socket
                    // thread must wait for the registration itself or the first
                    // packet (0x4124 login) finds no user object.
                    //
                    // TWO registries exist and they must not be confused:
                    //   client_users[cno]  <- find_user_by_client: every player
                    //                         connection (logind::new_user)
                    //   client_agents[cno] <- find_agent_by_client: INTERNAL
                    //                         comm agents (feature/common/
                    //                         agent.c::comm_set_client_no)
                    // Waiting on the agent alone (the previous code) never
                    // matched for a player: the loop burnt all 200 iterations
                    // (200 x 5ms + one LPC round trip each ~10ms = ~3.1s) before
                    // every read, which is exactly the "first packet after a new
                    // connection takes ~3.6s" the client reported.
                    auto root = lpc::masterSearchRoot();
                    uint64_t waitStart = GetTickCount64();
                    for (int tries = 0; tries < 200; ++tries) {
                        Value u = callMasterProgramFunction("find_user_by_client",
                                                            {make_int(clientNo)}, root);
                        userType = static_cast<int>(u.index());
                        if (u.index() == 7) break;   // login object registered
                        Value a = callMasterProgramFunction("find_agent_by_client",
                                                            {make_int(clientNo)}, root);
                        agentType = static_cast<int>(a.index());
                        if (a.index() == 7) break;   // internal comm agent
                        if (std::getenv("LPCVM_TRACE"))
                            std::cerr << "[game] waiting client=" << clientNo
                                      << " try=" << tries << " user=" << userType
                                      << " agent=" << agentType << "\\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    waitMs = static_cast<int>(GetTickCount64() - waitStart);
                }
                {
                    std::string peer = "?";
                    sockaddr_in sin{};
                    int slen = static_cast<int>(sizeof(sin));
                    if (getpeername(c, reinterpret_cast<sockaddr*>(&sin), &slen) == 0) {
                        char pbuf[64];
                        snprintf(pbuf, sizeof(pbuf), "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
                        peer = pbuf;
                    }
                    std::cerr << "[accept] t=" << lpc::msSinceStart() << "ms client=" << clientNo
                              << " wait=" << waitMs << "ms user-type=" << userType
                              << " agent-type=" << agentType
                              << " peer=" << peer << "\\n";
                }
""")

assert d.count(OLD) == 1, 'OLD not found exactly once (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
after = d.count(b'\r\n')
print('written; CRLF', after, 'bareLF', d.count(b'\n') - after)
print('find_user_by_client refs:', d.count(b'find_user_by_client'))
