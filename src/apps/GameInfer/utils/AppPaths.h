#pragma once

#include <QString>

namespace AppPaths
{
    [[nodiscard]] QString bundledDataDirectory();
    [[nodiscard]] QString configDirectory();
    [[nodiscard]] QString workspaceDirectory();
    [[nodiscard]] QString separatorModelDirectory();
}
