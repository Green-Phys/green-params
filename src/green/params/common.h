/*
 * Copyright (c) 2023 University of Michigan
 *
 */
#ifndef GREEN_PARAMS_COMMON_H
#define GREEN_PARAMS_COMMON_H

#include <string>

#include "except.h"

namespace green::params {
  /**
   * Convert string string with command line parameters int [argc, argv] pair
   *
   * @param str string to parse
   * @return return [argc, argv] pair.
   */
  inline std::pair<int, char**> get_argc_argv(std::string& str) {
    std::string        key;
    std::vector<char*> splits    = {(char*)str.c_str()};
    char               dquote    = '"';
    char               squote    = '\'';
    bool               in_squote = false;
    bool               in_dquote = false;
    bool               in_space  = false;
    for (int i = 1; i < str.size(); i++) {
      if (str[i] != ' ' && in_space) {
        in_space = false;
        splits.emplace_back(&str[i]);
      }
      if (str[i] == dquote)
        in_dquote = !in_dquote;
      else if (str[i] == squote)
        in_squote = !in_squote;
      else if (str[i] == ' ' && !in_dquote && !in_squote && !in_space) {
        str[i]   = '\0';
        in_space = true;
      }
    }
    if (in_squote || in_dquote) {
      throw params_str_parse_error("Unmatched quote in arguments string");
    }

    char** argv = new char*[splits.size()];
    for (int i = 0; i < splits.size(); i++) {
      argv[i] = splits[i];
    }

    return {(int)splits.size(), argv};
  }
}  // namespace green::params
#endif  // GREEN_PARAMS_COMMON_H
