# Install script for directory: /home/rabuske/NS3/source/ns-3.26/gccxml/GCC

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/rabuske/NS3/source/ns-3.26/gccxml-build/GCC/config_cmake/cmake_install.cmake")
  include("/home/rabuske/NS3/source/ns-3.26/gccxml-build/GCC/libiberty/cmake_install.cmake")
  include("/home/rabuske/NS3/source/ns-3.26/gccxml-build/GCC/libdecnumber/cmake_install.cmake")
  include("/home/rabuske/NS3/source/ns-3.26/gccxml-build/GCC/libcpp/cmake_install.cmake")
  include("/home/rabuske/NS3/source/ns-3.26/gccxml-build/GCC/gcc/cmake_install.cmake")

endif()

