#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include NBMakefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/coinbase-bot/coinbase.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=gnu++17
CXXFLAGS=-std=gnu++17

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=/usr/lib/arm-linux-gnueabihf/libcpprest.so /usr/lib/arm-linux-gnueabihf/libcrypto.so /usr/lib/arm-linux-gnueabihf/libssl.so /usr/lib/arm-linux-gnueabihf/libmagic.so /usr/lib/arm-linux-gnueabihf/libpthread.so /usr/lib/arm-linux-gnueabihf/libboost_system.so /usr/lib/arm-linux-gnueabihf/libboost_thread.so /usr/lib/gcc/arm-linux-gnueabihf/8/libstdc++fs.a /usr/lib/arm-linux-gnueabihf/libsndfile.so /usr/lib/arm-linux-gnueabihf/libasound.so

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${HOME}/Projects/Outputs/Release/mercury

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libcpprest.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libcrypto.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libssl.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libmagic.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libpthread.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libboost_system.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libboost_thread.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/gcc/arm-linux-gnueabihf/8/libstdc++fs.a

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libsndfile.so

${HOME}/Projects/Outputs/Release/mercury: /usr/lib/arm-linux-gnueabihf/libasound.so

${HOME}/Projects/Outputs/Release/mercury: ${OBJECTFILES}
	${MKDIR} -p ${HOME}/Projects/Outputs/Release
	${LINK.cc} -o ${HOME}/Projects/Outputs/Release/mercury ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/coinbase-bot/coinbase.o: coinbase-bot/coinbase.cpp
	${MKDIR} -p ${OBJECTDIR}/coinbase-bot
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -Idata_services -Ifinancial_services -Itclap/include -Imessaging_services -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/coinbase-bot/coinbase.o coinbase-bot/coinbase.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${HOME}/Projects/Outputs/Release/libcpprest.so ${HOME}/Projects/Outputs/Release/libcrypto.so ${HOME}/Projects/Outputs/Release/libssl.so ${HOME}/Projects/Outputs/Release/libmagic.so ${HOME}/Projects/Outputs/Release/libpthread.so ${HOME}/Projects/Outputs/Release/libboost_system.so ${HOME}/Projects/Outputs/Release/libboost_thread.so ${HOME}/Projects/Outputs/Release/libsndfile.so ${HOME}/Projects/Outputs/Release/libasound.so
	${RM} ${HOME}/Projects/Outputs/Release/mercury

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
