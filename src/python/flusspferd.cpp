#include <boost/python.hpp>
#include <flusspferd.hpp>
using namespace boost::python;
using namespace flusspferd;

namespace {
  void set_property(flusspferd::object &o, std::string const &str, root_string const &rs) {
    o.set_property(str, rs);
  }

  value evaluate_wrapper(std::string const &s) {
    return flusspferd::evaluate(s);
  }
}

BOOST_PYTHON_MODULE(flusspferd) {
  class_<value>("value")
    .def("to_std_string", &value::to_std_string);
  class_<context>("context");
  def("context_create", &context::create); // maybe this should be a module flusspferd.context
  class_<current_context_scope>("current_context_scope", boost::python::init<context>());
  def("gc", &flusspferd::gc);
  def("evaluate", &evaluate_wrapper);
  class_<flusspferd::root_string, boost::noncopyable>("root_string", boost::python::init<std::string>());
  def("global_", &flusspferd::global);
  class_<flusspferd::object>("object");
  def("set_property", &set_property);
}
