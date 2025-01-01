// SPDX-FileCopyrightText: 2025 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/xml_utils.h"

#include <pugixml.hpp>

namespace xml {

constexpr int kPreviewLength = 200;

bool XHTMLStripWalker::for_each(pugi::xml_node& node) {
  if (node.type() == pugi::node_pcdata) {
    Save(node.value());
  }
  if (node.type() == pugi::node_element) {
    if (std::string name = node.name(); name == "p" || name == "br") {
      Put('\n');
    }
  }
  return true;
}

bool XHTMLStripWalker::end(pugi::xml_node& node) {
  if (character_count_ > kPreviewLength) {
    result_short_ << "...";
  }
  return true;
}

void XHTMLStripWalker::Save(const char* str) {
  for (const char* c = str; *c != 0; ++c) {
    Put(*c);
  }
}

void XHTMLStripWalker::Put(char c) {
  // skip leading whitespace
  if (first_char_) {
    if (c == ' ' || c == '\n') {
      return;
    }
    first_char_ = false;
  }

  // max 2 consecutive newlines
  if (c == '\n') {
    consecutive_newlines_++;
    if (consecutive_newlines_ > 2) {
      return;
    }
  } else {
    consecutive_newlines_ = 0;
  }

  // first kPreviewLength characters are the summary
  // split only at whitespace to preserve utf8 correctness
  if (character_count_ > kPreviewLength and (c == ' ' || c == '\n')) {
    summary_ = false;
  }

  if (summary_) {
    result_short_ << c;
  } else {
    result_long_ << c;
  }
  character_count_++;
}

}  // namespace xml