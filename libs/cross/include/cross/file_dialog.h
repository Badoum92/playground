#pragma once
#include <filesystem>
#include <string>
#include <utility>

#include <exo/collections/span.h>
#include <exo/option.h>

namespace cross
{
// Extensions filters are pair of (description, filter) like {"Image", "*.png"} for example
Option<std::filesystem::path> file_dialog(exo::Span<const std::pair<std::string, std::string>> extensions = {});
}; // namespace cross
