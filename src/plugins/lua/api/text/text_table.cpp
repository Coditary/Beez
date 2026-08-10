#include "beez/plugin/lua/api/text/text_table.hpp"

#include "beez/plugin/lua/api/text/contains.hpp"
#include "beez/plugin/lua/api/text/diff.hpp"
#include "beez/plugin/lua/api/text/ends_with.hpp"
#include "beez/plugin/lua/api/text/join.hpp"
#include "beez/plugin/lua/api/text/regex_match.hpp"
#include "beez/plugin/lua/api/text/regex_replace.hpp"
#include "beez/plugin/lua/api/text/replace.hpp"
#include "beez/plugin/lua/api/text/replace_all.hpp"
#include "beez/plugin/lua/api/text/split.hpp"
#include "beez/plugin/lua/api/text/starts_with.hpp"
#include "beez/plugin/lua/api/text/template_string.hpp"
#include "beez/plugin/lua/api/text/to_case.hpp"
#include "beez/plugin/lua/api/text/to_lowercase.hpp"
#include "beez/plugin/lua/api/text/to_uppercase.hpp"
#include "beez/plugin/lua/api/text/trim.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindText(const std::shared_ptr<sol::state>& luaState)
{
    sol::table textTable = luaState->create_table();
    bindContains(textTable);
    bindStartsWith(textTable);
    bindEndsWith(textTable);
    bindToLowercase(textTable);
    bindToUppercase(textTable);
    bindToCase(textTable);
    bindReplace(textTable);
    bindReplaceAll(textTable);
    bindSplit(textTable, luaState);
    bindTrim(textTable);
    bindJoin(textTable);
    bindRegexMatch(textTable);
    bindRegexReplace(textTable);
    bindTemplateString(textTable);
    bindDiff(textTable, luaState);
    return textTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,bugprone-easily-swappable-parameters,readability-identifier-naming,modernize-use-starts-ends-with,modernize-use-auto,performance-unnecessary-value-param,readability-qualified-auto,readability-identifier-length)
