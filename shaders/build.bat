cls
rem --invert-y | --iy : invert position.Y in the vertex output. This can be an option instead of doing so in the shader?
glslangValidator  -V vkPrintM1.vert.glsl -o vkPrintM1.vert.spv
glslangValidator  -V vkPrintM1.frag.glsl -o vkPrintM1.frag.spv

glslangValidator  -V ixDraw/vkQuad.vert.glsl -o ixDraw/vkQuad.vert.spv
glslangValidator  -V ixDraw/vkQuad.frag.glsl -o ixDraw/vkQuad.frag.spv

glslangValidator  -V ixDraw/vkCircle.vert.glsl -o ixDraw/vkCircle.vert.spv
glslangValidator  -V ixDraw/vkCircle.frag.glsl -o ixDraw/vkCircle.frag.spv

glslangValidator  -V ixDraw/vkTriangle.vert.glsl -o ixDraw/vkTriangle.vert.spv
glslangValidator  -V ixDraw/vkTriangle.frag.glsl -o ixDraw/vkTriangle.frag.spv

glslangValidator  -V ixDraw/vkPoint.vert.glsl -o ixDraw/vkPoint.vert.spv
glslangValidator  -V ixDraw/vkPoint.frag.glsl -o ixDraw/vkPoint.frag.spv

glslangValidator  -V ixDraw/vkLine.vert.glsl -o ixDraw/vkLine.vert.spv
glslangValidator  -V ixDraw/vkLine.frag.glsl -o ixDraw/vkLine.frag.spv

glslangValidator  -V vkWindows.vert.glsl -o vkWindows.vert.spv
glslangValidator  -V vkWindows.frag.glsl -o vkWindows.frag.spv
