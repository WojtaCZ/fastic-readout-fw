set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_COLOR_MAKEFILE FALSE)
set(CMAKE_COLOR_DIAGNOSTICS TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

cmake_minimum_required(VERSION 3.6.0)

# Set the project name, otherwise it is based on the root folder name
#set (PROJECT "yournamehere")

# Set the device type
set(DEVICE STM32H753)

# Replace with appropriate linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H753ZITX_FLASH.ld)

# List the source files to be compiled
set(SOURCE_BASE ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
	${SOURCE_BASE}/main.cpp
	${SOURCE_BASE}/clock.cpp
	#${SOURCE_BASE}/status.cpp
	
	#${SOURCE_BASE}/spi.cpp
	#${SOURCE_BASE}/i2c.cpp
	startup/startup_stm32h753zitx.s
)



#
# User setup ends here
#



# If the project name is not set, base it on the root folder name
if (NOT DEFINED PROJECT)
	get_filename_component(PROJECT ${CMAKE_CURRENT_SOURCE_DIR} NAME)
	string(REPLACE " " "_" PROJECT ${PROJECT})
endif (NOT DEFINED PROJECT)

project(${PROJECT} LANGUAGES CXX C ASM VERSION 0.1.0)

# Include toolchain related definitions
include(${CMAKE_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake)

# Set a default build type if none was specified
set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
			STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		"Debug" "Release")
endif()

add_executable(${PROJECT_NAME}.elf ${SOURCES})

set_target_properties(${PROJECT_NAME}.elf PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME}.elf PROPERTIES C_STANDARD 11)
#set_target_properties(${PROJECT_NAME}.elf PROPERTIES COMPILE_FLAGS "-save-temps") 


set(W_ERRORS "-Werror=switch -Werror=return-type -Werror=stringop-overflow -Werror=parentheses -Wfatal-errors" CACHE INTERNAL "Warning to error promotion")
set(W_IGNORED "-Wno-unused-function " CACHE INTERNAL "Supressed warnings")
set(VALIDATION_OPTS "${W_ERRORS} ${W_IGNORED} -Wall -Wextra -Wundef -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wcast-align -Wvla -Wmissing-format-attribute -Wuninitialized -Winit-self -Wdouble-promotion -Wstrict-aliasing -Wno-unused-parameter -pedantic" CACHE INTERNAL "Sources validation options")
set(OTHER_C "-Werror=int-conversion -Werror=incompatible-pointer-types -Werror=implicit-function-declaration" CACHE INTERNAL "Other C specific options")
set(OTHER_CPP "-fno-use-cxa-atexit -Wint-to-pointer-cast -Wno-old-style-cast -Wno-volatile -Weffc++ -Wno-register -fno-exceptions -fno-threadsafe-statics" CACHE INTERNAL "Other CPP specific options")
set(OPTIMIZATIONS "-funsigned-char -funsigned-bitfields -fdiagnostics-color=always -fstack-protector-strong -finline-small-functions -findirect-inlining -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables" CACHE INTERNAL "Optimizations options")
set(LIBS "" CACHE INTERNAL "which libs to use")

set(CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} ${VALIDATION_OPTS} ${OPTIMIZATIONS} ${LIBS} ${OTHER_C}" CACHE INTERNAL "(Project) C Compiler options")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VALIDATION_OPTS} ${OPTIMIZATIONS} ${LIBS} ${OTHER_CPP}" CACHE INTERNAL "(Project) C++ Compiler options")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} " CACHE INTERNAL "(Project) ASM Compiler options")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -specs=nosys.specs -ffreestanding -fdiagnostics-color=always -Wl,--wrap,printf -Wl,--wrap,snprintf -Wl,--wrap,vsnprintf" CACHE INTERNAL "(Project) Project linker options")



target_include_directories(${PROJECT_NAME}.elf PRIVATE 
	${SOURCE_BASE}
	${CMAKE_SOURCE_DIR}/inc/
	${CMAKE_SOURCE_DIR}/lib/
	${CMAKE_SOURCE_DIR}/lib/CMSIS/Include
	${CMAKE_SOURCE_DIR}/lib/CMSIS/Device/ST/STM32H7xx/Include
)

# custom definitions
#add_definitions(-Dabc=123)

include(${CMAKE_SOURCE_DIR}/cmake/linker.cmake)

# Select the device definition
if (DEVICE MATCHES ^STM32H7)
	include(${CMAKE_SOURCE_DIR}/cmake/stm32h7.cmake)
elseif (DEVICE MATCHES ^STM32G4)
	include(${CMAKE_SOURCE_DIR}/cmake/stm32g4.cmake)
elseif (DEVICE MATCHES ^STM32F0)
	include(${CMAKE_SOURCE_DIR}/cmake/stm32f0.cmake)	
elseif (DEVICE MATCHES ^STM32F1)
	include(${CMAKE_SOURCE_DIR}/cmake/stm32f1.cmake)
elseif (DEVICE MATCHES ^STM32F4)
	include(${CMAKE_SOURCE_DIR}/cmake/stm32f4.cmake)
else ()
	message(FATAL_ERROR "Could not find a suitable device cmake definition for the specified device type!")
endif ()

set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

#link_directories(${CMAKE_SOURCE_DIR}/lib/libopencm3/lib/stm32/g4 )
#target_link_libraries(${PROJECT_NAME}.elf -L${CMAKE_SOURCE_DIR}/lib/libopencm3/lib)

add_custom_command(
	POST_BUILD
	TARGET ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
	COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
	COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")


add_custom_command(
	POST_BUILD
	TARGET ${PROJECT_NAME}.elf
	COMMAND echo ""
	COMMAND echo "Resulting section sizes:"
	COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
	COMMENT "Calculating the section sizes")

	
include(${CMAKE_SOURCE_DIR}/cmake/git.cmake)
