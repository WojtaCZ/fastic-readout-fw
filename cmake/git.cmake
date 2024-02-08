execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else()
    execute_process(
        COMMAND bash -c "git diff --quiet --exit-code"
        OUTPUT_VARIABLE GIT_DIFF)
    execute_process(
        COMMAND git describe --exact-match --tags
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
    
endif()

execute_process(
        COMMAND bash -c "git status --short"
        OUTPUT_VARIABLE GIT_DIRTY_STAT ERROR_QUIET)

if("${GIT_DIRTY_STAT}" STREQUAL "")
    set(GIT_DIRTY "clean")
else()
    set(GIT_DIRTY "dirty")
endif()

string(TIMESTAMP GIT_DATE "%d.%m. %Y")
string(TIMESTAMP GIT_TIME "%H:%M:%S")


set(VERSION "
#include <string>

namespace GIT{
    const std::string REVISION=\"${GIT_REV}${GIT_DIFF}\";
    const std::string DIRTY=\"${GIT_DIRTY}\";
    const std::string DATE=\"${GIT_DATE}\";
    const std::string TIME=\"${GIT_TIME}\";
    const std::string BRANCH=\"${GIT_BRANCH}\";
    const std::string TAG=\"${GIT_TAG}\";
}")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/git.hpp)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/inc/git.hpp VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/inc/git.hpp "${VERSION}")
endif()