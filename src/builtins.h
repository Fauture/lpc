#pragma once
#include <string>
#include <vector>

#include "value.h"

namespace lpc {

class Interpreter;

Value call_builtin(Interpreter* interp, const std::string& name, const std::vector<Value>& args);

} // namespace lpc
