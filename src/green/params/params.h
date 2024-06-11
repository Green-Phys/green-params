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
#include <unordered_set>

#include "common.h"
#include "except.h"

namespace green::params {

  namespace internal {
    template <typename T>
    struct is_vector_t : std::false_type {};
    template <typename T>
    struct is_vector_t<std::vector<T>> : std::true_type {};
    template <typename T>
    constexpr bool is_vector_v = is_vector_t<T>::value;
    template <typename T>
    constexpr bool is_valid_type =
        is_vector_v<T> || std::is_same_v<std::remove_const_t<T>, std::string> || std::is_arithmetic_v<T> || std::is_enum_v<T>;
  }  // namespace internal

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
    params_item(const std::string& name, argparse::Entry* entry, std::type_index argument_type) :
        entry_(entry), argument_type_(argument_type), optional_(false) {
      std::vector<std::string> names = argparse::split(name);
      name_                          = names[0];
      aka_.insert(aka_.begin(), names.begin() + 1, names.end());
      if (entry->has_value()) {
        optional_ = true;
      }
    }

    /**
     * Conversion operator to an arbitrary type T. If T is the same as `argument_type_` return the value stored in
     * `entry_`. Otherwise try to cast value type into T.
     *
     * @tparam T - type of the lhs argument
     * @return representation of the parameter in type T if conversion is possible
     */
    template <typename T, typename = std::enable_if_t<internal::is_valid_type<T>>>
    operator T() const {
      std::type_index lhs_type = typeid(T);
      if (lhs_type == argument_type_ && entry_->has_value()) {
        return entry_->value<T>();
      }
      T ret;
      try {
        argparse::ConvertType<T> convert;
        auto                     string_value = entry_->string_value();
        if (string_value.has_value()) {
          convert.convert(string_value.value());
          ret = convert.data;
        }
      } catch (std::exception& e) {
        throw params_convert_error(e.what());
      }
      return ret;
    }

    /**
     * \brief Explicit cast to a type T
     * \tparam T type of return value
     * \return return a representation of a parameter in type T
     */
    template <typename T>
    T as() const {
      return T(*this);
    }

    /**
     * \brief Assign a value to parameter item
     * \tparam T - type of a value
     * \param value - to be assigned
     * \return current parameter item
     */
    template <typename T>
    params_item& operator=(const T& value) {
      entry_->update_value(argparse::toString(value));
      return *this;
    }

    /**
     * Update entry stored value
     * @param new_value - new value to be stored
     */
    void update_entry(const std::string& new_value) {
      entry_->clean_error();
      entry_->update_value(new_value);
    }

    /**
     * Check if the value has been set in command line parameters
     *
     * @return true if the value has been set by user
     */
    [[nodiscard]] bool                            is_set() const { return entry_->is_set(); }

    /**
     * \brief Return a primary name of the parameter
     * \return name of a parameter
     */
    [[nodiscard]] const std::string&              name() const { return name_; }

    /**
     * \brief Each parameter could have multiple aliases that we want to get access to
     * \return list of all aliases of a current parameter
     */
    [[nodiscard]] const std::vector<std::string>& aka() const { return aka_; }

    /**
     * \brief Non-const version of function `aka()`
     * \return list of all aliases of a current parameter
     */
    [[nodiscard]] std::vector<std::string>&       aka() { return aka_; }

    /**
     * @return true if parameter has default value
     */
    [[nodiscard]] bool                            is_optional() const { return optional_; };

    /**
     * @return type_index this param_item has been defined with
     */
    [[nodiscard]] std::type_index                 argument_type() const { return argument_type_; }

    /**
     * @return pointer to argument parser entry
     */
    [[nodiscard]] argparse::Entry*                entry() const { return entry_; }

  private:
    std::string                name_;
    std::vector<std::string>   aka_;
    argparse::Entry*           entry_;
    std::type_index            argument_type_;
    std::optional<std::string> default_value_;
    bool                       optional_;

