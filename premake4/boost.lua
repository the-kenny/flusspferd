-- helper methods


-- TODO: does this want to be on premake. or something?
function get_shlib_format()
  if os.is("windows") then
    return "%s.dll"
  elseif os.is("macosx") then
    return "lib%s.dylib"
  else
    return "lib%s.so"
  end
end

function get_staticlib_format()
  if os.is("windows") then
    return "%s.lib"
  else
    return "lib%s.a"
  end
end

function table.push(self,...)
  if ... then
    local targs = {...}
    for _,v in pairs(targs) do
      table.insert(self, v)
    end
  end
end


local function to_list(s)
  if type(s) == "table" then
    return pairs(s) 
  end

  if os.is("windows") then
    return s:explode(";")
  else
    return s:explode(":")
  end
end

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

local tag_matchers = {
  version = "^%d+_%d+_?%d%$",
  threading = "^mt$",
  is_abitag = "^[sgydpn]+$",
}
local toolsets = { 'acc','borland','como','cw','dmc','darwin','gcc','hp_cxx','intel','kylix','msvc','qcc','sun','vacpp' }

local logfile

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
  local major = version / 1000000
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
  for _,lib_path in to_list(lib_paths) do
    local libname = path.join(lib_path, pattern:format('boost_' .. lib .. '*'))
    for _,k in ipairs(os.matchfiles(libname)) do
      table.insert(result, k)
    end
  end
  return result
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

local function _try_compile(source, options)
  local cc,cxx,loc,fname,fh,flags

  cc = _get_compiler()
  cxx = cc.cxx

  loc = project() or { ['location'] = '.' }
  loc = path.join(loc.location, '.premake-build')
  os.mkdir(loc)
  local logfile = path.join(loc, "build.log")

  fname = path.join(loc, "test.cpp")
  fh = assert(io.open(fname, "w"))
  fh:write(source)
  fh:close()

  local target = path.join(loc, "premake_test")
  if os.is("windows") then target = target .. ".exe" end


  cmd = {cxx, "-o", target, fname}
  if options.includes and #options.includes then
    table.push(cmd, unpack(cc.getincludedirs(options.includes)))
  end



  table.push(cmd, "2>>", logfile, ">>", logfile )
  cmd = table.concat(cmd, " ")
  fh = assert(io.open(logfile, "w+"))
  fh:write(("Running %s\n"):format(cmd))
  fh:close()
  os.execute(cmd)

  if os.isfile(target) then
    fh = assert(io.open(logfile, "a+"))
    fh:write(("\nCompile succeeded: %s\n"):format(target))
    fh:close()
    return target
  else
    return nil
  end
end

local function _run_cxx_code(source, options)
  local exe = _try_compile(source,options)

  if not exe then error("Error compiling code") end

  return os.capture(exe)
end

local function get_boost_version_number(dir)
  local ok,ver = pcall(_run_cxx_code, boost_code, { includes = {dir} })
  if ok then return tonumber(ver)
  else return -1
  end
end


local function set_default(kw, var, val)
  if kw[var] == nil then
    kw[var] = val
  end
end

--[[
	checks library tags

	see http://www.boost.org/doc/libs/1_35_0/more/getting_started/unix-variants.html 6.1
]]
local function tags_score(tags, kw)
  local score = 0
  local needed_tags = {
    threading = kw.tag_threading,
    abi = kw.tag_abi,
    toolset = kw.tag_toolset,
    version = kw.tag_version
  }

  if kw.tag_toolset == nil then
    local cc = _get_compiler() 
    local toolset = cc.cc

    -- TODO: compiler version support
    needed_tags.toolset = toolset
  end

  local found_tags = {}
  for _,tag in ipairs(tags) do
    for n,p in pairs(tag_matchers) do
      if tag:match(p) then
        found_tags[n] = tag
      end
    end

    for _,p in pairs(toolsets) do
      if tag:match('^' .. p .. '$') then
        found_tags.toolset = tag
      end
    end
  end

  for tagname,val in pairs(needed_tags) do
    if val ~= nil and found_tags[tagname] then
      if found_tags[tagname]:match(val) then
        score = score + kw['score_' .. tagname][1]
      else
        score = score + kw['score_' .. tagname][2]
      end
    end
  end

  return score
end

local function validate_boost(kw)
  local ver = kw.version or ""
  for _,x in pairs({'min_version','max_version','version'}) do
    set_default(kw, x, ver)
  end

  set_default(kw, 'lib', {})
  set_default(kw, 'libpath', boost_libpath)
  set_default(kw, 'cpppath', boost_cpppath)

  set_default(kw, 'tag_abi', '^[^%d]*$')

	set_default(kw, 'score_threading', {10, -10})
	set_default(kw, 'score_abi', {10, -10})
	set_default(kw, 'score_toolset', {1, -1})
	set_default(kw, 'score_version', {100, -100})

	set_default(kw, 'score_min', 0)
	set_default(kw, 'static', STATIC_NOSTATIC)
	set_default(kw, 'found_includes', false)
	set_default(kw, 'min_score', 0)

  set_default(kw, 'env', {})
	set_default(kw, 'errmsg', 'not found')
	set_default(kw, 'okmsg', 'ok')
end

