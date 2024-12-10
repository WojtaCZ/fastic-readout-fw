set(GIT_PATH ${CMAKE_SOURCE_DIR}/inc/git.hpp)
	
add_custom_command(
    PRE_BUILD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    TARGET ${PROJECT_NAME}.elf

    COMMAND echo \'/*  This file is automatically gennerated by CMake, do not modify! */ \' > ${GIT_PATH}
    COMMAND echo \'\#include<string>\' >> ${GIT_PATH}
    COMMAND echo \'\' >> ${GIT_PATH}

    COMMAND echo \'namespace git {\' >> ${GIT_PATH}

    COMMAND echo -n  \'    static constexpr std::string revision = \"\' >> ${GIT_PATH}
    COMMAND git log --pretty=format:%h -n 1 >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \'    static constexpr std::string branch = \"\' >> ${GIT_PATH}
    COMMAND git rev-parse --abbrev-ref HEAD | tr -d \"\\n\" >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \'    static constexpr std::string commit_date = \"\' >> ${GIT_PATH}
    COMMAND git log -1 --format='%at' | xargs -I{} date -d @{} +%Y/%m/%d | tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \';    static constexpr std::string commit_time = \"\' >> ${GIT_PATH}
    COMMAND git log -1 --format='%at' | xargs -I{} date -d @{} +%H:%M:%S | tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \';    static constexpr int commit_timestamp = \' >> ${GIT_PATH}
    COMMAND git log -1 --format='%at' | tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\;\' >> ${GIT_PATH}

    COMMAND echo -n \'    static constexpr std::string build_date = \"\' >> ${GIT_PATH}
    COMMAND date -u +%Y/%m/%d | tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \';    static constexpr std::string build_time = \"\' >> ${GIT_PATH}
    COMMAND date -u +%H:%M:%S | tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\"\;\' >> ${GIT_PATH}

    COMMAND echo -n \';    static constexpr int build_timestamp = \' >> ${GIT_PATH}
    COMMAND date -u +%s| tr -d \'\\n\' >> ${GIT_PATH}
    COMMAND echo \'\;\' >> ${GIT_PATH}

    COMMAND echo \'}\' >> ${GIT_PATH}
    
)