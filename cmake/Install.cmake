cmake_minimum_required(VERSION 3.26)

include(GNUInstallDirs)

install(
  TARGETS gal20_lib warnings std_lib_ver
  EXPORT gal20_export
  FILE_SET gal20_lib_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gal20)

install(
  EXPORT gal20_export
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gal20/cmake
  NAMESPACE gal::)

install(
  FILES ./cmake/gal20Config.cmake
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gal20/cmake)
