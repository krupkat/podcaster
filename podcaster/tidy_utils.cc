// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/tidy_utils.h"

#include <memory>

#include <spdlog/spdlog.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <tidyenum.h>

namespace tidy {

// Example code:  https://www.html-tidy.org/developer/

std::string ConvertToXHTML(const std::string& input) {
  auto free_tdoc = [](TidyDoc* doc) { tidyRelease(*doc); };
  std::unique_ptr<TidyDoc, decltype(free_tdoc)> tdoc(new TidyDoc{tidyCreate()},
                                                     free_tdoc);

  auto free_tbuf = [](TidyBuffer* buf) { tidyBufFree(buf); };
  std::unique_ptr<TidyBuffer, decltype(free_tbuf)> output(new TidyBuffer{0},
                                                          free_tbuf);
  std::unique_ptr<TidyBuffer, decltype(free_tbuf)> errbuf(new TidyBuffer{0},
                                                          free_tbuf);

  if (not tdoc or not output or not errbuf) {
    spdlog::error("Failed to allocate memory for tidy");
    return {input};
  }

  bool all_ok = true;
  all_ok &= tidyOptSetBool(*tdoc, TidyXhtmlOut, yes) != 0;
  all_ok &= tidyOptSetInt(*tdoc, TidyWrapLen, 0) != 0;
  if (not all_ok) {
    spdlog::error("HTML-tidy configuration error");
    return {input};
  }

  auto ret = tidySetErrorBuffer(*tdoc, errbuf.get());  // Capture diagnostics
  if (ret >= 0) {
    ret = tidyParseString(*tdoc, input.c_str());  // Parse the input
  }
  if (ret >= 0) {
    ret = tidyCleanAndRepair(*tdoc);  // Tidy it up!
  }
  if (ret >= 0) {
    ret = tidyRunDiagnostics(*tdoc);  // Kvetch
  }
  if (ret > 1) {  // If error, force output.
    ret = (tidyOptSetBool(*tdoc, TidyForceOutput, yes) != 0) ? ret : -1;
  }
  if (ret >= 0) {
    ret = tidySaveBuffer(*tdoc, output.get());  // Pretty Print
  }

  if (ret < 0) {
    spdlog::error("XHTML conversion error: {}", ret);
    return {input};
  }

  std::string result(reinterpret_cast<char*>(output->bp), output->size);
  return result;
}
}  // namespace tidy