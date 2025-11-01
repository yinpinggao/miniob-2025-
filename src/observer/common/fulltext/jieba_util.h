#pragma once
#include <string>
#include <vector>
#include "common/lang/string.h"
#include "common/rc.h"

class JiebaUtil {
public:
  static JiebaUtil &instance();
  RC tokenize(const string &text, vector<string> &tokens);
  static string format_tokens_as_json(const vector<string> &tokens);
private:
  JiebaUtil();
  ~JiebaUtil();
  class Impl;
  Impl *impl_ = nullptr;
};
