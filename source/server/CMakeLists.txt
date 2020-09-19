find_package(InferenceEngine REQUIRED)
find_package(OpenCV REQUIRED)




file(GLOB_RECURSE SISD_SERVER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp )
file(GLOB_RECURSE SISD_SERVER_COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp )

set(SISD_SERVER_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_executable(SISDServer ${SISD_SERVER_SRC} ${SISD_SERVER_COMMON_SRC})

target_include_directories(SISDServer PUBLIC "$<BUILD_INTERFACE:${SISD_SERVER_INC_DIR}>")

target_link_libraries(SISDServer ${Boost_LIBRARIES})
target_include_directories(SISDServer PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(SISDServer "${OpenCV_LIBRARIES}")
target_include_directories(SISDServer PUBLIC "${OpenCV_INCLUDE_DIRS}")

target_link_libraries(SISDServer "${InferenceEngine_LIBRARIES}")
target_include_directories(SISDServer PUBLIC "${InferenceEngine_INCLUDE_DIRS}")

target_link_libraries(SISDServer Threads::Threads dl)