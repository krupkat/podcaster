// SPDX-FileCopyrightText: 2025 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/xml_utils.h"

#include <string_view>

#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <utf8cpp/utf8.h>

namespace xml {

constexpr int kPreviewLength = 200;
constexpr char32_t kSpace = 0x20;
constexpr char32_t kNewline = 0x0A;
constexpr char32_t kZeroWidthWordJoiner = 0x2060;
constexpr char32_t kZeroWidthNoBreakSpace = 0xFEFF;
constexpr char32_t kZeroWidthSpace = 0x200B;

bool XHTMLStripWalker::for_each(pugi::xml_node& node) {
  if (node.type() == pugi::node_pcdata) {
    SaveUtf8(node.value());
  }
  if (node.type() == pugi::node_element) {
    if (std::string name = node.name(); name == "p" || name == "br") {
      SaveUtf8("\n");
    }
  }
  return true;
}

bool XHTMLStripWalker::end(pugi::xml_node& node) {
  if (not summary_) {
    result_short_ << "...";
  }
  return true;
}

void XHTMLStripWalker::SaveUtf8(const char* str) {
  utf8::unchecked::iterator cp_iter(str);

  while (true) {
    const char* start = cp_iter.base();
    char32_t code_point = *cp_iter++;
    const char* end = cp_iter.base();

    if (code_point == 0) {
      break;
    }
    if (code_point == kZeroWidthWordJoiner or
        code_point == kZeroWidthNoBreakSpace or code_point == kZeroWidthSpace) {
      continue;
    }

    Put(code_point, {start, end});
  }
}

void XHTMLStripWalker::Put(char32_t code_point, std::string_view bytes) {
  // skip leading whitespace
  if (first_char_) {
    if (code_point == kSpace || code_point == kNewline) {
      return;
    }
    first_char_ = false;
  }

  // max 2 consecutive newlines
  if (code_point == kNewline) {
    consecutive_newlines_++;
    if (consecutive_newlines_ > 2) {
      return;
    }
  } else {
    consecutive_newlines_ = 0;
  }

  // first kPreviewLength characters are the summary
  if (character_count_ > kPreviewLength) {
    summary_ = false;
  }

  if (summary_) {
    result_short_ << bytes;
  } else {
    result_long_ << bytes;
  }
  character_count_++;
}

}  // namespace xml