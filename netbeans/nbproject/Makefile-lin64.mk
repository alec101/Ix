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
CC=clang
CCC=clang++
CXX=clang++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=CLang-Linux
CND_DLIB_EXT=so
CND_CONF=lin64
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/511d968a/bone.o \
	${OBJECTDIR}/_ext/511d968a/camera.o \
	${OBJECTDIR}/_ext/c4122f25/glObject.o \
	${OBJECTDIR}/_ext/c4122f25/gloShader.o \
	${OBJECTDIR}/_ext/511d968a/light.o \
	${OBJECTDIR}/_ext/511d968a/material.o \
	${OBJECTDIR}/_ext/511d968a/mesh3D.o \
	${OBJECTDIR}/_ext/511d968a/object.o \
	${OBJECTDIR}/_ext/511d968a/print.o \
	${OBJECTDIR}/_ext/511d968a/resource.o \
	${OBJECTDIR}/_ext/511d968a/shaderSys.o \
	${OBJECTDIR}/_ext/511d968a/texStream.o \
	${OBJECTDIR}/_ext/511d968a/texSys.o \
	${OBJECTDIR}/_ext/511d968a/texture.o \
	${OBJECTDIR}/_ext/d2a31033/glDraw.o \
	${OBJECTDIR}/_ext/d2a31033/vkDraw.o \
	${OBJECTDIR}/_ext/5c0/ix.o \
	${OBJECTDIR}/_ext/5c0/ixVulkan.o \
	${OBJECTDIR}/_ext/d2aad2f1/common.o \
	${OBJECTDIR}/_ext/d2aad2f1/fileTEX.o \
	${OBJECTDIR}/_ext/d2aad2f1/ixConsole.o \
	${OBJECTDIR}/_ext/d6128080/button.o \
	${OBJECTDIR}/_ext/d6128080/dropList.o \
	${OBJECTDIR}/_ext/d6128080/edit.o \
	${OBJECTDIR}/_ext/d6128080/eventSys.o \
	${OBJECTDIR}/_ext/d6128080/menu.o \
	${OBJECTDIR}/_ext/d6128080/progressBar.o \
	${OBJECTDIR}/_ext/d6128080/radioButton.o \
	${OBJECTDIR}/_ext/d6128080/scroll.o \
	${OBJECTDIR}/_ext/d6128080/static.o \
	${OBJECTDIR}/_ext/d6128080/style.o \
	${OBJECTDIR}/_ext/d6128080/txtShared.o \
	${OBJECTDIR}/_ext/d6128080/winBase.o \
	${OBJECTDIR}/_ext/d6128080/winSys.o \
	${OBJECTDIR}/_ext/d6128080/window.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ../lib/ix.lin64.so

../lib/ix.lin64.so: ${OBJECTFILES}
	${MKDIR} -p ../lib
	${RM} ../lib/ix.lin64.so
	${AR} -rv ../lib/ix.lin64.so ${OBJECTFILES} 
	$(RANLIB) ../lib/ix.lin64.so

${OBJECTDIR}/_ext/511d968a/bone.o: ../GFX/bone.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/bone.o ../GFX/bone.cpp

${OBJECTDIR}/_ext/511d968a/camera.o: ../GFX/camera.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/camera.o ../GFX/camera.cpp

${OBJECTDIR}/_ext/c4122f25/glObject.o: ../GFX/glo/glObject.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/c4122f25
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/c4122f25/glObject.o ../GFX/glo/glObject.cpp

${OBJECTDIR}/_ext/c4122f25/gloShader.o: ../GFX/glo/gloShader.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/c4122f25
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/c4122f25/gloShader.o ../GFX/glo/gloShader.cpp

${OBJECTDIR}/_ext/511d968a/light.o: ../GFX/light.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/light.o ../GFX/light.cpp

${OBJECTDIR}/_ext/511d968a/material.o: ../GFX/material.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/material.o ../GFX/material.cpp

${OBJECTDIR}/_ext/511d968a/mesh3D.o: ../GFX/mesh3D.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/mesh3D.o ../GFX/mesh3D.cpp

${OBJECTDIR}/_ext/511d968a/object.o: ../GFX/object.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/object.o ../GFX/object.cpp

