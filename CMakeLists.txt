PROJECT(DISPATCHER_TCP)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Source files...
SET(dispatch_main "src/dispatch_main.cpp")
SET(ipclib "src/ipclib.cpp")
SET(cfgfile "src/getcfgfile.cpp")
SET(cfginfo "src/dispatch_cfg.cpp")
SET(chkpids "src/checker_pids.cpp")
SET(socket "src/Socket.cpp")
SET(dispatcher "src/dispatcher.cpp")
SET(connections "src/connections.cpp")
SET(thread_pair "src/thread_pair.cpp")

SET(tuxcli_main "src/tuxcli_main.cpp")

SET(client_main "src/client_main.cpp")

# To add logger, which is on plog dir inside src
INCLUDE_DIRECTORIES("src")

############################################################ JSON PARSER 
#  JSONCPP_INCLUDE_DIRS, where to find header, etc.
#  JSONCPP_LIBRARIES, the libraries needed to use jsoncpp.
#  JSONCPP_FOUND, If false, do not try to use jsoncpp.
#  JSONCPP_INCLUDE_PREFIX, include prefix for jsoncpp
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(jsoncpp REQUIRED)

message("--------------------------------")
message("PROJECT_SOURCE_DIR " ${PROJECT_SOURCE_DIR})
message("CMAKE_SOURCE_DIR   " ${CMAKE_SOURCE_DIR})
message("JSONCPP_INCLUDE_DIRS   " ${JSONCPP_INCLUDE_DIRS})
message("JSONCPP_LIBRARIES      " ${JSONCPP_LIBRARIES})
message("JSONCPP_FOUND          " ${JSONCPP_FOUND})
message("JSONCPP_INCLUDE_PREFIX " ${JSONCPP_INCLUDE_PREFIX})
message("PROJECT_NAME           " ${PROJECT_NAME})
message("--------------------------------")

ADD_EXECUTABLE(dispatcher_tcp ${dispatch_main} ${ipclib} ${cfgfile} ${cfginfo} ${chkpids} ${socket} ${dispatcher} ${connections} ${thread_pair})
# To link libraries needed for the linker
target_link_libraries(dispatcher_tcp ${JSONCPP_LIBRARIES} pthread)

ADD_EXECUTABLE(tuxcli_tcp ${tuxcli_main} ${cfgfile})
target_link_libraries(tuxcli_tcp ${JSONCPP_LIBRARIES})

ADD_EXECUTABLE(testing_client ${client_main} ${cfgfile} ${socket})
target_link_libraries(testing_client ${JSONCPP_LIBRARIES} pthread)
