RAYLIB_CMAKE_CMD =
'cmake .. -DCMAKE_BUILD_TYPE="Release" -DOPENGL_VERSION="4.3" -DBUILD_EXAMPLES="OFF" -DCUSTOMIZE_BUILD="ON" -DSUPPORT_FILEFORMAT_SVG="ON"'

function CheckRaylib()
  if not os.isfile("libs/libraylib.a") then
    os.chdir "external/raylib-5.0"
    os.mkdir "build"
    os.chdir "build"
    os.execute(RAYLIB_CMAKE_CMD)
    os.execute "make --jobs=12"
    os.copyfile("raylib/libraylib.a", "../../../libs")
    os.chdir "../../.."
    return
  end
  print "Raylib found"
end

function include_freetype()
    includedirs "/usr/include/freetype2"
    links "freetype"
end

function include_raylib()
  includedirs "external/raylib-5.0/build/raylib/include"
  links "raylib"
end

workspace "themis"
  location "build/"
  language "c++"
  cppdialect "c++17"
  configurations {"debug", "release", "profile", "addrs"}
  includedirs "external/"

  filter "configurations:debug"
    symbols "On"
    optimize "Off"

  filter "configurations:release"
    optimize "Full"

  filter "configurations:addrs"
    buildoptions {"-fsanitize=address", "-g3"}
    linkoptions {"-fsanitize=address"}
    symbols "On"

  filter "configurations:profile"
    optimize "off"
    symbols "On"
    buildoptions {"-pg"}
    linkoptions { "-pg" }

project "tree_sitter"
  language "c"
  cdialect "c99"
  files { "external/tree_sitter/src/**" }
  includedirs {"external/tree_sitter/include"}
  kind "StaticLib"
  filter "configurations:profile"


project "tree_sitter_json"
  language "c"
  cdialect "c99"
  files { "external/tree_sitter_json/*.c"}
  includedirs {"external/tree_sitter_json"}
  kind "StaticLib"
  filter "configurations:profile"


project "tree_sitter_cpp"
  language "c"
  cdialect "c99"
  files { "external/tree_sitter_cpp/*.c"}
  includedirs {"external/tree_sitter_cpp"}
  kind "StaticLib"
  filter "configurations:profile"

project "tree_sitter_c"
  language "c"
  cdialect "c99"
  files { "external/tree_sitter_c/*.c"}
  includedirs {"external/tree_sitter_c"}
  kind "StaticLib"
  filter "configurations:profile"


function def_cstaticlib()
  kind "StaticLib"
  buildoptions {"-Wall", "-Wextra"}
  language "c"
  cdialect "c11"
end


project "field_fusion"
  def_cstaticlib()
  prebuildcommands {"{COPYDIR} " .. _WORKING_DIR .. "/shaders/ " .. _WORKING_DIR .. "/build/shaders"}
  includedirs "external/field_fusion"
  include_freetype()
  files "external/field_fusion/**"
  links 'm'


function include_field_fusion()
  links "field_fusion"
  links 'm'
end

function include_treesitter()
  includedirs {"external/tree_sitter/include"}
  links { "tree_sitter", "tree_sitter_json", "tree_sitter_cpp", "tree_sitter_c"}
end


project "themis"
  CheckRaylib()

  language "c"
  cdialect "c11"
  kind "WindowedApp"
  files  "src/**"
  includedirs "src/"
 
  buildoptions {"-Wall", "-Wextra"}

  include_raylib()
  include_field_fusion()
  include_freetype()
  include_treesitter()

  postbuildcommands {"{COPYDIR} " .. _WORKING_DIR .. "/resources/ " .. _WORKING_DIR .. "/build/bin/%{cfg.buildcfg}"} 

  libdirs{ "libs/" }


