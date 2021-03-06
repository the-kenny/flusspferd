#include <flusspferd.hpp>
#include <iostream>
#include <ostream>
#include <cmath>

// This function (as seen from Javascript) prints its first parameter on the
// screen.
void print(flusspferd::string const &x) {
    std::cout << x << '\n';
}

// This function prints the contents of 'this' on the screen.
void print_object(flusspferd::object &o) {
    std::cout << o.call("toSource") << '\n';
}

// This function calculates exp(i) where i is an integral number.
double exp_i(int i) {
    return std::exp(i);
}

int main() {
    flusspferd::current_context_scope context_scope(
        flusspferd::context::create());

    flusspferd::object g = flusspferd::global();

    // Create a method of the global object (= global function) with name print
    // that calls print().
    flusspferd::create_native_function(g, "print", &print);

    // Create a global function that calls exp_i().
    flusspferd::create_native_function(g, "exp", &exp_i);

    flusspferd::object p = flusspferd::evaluate("Object.prototype").to_object();

    // Create a method of Object.prototype that calls print_object().
    // The difference between create_native_function and create_native_method is
    // that the latter passes Javascript's 'this' as parameter to the function.
    //
    // (Note that every normal object ultimately has as prototype the value of
    // Object.prototype. However create_native_method and create_native_function
    // create the properties with dont_enum, so doing this is safe.)
    flusspferd::create_native_method(p, "print", &print_object);

    // Print e.
    // (Note that 1.2 is rounded down to 1.)
    flusspferd::evaluate("print(exp(1.2))");

    // Print the contents of a literal object.
    flusspferd::evaluate("({a:1, b:2}).print()");
}
