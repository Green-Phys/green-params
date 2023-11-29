/*
 * Copyright (c) 2020-2022 University of Michigan.
 *
 */
#include "green/params/params.h"

#include <catch2/catch_test_macros.hpp>

using namespace std::string_literals;

enum myenum { GREEN, BLACK, YELLOW };

TEST_CASE("Params") {
  SECTION("Test Init") {
    auto p = green::params::params("DESCR");
    REQUIRE(p.description() == "DESCR");
  }

  SECTION("String parser") {
    SECTION("SIMPLE") {
      std::string args  = "test --a 33";
      auto [argc, argv] = green::params::get_argc_argv(args);
      REQUIRE(argc == 3);
    }
    SECTION("Long name") {
      std::string args  = "test    --a \"33 and some space\"";
      auto [argc, argv] = green::params::get_argc_argv(args);
      REQUIRE(argc == 3);
    }
    SECTION("Long name with single quote") {
      std::string args  = "test --a '33 and some space'";
      auto [argc, argv] = green::params::get_argc_argv(args);
      REQUIRE(argc == 3);
    }
    SECTION("Long name with mixed quote") {
      std::string args  = "test --a '33 \"and some\" space'";
      auto [argc, argv] = green::params::get_argc_argv(args);
      REQUIRE(argc == 3);
    }
    SECTION("Unmatched quote error") {
      std::string args = "test --a '33 and some space";
      REQUIRE_THROWS_AS(green::params::get_argc_argv(args), green::params::params_str_parse_error);
    }
  }

  SECTION("Parse Parameters") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test --a 33";
    p.define<int>("a", "A value");
    p.define<int>("b", "B value", 5);
    p.define<int>("c", "C value");
    p.parse(args);
    int  a = p["a"];
    long b = p["b"];
    int  c;
    REQUIRE(a == 33);
    REQUIRE(b == 5);
    REQUIRE_THROWS_AS(c = p["c"], green::params::params_value_error);
  }

  SECTION("Nonexisting INI File") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test --a 33 BLABLABLA";
    p.define<int>("a", "value");
    REQUIRE_THROWS_AS(p.parse(args), green::params::params_inifile_error);
  }

  SECTION("Parse Parameters from File") {
    SECTION("Only with file") {
      auto        p       = green::params::params("DESCR");
      std::string inifile = TEST_PATH + "/test.ini"s;
      std::string args    = "test " + inifile;
      p.define<int>("AA", "value from file");
      p.parse(args);
      int a = p["AA"];
      REQUIRE(a == 123);
    }
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --a 33 BLABLABLA";
    p.define<int>("AA", "value from file");
    p.define<int>("AAA.AA", "value from file section", 5);
    p.parse(args);
    int  a = p["AA"];
    long b = p["AAA.AA"];
    REQUIRE(a == 123);
    REQUIRE(b == 345);
  }

  SECTION("Nonexisting Argument") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test --a 33";
    p.define<int>("a", "value");
    p.parse(args);
    REQUIRE_THROWS_AS(p["b"], green::params::params_notfound_error);
  }

  SECTION("Argument without default value") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test";
    p.define<int>("a", "value");
    p.parse(args);
    REQUIRE_THROWS_AS(p["a"], green::params::params_value_error);
  }

  SECTION("Redefine Parameters from File") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --AA 33 --AAA.AA=4";
    p.define<int>("AA", "value from file");
    p.define<int>("AAA.AA", "value from file section", 5);
    p.parse(args);
    int  a = p["AA"];
    long b = p["AAA.AA"];
    REQUIRE(a == 33);
    REQUIRE(b == 4);
  }

  SECTION("Different Types") {
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + " --STRING.VEC=AA,BB,CC -Z r";
    p.define<std::string>("STRING.X,Y", "value from file");
    p.define<std::string>("XXX,YY,Z", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.define<std::vector<std::string>>("STRING.VEC", "vector value");
    p.define<std::vector<std::string>>("STRING.VEC2", "vector value");
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    p.parse(args);
    std::string              a    = p["STRING.X"];
    std::string              b    = p["STRING.Y"];
    std::vector<std::string> vec  = p["STRING.VEC"];
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
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    REQUIRE_FALSE(p.parse(args));
  }

  SECTION("Add Definition") {
    SECTION("Parse before definitons") {
      auto        p    = green::params::params("DESCR");
      std::string args = "test --A 2 --C 3 --D 4";
      REQUIRE_NOTHROW(p.parse(args));
      p.define<int>("A", "value from command line");
      REQUIRE_NOTHROW(p.build());
      int a = p["A"];
      REQUIRE(a == 2);
    }
    auto        p       = green::params::params("DESCR");
    std::string inifile = TEST_PATH + "/test.ini"s;
    std::string args    = "test " + inifile + "  ";
    p.parse(args);
    p.define<int>("A", "value from command line");
    p.define<std::string>("STRING.X", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    std::string a = p["STRING.X"];
    std::string b = p["STRING.Y"];
    int         A;
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
    p.define<std::string>("STRING.X", "value from file");
    p.define<std::string>("STRING.Y", "value from file section");
    p.parse(args);
    p.define<myenum>("ENUMTYPE", "value from file section", BLACK);
    const green::params::params& p2 = p;
    REQUIRE_THROWS_AS(p2["STRING.X"], green::params::params_notbuilt_error);
    p["STRING.X"];
    REQUIRE_NOTHROW(p2["STRING.X"]);
    REQUIRE(std::string(p2["STRING.X"]) == "123456"s);
  }
#endif
  SECTION("Redefinition") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test -X 12 --TTT 45";
    p.define<int>("X,XXX,ZZZ", "value from file");
    p.define<int>("Y,YYY,WWW", "value from file");
    p.define<int>("A", "non-optional value");
    p.define<int>("K", "optional value", 10);
    REQUIRE_THROWS_AS(p.define<long>("X", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_THROWS_AS(p.define<long>("XXX", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_THROWS_AS(p.define<long>("ZZZ", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_NOTHROW(p.define<int>("X", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("XXX", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("ZZZ", "redefined X"));
    REQUIRE_THROWS_AS(p.define<int>("X,Y", "redefined X"), green::params::params_redefinition_error);
    REQUIRE_NOTHROW(p.define<int>("X,XXX", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("X,XXX,QQQ", "redefined X"));
    REQUIRE_NOTHROW(p.define<int>("Y,TTT", "redefined Y"));
    p.define<int>("A,B", "make optional", 1);
    p.define<int>("M,K", "should still be optional");
    p.parse(args);
    REQUIRE(int(p["X"]) == int(p["QQQ"]));
    REQUIRE(int(p["Y"]) == 45);
    REQUIRE(int(p["A"]) == 1);
    REQUIRE(int(p["K"]) == 10);
  }

  SECTION("Convert default value") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test ";
    p.define<int>("AA", "value from file");
    std::vector<int> def{1, 2, 3, 4};
    p.define<std::vector<int>>("VEC", "value from file", def);
    p.parse(args);
    const auto& p2 = p;
    REQUIRE_THROWS_AS(long(p["AA"]), green::params::params_value_error);
    REQUIRE_THROWS_AS(long(p["AAA"]), green::params::params_notfound_error);
    REQUIRE_THROWS_AS(long(p2["AA"]), green::params::params_value_error);
    REQUIRE_THROWS_AS(long(p2["AAA"]), green::params::params_notfound_error);
    std::vector<long> vec_long = p["VEC"];
    REQUIRE(std::equal(def.begin(), def.end(), vec_long.begin()));
  }

  SECTION("Check help and print don't throw") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test ";
    p.parse(args);
    REQUIRE_NOTHROW(p.help());
    REQUIRE_NOTHROW(p.print());
  }

  SECTION("Params Set") {
    auto        p    = green::params::params("DESCR");
    std::string args = "test";
    p.define<int>("X,XXX,ZZZ", "value 1", 1);
    p.define<int>("Y,YYY,WWW", "value 2", 2);
    p.define<int>("A", "value 3", 3);
    p.define<int>("K", "value 4", 10);
    REQUIRE(p.params_set().size() == 4);
    p.parse(args);
    p.define<int>("X,XXX,QQQ", "redefined X");
    REQUIRE(p.params_set().size() == 4);
    p.parse(args);
    p.define<int>("A,B", "redefined A");
    p.define<int>("C", "define C");
    REQUIRE(p.params_set().size() == 5);
  }
}
