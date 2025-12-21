#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D tex0;           // Source texture
uniform sampler2D pixelMapTex;    // Pixel mapping texture
uniform vec2 resolution;          // Screen resolution
uniform float numShards;

// Background color as individual components for better compatibility
uniform float bgColorR;
uniform float bgColorG;
uniform float bgColorB;
uniform float bgColorA;

varying vec2 texCoordVarying;
varying vec2 screenCoordVarying;

// Helper function for high-quality texture sampling
vec4 sampleTextureBilinear(sampler2D tex, vec2 uv) {
    // Get texture dimensions
    vec2 texSize = resolution;
    
    // Calculate texel coordinates
    vec2 texelCoord = uv * texSize;
    
    // Calculate the four nearest texel centers
    vec2 texelCenter = floor(texelCoord) + 0.5;
    
    // Calculate fractional offset
    vec2 f = texelCoord - texelCenter + 0.5;
    
    // Sample the four nearest texels
    vec4 s00 = texture2D(tex, (texelCenter + vec2(-0.5, -0.5)) / texSize);
    vec4 s10 = texture2D(tex, (texelCenter + vec2(0.5, -0.5)) / texSize);
    vec4 s01 = texture2D(tex, (texelCenter + vec2(-0.5, 0.5)) / texSize);
    vec4 s11 = texture2D(tex, (texelCenter + vec2(0.5, 0.5)) / texSize);
    
    // Bilinear interpolation
    return mix(mix(s00, s10, f.x), mix(s01, s11, f.x), f.y);
}

void main() {
    // Get normalized screen coordinates with high precision
    vec2 normScreenCoord = gl_FragCoord.xy / resolution;
    
    // Sample pixel map at current position with high precision
    vec4 mapData = texture2D(pixelMapTex, normScreenCoord);
    
    // Check if pixel is mapped (alpha > 0)
    if(mapData.a > 0.0) {
        // Check if mapped coordinates are within valid range [0,1]
        if(mapData.r >= 0.0 && mapData.r <= 1.0 && 
           mapData.g >= 0.0 && mapData.g <= 1.0) {
            // Valid coordinates - sample source texture with high precision
            vec2 sourceCoord = vec2(mapData.r, mapData.g);
            
            // Use high-quality sampling
            gl_FragColor = sampleTextureBilinear(tex0, sourceCoord);
        } else {
            // Invalid source coordinates - show magenta
            gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
        }
    } else {
        // Outside all masks - use background color from individual components
        gl_FragColor = vec4(bgColorR, bgColorG, bgColorB, bgColorA);
    }
} 