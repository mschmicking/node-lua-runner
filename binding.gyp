{
  "variables": {
    # Node's common.gypi dereferences this under OS=="android" but never defaults
    # it, which breaks the build on Android/Termux. Harmless everywhere else.
    "android_ndk_path%": "",
    "lua_sources": [
      "vendor/lua/lapi.c",
      "vendor/lua/lauxlib.c",
      "vendor/lua/lbaselib.c",
      "vendor/lua/lcode.c",
      "vendor/lua/ldblib.c",
      "vendor/lua/ldebug.c",
      "vendor/lua/ldo.c",
      "vendor/lua/ldump.c",
      "vendor/lua/lfunc.c",
      "vendor/lua/lgc.c",
      "vendor/lua/linit.c",
      "vendor/lua/liolib.c",
      "vendor/lua/llex.c",
      "vendor/lua/lmathlib.c",
      "vendor/lua/lmem.c",
      "vendor/lua/loadlib.c",
      "vendor/lua/lobject.c",
      "vendor/lua/lopcodes.c",
      "vendor/lua/loslib.c",
      "vendor/lua/lparser.c",
      "vendor/lua/lstate.c",
      "vendor/lua/lstring.c",
      "vendor/lua/lstrlib.c",
      "vendor/lua/ltable.c",
      "vendor/lua/ltablib.c",
      "vendor/lua/ltm.c",
      "vendor/lua/lundump.c",
      "vendor/lua/lvm.c",
      "vendor/lua/lzio.c"
    ]
  },
  "targets": [
    {
      "target_name": "nodelua",
      "sources": [
        "src/utils.cc",
        "src/luastate.cc",
        "src/nodelua.cc",
        "vendor/lfs/lfs.c",
        "<@(lua_sources)"
      ],
      "include_dirs": [
        "vendor/lua",
        "vendor/lfs",
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      # Lua signals errors with longjmp, which would skip C++ destructors if an
      # exception ever unwound through its frames. Keep exceptions off and use
      # ThrowAsJavaScriptException instead.
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NAPI_VERSION=8"
      ],
      "conditions": [
        [
          "OS=='linux'",
          {
            "defines": [
              "LUA_USE_POSIX",
              "LUA_DL_DLOPEN"
            ],
            "libraries": [
              "-ldl"
            ]
          }
        ],
        [
          "OS=='mac'",
          {
            "defines": [
              "LUA_USE_POSIX",
              "LUA_DL_DYLD"
            ],
            "xcode_settings": {
              "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
              "CLANG_CXX_LIBRARY": "libc++",
              "MACOSX_DEPLOYMENT_TARGET": "10.13"
            }
          }
        ],
        [
          "OS=='win'",
          {
            "defines": [
              "_CRT_SECURE_NO_WARNINGS"
            ]
          }
        ]
      ]
    }
  ]
}
