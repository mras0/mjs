#include <sstream>
#include <string>

#include <mjs/value.h>
#include <mjs/object.h>
#include <mjs/gc_heap.h>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace mjs;

int main( int argc, char* argv[] ) {
    scoped_gc_heap heap{4096};
    const int ret = Catch::Session().run( argc, argv );
    heap.garbage_collect();
    assert(heap.calc_used() == 0);
    return ret;
}

std::ostream& operator<<(std::ostream& os, const std::vector<string>& vs) {
    os << "{";
    for (size_t i = 0; i < vs.size(); ++i) {
        os << (i ? ", " : "") << vs[i];
    }
    return os << "}";
}

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
}

TEST_CASE("object") {
    auto o = object::make(string{"Object"}, nullptr);
    REQUIRE(o->property_names() == (std::vector<string>{}));
    const auto n = string{"test"};
    const auto n2 = string{"foo"};
    REQUIRE(!o->has_property(n.view()));
    REQUIRE(!o->has_property(n2.view()));
    REQUIRE(o->can_put(n.view()));
    o->put(n, value{42.0});
    REQUIRE(o->has_property(n.view()));
    REQUIRE(o->can_put(n.view()));
    o->put(n2, value{n2}, property_attribute::dont_enum | property_attribute::dont_delete | property_attribute::read_only);
    REQUIRE(o->has_property(n2.view()));
    REQUIRE(!o->can_put(n2.view()));
    REQUIRE(o->get(n.view()) == value{42.0});
    REQUIRE(o->get(n2.view()) == value{n2});
    REQUIRE(o->property_names() == (std::vector<string>{n}));
    o->put(n, value{n});
    REQUIRE(o->get(n.view()) == value{n});
    REQUIRE(o->delete_property(n.view()));
    REQUIRE(!o->has_property(n.view()));
    REQUIRE(o->can_put(n.view()));
    REQUIRE(o->has_property(n2.view()));
    REQUIRE(!o->can_put(n2.view()));
    REQUIRE(!o->delete_property(n2.view()));
    REQUIRE(o->has_property(n2.view()));
    REQUIRE(o->get(n2.view()) == value{n2});
    REQUIRE(o->property_names() == (std::vector<string>{}));
}

TEST_CASE("Type Converions") {
    // TODO: to_primitive hint
    REQUIRE(to_primitive(value::undefined) == value::undefined);
    REQUIRE(to_primitive(value::null) == value::null);
    REQUIRE(to_primitive(value{true}) == value{true});
    REQUIRE(to_primitive(value{42.0}) == value{42.0});
    REQUIRE(to_primitive(value{string{"test"}}) == value{string{"test"}});
    // TODO: Object

    REQUIRE(!to_boolean(value::undefined));
    REQUIRE(!to_boolean(value::null));
    REQUIRE(!to_boolean(value{false}));
    REQUIRE(to_boolean(value{true}));
    REQUIRE(!to_boolean(value{NAN}));
    REQUIRE(!to_boolean(value{0.0}));
    REQUIRE(to_boolean(value{42.0}));
    REQUIRE(!to_boolean(value{string{""}}));
    REQUIRE(to_boolean(value{string{"test"}}));
    REQUIRE(to_boolean(value{object::make(string{"Object"}, nullptr)}));

    REQUIRE(value{to_number(value::undefined)} == value{NAN});
    REQUIRE(to_number(value::null) == 0);
    REQUIRE(to_number(value{false}) == 0);
    REQUIRE(to_number(value{true}) == 1);
    REQUIRE(to_number(value{42.0}) == 42.0);
    REQUIRE(to_number(value{string{"42.25"}}) == 42.25);
    REQUIRE(to_number(value{string{"1e80"}}) == 1e80);
    REQUIRE(to_number(value{string{"-60"}}) == -60);
    // TODO: Object


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
    // TODO: Object
}

TEST_CASE("NumberToString") {
    REQUIRE(to_string(0.005                    ) == string{"0.005"});
    REQUIRE(to_string(0.000005                 ) == string{"0.000005"});
    REQUIRE(to_string((0.000005+1e-10)         ) == string{"0.000005000100000000001"});
    REQUIRE(to_string(0.0000005                ) == string{"5e-7"});
    REQUIRE(to_string(1234.0                   ) == string{"1234"});
    REQUIRE(to_string(1e20                     ) == string{"100000000000000000000"});
    REQUIRE(to_string(1e21                     ) == string{"1e+21"});
// FIXME: Why doesn't this work on w/ g++ 8.2.0 on Linux?
//    REQUIRE(to_string(1.7976931348623157e+308  ) == string{"1.7976931348623157e+308"});
    REQUIRE(to_string(5e-324                   ) == string{"5e-324"});
}
