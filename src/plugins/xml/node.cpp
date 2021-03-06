// vim:ts=2:sw=2:expandtab:autoindent:filetype=cpp:
/*
Copyright (c) 2008, 2009 Aristid Breitkreuz, Ash Berlin, Ruediger Sonderfeld

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated nodeation files (the "Software"), to deal
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

#include "node.hpp"
#include "document.hpp"
#include "html_document.hpp"
#include "text.hpp"
#include "namespace.hpp"
#include "reference.hpp"
#include "processing_instruction.hpp"
#include "attribute.hpp"
#include "flusspferd/string.hpp"
#include "flusspferd/create.hpp"
#include "flusspferd/tracer.hpp"
#include "flusspferd/exception.hpp"
#include "flusspferd/local_root_scope.hpp"

using namespace flusspferd;
using namespace flusspferd::xml;

object node::create(xmlNodePtr ptr) {
  if (!ptr)
    return object();

  if (ptr->_private)
    return *static_cast<object*>(ptr->_private);

  switch (ptr->type) {
  case XML_DOCUMENT_NODE:
    return create_native_object<document>(object(), xmlDocPtr(ptr));
  case XML_HTML_DOCUMENT_NODE:
    return create_native_object<html_document>(object(), htmlDocPtr(ptr));
  case XML_TEXT_NODE:
    return create_native_object<text>(object(), ptr);
  case XML_COMMENT_NODE:
    return create_native_object<comment>(object(), ptr);
  case XML_CDATA_SECTION_NODE:
    return create_native_object<cdata_section>(object(), ptr);
  case XML_ENTITY_REF_NODE:
    return create_native_object<reference_>(object(), ptr);
  case XML_PI_NODE:
    return create_native_object<processing_instruction>(object(), ptr);
  case XML_ATTRIBUTE_NODE:
    return create_native_object<attribute_>(object(), xmlAttrPtr(ptr));
  default:
    return create_native_object<node>(object(), ptr);
  }
}

void node::create_all_children(
    xmlNodePtr ptr, bool children, bool properties)
{
  if (ptr->type == XML_ENTITY_REF_NODE)
    return;

  if (ptr->type == XML_ELEMENT_NODE || ptr->type == XML_ATTRIBUTE_NODE) {
    if (ptr->ns)
      namespace_::create(ptr->ns);
  }

  if (ptr->type == XML_ELEMENT_NODE) {
    xmlNsPtr ns = ptr->nsDef;
    while (ns) {
      namespace_::create(ns);
      ns = ns->next;
    }
  }

  if (children) {
    for (xmlNodePtr child = ptr->children; child; child = child->next)
      create(child);
  }

  if (properties && ptr->type == XML_ELEMENT_NODE) {
    for (xmlAttrPtr prop = ptr->properties; prop; prop = prop->next) {
      create(xmlNodePtr(prop));
      for (xmlNodePtr child = prop->children; child; child = child->next)
        create(child);
    }
  }
}

xmlNodePtr node::c_from_js(object const &obj) {
  if (obj.is_null())
    return 0;

  try {
    return flusspferd::get_native<node>(obj).c_obj();
  } catch (std::exception&) {
    return 0;
  }
}

node::node(object const &obj, xmlNodePtr ptr)
  : base_type(obj), ptr(ptr)
{
  ptr->_private = static_cast<object*>(this);
  create_all_children(ptr);

  init();
}

node::node(object const &obj, call_context &x)
  : base_type(obj)
{
  local_root_scope scope;

  xmlDocPtr doc = document::c_from_js(x.arg[0].to_object());

  std::size_t offset = !doc ? 0 : 1;

  value name_v(x.arg[offset + 0]);
  if (!name_v.is_string())
    throw exception("Could not create XML node: name has to be a string");
  xmlChar const *name = (xmlChar const *) name_v.to_string().c_str();

  xmlChar *prefix = 0;

  if (xmlChar const *colon = xmlStrchr(name, ':')) {
    std::size_t len = colon - name;
    prefix = xmlStrsub(name, 0, len);
    name += len + 1;
  }

  try {
    value ns_v(x.arg[offset + 1]);

    xmlNsPtr ns = 0;
    if (ns_v.is_object()) {
      ns = namespace_::c_from_js(ns_v.get_object());
      if (prefix) {
        if (ns->prefix) xmlFree((xmlChar*) ns->prefix);
        ns->prefix = prefix;
        prefix = 0; // don't free
      }
    }

    ptr = xmlNewDocNode(doc, ns, name, 0);

    if (!ptr)
      throw exception("Could not create XML node");

    ptr->_private = static_cast<object*>(this);

    if (ns_v.is_string()) {
      ns = xmlNewNs(ptr, (xmlChar const *) ns_v.get_string().c_str(), prefix);
      namespace_::create(ns);
      ptr->ns = ns;
    }
  } catch (...) {
    if (prefix) xmlFree(prefix);
    throw;
  }

  if (prefix) xmlFree(prefix);

  init();
}

node::~node() {
  if (ptr && ptr->_private == static_cast<object*>(this)) {
    xmlUnlinkNode(ptr);

    xmlNodePtr x = ptr->children;
    while (x) {
      x->parent = 0;
      x = x->next;
    }
    ptr->children = 0;
    ptr->last = 0;

    if (ptr->type == XML_ELEMENT_NODE) {
      ptr->properties = 0;
      ptr->nsDef = 0;
    }

    if (ptr->type == XML_ELEMENT_NODE || ptr->type == XML_ATTRIBUTE_NODE)
      ptr->ns = 0;

    xmlFreeNode(ptr);
  }
}

void node::init() {
}

static void trace_children(tracer &trc, xmlNodePtr ptr) {
  while (ptr) {
    trc("node-children", *static_cast<object*>(ptr->_private));
    ptr = ptr->next;
  }
}

static void trace_parents(tracer &trc, xmlNodePtr ptr) {
  while (ptr) {
    trc("node-parent", *static_cast<object*>(ptr->_private));
    ptr = ptr->parent;
  }
}

void node::trace(tracer &trc) {
  trc("node-self", *static_cast<object*>(ptr->_private));

  if (ptr->type != XML_ENTITY_REF_NODE)
    trace_children(trc, ptr->children);

  trace_parents(trc, ptr->parent);

  if (ptr->doc)
    trc("node-doc", *static_cast<object*>(ptr->doc->_private));

  if (ptr->next)
    trc("node-next", *static_cast<object*>(ptr->next->_private));

  if (ptr->prev)
    trc("node-prev", *static_cast<object*>(ptr->prev->_private));

  if (ptr->type == XML_ELEMENT_NODE && ptr->properties)
    trc("node-properties", *static_cast<object*>(ptr->properties->_private));

  if (ptr->type == XML_ELEMENT_NODE || ptr->type == XML_ATTRIBUTE_NODE) {
    if (ptr->ns)
      trc("node-ns", *static_cast<object*>(ptr->ns->_private));
  }

  if (ptr->type == XML_ELEMENT_NODE) {
    xmlNsPtr nsDef = ptr->nsDef;
    while (nsDef) {
      trc("node-nsDef", *static_cast<object*>(nsDef->_private));
      nsDef = nsDef->next;
    }
  }
}

void node::set_name(std::string const &s) {
  xmlNodeSetName(ptr, (xmlChar const *) s.c_str());
}

std::string node::get_name() {
  if (!ptr->name)
    return std::string();
  else
    return std::string((char const *) ptr->name);
}

void node::set_lang(std::string const &x) {
  xmlNodeSetLang(ptr, (xmlChar const *) x.c_str());
  create_all_children(ptr, false, true);
}

std::string node::get_lang() {
  xmlChar const *lang = xmlNodeGetLang(ptr);
  if (!lang)
    return std::string();
  else
    return std::string((char const *) lang);
}

void node::set_content(boost::optional<std::string> const &x) {
  xmlNodeSetContent(ptr, 0);
  if (x)
    xmlNodeAddContent(ptr, (xmlChar const *) x->c_str());
  create_all_children(ptr, true, false);
}

boost::optional<std::string> node::get_content() {
  xmlChar *content = xmlNodeGetContent(ptr);
  if (!content) {
    return boost::none;
  } else {
    try {
      std::string result((char const *) content);
      xmlFree(content);
      return result;
    } catch (...) {
      xmlFree(content);
      throw;
    }
  }
}

object node::get_parent() {
  return create(ptr->parent);
}

void node::set_parent(object new_parent) {
  if (new_parent.is_null()) {
    if (ptr->parent) {
      if (ptr->type == XML_ATTRIBUTE_NODE) {
        if (ptr->parent->type == XML_ELEMENT_NODE && !ptr->prev)
          ptr->parent->properties = 0;
      } else {
        ptr->parent->last = ptr->prev;
        if (!ptr->prev)
          ptr->parent->children = 0;
      }

      if (ptr->prev) {
        ptr->prev->next = 0;
        ptr->prev = 0;
      }
      ptr->parent = 0;
      xmlSetListDoc(ptr, 0);
    }
    return;
  }
  
  xmlNodePtr parent = c_from_js(new_parent);

  if (!parent)
    return;

  xmlNodePtr old_parent = ptr->parent;
  if (ptr->prev)
    ptr->prev->next = 0;
  ptr->prev = 0;
  if (ptr->type == XML_ATTRIBUTE_NODE) {
    if (parent->type != XML_ELEMENT_NODE) {
      ptr->parent = parent;
      xmlAttrPtr ptr = xmlAttrPtr(this->ptr);
      if (xmlAttrPtr last = parent->properties) {
        while (last->next)
          last = last->next;
        last->next = ptr;
        ptr->prev = last;
      } else {
        parent->properties = ptr;
        ptr->prev = 0;
      }
      while (ptr->next) {
        ptr = ptr->next;
        ptr->parent = parent;
      }
    }
  } else {
    ptr->parent = parent;
    if (old_parent) {
      old_parent->last = ptr->prev;
      if (!ptr->prev) 
        old_parent->children = 0;
    }
    if (parent->last) {
      ptr->prev = parent->last;
      parent->last->next = ptr;
    } else {
      ptr->prev = 0;
      parent->children = ptr;
      parent->last = ptr;
    }
    while (parent->last->next) {
      parent->last = parent->last->next;
      parent->last->parent = parent;
    }
  }
  xmlSetListDoc(ptr, parent->doc);
}

object node::get_next_sibling() {
  return create(ptr->next);
}

void node::set_next_sibling(object new_next) {
  if (new_next.is_null()) {
    if (ptr->parent)
      if (ptr->type != XML_ATTRIBUTE_NODE)
        ptr->parent->last = ptr;
    ptr->next = 0;
  }
  
  xmlNodePtr next = c_from_js(new_next);

  if (!next)
    return;

  ptr->next = next;
  if (xmlNodePtr np = next->prev) {
    if (np->parent)
      if (next->type != XML_ATTRIBUTE_NODE)
        np->parent->last = np;
    np->next = 0;
  } else if (next->parent) {
    if (next->type == XML_ATTRIBUTE_NODE) {
      if (next->parent->type == XML_ELEMENT_NODE)
        next->parent->properties = 0;
    } else {
      next->parent->children = 0;
      next->parent->last = 0;
    }
  }
  next->prev = ptr;
  if (next->type != XML_ATTRIBUTE_NODE) {
    while (next) {
      next->parent = ptr->parent;
      if (!next->next && next->parent)
        next->parent->last = next;
      next = next->next;
    }
  }
  xmlSetListDoc(ptr->next, ptr->doc);
}

object node::get_previous_sibling() {
  return create(ptr->prev);
}

void node::set_previous_sibling(object new_prev) {
  if (new_prev.is_null()) {
    if (ptr->parent) {
      if (ptr->type == XML_ATTRIBUTE_NODE) {
        if (ptr->parent->type == XML_ELEMENT_NODE)
          ptr->parent->properties = xmlAttrPtr(ptr);
      } else {
        ptr->parent->children = ptr;
      }
    }
    ptr->prev = 0;
  }
    
  xmlNodePtr prev = c_from_js(new_prev);

  if (!prev)
    return;

  ptr->prev = prev;
  if (xmlNodePtr pn = prev->next) {
    if (pn->parent) {
      if (pn->type == XML_ATTRIBUTE_NODE) {
        if (ptr->parent->type == XML_ELEMENT_NODE)
          ptr->parent->properties = xmlAttrPtr(pn);
      } else {
        pn->parent->children = pn;
      }
    }
    pn->prev = 0;
  } else if (prev->parent) {
    if (prev->type != XML_ATTRIBUTE_NODE) {
      prev->parent->children = 0;
      prev->parent->last = 0;
    } else {
      if (ptr->parent->type == XML_ELEMENT_NODE)
        prev->parent->properties = 0;
    }
  }
  prev->next = ptr;
  while (prev) {
    xmlSetTreeDoc(prev, ptr->doc);
    prev->parent = ptr->parent;
    if (!prev->prev && prev->parent)  {
      if (prev->type == XML_ATTRIBUTE_NODE) {
        if (prev->parent->type == XML_ELEMENT_NODE)
          prev->parent->properties = xmlAttrPtr(prev);
      } else {
        prev->parent->children = prev;
      }
    }
    prev = prev->prev;
  }
}

object node::get_first_child() {
  return create(ptr->type != XML_ENTITY_REF_NODE ? ptr->children : 0);
}

void node::set_first_child(object first_child) {
  if (first_child.is_null()) {
    xmlNodePtr old = ptr->children;
    while (old) {
      old->parent = 0;
      old = old->next;
    }
    ptr->children = 0;
    ptr->last = 0;
  }
    
  xmlNodePtr child = c_from_js(first_child);

  if (!child)
    return;

  xmlNodePtr old = ptr->children;
  while (old) {
    old->parent = 0;
    old = old->next;
  }
  ptr->children = child;
  child->parent = ptr;
  while (child->next) {
    child = child->next;
    child->parent = ptr;
  }
  ptr->last = child;
}

object node::get_last_child() {
  return create(ptr->type != XML_ENTITY_REF_NODE ? ptr->last : 0);
}

object node::get_first_sibling() {
  if (ptr->parent)
    return create(ptr->parent->children);

  xmlNodePtr ptr = this->ptr;
  while (ptr->prev)
    ptr = ptr->prev;
  return create(ptr);
}

object node::get_last_sibling() {
  if (ptr->parent)
    return create(ptr->parent->last);

  xmlNodePtr ptr = this->ptr;
  while (ptr->next)
    ptr = ptr->next;
  return create(ptr);
}

object node::get_document() {
  return create(xmlNodePtr(ptr->doc));
}

std::string node::get_type() {
  switch (ptr->type) {
  case XML_ELEMENT_NODE:       return "ELEMENT";               break;
  case XML_ATTRIBUTE_NODE:     return "ATTRIBUTE";             break;
  case XML_TEXT_NODE:          return "TEXT";                  break;
  case XML_CDATA_SECTION_NODE: return "CDATA-SECTION";         break;
  case XML_ENTITY_REF_NODE:    return "ENTITY-REFERENCE";      break;
  case XML_ENTITY_NODE:        return "ENTITY";                break;
  case XML_PI_NODE:            return "PI";                    break;
  case XML_COMMENT_NODE:       return "COMMENT";               break;
  case XML_DOCUMENT_NODE:      return "DOCUMENT";              break;
  case XML_DOCUMENT_TYPE_NODE: return "DOCUMENT-TYPE";         break;
  case XML_DOCUMENT_FRAG_NODE: return "DOCUMENT-FRAGMENT";     break;
  case XML_NOTATION_NODE:      return "NOTATION";              break;
  case XML_HTML_DOCUMENT_NODE: return "HTML-DOCUMENT";         break;
  case XML_DTD_NODE:           return "DTD";                   break;
  case XML_ELEMENT_DECL:       return "ELEMENT-DECLARATION";   break;
  case XML_ATTRIBUTE_DECL:     return "ATTRIBUTE-DECLARATION"; break;
  case XML_ENTITY_DECL:        return "ENTITY-DECLARATION";    break;
  case XML_NAMESPACE_DECL:     return "NAMESPACE-DECLARATION"; break;
  case XML_XINCLUDE_START:     return "XINCLUDE-START";        break;
  case XML_XINCLUDE_END:       return "XINCLUDE-END";          break;
  default:                     return "OTHER";                 break;
  }
}

object node::get_namespace() {
  if (ptr->type != XML_ELEMENT_NODE && ptr->type != XML_ATTRIBUTE_NODE)
    return object();
  return namespace_::create(ptr->ns);
}

void node::set_namespace(object new_ns) {
  if (new_ns.is_null()) {
    ptr->ns->context = 0;
    ptr->ns = 0;
    return;
  }

  xmlNsPtr ns = namespace_::c_from_js(new_ns);
  if (!ns)
    return;
  ptr->ns = ns;
}

object node::get_namespaces() {
  if (ptr->type != XML_ELEMENT_NODE && ptr->type != XML_ATTRIBUTE_NODE)
    return object();

  xmlNsPtr *nsList = xmlGetNsList(ptr->doc, ptr);

  try {
    object array = create_array();

    if (!nsList)
      return array;

    for (xmlNsPtr *p = nsList; *p; ++p) {
      array.call("push", namespace_::create(*p));
    }

    xmlFree(nsList);

    return array;
  } catch (...) {
    if (nsList) xmlFree(nsList);
    throw;
  }
}

object node::get_first_attribute() {
  return create(ptr->type == XML_ELEMENT_NODE
                  ? xmlNodePtr(ptr->properties)
                  : 0);
}

void node::set_first_attribute(object attribute) {
  if (ptr->type != XML_ELEMENT_NODE)
    return;

  if (attribute.is_null()) {
    ptr->properties = 0;
  } else {
    xmlNodePtr attr = c_from_js(attribute);
    if (attr && attr->type == XML_ATTRIBUTE_NODE) {
      ptr->properties = xmlAttrPtr(attr);
      attr->parent = ptr;
    }
  }
}

object node::copy(bool recursive) {
  xmlNodePtr copy = xmlCopyNode(ptr, recursive);
  if (!copy)
    throw exception("Could not copy XML node");
  return create(copy);
}

void node::unlink() {
  xmlUnlinkNode(ptr);
}

void node::add_content(string const &content) {
  xmlChar const *text = (xmlChar const *) content.c_str();
  xmlNodeAddContent(ptr, text);
  create_all_children(ptr, true, false);
}

void node::add_node(call_context &x) {
  if (ptr->doc) {
    arguments arg;
    arg.push_root(create(xmlNodePtr(ptr->doc)));

    // name
    arg.push_back(x.arg[0]);

    // namespace
    if (x.arg[1].is_string()) {
      value ns = call("searchNamespaceByURI", x.arg[1]);
      if (ns.is_object() && !ns.is_null())
        x.arg[1] = ns;
    }
    arg.push_back(x.arg[1]);
    x.arg = arg;
  }
  object obj = create_native_object<node>(object(), boost::ref(x));
  obj.set_property("parent", *this);
  x.result = obj;
}

void node::add_namespace(call_context &x) {
  value ns = call("searchNamespaceByURI", x.arg[0]);
  if (ns.is_object() && !ns.is_null()) {
    x.result = ns;
    return;
  }

  arguments arg;
  arg.push_back(*this);
  arg.push_back(x.arg[0]); // href
  arg.push_back(x.arg[1]); // prefix
  x.arg = arg;
  object obj = create_native_object<namespace_>(object(), boost::ref(x));
  x.result = obj;
}

void node::add_attribute(call_context &x) {
  arguments arg;
  arg.push_back(*this);
  for (arguments::iterator it = x.arg.begin(); it != x.arg.end(); ++it)
    arg.push_root(*it);
  x.arg = arg;
  x.result = create_native_object<attribute_>(object(), boost::ref(x));
  purge();
}

void node::set_attribute(call_context &x) {
  if (x.arg.size() < 3)
    throw exception("Could not set XML attribute: too few parameters");

  if (!x.arg[1].is_object())
    throw exception("Could not set XML attribute: "
                    "namespace has to be an object");

  find_attribute(x);

  if (x.result.is_null()) {
    add_attribute(x);
  } else {
    object o = x.result.to_object();
    o.set_property("content", x.arg[2]);
  }
}

void node::unset_attribute(call_context &x) {
  find_attribute(x);

  if (!x.result.is_null() && x.result.is_object())
    x.result.get_object().call("unlink");

  x.result = value();
}

void node::find_attribute(call_context &x) {
  local_root_scope scope;

  if (!x.arg[0].is_string())
    throw exception("Could not find XML attribute: name has to be a string");

  xmlChar const *name = (xmlChar const *) x.arg[0].get_string().c_str();

  xmlChar const *ns_href = 0;
  if (x.arg[1].is_string()) {
    ns_href = (xmlChar const *) x.arg[1].get_string().c_str();
  } else if (!x.arg[1].is_undefined_or_null()) {
    xmlNsPtr ns = namespace_::c_from_js(x.arg[1].to_object());
    if (!ns)
      throw exception("Could not find XML attribute: "
                      "no valid namespace specified");
    ns_href = ns->href;
  }

  xmlAttrPtr prop = xmlHasNsProp(ptr, name, ns_href);

  x.result = create(xmlNodePtr(prop));
}

void node::get_attribute(call_context &x) {
  find_attribute(x);

  if (x.result.is_object() && !x.result.is_null()) {
    object o = x.result.get_object();
    x.result = o.get_property("content");
  } else {
    x.result = value();
  }
}

void node::add_child(node &nd) {
  nd.unlink();
  add_child_list(nd);
}

void node::add_child_list(node &nd) {
  nd.set_property("parent", *this);
  purge();
}

string node::to_string() {
  local_root_scope scope;

  string type = get_property("type").to_string();
  string name = get_property("name").to_string();

  return string::concat(string::concat(type, ":"), name);
}

object node::search_namespace_by_prefix(value const &prefix_) {
  local_root_scope scope;
  xmlChar const *prefix = 0; 
  if (!prefix_.is_string() && !prefix_.is_undefined() && !prefix_.is_null())
    throw exception("Could not search for non-string namespace prefix");
  if (prefix_.is_string())
    prefix = (xmlChar const *) prefix_.get_string().c_str();
  xmlNsPtr ns = xmlSearchNs(ptr->doc, ptr, prefix);
  return namespace_::create(ns);
}

object node::search_namespace_by_uri(string const &uri_) {
  xmlChar const *uri = (xmlChar const *) uri_.c_str();
  xmlNsPtr ns = xmlSearchNsByHref(ptr->doc, ptr, uri);
  return namespace_::create(ns);
}

void node::purge() {
  if (ptr->type == XML_ELEMENT_NODE) {
    for (xmlAttrPtr prop = ptr->properties; prop; prop = prop->next) {
      for (xmlAttrPtr prop2 = prop->next; prop2; prop2 = prop2 ->next) {
        xmlChar const *href1 = prop->ns ? prop->ns->href : 0;
        xmlChar const *href2 = prop2->ns ? prop2->ns->href : 0;
        if (bool(href1) != bool(href2))
          continue;
        if (href1 && href1 != href2 && xmlStrcmp(href1, href2))
          continue;
        if (prop->name == 0 || prop2->name == 0)
          continue;
        if (xmlStrcmp(prop->name, prop2->name) == 0)
          xmlUnlinkNode(xmlNodePtr(prop));
      }
    }
  }

  if (!ptr->children)
    return;

  for (xmlNodePtr child = ptr->children->next; child;) {
    xmlNodePtr next = child->next;
    if (child->type == XML_TEXT_NODE && child->prev &&
        child->prev->type == XML_TEXT_NODE)
    {
      xmlNodeAddContent(child->prev, child->content);
      xmlUnlinkNode(child);
    }
    child = next;
  }
}
