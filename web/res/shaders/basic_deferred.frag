#version 300 es
precision highp float;

in vec4 vColor;
in float vDepth;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out float depthValue;

void main() {
  fragColor = vColor;
  fragNormal = vec4(0.0);
  depthValue = vDepth;
}
