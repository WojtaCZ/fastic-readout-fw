#---------------------------------------------------------------------------------------
# Set target specific compiler/linker flags
#---------------------------------------------------------------------------------------

# Object build options
# -mcpu=cortex-m4       SepcifiesTarget ARM processor.
# -mfpu=fpv4-sp-d16     Specifies floating-point hardware.
# -mfloat-abi=softfp    Allows the generation of code using hardware floating-point instructions, but still uses the soft-float calling conventions.

# adjust 
set(MCU_OPTS "-mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -mabi=aapcs" CACHE INTERNAL "(Target) Architecture options")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${MCU_OPTS}" CACHE INTERNAL "(Target) C Compiler options")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MCU_OPTS}" CACHE INTERNAL "(Target) C++ Compiler options")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_OPTS}" CACHE INTERNAL "(Target) ASM Compiler options")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_OPTS} -T${LINKER_SCRIPT}" CACHE INTERNAL "(Target) Linker options")