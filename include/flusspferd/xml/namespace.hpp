// vim:ts=2:sw=2:expandtab:autoindent:filetype=cpp:
/*
Copyright (c) 2008 Aristid Breitkreuz, Ruediger Sonderfeld

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef FLUSSPFERD_XML_NAMESPACE_HPP
#define FLUSSPFERD_XML_NAMESPACE_HPP

#include "../native_object_base.hpp"
#include "../class.hpp"
#include <boost/noncopyable.hpp>
#include <libxml/tree.h>

namespace flusspferd { namespace xml {

class namespace_ : public native_object_base {
public:
  struct class_info : flusspferd::class_info {
    static char const *full_name() { return "XML.Namespace"; }

    static char const *constructor_name() { return "Namespace"; }
    typedef boost::mpl::size_t<3> constructor_arity;

    static object create_prototype();
  };

  namespace_(object const &, call_context &);
  namespace_(object const &, xmlNsPtr ptr);
  ~namespace_();

  static object create(xmlNsPtr ptr);

  xmlNsPtr c_obj() const {
    return ptr;
  }

  static xmlNsPtr c_from_js(object const &o);

protected:
  void trace(tracer &);

private:
  void init();

private: // JS methods
  string to_string();

private: // JS properties
  boost::optional<std::string> get_href();
  void set_href(boost::optional<std::string> const &x);
  boost::optional<std::string> get_prefix();
  void set_prefix(boost::optional<std::string> const &x);

private:
  xmlNsPtr ptr;
};

}}

#endif
