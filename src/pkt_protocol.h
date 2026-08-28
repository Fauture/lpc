#pragma once
// Packet protocol analyser (vm_pkt_analyser.c / vm_efun_magic.c).
// The game client protocol is defined by comm_desc.txt: each command has a
// signature like "void msg_l_auth(int32 result, int32 auth_key, string2 msg)"
// that describes the body layout.  LPC registers interfaces with
// pkt_add_interface(cmdNo, "name", function, encapType) while loading the
// comm description; when a packet arrives, vm_pktAnalyse matches it against
// the interface table, parses the body per the field descriptions and applies
// the registered LPC function.
//
// Wire field types (from the driver's pkt declarations):
//   int8/int16/int32/int64  - big-endian integers
//   string / string2 / string4 - length-prefixed string (1/2/4 byte length)
//   buffer / buffer2 / buffer4 - length-prefixed raw bytes
//   <type>[]                 - array of the given type
//   mapping / encap:mapping  - encapsulated structure

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

// One field in a packet definition.
struct PktField {
    std::string type;      // int8,int16,int32,int64,string,string2,string4,
                           // buffer,buffer2,buffer4, or user type name
    std::string name;      // field name
    bool isArray = false;  // trailing []
    int fixedLen = 0;      // for bufferN/stringN: N
};

// One command interface (one line of comm_desc.txt).
struct PktInterface {
    uint32_t flag = 0;          // interface_flag (Client_user/Internal_comm/...)
    uint16_t cmdNo = 0;         // command number (assigned by interface order)
    std::string name;           // command name e.g. "msg_l_auth"
    std::vector<PktField> fields;   // parsed from the signature
    Value func;                 // LPC function closure to apply
    int encapType = 0;          // encap_type
    uint16_t attrib = 0;        // command attribute (from the section)
};

// pkt_init_analyser(max_interfaces, max_fields)
void pktInitAnalyser(int maxInterfaces, int maxFields);

// pkt_add_interface(cmdNo, signature, function, encapType) - register one
// command interface from its signature line.  flag is the command filter
// (the interface_flag from the comm_desc section, e.g. CLIENT_USER=1).
void pktAddInterface(int cmdNo, const std::string& signature, const Value& func, int encapType, uint32_t flag = 0);

// pkt_add_def(name, signature) - register an encapsulated type definition.
void pktAddDef(const std::string& name, const std::string& signature);

// Parse a packet body for cmdNo; if an interface exists, parse fields and
// call the registered LPC function (vm_applyFun).  Returns 1 if handled,
// 0 if no interface matched (caller falls back to cmdIndex), <0 on error.
int pktAnalyse(int clientNo, uint32_t attrib, uint16_t cmdNo,
               const uint8_t* body, size_t len);

// Look up an interface by command number (for cmdIndex fallback debugging).
const PktInterface* pktFindInterface(uint16_t cmdNo);

// Build the reply body for an interface: serialize the field values (the
// "$<"[1..<1] argument array from send_message) per the interface's field
// layout.  encapType 0 -> mapping name->value, 1 -> positional array.
// Returns the body bytes (starts with the msg_no?  no - caller adds it).
// Empty string when the interface is unknown (original returns length 0).
std::string pktPadInterface(uint16_t msgNo, const Value& args);

// Load comm_desc.txt content: parse sections and register all interfaces via
// the given callback (used by the load_comm_desc efun).
void pktLoadCommDesc(const std::string& content,
                     const std::function<void(int flag, const std::string& sig,
                                              const Value& fn, int encap)>& reg);

} // namespace lpc
