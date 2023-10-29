/*
* Copyright (c) 2020-2022 University of Michigan.
*
*/
#include "green/params/params.h"

#include <catch2/catch_test_macros.hpp>

using namespace std::string_literals;

enum myenum { GREEN, BLACK, YELLOW };

std::pair<int, char**> get_argc_argv(std::string& str) {
  std::string        key;
  std::vector<char*> splits = {(char*)str.c_str()};
  for (int i = 1; i < str.size(); i++) {
    if (str[i] == ' ') {
      str[i] = '\0';
      splits.emplace_back(&str[++i]);
    }
  }
  char** argv = new char*[splits.size()];
  for (int i = 0; i < splits.size(); i++) {
    argv[i] = splits[i];
  }

  return {(int)splits.size(), argv};
}

TEST_CASE("Params") {
  SECTION("Test Init") {
    auto p = green::params::params("DESCR");
    REQUIRE(p.description() == "DESCR");
  }

  SECTION("Parse Parameters") {
    auto        p     = green::params::params("DESCR");
    std::string args  = "test --a 33";
    auto [argc, argv] = get_argc_argv(args);
    p.define<int>("a", "A value");
    p.define<int>("b", "B value", 5);
    p.define<int>("c", "C value");
    p.parse(argc, argv);
    int  a = p["a"];
    long b = p["b"];
    int  c;
    REQUIRE(a == 33);
    REQUIRE(b == 5);
    REQUIRE_THROWS_AS(c = p["c"], green::params::params_value_error);
  }

  SECTION("Nonexisting INI File") {
    auto        p     = green::params::params("DESCR");
    std::string args  = "test --a 33 BLABLABLA";
    auto [argc, argv] = get_argc_argv(args);
    p.define<int>("a", "value");
    REQUIRE_THROWS_AS(p.parse(argc, argv), green::params::params_inifile_error);
  }

  SECTION("Parse Parameters from File") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --a 33 BLABLABLA";
    auto [argc, argv]   = get_argc_argv(args);
    p.define<int>("AA", "value from file");
    p.define<int>("AAA.AA", "value from file section", 5);
    p.parse(argc, argv);
    int  a = p["AA"];
    long b = p["AAA.AA"];
    REQUIRE(a == 123);
    REQUIRE(b == 345);
  }

  SECTION("Nonexisting Argument") {
    auto        p     = green::params::params("DESCR");
    std::string args  = "test --a 33";
    auto [argc, argv] = get_argc_argv(args);
    p.define<int>("a", "value");
    p.parse(argc, argv);
    REQUIRE_THROWS_AS(p["b"], green::params::params_notfound_error);
  }

  SECTION("Redefine Parameters from File") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --AA 33 --AAA.AA=4";
    auto [argc, argv]   = get_argc_argv(args);
    p.define<int>("AA", "value from file");
    p.define<int>("AAA.AA", "value from file section", 5);
    p.parse(argc, argv);
    int  a = p["AA"];
    long b = p["AAA.AA"];
    REQUIRE(a == 33);
    REQUIRE(b == 4);
  }

  SECTION("Different Types") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --STRING.VEC=AA,BB,CC -Z r";
    auto [argc, argv]   = get_argc_argv(args);
    p.define<std::string>("STRING.X,Y", "value from file");
    p.define<std::string>("XXX,YY,Z", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.define<std::vector<std::string> >("STRING.VEC", "vector value");
    p.define<std::vector<std::string> >("STRING.VEC2", "vector value");
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    p.parse(argc, argv);
    std::string a = p["STRING.X"];
    std::string b = p["STRING.Y"];
    std::vector<std::string> vec = p["STRING.VEC"];
    std::vector<std::string> vec2 = p["STRING.VEC2"];
    REQUIRE(vec.size() == 3);
    REQUIRE(vec2.size() == 4);
    REQUIRE(vec[0] == "AA");
    int c;
    int d;
    REQUIRE(a == "123456");
    REQUIRE(b == "ALPHA");
    REQUIRE_NOTHROW(c = p["STRING.X"]);
    REQUIRE(std::string(p["XXX"]) == "r"s);
    REQUIRE(c == 123456);
    REQUIRE_THROWS_AS(d = p["STRING.Y"], green::params::params_convert_error);
  }

  SECTION("Help") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test -?";  // + inifile;
    auto [argc, argv]   = get_argc_argv(args);
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    REQUIRE_FALSE(p.parse(argc, argv));
  }

  SECTION("Add Definition") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + "  ";
    auto [argc, argv]   = get_argc_argv(args);
    p.parse(argc, argv);
    p.define<int>("A", "value from command line");
    p.define<std::string>("STRING.X", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    std::string a = p["STRING.X"];
    std::string b = p["STRING.Y"];
    int A;
    REQUIRE_THROWS_AS(A = p["A"], green::params::params_value_error);
    myenum x = p["ENUMTYPE"];
    REQUIRE(a == "123456");
    REQUIRE(b == "ALPHA");
    REQUIRE(x == BLACK);
  }
#ifndef NDEBUG
  SECTION("Access Not Parsed") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile;
    auto [argc, argv]   = get_argc_argv(args);
    p.define<std::string>("STRING.X", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    REQUIRE_THROWS_AS(p.print(), green::params::params_notparsed_error);
    REQUIRE_THROWS_AS(p.help(), green::params::params_notparsed_error);
  }

  SECTION("Const access Not Built") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile;
    auto [argc, argv]   = get_argc_argv(args);
    p.define<std::string>("STRING.X", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.parse(argc, argv);
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    const green::params::params & p2 = p;
    REQUIRE_THROWS_AS(p2["STRING.X"], green::params::params_notbuilt_error);
    p["STRING.X"];
    REQUIRE_NOTHROW(p2["STRING.X"]);
    REQUIRE(std::string(p2["STRING.X"]) == "123456"s);
  }
#endif
  SECTION("Redefinition") {
    auto        p       = green::params::params("DESCR");
    std::string args    = "test -X 12";
    auto [argc, argv]   = get_argc_argv(args);
    p.define<int>("X,XXX,ZZZ", "value from file");
    p.define<int>("Y,YYY,WWW", "value from file");
    REQUIRE_THROWS_AS(p.define<long>("X", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_THROWS_AS(p.define<long>("XXX", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_THROWS_AS(p.define<long>("ZZZ", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_NOTHROW(p.define<int>("X", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("XXX", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("ZZZ", "redefined X"));
    REQUIRE_THROWS_AS(p.define<int>("X,Y", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_NOTHROW(p.define<int>("X,XXX", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("X,XXX,QQQ", "redefined X"));
    p.parse(argc, argv);
    REQUIRE(int(p["X"]) == int(p["QQQ"]));
  }

}
