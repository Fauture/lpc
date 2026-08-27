#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

class Interpreter;

// Thrown by input_string() when the serving telnet client has disconnected.
// The original driver kills the pended process when the session closes; the
// VM thread runner catches this quietly and ends the daemon thread instead of
// letting the LPC while(1) shell loop spin on repeated EOF.
struct TelnetClosedError : std::runtime_error {
    explicit TelnetClosedError(const std::string& m) : std::runtime_error(m) {}
};

Value call_builtin(Interpreter* interp, const std::string& name, const std::vector<Value>& args);

// vm_interSscanf (0x8109908): C-style sscanf.  Parses input against fmt,
// appends one output Value per conversion (in order) to `out`, and returns the
// number of successful conversions.  Supports %s %d %u %x %c %f %* (suppress)
// and literal/whitespace matching.  The caller writes each output back into the
// corresponding reference argument.
int lpcSscanf(const std::string& input, const std::string& fmt, std::vector<Value>& out);

// Telnet client plumbing shared with the VM thread runner (original_vm.cpp).
// The daemon thread that serves one telnet connection registers itself with
// its client number so input_string()/printf() route to the right socket.
void setThreadTelnetClient(int clientNo);
int threadTelnetClient();

// Driver-level telnet shell configured by the -w/-p command line options
// (mirroring the original driver: only when a port is configured does the
// driver listen for telnet).  startDriverTelnet(port) binds and accepts
// connections that go through the driver shell (password check +
// execute_data).  Returns 1 on success.
int startDriverTelnet(int port);
void setDriverTelnetPassword(const std::string& pwd);

} // namespace lpc
