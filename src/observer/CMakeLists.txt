MESSAGE(STATUS "This is CMAKE_CURRENT_SOURCE_DIR dir " ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE ALL_SRC *.cpp *.c)
SET(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
MESSAGE("MAIN SRC: " ${MAIN_SRC})
FOREACH (F ${ALL_SRC})

    IF (NOT ${F} STREQUAL ${MAIN_SRC})
        SET(LIB_SRC ${LIB_SRC} ${F})
        MESSAGE("Use " ${F})
    ENDIF()

ENDFOREACH (F)

SET(LIBEVENT_STATIC_LINK TRUE)
FIND_PACKAGE(Libevent CONFIG REQUIRED)

# JsonCpp cannot work correctly with FIND_PACKAGE

SET(LIBRARIES common pthread dl libevent::core libevent::pthreads libjsoncpp.a)

# 指定目标文件位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
MESSAGE("Binary directory:" ${EXECUTABLE_OUTPUT_PATH})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
MESSAGE("Archive directory:" ${LIBRARY_OUTPUT_PATH})

ADD_EXECUTABLE(observer ${MAIN_SRC})
TARGET_LINK_LIBRARIES(observer observer_static)

ADD_LIBRARY(observer_static STATIC ${LIB_SRC})

# 为 jieba_util.cpp 禁用未使用变量警告（limonp 第三方库有未使用变量）
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/common/fulltext/jieba_util.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-unused-variable"
)

# 为 explain_physical_operator.cpp 禁用数组边界和字符串溢出警告（GCC 13 对 std::vector<bool> 的严格检查）
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/sql/operator/explain_physical_operator.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-array-bounds -Wno-stringop-overflow"
)

TARGET_INCLUDE_DIRECTORIES(observer_static PRIVATE ${PROJECT_SOURCE_DIR}/deps/3rd/cppjieba/include ${PROJECT_SOURCE_DIR}/deps/3rd/usr/local/include)
INCLUDE (readline)
MINIOB_FIND_READLINE()
IF (HAVE_READLINE)
    TARGET_LINK_LIBRARIES(observer_static ${READLINE_LIBRARY})
    TARGET_INCLUDE_DIRECTORIES(observer_static PRIVATE ${READLINE_INCLUDE_DIR})
    ADD_DEFINITIONS(-DUSE_READLINE)
    MESSAGE ("observer_static use readline")
ELSE ()
    MESSAGE ("readline is not found")
ENDIF()

SET_TARGET_PROPERTIES(observer_static PROPERTIES OUTPUT_NAME observer)
TARGET_LINK_LIBRARIES(observer_static ${LIBRARIES})

# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚本类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS observer observer_static 
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
