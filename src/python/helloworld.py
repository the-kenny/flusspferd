import flusspferd
scope = flusspferd.current_context_scope(flusspferd.context_create())
str = flusspferd.root_string("Hello, World!\n")
flusspferd.gc()
flusspferd.set_property(flusspferd.global_(), "str", str)
v = flusspferd.evaluate("str.replace('World', 'Flusspferd')")
print v.to_std_string()
