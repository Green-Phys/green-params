/*
 * Copyright (c) 2020-2022 University of Michigan.
 *
 */

#ifndef GREEN_PARAMS_H
#define GREEN_PARAMS_H

#include <argparse/argparse.h>
#include <ini/iniparser.h>

#include <iostream>
#include <memory>
#include <typeindex>

#include "except.h"

namespace green::params {

  /**
   *  Class to store command-line parameter wrapped around argparse parameter entry
   */
  class params_item {
  public:
    /**
     * Create a parameter with specific name, argparse parameter entry, type and optional default value
     *
     * @param name  - name of the parameter
     * @param entry - argparse parameter entry
     * @param argument_type - type of the parameter
     * @param default_value - defalt value (optional)
     */
    params_item(const std::string& name, argparse::Entry* entry, std::type_index argument_type,
                std::optional<std::string> default_value = std::nullopt) :
        name_(name),
        entry_(entry), argument_type_(argument_type) {
      if (default_value.has_value()) {
        default_value_ = default_value.value();
        optional_      = true;
      }
    }

    /**
     * Conversion operator to an arbitrary type T. If T is the same as `argument_type_` return the value stored in
     * `entry_`. Otherwise try to cast value type into T.
     *
     * @tparam T - type of the lhs argument
     * @return representation of the parameter in type T if conversion is possible
     */
    template <typename T>
    operator T() const {
      std::type_index lhs_type = typeid(T);
      if (lhs_type == argument_type_.value() && entry_->has_value()) {
        return entry_->value<T>();
      }
      try {
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
      } catch (std::exception & e) {
        throw params_convert_error(e.what());
      }
      throw params_value_error("No value provided for non-optional parameter " + name_);
    }

    /**
     * Update entry stored value
     * @param new_value - new value to be stored
     */
    void update_entry(const std::string& new_value) { entry_->updata_value(new_value); }

    /**
     * Check if the value has been set in command line parameters
     *
     * @return
     */
    bool is_set() const { return entry_->is_set(); }

  private:
    std::string                    name_;
    argparse::Entry*               entry_;
    std::optional<std::type_index> argument_type_;
    std::optional<std::string>     default_value_;
    bool                           optional_;
  };

  /**
   * Class to store a pairs of defined parameters names and values
   */
  class params {
  public:
    /**
     * Create parameters dictionary
     *
     * @param description - name of the parameters (used for printing)
     */
    params(const std::string& description = "") : parsed_(false), built_(false), description_(description), inifile_(nullptr) {
      inifile_ = &args_.arg_t<std::string>("Parameters INI File");
    }

    /**
     * Define parameter `name` of type T without default value
     *
     * @tparam T - type of the parameter
     * @param name - name of the parameter
     * @param descr - user-friendly description of the parameter
     * @param default_value - optional default value
     */
    template <typename T>
    void define(const std::string& name, const std::string& descr, const std::optional<T>& default_value = std::nullopt) {
      built_                 = false;
      argparse::Entry* entry = &args_.kwarg_t<T>(name, descr);
      if (default_value.has_value()) entry->set_default(default_value.value());
      parameters_map_[name] = std::make_unique<params_item>(name, entry, typeid(T));
    }

    /**
     * @return the user firendly description of parameters dictionary
     */
    std::string        description() const { return description_; }

    /**
     * Subscript operator to access parameter by name. Can be assigned to any type that can accomodate the parameter value.
     *
     * @param param_name - name of the parameter to return
     * @return const reference to the parameter with specific name
     */
    const params_item& operator[](const std::string& param_name) {
#ifndef NDEBUG
      if (!parsed_) throw params_notparsed_error("Parameters has to be parsed before access.");
#endif
      if (!built_) build();
      if (parameters_map_.count(param_name) <= 0) {
        throw params_notfound_error("Parameter " + param_name + " is not found.");
      }
      return *parameters_map_.at(param_name).get();
    }

    /**
     * Parse standard command line arguments. As usual, the first parameter should be program name.
     *
     * @param argc - number of command line arguments
     * @param argv - values of command line arguments
     * @return false if help requested, true otherwise
     */
    bool parse(int argc, char* argv[]) {
      args_.parse(argc, argv, false);
      bool help_requested = build();
      parsed_             = true;
      return !help_requested;
    }

    /**
     * Print all the parameters and their current values
     */
    void print() {
#ifndef NDEBUG
      if (!parsed_) throw params_notparsed_error("Parameters has to be parsed before print.");
#endif
      if (!built_) build();
      std::cout << description_ << std::endl;
      args_.print();
    }

    /**
     * Print a help message
     */
    void help() {
#ifndef NDEBUG
      if (!parsed_) throw params_notparsed_error("Parameters has to be parsed before print.");
#endif
      if (!built_) build();
      std::cout << description_ << std::endl;
      args_.help();
    }

  private:
    bool                                                          parsed_;
    bool                                                          built_;
    argparse::Args                                                args_;
    std::unordered_map<std::string, std::unique_ptr<params_item>> parameters_map_;
    std::string                                                   description_;
    argparse::Entry*                                              inifile_;
    std::vector<std::string>                                      commandline_params_;

    bool                                                          build() {
      bool help_requested = args_.build(false);
      if (help_requested) return true;
      if (inifile_->has_value() && std::filesystem::exists(inifile_->string_value().value())) {
        INI::File ft;
        ft.Load(inifile_->string_value().value(), true);
        for (auto& [name, param] : parameters_map_) {
          params_item& param_val = *param.get();
          if (param_val.is_set()) continue;
          std::string parsed_name(name);
          std::replace(parsed_name.begin(), parsed_name.end(), '.', ':');
          INI::Value val = ft.GetValue(parsed_name);
          if (val.IsValid()) {
            param_val.update_entry(val.AsString());
          }
        }
      } else if (inifile_->has_value()) {
        throw params_inifile_error("First positional argument should be a name of a valid parameter INI file. " +
                                                                                            inifile_->string_value().value());
      }
      built_ = true;
      return false;
    }
  };

}  // namespace green::params

#endif  // GREEN_PARAMS_H
