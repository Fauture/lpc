#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lpc {

// Minimal MySQL client implementing the client/server protocol 4.1+ (no
// external libraries: plain TCP + native-password SHA1 auth + COM_QUERY with
// text result sets).  Targets the MySQL 5.1.73 server the game uses
// (192.168.200.128:3306, configured in aaa/aaa.ini).
class MysqlClient {
public:
    MysqlClient();
    ~MysqlClient();
    MysqlClient(const MysqlClient&) = delete;
    MysqlClient& operator=(const MysqlClient&) = delete;

    // Connects and authenticates.  On failure returns false and fills error.
    bool connect(const std::string& host, int port, const std::string& user,
                 const std::string& password, const std::string& database,
                 std::string& error);
    bool isConnected() const { return sock_ != 0; }
    void close();

    // A cell as returned by the server: a number (numeric column) or a raw
    // string; isNull for SQL NULL.  Date/time columns stay strings.
    struct Cell {
        bool isNull = false;
        bool isNumber = false;
        int64_t num = 0;
        std::string str;
    };
    struct Result {
        std::vector<std::string> cols;
        std::vector<std::vector<Cell>> rows;
        int64_t affectedRows = 0;
        int64_t insertId = 0;
        bool hasResultSet = false;  // true for SELECT-like statements
        std::vector<uint8_t> colTypes_;  // per-column server type codes
    };

    // Executes one statement.  Returns false (with error) on network problems
    // or server ERR packets.  Write statements fill affectedRows/insertId.
    bool execute(const std::string& sql, Result& out, std::string& error);

private:
    bool readPacket(std::vector<uint8_t>& body, std::string& error);
    bool writePacket(const std::vector<uint8_t>& body, std::string& error);
    bool handshake(const std::string& user, const std::string& password,
                   const std::string& database, std::string& error);

    uintptr_t sock_ = 0;  // SOCKET (kept opaque to avoid winsock in the header)
    uint8_t seq_ = 0;
    std::string serverSalt_;
    uint32_t serverCapabilities_ = 0;
};

// Parse a connection target: "port : host" (as built by dbd's load_db_config),
// "host:port", or plain "host".  Returns false if empty.
bool parseMysqlHost(const std::string& s, std::string& host, int& port);

}  // namespace lpc
