#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace beez::plugin::lua::crypto_detail
{

[[nodiscard]] std::vector<std::string> supportedHashAlgorithms();
[[nodiscard]] std::vector<std::string> supportedEncodingAlgorithms();

[[nodiscard]] bool isHashAlgorithm(std::string_view name);
[[nodiscard]] bool isEncodingAlgorithm(std::string_view name);

[[nodiscard]] std::string hashString(std::string_view data, std::string_view algorithm);
[[nodiscard]] std::string hashFile(const std::filesystem::path& path, std::string_view algorithm);

[[nodiscard]] std::string encodeString(std::string_view data, std::string_view encoding = "hex");
[[nodiscard]] std::string
encodeWithKey(std::string_view data, std::string_view key, std::string_view hashAlgorithm);

}  // namespace beez::plugin::lua::crypto_detail
