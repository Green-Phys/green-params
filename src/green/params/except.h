/*
 * Copyright (c) 2020-2022 University of Michigan.
 *
 */

#ifndef GREEN_PARAMS_EXCEPT_H
#define GREEN_PARAMS_EXCEPT_H

#include <stdexcept>

namespace green::params {
  class params_value_error : public std::runtime_error {
  public:
    params_value_error(const std::string& string) : runtime_error(string) {}
  };

  class params_inifile_error : public std::runtime_error {
  public:
    params_inifile_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notparsed_error : public std::runtime_error {
  public:
    params_notparsed_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notbuilt_error : public std::runtime_error {
  public:
    params_notbuilt_error(const std::string& string) : runtime_error(string) {}
  };

  class params_notfound_error : public std::runtime_error {
  public:
    params_notfound_error(const std::string& string) : runtime_error(string) {}
  };

  class params_convert_error : public std::runtime_error {
  public:
    params_convert_error(const std::string& string) : runtime_error(string) {}
  };

}  // namespace green::params

#endif  // GREEN_PARAMS_EXCEPT_H