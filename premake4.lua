-- vim:ts=2:sw=2:expandtab:autoindent:filetype=lua:
--
-- Copyright (c) 2008, 2009 Aristid Breitkreuz, Ash Berlin, Ruediger Sonderfeld
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:

-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.

-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
-- THE SOFTWARE.

dofile "premake4/boost.lua"

newoption {
  trigger = "spidermonkey-include",
  description = "spidermonkey include directory",
  value = "path",
}
newoption {
  trigger = "spidermonkey-libs",
  description = "spidermonkey lib directory",
  value = "path",
}

solution "flusspferd"
  configurations { "Debug", "Release" }
  location "build"

-- Setup that is shared between the compile and usage sections
local libfp_setup = function()
  includedirs {
    "./include",
  }

  defines {
    --"JS_THREADSAFE",
    --"JS_C_STRINGS_ARE_UTF8",
    'FLUSSPFERD_VERSION=\\"0z0\\"'
  }

  local inc_path = path.expand(_OPTIONS['spidermonkey-include'])
  local newpath = os.findheader('js/js-config.h', inc_path)
  if newpath then
    if newpath == inc_path then
      includedirs { inc_path }
    end
  else
    error("js/js-config.h not found",0)
  end

  local lib_path = path.expand(_OPTIONS['spidermonkey-libs'])
  newpath = os.findlib('mozjs', lib_path)
  if newpath then
    links { 'mozjs'}
    libdirs { lib_path }
  else
    error("mozjs lib not found", 0)
  end

  check_boost {
    lib = { 'thread', 'filesystem', 'system' },
    min_version = '1.36.0',
    mandatory = 1
  }

  configuration "not windows"
    defines { "XP_UNIX" }

  configuration "windows"
    defines { "XP_WIN" }
end

project "libflusspferd"
  location "build"
  language "C++"
  kind "SharedLib"

  -- Set the base name different from the project name
  targetname "flusspferd"

  files {
    "src/spidermonkey/**.cpp"
  }



  libfp_setup()

usage "libflusspferd"
  libfp_setup()

project "flusspferd"
  location "build"
  language "C++"
  kind "ConsoleApp"
  files {
    "src/programs/**.cpp"
  }

  defines {
    'INSTALL_PREFIX=\\"' .. _MAKE.esc(project().location) .. '\\"'
  }

  links { "libflusspferd" }

  if os.findlib('edit') then
    links { 'edit' }
    defines { 'HAVE_EDITLINE' }
    if os.findheader("editline/history.h") then
      defines { 'HAVE_EDITLINE_HISTORY_H' }
    end
  end

