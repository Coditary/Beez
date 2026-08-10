#pragma once

#include <yyjson.h>

#include <string>

namespace beez::plugin::lua::data_detail
{

[[nodiscard]] bool validateJson(yyjson_val* data, yyjson_val* schema, std::string& error);

}  // namespace beez::plugin::lua::data_detail
