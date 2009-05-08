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

solution "flusspferd"
  configurations { "Debug", "Release" }
  location "build"

-- Setup that is shared between the compile and usage sections
local libfp_setup = function()
  includedirs {
    "./include",
    "/usr/local/include/boost-1_39"
  }

  defines {
    "JS_THREADSAFE",
    "JS_C_STRINGS_ARE_UTF8",
    _MAKE.esc('FLUSSPFERD_VERSION=\\"0.0\\"')
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
    "src/programms/**.cpp"
  }

  links { "libflusspferd" }

  if os.findlib('edit') then
    defines { 'HAVE_EDITLINE' }
    if os.findheader("editline/history.h") then
      defines { 'HAVE_EDITLINE_HISTORY_H' }
    end
  end

