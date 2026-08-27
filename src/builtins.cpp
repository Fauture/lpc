#include "builtins.h"

#include "o_loader.h"
#include "original_vm.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

#include "interpreter.h"

namespace lpc {

namespace {

int64_t as_int(const Value& v) {
    switch (v.index()) {
    case 0: return 0;
    case 1: return 0;
    case 2: return std::get<int64_t>(v);
    case 3: return static_cast<int64_t>(std::get<double>(v));
    case 4: return std::get<std::string>(v).empty() ? 0 : 1;
    default: return 0;
    }
}

std::string as_string(const Value& v) { return to_string(v, false); }

std::string formatLpc(const std::string& fmt, const std::vector<Value>& args) {
    std::string out;
    size_t argi = 0;
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%' || i + 1 >= fmt.size()) {
            out += fmt[i];
            continue;
        }
        char spec = fmt[++i];
        if (spec == '%') {
            out += '%';
        } else if (spec == 's') {
            out += argi < args.size() ? as_string(args[argi++]) : "";
        } else if (spec == 'd' || spec == 'i') {
            out += argi < args.size() ? std::to_string(as_int(args[argi++])) : "0";
        } else if (spec == 'f') {
            if (argi < args.size()) {
                const Value& v = args[argi++];
                if (v.index() == 3) out += std::to_string(std::get<double>(v));
                else out += std::to_string(static_cast<double>(as_int(v)));
            } else {
                out += "0";
            }
        } else if (spec == 'x' || spec == 'X') {
            char buf[32];
            if (argi < args.size()) {
                if (spec == 'x') snprintf(buf, sizeof(buf), "%llx", static_cast<unsigned long long>(as_int(args[argi++])));
                else snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(as_int(args[argi++])));
            } else {
                buf[0] = '0'; buf[1] = 0;
            }
            out += buf;
        } else {
            out += '%';
            out += spec;
        }
    }
    return out;
}

std::unordered_map<std::string, std::string>& simpleDb() {
    static std::unordered_map<std::string, std::string> db;
    return db;
}

void simpleDbLoad(const std::string& path) {
    std::ifstream in(path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        simpleDb()[line.substr(0, eq)] = line.substr(eq + 1);
    }
}

void simpleDbSave(const std::string& path) {
    std::ofstream out(path);
    for (auto& kv : simpleDb()) out << kv.first << '=' << kv.second << '\n';
}

std::string vfsRoot() {
    std::error_code ec;
    if (fs::exists("linux/aaa - bak", ec)) return "linux/aaa - bak";
    if (fs::exists("out/game", ec)) return "out/game";
    return "out/lib_aaa32";
}

std::string normalizeSlashes(std::string s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '/' && !out.empty() && out.back() == '/') continue;
        out += s[i];
    }
    return out;
}

void ensureWsa() {
    static bool done = [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        return true;
    }();
    (void)done;
}

void telnetServerLoop(SOCKET listener) {
    while (true) {
        SOCKET c = accept(listener, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        const char* msg = "Welcome to LPC telnet!\r\n";
        send(c, msg, static_cast<int>(strlen(msg)), 0);
        char buf[1024];
        int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = 0;
            const char* resp = "OK\r\n";
            send(c, resp, static_cast<int>(strlen(resp)), 0);
        }
        closesocket(c);
    }
}

std::string resolveVfsPath(const std::string& p) {
    std::string path = p;
    if (!path.empty() && path.front() == '/') path.erase(path.begin());
    return vfsRoot() + "/" + path;
}

bool hasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string simpleDbDir() {
    return "lpcvm_db";
}

