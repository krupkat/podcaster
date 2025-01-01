// SPDX-FileCopyrightText: 2025 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sstream>
#include <string_view>

#include <pugixml.hpp>

namespace xml {

class XHTMLStripWalker : public pugi::xml_tree_walker {
 public:
  XHTMLStripWalker() {}

  bool for_each(pugi::xml_node& node) override;

  bool end(pugi::xml_node& node) override;

  std::string ResultShort() const { return result_short_.str(); }
  std::string ResultLong() const { return result_long_.str(); }

 private:
  void SaveUtf8(const char* str);
  void Put(char32_t code_point, std::string_view bytes);

  bool first_char_ = true;
  bool summary_ = true;
  int character_count_ = 0;
  int consecutive_newlines_ = 0;

  std::stringstream result_short_;
  std::stringstream result_long_;
};

}  // namespace xml