if(TARGET lua)
    return()
endif()

file(GLOB LUA_SOURCES ${THE_FORGE_DIR}/Common_3/ThirdParty/OpenSource/lua/*.c)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${THE_FORGE_DIR}/Common_3/ThirdParty/OpenSource/lua)
