#version 120

varying vec2 texCoordVarying;
varying vec2 screenCoordVarying;

void main() {
    texCoordVarying = gl_MultiTexCoord0.xy;
    screenCoordVarying = gl_Vertex.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
} 