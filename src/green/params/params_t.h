/*
 * Copyright (c) 2020-2022 University of Michigan.
 *
 */

#ifndef GREEN_PARAMS_H
#define GREEN_PARAMS_H

#include <ini/iniparser.h>

#include <argparse/argparse.hpp>
#include <iostream>
#include <memory>
#include <typeindex>

namespace green::params {

  /**
   *  Class to store command-line  parameters
   */
  class param_t {
  public:
    param_t(const std::string& name, argparse::Entry* entry, std::type_index argument_type,
            std::optional<std::string> default_value = std::nullopt) :
        name_(name),
        entry_(entry), argument_type_(argument_type) {
      if (default_value.has_value()) {
        default_value_ = default_value.value();
        optional_      = true;
      }
    }

    template <typename T>
    operator T() const {
      std::type_index lhs_type = typeid(T);
      if (lhs_type == argument_type_.value() && entry_->has_value()) {
        return entry_->value<T>();
      }
      argparse::ConvertType<T> convert;
      auto                     string_value = entry_->string_value();
      if (string_value.has_value()) {
        convert.convert(string_value.value());
        return convert.data;
      }
      if (default_value_.has_value()) {
        convert.convert(default_value_.value());
        return convert.data;
      }
      throw std::runtime_error("No value provided for non-optional parameter " + name_);
    }

    void update_entry(const std::string& new_value) { entry_->updata_value(new_value); }

    bool is_set() const { return entry_->is_set(); }

  private:
    std::string                    name_;
    argparse::Entry*               entry_;
    std::optional<std::type_index> argument_type_;
    std::optional<std::string>     default_value_;
    bool                           optional_;
  };

  class params_t {
  public:
    params_t(const std::string& description = "") : description_(description), inifile_(nullptr), parsed_(false) {
      inifile_ = &args_.arg_t<std::string>("Parameters INI File");
    }

    template <typename T>
    void define(const std::string& name, const std::string& descr) {
      parsed_               = false;
      parameters_map_[name] = std::make_unique<param_t>(name, &args_.kwarg_t<T>(name, descr), typeid(T));
    }

    template <typename T>
    void define(const std::string& name, const std::string& descr, const T& default_value) {
      parsed_               = false;
      parameters_map_[name] = std::make_unique<param_t>(name, &args_.kwarg_t<T>(name, descr).set_default(default_value),
                                                        typeid(T), argparse::toString(default_value));
    }

    std::string    description() const { return description_; }

    const param_t& operator[](const std::string& param_name) const {
      if (parameters_map_.count(param_name) <= 0) {
        throw std::runtime_error("Parameter " + param_name + " is not found.");
      }
      return *parameters_map_.at(param_name).get();
    }

    bool parse(int argc, char* argv[]) {
      commandline_params_ = std::make_pair(argc, (const char**)argv);
      bool help_requested = args_.parse(argc, argv, false);
      if (help_requested) return false;
      if (inifile_->has_value() && std::filesystem::exists(inifile_->string_value().value())) {
        INI::File ft;
        ft.Load(inifile_->string_value().value(), true);
        for (auto& [name, param] : parameters_map_) {
          param_t& param_val = *param.get();
          if (param_val.is_set()) continue;
          std::string parsed_name(name);
          std::replace(parsed_name.begin(), parsed_name.end(), '.', ':');
          INI::Value val = ft.GetValue(parsed_name);
          if (val.IsValid()) {
            param_val.update_entry(val.AsString());
          }
        }
      } else if (inifile_->has_value()) {
        throw std::runtime_error("First positional argument should be a name of a valid parameter INI file. " +
                                 inifile_->string_value().value());
      }
      parsed_ = true;
      return true;
    }

    void print() { args_.print(); }

  private:
    bool                                                      parsed_;
    argparse::Args                                            args_;
    std::unordered_map<std::string, std::unique_ptr<param_t>> parameters_map_;
    std::string                                               description_;
    argparse::Entry*                                          inifile_;
    std::pair<int, const char**>                              commandline_params_;
  };

}  // namespace green::params

#endif  // GREEN_PARAMS_H