${OBJECTDIR}/_ext/511d968a/print.o: ../GFX/print.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/print.o ../GFX/print.cpp

${OBJECTDIR}/_ext/511d968a/resource.o: ../GFX/resource.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/resource.o ../GFX/resource.cpp

${OBJECTDIR}/_ext/511d968a/shaderSys.o: ../GFX/shaderSys.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/shaderSys.o ../GFX/shaderSys.cpp

${OBJECTDIR}/_ext/511d968a/texStream.o: ../GFX/texStream.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/texStream.o ../GFX/texStream.cpp

${OBJECTDIR}/_ext/511d968a/texSys.o: ../GFX/texSys.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/texSys.o ../GFX/texSys.cpp

${OBJECTDIR}/_ext/511d968a/texture.o: ../GFX/texture.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/511d968a
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511d968a/texture.o ../GFX/texture.cpp

${OBJECTDIR}/_ext/d2a31033/glDraw.o: ../draw/glDraw.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a31033
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a31033/glDraw.o ../draw/glDraw.cpp

${OBJECTDIR}/_ext/d2a31033/vkDraw.o: ../draw/vkDraw.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d2a31033
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2a31033/vkDraw.o ../draw/vkDraw.cpp

${OBJECTDIR}/_ext/5c0/ix.o: ../ix.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/ix.o ../ix.cpp

${OBJECTDIR}/_ext/5c0/ixVulkan.o: ../ixVulkan.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/ixVulkan.o ../ixVulkan.cpp

${OBJECTDIR}/_ext/d2aad2f1/common.o: ../util/common.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d2aad2f1
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2aad2f1/common.o ../util/common.cpp

${OBJECTDIR}/_ext/d2aad2f1/fileTEX.o: ../util/fileTEX.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d2aad2f1
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2aad2f1/fileTEX.o ../util/fileTEX.cpp

${OBJECTDIR}/_ext/d2aad2f1/ixConsole.o: ../util/ixConsole.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d2aad2f1
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d2aad2f1/ixConsole.o ../util/ixConsole.cpp

${OBJECTDIR}/_ext/d6128080/button.o: ../winSys/button.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/button.o ../winSys/button.cpp

${OBJECTDIR}/_ext/d6128080/dropList.o: ../winSys/dropList.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/dropList.o ../winSys/dropList.cpp

${OBJECTDIR}/_ext/d6128080/edit.o: ../winSys/edit.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/edit.o ../winSys/edit.cpp

${OBJECTDIR}/_ext/d6128080/eventSys.o: ../winSys/eventSys.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/eventSys.o ../winSys/eventSys.cpp

${OBJECTDIR}/_ext/d6128080/menu.o: ../winSys/menu.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/menu.o ../winSys/menu.cpp

${OBJECTDIR}/_ext/d6128080/progressBar.o: ../winSys/progressBar.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/progressBar.o ../winSys/progressBar.cpp

${OBJECTDIR}/_ext/d6128080/radioButton.o: ../winSys/radioButton.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/radioButton.o ../winSys/radioButton.cpp

${OBJECTDIR}/_ext/d6128080/scroll.o: ../winSys/scroll.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/scroll.o ../winSys/scroll.cpp

${OBJECTDIR}/_ext/d6128080/static.o: ../winSys/static.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/static.o ../winSys/static.cpp

${OBJECTDIR}/_ext/d6128080/style.o: ../winSys/style.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/style.o ../winSys/style.cpp

${OBJECTDIR}/_ext/d6128080/txtShared.o: ../winSys/txtShared.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/txtShared.o ../winSys/txtShared.cpp

${OBJECTDIR}/_ext/d6128080/winBase.o: ../winSys/winBase.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/winBase.o ../winSys/winBase.cpp

${OBJECTDIR}/_ext/d6128080/winSys.o: ../winSys/winSys.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/winSys.o ../winSys/winSys.cpp

${OBJECTDIR}/_ext/d6128080/window.o: ../winSys/window.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/d6128080
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -DIX_USE_OPENGL -DIX_USE_VULKAN -DOSI_USE_OPENGL -DOSI_USE_VKO -I../.. -I../../Vulkan-Headers/include/vulkan -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/d6128080/window.o ../winSys/window.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
