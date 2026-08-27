#pragma once
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

class Interpreter;

Value call_builtin(Interpreter* interp, const std::string& name, const std::vector<Value>& args);

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
