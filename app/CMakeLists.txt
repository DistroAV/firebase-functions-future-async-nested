cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(FirebaseExample VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(${PROJECT_NAME} main.cpp)

# "`firebase_app` is required and must always be listed last"
set(firebase_libs firebase_app_check firebase_auth firebase_firestore firebase_functions firebase_app)
if(APPLE)
  set(ADDITIONAL_LIBS
      gssapi_krb5
      pthread
      "-framework CoreFoundation"
      "-framework Foundation"
      "-framework GSS"
      "-framework Security"
      "-framework SystemConfiguration")
elseif(MSVC)
  set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 iphlpapi psapi userenv)
else()
  set(ADDITIONAL_LIBS pthread)
endif()

set(FIREBASE_SDK_DIR ${CMAKE_SOURCE_DIR}/libs/firebase_cpp_sdk)
add_subdirectory(${FIREBASE_SDK_DIR} EXCLUDE_FROM_ALL)
target_include_directories(${PROJECT_NAME} PRIVATE ${FIREBASE_SDK_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${firebase_libs} ${ADDITIONAL_LIBS})
