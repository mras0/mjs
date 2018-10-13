#include <sstream>
#include <string>

#include "value.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main( int argc, char* argv[] ) {
  // global setup...

  int result = Catch::Session().run( argc, argv );

  // global clean-up...

  return result;
}
using namespace mjs;
//     undefined, null, boolean, number, string, object

TEST_CASE("value - undefined") {
    REQUIRE(value::undefined.type() == value_type::undefined);
}

TEST_CASE("value - null") {
    REQUIRE(value::null.type() == value_type::null);
}

TEST_CASE("value - boolean") {
    const value f{false};
    const value t{true};
    REQUIRE(f.type() == value_type::boolean);
    REQUIRE(t.type() == value_type::boolean);
    REQUIRE(f.boolean_value() == false);
    REQUIRE(t.boolean_value() == true);
}

TEST_CASE("value - number") {
    REQUIRE(value{0.0}.type() == value_type::number);
    REQUIRE(value{NAN}.type() == value_type::number);
    REQUIRE(value{42.0}.type() == value_type::number);
    REQUIRE(value{0.0}.number_value() == 0);
    REQUIRE(std::isnan(value{NAN}.number_value()));
    REQUIRE(value{42.0}.number_value() == 42);
}

TEST_CASE("value - string") {
    REQUIRE(value{string{""}}.type() == value_type::string);
    REQUIRE(value{string{"Hello"}}.type() == value_type::string);
    REQUIRE(value{string{std::wstring_view{L"test"}}}.type() == value_type::string);
    REQUIRE(string{"test "} + string{"42"} == string{"test 42"});
    string s{"test"};
    s += string{" xx"};
    REQUIRE(s == string{"test xx"});
}

TEST_CASE("value - object") {
    // TODO
}

TEST_CASE("Type Converions") {
    // TODO: Object
    // TODO: to_primitive hint
    REQUIRE(to_primitive(value::undefined) == value::undefined);
    REQUIRE(to_primitive(value::null) == value::null);
    REQUIRE(to_primitive(value{true}) == value{true});
    REQUIRE(to_primitive(value{42.0}) == value{42.0});
    REQUIRE(to_primitive(value{string{"test"}}) == value{string{"test"}});

    REQUIRE(!to_boolean(value::undefined));
    REQUIRE(!to_boolean(value::null));
    REQUIRE(!to_boolean(value{false}));
    REQUIRE(to_boolean(value{true}));
    REQUIRE(!to_boolean(value{NAN}));
    REQUIRE(!to_boolean(value{0.0}));
    REQUIRE(to_boolean(value{42.0}));
    REQUIRE(!to_boolean(value{string{""}}));
    REQUIRE(to_boolean(value{string{"test"}}));

    REQUIRE(value{to_number(value::undefined)} == value{NAN});
    REQUIRE(to_number(value::null) == 0);
    REQUIRE(to_number(value{false}) == 0);
    REQUIRE(to_number(value{true}) == 1);
    REQUIRE(to_number(value{42.0}) == 42.0);
    REQUIRE(to_number(value{string{"42.25"}}) == 42.25);
    REQUIRE(to_number(value{string{"1e80"}}) == 1e80);
    REQUIRE(to_number(value{string{"-60"}}) == -60);
    

    REQUIRE(to_integer(value{-42.25}) == -42);
    REQUIRE(to_integer(value{1.0*(1ULL<<32)}) == (1ULL<<32));

    REQUIRE(to_uint32(0x1'ffff'ffff) == 0xffff'ffff);
    REQUIRE(to_int32(0x1'ffff'ffff) == -1);
    REQUIRE(to_uint16(0x1'ffff'ffff) == 0xffff);

    REQUIRE(to_string(value::undefined) == string{"undefined"});
    REQUIRE(to_string(value::null) == string{"null"});
    REQUIRE(to_string(value{false}) == string{"false"});
    REQUIRE(to_string(value{true}) == string{"true"});
    REQUIRE(to_string(value{NAN}) == string{"NaN"});
    REQUIRE(to_string(value{-0.0}) == string{"0"});
    REQUIRE(to_string(value{+INFINITY}) == string{"Infinity"});
    REQUIRE(to_string(value{-INFINITY}) == string{"-Infinity"});
    REQUIRE(to_string(value{42.25}) == string{"42.25"});
    REQUIRE(to_string(value{string{""}}) == string{""});
    REQUIRE(to_string(value{string{"test"}}) == string{"test"});
}