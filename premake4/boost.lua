
local boost_code = [[
#include <iostream>
#include <boost/version.hpp>
int main() { std::cout << BOOST_VERSION << std::endl; }
]]


local boost_libpath = {'/usr/lib', '/usr/local/lib', '/opt/local/lib', '/sw/lib', '/lib'}
local boost_cpppath = {'/usr/include', '/usr/local/include', '/opt/local/include', '/sw/include'}

local STATIC_NOSTATIC = 'nostatic'
local STATIC_BOTH = 'both'
local STATIC_ONLYSTATIC = 'onlystatic'

local is_versiontag = "%d+_%d+_?%d%"
local is_threadingtag = "mt"
local is_abitag = "[sgydpn]+"
local toolsets = { 'acc','borland','como','cw','dmc','darwin','gcc','hp_cxx','intel','kylix','msvc','qcc','sun','vacpp' }

newoption {
  trigger='boost-includes',
  description = 'boost include directory',
  value = '/usr/local/include/boost-1_35'
}

newoption {
  trigger='boost-libs',
  description = 'boost lib directory',
  value = '/usr/local/lib/boost-1_35'
}

local function string_to_version(s)
  local version = {s:match('(%d+).(%d+).(%d+)')}
  if #version < 3 then return 0 end
  return version[1]*100000 + version[2]*100 + version[3]
end

local function version_string(version)
  local major = versoin / 1000000
  local minor = version / 100 % 1000
  local minor_minor = version % 100
  if minor_minor == 0 then
    return ("%d_%d"):format(major, minor)
  else
    return ("%d_%d_%d"):format(major, minor, minor_minor)
  end
end

local function libfiles(lib, pattern, lib_paths)
  local result = {}
  for _,lib_path in ipairs(lib_paths) do
    local libname = path.join(lib_path) .. pattern:format('boost_' + lib + '*')
    if os.isfile(libname) then
      table.insert(result, lib_path)
    end
  end
  return result
end

local function get_boost_version_number(self, dir)
  -- silently retrieve the boost version number
  --[[try:
    return self.run_c_code(compiler='cxx', code=boost_code, includes=dir, execute=1, env=self.env.copy(), type='cprogram', compile_mode='cxx', compile_filename='test.cpp')
  except Configure.ConfigurationError, e:
    return -1]]
end

local function _get_compiler()
  local cc = premake[_OPTIONS.cc]

  if not cc then
    local ok, err
    ok, err = premake.checktools()
    if (not ok) then error("Error: " .. err, 0) end

    cc = premake[_OPTIONS.cc]

    -- if still no cc, then we can't continue
    if not cc then error("Error: boost requires a C++ project") end
  end
  return cc
end

local function _try_compile(source, includes)
  local cc,cxx,loc,fname,fh,flags

  cc = _get_compiler()
  cxx = cc.cxx

  loc = project() or { ['location'] = '.' }
  loc = path.join(loc.location, '.premake-build')
  os.mkdir(loc)
  print(loc)

  fname = path.join(loc, "test.cpp")
  fh = assert(io.open(fname, "w"))
  fh:write(source)
  fh:close()

  if #includes then flags = table.concat(cc.getincludedirs(includes), " ")
  else flags = ""
  end
  cmd = ("%s -o %s/a.out %s "):format(cxx, loc, fname) .. flags 
  print(cmd)
  os.execute(cmd)

  --os.rmdir(loc)
end


function boost()
  if not _ACTION or _ACTION == "help" then return end
 
  local cc = _get_compiler()
  _try_compile(boost_code, {'/usr/local/include/boost-1_37/'})

end
