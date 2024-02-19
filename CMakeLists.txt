set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
cmake_minimum_required(VERSION 3.6.0)
#adjust your project name here
#then please adjust binary (elf) names in launch.json accordingly
project(FASTIC LANGUAGES CXX C ASM VERSION 0.1.0)


set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_COLOR_MAKEFILE FALSE)
set(CMAKE_COLOR_DIAGNOSTICS FALSE)

#set(TOOLCHAIN_PREFIX arm-none-eabi-)


# Set a default build type if none was specified
set(default_build_type "Debug")

# if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
#   set(default_build_type "Debug")
# endif()
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
			STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		"Debug" "Release")
endif()

#replace with appropriate linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32H753ZITX_FLASH.ld)

#base path for sources
set(SOURCE_BASE ${CMAKE_SOURCE_DIR}/src)

# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build

set(SOURCES
	# list your c/pp sources here
	# cmake can do source globbing, but it is generally not recommended

	${SOURCE_BASE}/main.cpp
	${SOURCE_BASE}/clock.cpp
	${SOURCE_BASE}/timer.cpp
	${SOURCE_BASE}/status.cpp
	${SOURCE_BASE}/spi.cpp
	startup/startup_stm32h753zitx.s
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})


set_target_properties(${PROJECT_NAME}.elf PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME}.elf PROPERTIES C_STANDARD 11)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
	${SOURCE_BASE}
	${CMAKE_SOURCE_DIR}/inc
	${CMAKE_SOURCE_DIR}/lib/CMSIS/Include
	${CMAKE_SOURCE_DIR}/lib/CMSIS/Device/ST/STM32H7xx/Include


	# ...
)

# custom definitions
add_definitions(-Doh_you_know=0xB00B13S)

set(W_ERRORS "-Werror=switch -Werror=return-type -Werror=stringop-overflow -Werror=parentheses -Wfatal-errors" CACHE INTERNAL "Warning to error promotion")
set(W_IGNORED "-Wno-unused-function " CACHE INTERNAL "Supressed warnings")
set(VALIDATION_OPTS "${W_ERRORS} ${W_IGNORED} -Wall -Wextra -Wundef -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wcast-align -Wvla -Wmissing-format-attribute -Wuninitialized -Winit-self -Wdouble-promotion -Wstrict-aliasing -Wno-unused-parameter -pedantic" CACHE INTERNAL "Sources validation options")
set(OTHER_C "-Werror=int-conversion -Werror=incompatible-pointer-types -Werror=implicit-function-declaration" CACHE INTERNAL "Other C specific options")
set(OTHER_CPP "-fno-use-cxa-atexit -Wint-to-pointer-cast -Wno-old-style-cast -Wno-volatile -Weffc++ -Wno-register -fno-exceptions -fno-threadsafe-statics" CACHE INTERNAL "Other CPP specific options")
set(OPTIMIZATIONS "-funsigned-char -funsigned-bitfields -fdiagnostics-color=always -fstack-protector-strong -finline-small-functions -findirect-inlining -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables" CACHE INTERNAL "Optimizations options")
set(LIBS "--specs=nano.specs --specs=nosys.specs" CACHE INTERNAL "which libs to use")

set(CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} ${VALIDATION_OPTS} ${OPTIMIZATIONS} ${LIBS} ${OTHER_C}" CACHE INTERNAL "(Project) C Compiler options")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VALIDATION_OPTS} ${OPTIMIZATIONS} ${LIBS} ${OTHER_CPP}" CACHE INTERNAL "(Project) C++ Compiler options")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} " CACHE INTERNAL "(Project) ASM Compiler options")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -ffreestanding -fdiagnostics-color=always -Wl,--wrap,printf -Wl,--wrap,snprintf -Wl,--wrap,vsnprintf" CACHE INTERNAL "(Project) Project linker options")

#set your device name
# set(DEVICE STM32F042)
# set(DEVICE STM32F103)
# set(DEVICE STM32F105)
#set(DEVICE STM32F401)
# set(DEVICE STM32F407)
 set(DEVICE STM32H753)
# set(DEVICE STM32F446)
#od any other...
# set(DEVICE STM32G431)

include(${CMAKE_SOURCE_DIR}/cmake/linker.cmake)

#select appropriate config
# include(stm32F0)
# include(stm32F1)
include(${CMAKE_SOURCE_DIR}/cmake/stm32H7.cmake)



set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

#link_directories(${CMAKE_SOURCE_DIR}/lib/libopencm3/lib/stm32/g4 )
#target_link_libraries(${PROJECT_NAME}.elf -L${CMAKE_SOURCE_DIR}/lib/libopencm3/lib)

add_custom_command(
	POST_BUILD
	TARGET ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
	# COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
	COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

add_custom_command(
	POST_BUILD
	TARGET ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
	)