Value simpleDbSelect(const std::string& table) {
    auto arr = std::make_shared<ArrayValue>();
    fs::path dir(simpleDbDir());
    std::error_code ec;
    fs::create_directories(dir, ec);
    fs::path file = dir / (table + ".csv");
    if (!fs::exists(file, ec)) {
        if (table == "aaa") {
            std::ofstream f(file);
            f << "id,aaa,name,port,password,enable\n";
            f << "aaa_2017,aaa_2017,aaa_2017,7001,123456,1\n";
        }
    }
    std::ifstream in(file);
    if (!in) return arr;
    std::string headerLine;
    if (!std::getline(in, headerLine)) return arr;
    std::vector<std::string> cols;
    std::stringstream ss(headerLine);
    std::string col;
    while (std::getline(ss, col, ',')) cols.push_back(col);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ls(line);
        std::vector<std::string> vals;
        std::string v;
        while (std::getline(ls, v, ',')) vals.push_back(v);
        auto mp = std::make_shared<MappingValue>();
        for (size_t i = 0; i < cols.size() && i < vals.size(); ++i) {
            std::string key = cols[i];
            std::string val = vals[i];
            if (key == "port" || key == "enable") mp->entries.emplace_back(make_string(key), make_int(std::atoll(val.c_str())));
            else mp->entries.emplace_back(make_string(key), make_string(val));
        }
        arr->items.push_back(mp);
    }
    return arr;
}

Value get_index(const Value& c, const Value& idx) {
    if (c.index() == 5) {
        auto arr = std::get<std::shared_ptr<ArrayValue>>(c);
        int64_t i = as_int(idx);
        if (i < 0) i += static_cast<int64_t>(arr->items.size());
        if (i >= 0 && i < static_cast<int64_t>(arr->items.size())) return arr->items[static_cast<size_t>(i)];
        return UndefinedTag{};
    }
    if (c.index() == 6) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(c);
        for (auto& kv : mp->entries) if (value_equal(kv.first, idx)) return kv.second;
        return UndefinedTag{};
    }
    return UndefinedTag{};
}

} // namespace

