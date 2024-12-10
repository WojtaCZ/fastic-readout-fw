#---------------------------------------------------------------------------------------
# Set target specific compiler/linker flags
#---------------------------------------------------------------------------------------

# Object build options
# -mcpu=cortex-m3       SepcifiesTarget ARM processor.

# adjust 
set(MCU_OPTS "-mthumb -mcpu=cortex-m3 -mthumb -mabi=aapcs" CACHE INTERNAL "(Target) Architecture options")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${MCU_OPTS}" CACHE INTERNAL "(Target) C Compiler options")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MCU_OPTS}" CACHE INTERNAL "(Target) C++ Compiler options")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_OPTS}" CACHE INTERNAL "(Target) ASM Compiler options")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_OPTS} -T${LINKER_SCRIPT}" CACHE INTERNAL "(Target) Linker options")