    friend class params;
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
      inifile_ = &args_.arg_t<std::string>("Parameters INI File").set_default("");
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
      if (name.empty()) {
        throw params_empty_name_error("Can not define parameter with an empty name");
      }
      built_                              = false;
      auto [names, redefinied, old_entry] = check_redefiniton<T>(argparse::split(name));
      argparse::Entry* entry              = redefinied ? old_entry : &args_.kwarg_t<T>(name, descr);
      entry->clean_error();
      if constexpr (internal::is_vector_v<T>) entry->multi_argument();
      if (default_value.has_value()) entry->set_default(default_value.value());
      std::shared_ptr<params_item> ptr;
      if (!redefinied) {
        ptr = std::make_shared<params_item>(name, entry, typeid(T));
      } else {
        for (auto curr_name : argparse::split(name)) {
          if (parameters_map_.count(curr_name) > 0) {
            ptr            = parameters_map_[curr_name];
            ptr->optional_ = ptr->optional_ || default_value.has_value();
            break;
          }
        }
      }
      for (auto curr_name : names) {
        parameters_map_[curr_name] = ptr;
        if (redefinied) {
          args_.update_definition(curr_name, entry);
          ptr->aka().push_back(curr_name);
        }
      }
      params_set_.insert(ptr);
    }

    /**
     * @return the user firendly description of parameters dictionary
     */
    std::string  description() const { return description_; }

    /**
     * Subscript operator to access parameter by name. Can be assigned to any type that can accomodate the parameter value.
     *
     * @param param_name - name of the parameter to return
     * @return const reference to the parameter with specific name
     */
    params_item& operator[](const std::string& param_name) {
#ifndef NDEBUG
      if (!parsed_) throw params_notparsed_error("Parameters has to be parsed before access.");
#endif
      if (!built_) build();
      if (parameters_map_.count(param_name) <= 0) {
        throw params_notfound_error("Parameter " + param_name + " is not found.");
      }
      params_item& item = *parameters_map_.at(param_name).get();
      if (!item.is_optional() && !item.is_set() || item.entry()->has_error()) {
        if (item.entry()->has_error()) {
          throw params_value_error("Accessing incorrectly filled parameter '" + param_name + "'\n" + item.entry()->get_error());
        }
        throw params_value_error("Accessing non-optional parameter '" + param_name + "' with no value set.");
      }
      return item;
    }
    /**
     * Subscript operator to access parameter by name. Can be assigned to any type that can accomodate the parameter value.
     *
     * @param param_name - name of the parameter to return
     * @return const reference to the parameter with specific name
     */
    const params_item& operator[](const std::string& param_name) const {
#ifndef NDEBUG
      if (!parsed_) throw params_notparsed_error("Parameters has to be parsed before access.");
      if (!built_) throw params_notbuilt_error("Parameters has to be built before access if passing const params.");
#endif
      if (parameters_map_.count(param_name) <= 0) {
        throw params_notfound_error("Parameter " + param_name + " is not found.");
      }
      const params_item& item = *parameters_map_.at(param_name).get();
      if (!item.is_optional() && !item.is_set() || item.entry()->has_error()) {
        if (item.entry()->has_error()) {
          throw params_value_error("Accessing incorrectly filled parameter '" + param_name + "'\n" + item.entry()->get_error());
        }
        throw params_value_error("Accessing non-optional parameter " + param_name + " with no value set.");
      }
      return item;
    }

    /**
     * Check if parameter value is set
     *
     * @param param_name name of the parameter
     * @return true if the value has been set by user either in command line or in parameter file, false otherwise or if parameter does
     * not exist.
     */
    [[nodiscard]] bool is_set(const std::string& param_name) const {
      if (parameters_map_.count(param_name) <= 0) {
        return false;
      }
      return parameters_map_.at(param_name).get()->is_set();
    }

    /**
     * Parse command line arguments represented as a string. As usual, the first parameter should be program name.
     * @param s - string with command line arguments
     * @return false if help requested, true otherwise
     */
    bool parse(const std::string& s) {
      std::string to_parse = s;
      auto [argc, argv]    = get_argc_argv(to_parse);
      return parse(argc, argv);
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
      parsed_ = true;
      if (parameters_map_.empty() && argc > 2)
        return false;  // we provided command line parameters but haven't defined any them yet
      bool help_requested = build();
      return !help_requested;
    }

    /**
     * Rebuild parameters.
     * @return true if help requested
     */
    bool build() { return build_internal(); }

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

    [[nodiscard]] const std::unordered_set<std::shared_ptr<params_item>>& params_set() const { return params_set_; }

  private:
    bool                                                          parsed_;
    bool                                                          built_;
    argparse::Args                                                args_;
    std::unordered_map<std::string, std::shared_ptr<params_item>> parameters_map_;
    std::unordered_set<std::shared_ptr<params_item>>              params_set_;
    std::string                                                   description_;
    argparse::Entry*                                              inifile_;

    inline bool                                                   build_internal() {
      bool help_requested = args_.build(false);
      if (help_requested) return true;
      if (inifile_->has_value() && !inifile_->string_value().value().empty() &&
          std::filesystem::exists(inifile_->string_value().value())) {
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
      } else if (inifile_->has_value() && !inifile_->string_value().value().empty()) {
        throw params_inifile_error("First positional argument should be a name of a valid parameter INI file. " +
                                                                                     inifile_->string_value().value());
      }
      built_ = true;
      return false;
    }
    template <typename T>
    auto check_redefiniton(const std::vector<std::string>& names) {
      std::vector<std::string> new_names;
      std::type_index          lhs_type = typeid(T);
      argparse::Entry*         p        = nullptr;
      for (auto name : names) {
        if (parameters_map_.find(name) != parameters_map_.end() && parameters_map_[name]->argument_type() != lhs_type) {
          throw params_redefinition_error("Parameter " + name + " has already been defined but has different type.");
        } else if (parameters_map_.find(name) != parameters_map_.end()) {
          if (!p) {
            p = parameters_map_[name]->entry();
          } else if (p != parameters_map_[name]->entry()) {
            throw params_redefinition_error("Two or more aliases of the current parameter " + name +
                                            " has already been defined but point to a different parameter item.");
          }
        } else {
          new_names.push_back(name);
        }
      }
      return std::tuple(new_names, p != nullptr, p);
    }
  };

}  // namespace green::params

#endif  // GREEN_PARAMS_H
