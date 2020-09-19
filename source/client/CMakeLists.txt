file(GLOB_RECURSE SISD_CLIENT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp )
file(GLOB_RECURSE SISD_CLIENT_COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp )

set(SISD_CLIENT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_executable(SISDClient ${SISD_CLIENT_SRC} ${SISD_CLIENT_COMMON_SRC})

target_include_directories(SISDClient PUBLIC "$<BUILD_INTERFACE:${SISD_CLIENT_INC_DIR}>")

target_link_libraries(SISDClient ${Boost_LIBRARIES})
target_include_directories(SISDClient PUBLIC ${Boost_INCLUDE_DIRS})

# target_link_libraries(SISDClient "${OpenCV_LIBRARIES}")
# target_include_directories(SISDClient PUBLIC "${OpenCV_INCLUDE_DIRS}")

target_link_libraries(SISDClient Threads::Threads dl)