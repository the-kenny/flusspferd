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

#include "flusspferd/exception.hpp"
#include "flusspferd/string.hpp"
#include "flusspferd/root.hpp"
#include "flusspferd/arguments.hpp"
#include "flusspferd/object.hpp"
#include "flusspferd/init.hpp"
#include "flusspferd/current_context_scope.hpp"
#include "flusspferd/implementation/value.hpp"
#include "flusspferd/implementation/init.hpp"
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <js/jsapi.h>

using namespace flusspferd;

namespace {
  std::string exception_message(std::string const &what) {
    std::string ret = what;
    jsval v;
    JSContext *const cx = Impl::current_context();

    if (JS_GetPendingException(cx, &v)) {
      value val = Impl::wrap_jsval(v);
      ret += ": exception `" + val.to_string().to_string() + '\'';

      return ret;
    }

    return ret;
  }
}

class exception::impl {
public:
  impl() {}

  context ctx;
  boost::scoped_ptr<root_value> exception_value;
};

exception::exception(std::string const &what)
  : std::runtime_error(exception_message(what))
{
  boost::shared_ptr<impl> p(new impl);

  p->exception_value.reset(new root_value);
  p->ctx = get_current_context();

  JSContext *ctx = Impl::get_context(p->ctx);

  value &v = *p->exception_value;
  if (JS_GetPendingException(ctx, Impl::get_jsvalp(v))) {
    JS_ClearPendingException(ctx);
  } else {
    try {
      arguments arg(std::vector<value>(1));
      root_value message((string(what)));
      arg[0] = message;
      v = global().call("Error", arg);
    } catch (...) { }
  }

  this->p = p;
}

exception::~exception() throw()
{
  if (p->exception_value) {
    current_context_scope scope(p->ctx);
    p->exception_value.reset();
  }
}

void exception::throw_js() {
  JS_SetPendingException(Impl::current_context(), Impl::get_jsval(*p->exception_value));
}