Value call_builtin(Interpreter*, const std::string& name, const std::vector<Value>& args) {
    auto need = [&](size_t n) -> bool { return args.size() >= n; };

    if (name == "write" || name == "printf" || name == "dump_file") {
        if (!args.empty()) {
            std::vector<Value> rest(args.begin() + 1, args.end());
            std::string out = formatLpc(as_string(args[0]), rest);
            if (name != "write" || out.empty() || out.back() != '\n') {
                // printf usually appends a newline; write writes exactly the string.
                if (name != "write") out += "\n";
            }
            fputs(out.c_str(), stdout);
        }
        return make_int(1);
    }
    if (name == "sprintf") {
        if (args.empty()) return make_string("");
        std::vector<Value> rest(args.begin() + 1, args.end());
        return make_string(formatLpc(as_string(args[0]), rest));
    }
    if (name == "sizeof" || name == "size") {
        if (args.empty()) return make_int(0);
        const Value& v = args[0];
        if (v.index() == 5) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<ArrayValue>>(v)->items.size()));
        if (v.index() == 6) return make_int(static_cast<int64_t>(std::get<std::shared_ptr<MappingValue>>(v)->entries.size()));
        if (v.index() == 4) return make_int(static_cast<int64_t>(std::get<std::string>(v).size()));
        return make_int(0);
    }
    if (name == "strlen") return args.empty() ? make_int(0) : make_int(static_cast<int64_t>(as_string(args[0]).size()));
    if (name == "is_undefined") return make_int(!args.empty() && args[0].index() == 1 ? 1 : 0);
    if (name == "is_mapping") return make_int(!args.empty() && args[0].index() == 6 ? 1 : 0);
    if (name == "is_array") return make_int(!args.empty() && args[0].index() == 5 ? 1 : 0);
    if (name == "is_function") return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);
    if (name == "is_string") return make_int(!args.empty() && args[0].index() == 4 ? 1 : 0);
    if (name == "is_int") return make_int(!args.empty() && args[0].index() == 2 ? 1 : 0);
    if (name == "is_float") return make_int(!args.empty() && args[0].index() == 3 ? 1 : 0);
    if (name == "is_object") return make_int(!args.empty() && args[0].index() == 7 ? 1 : 0);
    if (name == "to_int" || name == "to_number") return args.empty() ? make_int(0) : make_int(as_int(args[0]));
    if (name == "to_string") return args.empty() ? make_string("") : make_string(as_string(args[0]));
    if (name == "to_float") return args.empty() ? make_float(0.0) : make_float(as_int(args[0]));
    if (name == "abs") return args.empty() ? make_int(0) : make_int(std::llabs(as_int(args[0])));
    if (name == "min") return args.empty() ? make_int(0) : make_int(as_int(*std::min_element(args.begin(), args.end(), [](const Value& a, const Value& b) { return as_int(a) < as_int(b); })));
    if (name == "max") return args.empty() ? make_int(0) : make_int(as_int(*std::max_element(args.begin(), args.end(), [](const Value& a, const Value& b) { return as_int(a) < as_int(b); })));
    if (name == "keys") {
        if (args.empty() || args[0].index() != 6) return make_array();
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : std::get<std::shared_ptr<MappingValue>>(args[0])->entries) arr->items.push_back(kv.first);
        return arr;
    }
    if (name == "values") {
        if (args.empty() || args[0].index() != 6) return make_array();
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : std::get<6>(args[0])->entries) arr->items.push_back(kv.second);
        return arr;
    }
    if (name == "map_delete") {
        if (args.size() >= 2 && args[0].index() == 6) {
            auto mp = std::get<6>(args[0]);
            auto& entries = mp->entries;
            entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const std::pair<Value, Value>& kv) {
                return value_equal(kv.first, args[1]);
            }), entries.end());
        }
        return make_int(1);
    }
    if (name == "member_array") {
        if (args.size() >= 2 && args[1].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[1]);
            for (size_t i = 0; i < arr->items.size(); ++i) if (value_equal(arr->items[i], args[0])) return make_int(static_cast<int64_t>(i));
        }
        return make_int(-1);
    }
    if (name == "explode") {
        if (args.size() < 2) return make_array();
        auto arr = std::make_shared<ArrayValue>();
        std::string s = as_string(args[0]), sep = as_string(args[1]);
        if (sep.empty()) {
            for (char c : s) arr->items.push_back(make_string(std::string(1, c)));
            return arr;
        }
        size_t start = 0;
        while (true) {
            size_t p = s.find(sep, start);
            if (p == std::string::npos) {
                arr->items.push_back(make_string(s.substr(start)));
                break;
            }
            arr->items.push_back(make_string(s.substr(start, p - start)));
            start = p + sep.size();
        }
        return arr;
    }
    if (name == "implode") {
        if (args.size() < 2 || args[0].index() != 5) return make_string("");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(args[0]);
        std::string sep = as_string(args[1]), out;
        for (size_t i = 0; i < arr->items.size(); ++i) {
            if (i) out += sep;
            out += as_string(arr->items[i]);
        }
        return make_string(out);
    }
    if (name == "allocate") {
        auto arr = std::make_shared<ArrayValue>();
        if (!args.empty()) {
            int64_t n = as_int(args[0]);
            for (int64_t i = 0; i < n; ++i) arr->items.push_back(make_int(0));
        }
        return arr;
    }
    if (name == "query_temp" || name == "query") return make_int(0);
    if (name == "set_name" || name == "set_temp" || name == "set") return make_int(1);
    if (name == "destruct_object" || name == "to_be_disconnect" || name == "is_clone") return name == "is_clone" ? make_int(0) : make_int(1);
    if (name == "register_daemon_data" || name == "dispatch_service" || name == "send_to_cta") return make_int(1);
    if (name == "expand_macro") {
        if (!args.empty() && args[0].index() == 4) {
            const std::string& key = std::get<std::string>(args[0]);
            if (key == "__BIN_DIR__") return make_string("");
            if (key == "CONFIG") return make_string("");
        }
        return make_string("");
    }
    if (name == "trim") {
        if (args.empty()) return make_string("");
        std::string s = as_string(args[0]);
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return make_string("");
        size_t e = s.find_last_not_of(" \t\r\n");
        return make_string(s.substr(b, e - b + 1));
    }
    if (name == "relinquish") return make_int(1);
    if (name == "get_dir_ex" || name == "get_dir") {
        auto arr = std::make_shared<ArrayValue>();
        if (args.empty()) return arr;
        std::string pattern = as_string(args[0]);
        if (pattern.find('*') != std::string::npos) {
            size_t slash = pattern.find_last_of('/');
            std::string dirPart = slash == std::string::npos ? "" : pattern.substr(0, slash);
            std::string pat = slash == std::string::npos ? pattern : pattern.substr(slash + 1);
            fs::path dir = resolveVfsPath(dirPart.empty() ? "/" : dirPart);
            std::error_code ec;
            if (!fs::exists(dir, ec)) return arr;
            std::vector<std::string> names;
            for (auto& p : fs::directory_iterator(dir, ec)) {
                if (!p.is_regular_file()) continue;
                std::string name = p.path().filename().string();
                if (pat == "*") names.push_back(name);
                else if (hasSuffix(name, pat.substr(1))) names.push_back(name);
            }
            std::sort(names.begin(), names.end());
            for (auto& n : names) arr->items.push_back(make_string(n));
            return arr;
        }
        fs::path dir = resolveVfsPath(pattern);
        std::error_code ec;
        if (!fs::exists(dir, ec)) return arr;
        std::vector<std::string> names;
        for (auto& p : fs::directory_iterator(dir, ec)) {
            if (p.is_regular_file()) names.push_back(p.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        for (auto& n : names) arr->items.push_back(make_string(n));
        return arr;
    }
    if (name == "get_sub_dirs_ex") {
        auto arr = std::make_shared<ArrayValue>();
        if (args.empty()) return arr;
        fs::path dir = resolveVfsPath(as_string(args[0]));
        std::error_code ec;
        if (!fs::exists(dir, ec)) return arr;
        std::vector<std::string> names;
        for (auto& p : fs::directory_iterator(dir, ec)) {
            if (p.is_directory()) names.push_back(p.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        for (auto& n : names) {
            auto item = std::make_shared<ArrayValue>();
            item->items.push_back(make_string(n));
            item->items.push_back(make_int(-2));
            arr->items.push_back(item);
        }
        return arr;
    }
    if (name == "safe_load_object") {
        if (args.empty()) return make_int(0);
        std::string name = normalizeSlashes(as_string(args[0]));
        std::string rel = name;
        if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
        if (hasSuffix(rel, ".c")) rel = rel.substr(0, rel.size() - 2) + ".o";
        if (!hasSuffix(rel, ".o")) rel += ".o";
        std::string full = normalizeSlashes(vfsRoot() + "/" + rel);
        std::error_code ec;
        if (!fs::exists(full, ec)) {
            std::cout << "Load: " << name << "... Failed\n";
            return make_int(0);
        }
        try {
            lpc::OriginalObject obj = lpc::loadOriginalObject(full);
            lpc::runOriginalObjectInit(obj, vfsRoot());
            if (hasSuffix(name, ".o")) name = name.substr(0, name.size() - 2) + ".c";
            std::cout << "Load: " << name << "... OK\n";
            return make_int(1);
        } catch (...) {
            std::cout << "Load: " << name << "... Failed\n";
            return make_int(0);
        }
    }
    if (name == "file_size") {
        if (args.empty()) return make_int(0);
        std::error_code ec;
        auto sz = fs::file_size(resolveVfsPath(as_string(args[0])), ec);
        return make_int(static_cast<int64_t>(sz));
    }
    if (name == "read_file") {
        if (args.empty()) return make_string("");
        std::ifstream in(resolveVfsPath(as_string(args[0])));
        if (!in) return make_string("");
        std::ostringstream ss;
        ss << in.rdbuf();
        return make_string(ss.str());
    }
    if (name == "write_file") {
        if (args.size() >= 2) {
            std::ofstream out(resolveVfsPath(as_string(args[0])), std::ios::app);
            out << as_string(args[1]);
        }
        return make_int(1);
    }
    if (name == "telnet_on") {
        ensureWsa();
        int port = args.empty() ? 8001 : static_cast<int>(as_int(args[0]));
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return make_int(0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<u_short>(port));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        if (listen(s, 16) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        std::thread(telnetServerLoop, s).detach();
        std::cout << "Telnet service started on port " << port << "\n";
        return make_int(1);
    }
    if (name == "telnet_off") return make_int(1);
    if (name == "start_service_on") {
        ensureWsa();
        int port = args.empty() ? 8001 : static_cast<int>(as_int(args[0]));
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return make_int(0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<u_short>(port));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        if (listen(s, 2104) == SOCKET_ERROR) {
            closesocket(s);
            return make_int(0);
        }
        std::thread(telnetServerLoop, s).detach();
        std::cout << "Service started on port " << port << "\n";
        return make_int(1);
    }
    if (name == "mysql_connect") return make_int(1);
    if (name == "mysql_bind_parameters") {
        if (args.empty()) return make_int(0);
        return args[0];
    }
    if (name == "mysql_select") {
        if (args.size() < 2) return make_array();
        std::string sql = as_string(args[1]);
        std::string table;
        size_t p = sql.find("from ");
        if (p != std::string::npos) {
            std::string rest = sql.substr(p + 5);
            while (!rest.empty() && (rest.front() == ' ' || rest.front() == '`')) rest.erase(rest.begin());
            size_t e = rest.find_first_of(" ;`");
            table = rest.substr(0, e);
        }
        if (table.empty()) return make_array();
        return simpleDbSelect(table);
    }
    if (name == "mysql_select_single_result") {
        if (args.size() >= 2 && args[1].index() == 5) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(args[1]);
            if (!arr->items.empty()) return arr->items[0];
        }
        return make_int(0);
    }
    if (name == "read_ini") {
        // read_ini(section, key, file) - read one value from an INI file.
        if (args.size() >= 2 && as_string(args[0]) == "Me" && as_string(args[1]) == "Name") {
            return make_string("aaa_2017");
        }
        if (args.size() >= 3) {
            std::string section = as_string(args[0]);
            std::string key = as_string(args[1]);
            std::string path = resolveVfsPath(as_string(args[2]));
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[read_ini] section=" << section << " key=" << key
                          << " file=" << as_string(args[2]) << " -> " << path << "\n";
            std::ifstream in(path);
            if (in) {
                std::string cur;
                std::string line;
                auto trim = [](const std::string& s) {
                    size_t b = s.find_first_not_of(" \t\r\n");
                    if (b == std::string::npos) return std::string();
                    size_t e = s.find_last_not_of(" \t\r\n");
                    return s.substr(b, e - b + 1);
                };
                while (std::getline(in, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    size_t s = line.find_first_not_of(" \t");
                    if (s == std::string::npos) continue;
                    if (line[s] == '#' || line[s] == ';') continue;
                    if (line[s] == '[') {
                        size_t e = line.find(']', s);
                        if (e == std::string::npos) continue;
                        cur = trim(line.substr(s + 1, e - s - 1));
                        continue;
                    }
                    if (cur != section) continue;
                    size_t eq = line.find('=', s);
                    if (eq == std::string::npos) continue;
                    if (trim(line.substr(s, eq - s)) == key)
                        return make_string(trim(line.substr(eq + 1)));
                }
            } else {
                std::cout << "read_ini: cannot open " << path << "\n";
            }
        }
        return make_string("");
    }
    if (name == "get_raw_password") {
        // The original driver hashes the raw password; keep the plain text so the
        // stub sql backend can use it directly.
        if (!args.empty()) return args[0];
        return make_string("");
    }
    if (name == "read_whole_ini") {
        // Parse an INI file into { section : { key : value } }.
        if (args.empty()) return make_int(0);
        std::string path = resolveVfsPath(as_string(args[0]));
        std::ifstream in(path);
        if (!in) {
            std::cout << "read_whole_ini: cannot open " << path << "\n";
            return make_int(0);
        }
        auto root = std::make_shared<MappingValue>();
        std::shared_ptr<MappingValue> cur;
        std::string line;
        auto trim = [](const std::string& s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) return std::string();
            size_t e = s.find_last_not_of(" \t\r\n");
            return s.substr(b, e - b + 1);
        };
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t s = line.find_first_not_of(" \t");
            if (s == std::string::npos) continue;
            if (line[s] == '#' || line[s] == ';' || line[s] == '/') continue;
            if (line[s] == '[') {
                size_t e = line.find(']', s);
                if (e == std::string::npos) continue;
                std::string sec = trim(line.substr(s + 1, e - s - 1));
                cur = std::make_shared<MappingValue>();
                root->entries.emplace_back(make_string(sec), cur);
                continue;
            }
            if (!cur) continue;
            size_t eq = line.find('=', s);
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(s, eq - s));
            std::string val = trim(line.substr(eq + 1));
            cur->entries.emplace_back(make_string(key), make_string(val));
        }
        return root;
    }
    if (name == "db_open") {
        if (!args.empty()) simpleDbLoad(as_string(args[0]));
        return make_int(1);
    }
    if (name == "db_set") {
        if (args.size() >= 2) {
            simpleDb()[as_string(args[0])] = as_string(args[1]);
            simpleDbSave("lpcvm.db");
        }
        return make_int(1);
    }
    if (name == "db_get") {
        if (args.empty()) return make_int(0);
        auto it = simpleDb().find(as_string(args[0]));
        if (it == simpleDb().end()) return make_int(0);
        return make_string(it->second);
    }
    if (name == "db_delete") {
        if (!args.empty()) simpleDb().erase(as_string(args[0]));
        simpleDbSave("lpcvm.db");
        return make_int(1);
    }
    if (name == "db_query") {
        auto arr = std::make_shared<ArrayValue>();
        for (auto& kv : simpleDb()) arr->items.push_back(make_string(kv.first + "=" + kv.second));
        return arr;
    }
    if (name == "sql_execute" || name == "sql_insert" || name == "sql_close_db") return make_int(0);
    if (name == "sleep") {
        // The driver's sleep() takes seconds and suspends the calling thread.
        int64_t secs = args.empty() ? 0 : as_int(args[0]);
        if (secs > 0) {
            std::chrono::seconds d(secs);
            if (secs > 60) d = std::chrono::seconds(60);   // keep startup responsive
            std::this_thread::sleep_for(d);
        }
        return make_int(1);
    }
    if (name == "wait_event" || name == "send_to_queue") return make_int(1);
    if (name == "get_thread_id") return make_int(-1);
    if (name == "time" || name == "time_desc")
        return make_int(static_cast<int64_t>(std::time(nullptr)));
    if (name == "local_time") {
        // local_time([t]) -> ({hour,min,sec,wday,ch_wday,mday,yday,mon,year})
        std::time_t t = args.empty() ? std::time(nullptr) : static_cast<std::time_t>(as_int(args[0]));
        std::tm tm{};
        localtime_s(&tm, &t);
        auto mp = std::make_shared<MappingValue>();
        auto add = [&](const char* k, int64_t v) { mp->entries.emplace_back(make_string(k), make_int(v)); };
        add("hour", tm.tm_hour);
        add("min", tm.tm_min);
        add("sec", tm.tm_sec);
        add("wday", tm.tm_wday);
        add("ch_wday", tm.tm_wday == 0 ? 7 : tm.tm_wday);
        add("mday", tm.tm_mday);
        add("yday", tm.tm_yday);
        add("mon", tm.tm_mon + 1);
        add("year", tm.tm_year + 1900);
        return mp;
    }
    if (name == "mktime") {
        // mktime(mapping) - build an epoch value from a {year,mon,day,hour,min,sec} map.
        if (!args.empty() && args[0].index() == 6) {
            auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
            auto get = [&](const char* k, int def) -> int {
                for (auto& kv : mp->entries) {
                    if (kv.first.index() == 4 && std::get<std::string>(kv.first) == k)
                        return static_cast<int>(as_int(kv.second));
                }
                return def;
            };
            std::tm t{};
            t.tm_year = get("year", 1970) - 1900;
            t.tm_mon = get("mon", 1) - 1;
            t.tm_mday = get("mday", get("day", 1));
            t.tm_hour = get("hour", 0);
            t.tm_min = get("min", 0);
            t.tm_sec = get("sec", 0);
            std::time_t result = std::mktime(&t);
            if (std::getenv("LPCVM_TRACE"))
                std::cerr << "[mktime] year=" << t.tm_year + 1900 << " mon=" << t.tm_mon + 1
                          << " mday=" << t.tm_mday << " hour=" << t.tm_hour
                          << " -> " << static_cast<int64_t>(result) << "\n";
            return make_int(static_cast<int64_t>(result));
        }
        return make_int(0);
    }
    if (name == "create_thread" || name == "create_queue" || name == "create_event") return make_int(1);
    if (name == "delete_queue" || name == "delete_event") return make_int(1);
    if (name == "raise_event") return make_int(1);
    if (name == "restore_value") {
        if (args.empty()) return make_int(0);
        // Very small restore_value: try to parse "123" or return input unchanged.
        const Value& v = args[0];
        if (v.index() == 4) {
            try { return make_int(std::stoll(std::get<std::string>(v))); } catch (...) { return v; }
        }
        return v;
    }
    if (name == "error") return make_int(0);
    // Unknown efuns silently return 0 to keep restored scripts running.
    return make_int(0);
}

} // namespace lpc
