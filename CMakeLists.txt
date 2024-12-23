cmake_minimum_required (VERSION 2.8)
project(lin_socket_io)

ADD_DEFINITIONS(-DNO_OPENSSL)

set(IncludeLists
	/usr/include/
	/usr/local/include/
	${PROJECT_SOURCE_DIR}/
)  
set(LinkLists
	/usr/lib
	/usr/lib/x86_64-linux-gnu
	/usr/local/lib
	${PROJECT_SOURCE_DIR}/lib/so
)

set(LibLists
	glog
	)

include_directories(${IncludeLists})
link_directories(${LinkLists})

set(SrcLists
	${PROJECT_SOURCE_DIR}/core/queue.cpp
	${PROJECT_SOURCE_DIR}/core/handler.cpp
	${PROJECT_SOURCE_DIR}/core/countdown.cpp
	${PROJECT_SOURCE_DIR}/core/scheduler.cpp
	${PROJECT_SOURCE_DIR}/core/worker.cpp
	${PROJECT_SOURCE_DIR}/core/future.cpp
	${PROJECT_SOURCE_DIR}/core/framework.cpp
	${PROJECT_SOURCE_DIR}/core/socket_helper.cpp
	${PROJECT_SOURCE_DIR}/core/selector.cpp
	${PROJECT_SOURCE_DIR}/core/selector_epoll.cpp
	${PROJECT_SOURCE_DIR}/core/manager.cpp
	${PROJECT_SOURCE_DIR}/core/tcp_connection.cpp
	${PROJECT_SOURCE_DIR}/core/tcp_client.cpp
	${PROJECT_SOURCE_DIR}/core/tcp_listener.cpp
	${PROJECT_SOURCE_DIR}/core/tcp_server.cpp
	${PROJECT_SOURCE_DIR}/core/udp_connection.cpp
	${PROJECT_SOURCE_DIR}/core/udp_client.cpp
	${PROJECT_SOURCE_DIR}/core/udp_listener.cpp
	${PROJECT_SOURCE_DIR}/core/udp_server.cpp
	${PROJECT_SOURCE_DIR}/core/udp_socket.cpp
	${PROJECT_SOURCE_DIR}/core/continue.cpp

	${PROJECT_SOURCE_DIR}/utils/varint.h
	${PROJECT_SOURCE_DIR}/utils/varint.cpp
	${PROJECT_SOURCE_DIR}/utils/date_time.h
	${PROJECT_SOURCE_DIR}/utils/date_time.cpp
	${PROJECT_SOURCE_DIR}/utils/url_parser.h
	${PROJECT_SOURCE_DIR}/utils/url_parser.cpp
	${PROJECT_SOURCE_DIR}/utils/disk_helper.h
	${PROJECT_SOURCE_DIR}/utils/disk_helper.cpp
	${PROJECT_SOURCE_DIR}/utils/base64.h
	${PROJECT_SOURCE_DIR}/utils/base64.cpp
	${PROJECT_SOURCE_DIR}/utils/sha1.h
	${PROJECT_SOURCE_DIR}/utils/sha1.cpp

)

if(CMAKE_BUILD_TYPE MATCHES "Release")
	SET(CMAKE_CXX_FLAGS "-std=c++11 -g -O2 -pthread -fopenmp")
else()
	SET(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -Wno-unused-variable -pthread")
endif()

add_library(lin_socket_io STATIC ${SrcLists})
target_link_libraries(lin_socket_io ${LibLists})

