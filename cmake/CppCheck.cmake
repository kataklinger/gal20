cmake_minimum_required(VERSION 3.26)

function(AddCppCheck target)
  find_program(CPPCHECK_PATH cppcheck REQUIRED)
  set_target_properties(
    ${target} PROPERTIES
    CXX_CPPCHECK "${CPPCHECK_PATH};--enable=warning;--suppress=missingReturn;--error-exitcode=10")
endfunction()
