/*
 * Copyright (c) 2020-2022 University of Michigan.
 *
 */

#ifndef GREEN_PARAMS_EXCEPT_H
#define GREEN_PARAMS_EXCEPT_H

#include <stdexcept>

namespace green::params {
  class params_str_parse_error : public std::runtime_error {
  public:
    explicit params_str_parse_error(const std::string& string) : runtime_error(string) {}
  };

  class params_redefinition_error : public std::runtime_error {
  public:
    explicit params_redefinition_error(const std::string& string) : runtime_error(string) {}
  };

  class params_value_error : public std::runtime_error {
  public:
    explicit params_value_error(const std::string& string) : runtime_error(string) {}
  };

  class params_inifile_error : public std::runtime_error {
  public:
    explicit params_inifile_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notparsed_error : public std::runtime_error {
  public:
    explicit params_notparsed_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notbuilt_error : public std::runtime_error {
  public:
    explicit params_notbuilt_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notfound_error : public std::runtime_error {
  public:
    explicit params_notfound_error(const std::string& string) : runtime_error(string) {}
  };

  class params_convert_error : public std::runtime_error {
  public:
    explicit params_convert_error(const std::string& string) : runtime_error(string) {}
  };

  class params_empty_name_error : public std::runtime_error {
  public:
    explicit params_empty_name_error(const std::string& string) : runtime_error(string) {}
  };
}  // namespace green::params

#endif  // GREEN_PARAMS_EXCEPT_H
