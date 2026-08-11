#include "beez/plugin/lua/api/archive/archive_table.hpp"

#include "beez/plugin/lua/api/archive/compress.hpp"
#include "beez/plugin/lua/api/archive/extract.hpp"
#include "beez/plugin/lua/api/archive/extract_file.hpp"
#include "beez/plugin/lua/api/archive/list.hpp"
#include "beez/plugin/lua/api/archive/read_text.hpp"

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>

namespace beez::plugin::lua
{

sol::table bindArchive(const std::shared_ptr<sol::state>& luaState, const core::Context& context)
{
    sol::table archiveTable = luaState->create_table();
    bindArchiveExtract(archiveTable, luaState, context);
    bindArchiveCompress(archiveTable, luaState, context);
    bindArchiveList(archiveTable, luaState, context);
    bindArchiveExtractFile(archiveTable, luaState, context);
    bindArchiveReadText(archiveTable, context);
    return archiveTable;
}

}  // namespace beez::plugin::lua
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