--[[
	check every path in kw['cpppath'] for subdir
	that either starts with boost- or is named boost.

	Then the version is checked and selected accordingly to
	min_version/max_version. The highest possible version number is
	selected!

	If no versiontag is set the versiontag is set accordingly to the
	selected library and CPPPATH_BOOST is set.
]]
local function find_boost_includes(kw)
  local ret
  local boostPath = _OPTIONS["boost-includes"]
  if boostPath then
    -- TODO: expand ~ if need be?
  else
    boostPath = kw.cpppath
  end

	local min_version = string_to_version(kw.min_version or '')
	local max_version = string_to_version(kw.max_version or '')
  if max_version == 0 then max_version = (math.pow(2,30) - 1) end


  local version = 0
  local boost_path

  for _,include_path in to_list(boostPath) do
    local h = os.matchdirs(path.join(include_path, "boost*"))

    for _,path in ipairs(h) do

      -- we found a /boost or /boost-1_37 dir. work out which
      -- Get the last dir
      local last_dir = path:match("[/\\]([^/\\]+)$") or path
      if last_dir == "boost" then
        path = include_path
        ret = get_boost_version_number(path)
      elseif last_dir:startswith("boost-") then
        ret = get_boost_version_number(path)
      end

      if ret ~= -1 and ret >= min_version and ret <= max_version and ret > version then
        boost_path = path
        version = ret
      end
    end
  end
  
  if not version then
    -- As a fallback, just try looking in the default compiler include paths
    ret = get_boost_version_number(path)
    if ret ~= -1 and ret >= min_version and ret <= max_version and ret > version then
      boost_path = nil
      version = ret
    end

    if not version then
      error( ("boost headers not found! (required version min: %s, max: %s)"):format( kw.min_version, kw.max_version))
    end
  end

  local found_version = version_string(version)
  local versiontag = '^' .. found_version .. '$'
  if kw.tag_version == nil then
    kw.tag_version = versiontag
  elseif kw.tag_version ~= versiontag then
		printf('boost header version %s and tag_version %s do not match!', versiontag, kw.tag_version)
  end
  kw.found_includes = true
  kw.env.BOOST_VERSION = found_version
  kw.env.CPPPATH_BOOST = boost_path

  return ("Version %s (%s)"):format(found_version, boost_path)
end

local function find_boost_library(lib, kw)
  local function find_library_from_list(lib, files)
    local pat = ".*boost_(.*)[.][^.]+$";
    local result = { nil,nil }
    local res_score = tonumber(kw.min_score) - 1
    local cur_score = 0;
    for _,file in ipairs(files) do
      local libname = file:match(pat)
      if libname then
        local libtags = {}
        for n,tag in ipairs(libname:explode('-')) do
          if n > 1 then table.insert(libtags, tag) end
        end
        local cur_score = tags_score(libtags, kw)
        if cur_score > res_score then
          result = { libname, file }
          res_score = cur_score
        end
      end
      return unpack(result)
    end
  end


  local ret
  local lib_paths = _OPTIONS["boost-libs"]
  if lib_paths then
    -- TODO: expand ~ if need be?
  else
    lib_paths = kw.libpath
  end

  local libname,file,files

  if kw.static == STATIC_NOSTATIC or kw.static == STATIC_BOTH then
    files = libfiles(lib, get_shlib_format(), lib_paths)
    libname,file = find_library_from_list(lib,files)
  end
  if libname == nil and (kw.static == STATIC_ONLYSTATIC or kw.static == STATIC_BOTH) then
    local pat = get_staticlib_format()
    if os.is("windows") then
      -- Boost's static libs on windows are named libboost foo. most libs on
      -- windows dont have this prefix
      pat = 'lib' .. pat
    end
    files = libfiles(lib, pat, lib_paths)
    libname,file = find_library_from_list(lib,files)
  end
  if libname ~= nil then
    local dir = path.getdirectory(file)
    file = path.getbasename(file)
    if os.is("windows") then 
      libname = "libboost_" .. libname
    else
      libname = "boost_" .. libname
    end
    links { libname }
    libdirs { dir }
    return;
  end
end

--[[
	This should be the main entry point

- min_version
- max_version
- version
- include_path
- lib_path
- lib
- toolsettag   - None or a regexp
- threadingtag - None or a regexp
- abitag       - None or a regexp
- versiontag   - WARNING: you should rather use version or min_version/max_version
- static       - look for static libs (values:
	  'nostatic'   or STATIC_NOSTATIC   - ignore static libs (default)
	  'both'       or STATIC_BOTH       - find static libs, too
	  'onlystatic' or STATIC_ONLYSTATIC - find only static libs
- score_version
- score_abi
- scores_threading
- score_toolset
 * the scores are tuples (match_score, nomatch_score)
   match_score is the added to the score if the tag is matched
   nomatch_score is added when a tag is found and does not match
- min_score
]]
function check_boost(options)
  local loc = project() or { ['location'] = '.' }
  loc = path.join(loc.location, '.premake-build')
  os.mkdir(loc)

  local logfile = path.join(loc, "build.log")

  local ret
  local ok

  if not _ACTION or _ACTION == "help" then return end

  if options == nil then error("Usage: check_boost(options_table)") end

  validate_boost(options)

  if not options.found_includes then
    ok,ret = pcall(find_boost_includes, options)
    if ok then
      if options.env.CPPPATH_BOOST then
        includedirs { options.env.CPPPATH_BOOST } 
      end
    elseif options.mandatory then
      error("Cannot find boost headers!")
    end
  end

  for _,lib in ipairs(options.lib or {}) do
    find_boost_library(lib, options)
  end

end
