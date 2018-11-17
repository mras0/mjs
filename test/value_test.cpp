#include <sstream>
#include <string>

#include <mjs/value.h>
#include <mjs/object.h>
#include <mjs/gc_heap.h>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace mjs;

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
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
    gc_heap h{128};
    REQUIRE(value{string{h,""}}.type() == value_type::string);
    REQUIRE(value{string{h,"Hello"}}.type() == value_type::string);
    REQUIRE(value{string{h,std::wstring_view{L"test"}}}.type() == value_type::string);
    REQUIRE(string{h,"test "} + string{h,"42"} == string{h,"test 42"});

    h.garbage_collect();
    assert(h.calc_used() == 0);
}

TEST_CASE("object") {
    gc_heap h{128};
    {
        auto o = object::make(h, string{h, "Object"}, nullptr);
        REQUIRE(o->property_names() == (std::vector<string>{}));
        const auto n = string{h,"test"};
        const auto n2 = string{h,"foo"};
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

    h.garbage_collect();
    assert(h.calc_used() == 0);
}

TEST_CASE("Type Conversions") {
    gc_heap h{1<<8};
    // TODO: to_primitive hint
    REQUIRE(to_primitive(value::undefined) == value::undefined);
    REQUIRE(to_primitive(value::null) == value::null);
    REQUIRE(to_primitive(value{true}) == value{true});
    REQUIRE(to_primitive(value{42.0}) == value{42.0});
    REQUIRE(to_primitive(value{string{h, "test"}}) == value{string{h, "test"}});
    // TODO: Object

    REQUIRE(!to_boolean(value::undefined));
    REQUIRE(!to_boolean(value::null));
    REQUIRE(!to_boolean(value{false}));
    REQUIRE(to_boolean(value{true}));
    REQUIRE(!to_boolean(value{NAN}));
    REQUIRE(!to_boolean(value{0.0}));
    REQUIRE(to_boolean(value{42.0}));
    REQUIRE(!to_boolean(value{string{h,""}}));
    REQUIRE(to_boolean(value{string{h,"test"}}));
    REQUIRE(to_boolean(value{object::make(h, string{h,"Object"}, nullptr)}));

    REQUIRE(value{to_number(value::undefined)} == value{NAN});
    REQUIRE(to_number(value::null) == 0);
    REQUIRE(to_number(value{false}) == 0);
    REQUIRE(to_number(value{true}) == 1);
    REQUIRE(to_number(value{42.0}) == 42.0);
    REQUIRE(to_number(value{string{h,"42.25"}}) == 42.25);
    REQUIRE(to_number(value{string{h,"1e80"}}) == 1e80);
    REQUIRE(to_number(value{string{h,"-60"}}) == -60);
    // TODO: Object


    REQUIRE(to_integer(value{-42.25}) == -42);
    REQUIRE(to_integer(value{1.0*(1ULL<<32)}) == (1ULL<<32));

    REQUIRE(to_uint32(0x1'ffff'ffff) == 0xffff'ffff);
    REQUIRE(to_int32(0x1'ffff'ffff) == -1);
    REQUIRE(to_uint16(0x1'ffff'ffff) == 0xffff);

    REQUIRE(to_string(h, value::undefined) == string{h, "undefined"});
    REQUIRE(to_string(h, value::null) == string{h, "null"});
    REQUIRE(to_string(h, value{false}) == string{h, "false"});
    REQUIRE(to_string(h, value{true}) == string{h, "true"});
    REQUIRE(to_string(h, value{NAN}) == string{h, "NaN"});
    REQUIRE(to_string(h, value{-0.0}) == string{h, "0"});
    REQUIRE(to_string(h, value{+INFINITY}) == string{h, "Infinity"});
    REQUIRE(to_string(h, value{-INFINITY}) == string{h, "-Infinity"});
    REQUIRE(to_string(h, value{42.25}) == string{h, "42.25"});
    REQUIRE(to_string(h, value{string{h, ""}}) == string{h, ""});
    REQUIRE(to_string(h, value{string{h, "test"}}) == string{h, "test"});
    // TODO: Object

    h.garbage_collect();
    assert(h.calc_used() == 0);
}

TEST_CASE("NumberToString") {
    gc_heap h{128};
    REQUIRE(to_string(h, 0.005                    ) == string{h, "0.005"});
    REQUIRE(to_string(h, 0.000005                 ) == string{h, "0.000005"});
    REQUIRE(to_string(h, (0.000005+1e-10)         ) == string{h, "0.000005000100000000001"});
    REQUIRE(to_string(h, 0.0000005                ) == string{h, "5e-7"});
    REQUIRE(to_string(h, 1234.0                   ) == string{h, "1234"});
    REQUIRE(to_string(h, 1e20                     ) == string{h, "100000000000000000000"});
    REQUIRE(to_string(h, 1e21                     ) == string{h, "1e+21"});
// FIXME: Why doesn't this work on w/ g++ 8.2.0 on Linux?
//    REQUIRE(to_string(h, 1.7976931348623157e+308  ) == string{h, "1.7976931348623157e+308"});
    REQUIRE(to_string(h, 5e-324                   ) == string{h, "5e-324"});

    h.garbage_collect();
    assert(h.calc_used() == 0);
}
