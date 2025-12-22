// Vertex shader source
const vertexShaderSource = `#version 300 es
in vec2 a_position;
in vec2 a_texCoord;

out vec2 v_texCoord;
out vec2 v_screenCoord;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
    v_screenCoord = a_position;
}
`;

// Fragment shader source - converted from warpAllShader.frag
const fragmentShaderSource = `#version 300 es
precision highp float;

uniform sampler2D u_sourceTex;
uniform vec2 u_resolution;
uniform int u_numShards;
uniform int u_currentShardIndex;
uniform bool u_debugView;

// Individual matrices (up to 14 shards)
uniform mat4 u_invH0;
uniform mat4 u_invH1;
uniform mat4 u_invH2;
uniform mat4 u_invH3;
uniform mat4 u_invH4;
uniform mat4 u_invH5;
uniform mat4 u_invH6;
uniform mat4 u_invH7;
uniform mat4 u_invH8;
uniform mat4 u_invH9;
uniform mat4 u_invH10;
uniform mat4 u_invH11;
uniform mat4 u_invH12;
uniform mat4 u_invH13;

uniform vec4 u_bgColor;

// Debug transformation uniforms
uniform bool u_flipY;
uniform bool u_flipX;
uniform float u_inputScale;
uniform float u_outputScale;

in vec2 v_texCoord;
in vec2 v_screenCoord;

out vec4 outColor;

// Debug function to visualize shard regions
vec4 debugColor(int shardIndex) {
    if (shardIndex == 0) return vec4(1.0, 0.0, 0.0, 1.0); // Red
    if (shardIndex == 1) return vec4(0.0, 1.0, 0.0, 1.0); // Green
    if (shardIndex == 2) return vec4(0.0, 0.0, 1.0, 1.0); // Blue
    if (shardIndex == 3) return vec4(1.0, 1.0, 0.0, 1.0); // Yellow
    if (shardIndex == 4) return vec4(1.0, 0.0, 1.0, 1.0); // Magenta
    if (shardIndex == 5) return vec4(0.0, 1.0, 1.0, 1.0); // Cyan
    if (shardIndex == 6) return vec4(1.0, 0.5, 0.0, 1.0); // Orange
    if (shardIndex == 7) return vec4(0.5, 0.0, 1.0, 1.0); // Purple
    if (shardIndex == 8) return vec4(0.0, 0.5, 0.5, 1.0); // Teal
    if (shardIndex == 9) return vec4(0.5, 0.5, 0.0, 1.0); // Olive
    if (shardIndex == 10) return vec4(0.5, 0.0, 0.0, 1.0); // Maroon
    if (shardIndex == 11) return vec4(0.0, 0.5, 0.0, 1.0); // Dark Green
    if (shardIndex == 12) return vec4(0.0, 0.4, 0.2, 1.0);
    if (shardIndex == 13) return vec4(0.0, 0.4, 0.3, 1.0);
    return vec4(0.5, 0.5, 0.5, 1.0); // Gray
}

// Helper function to get the correct matrix for the shard index
mat4 getInvH(int index) {
    if (index == 0) return u_invH0;
    if (index == 1) return u_invH1;
    if (index == 2) return u_invH2;
    if (index == 3) return u_invH3;
    if (index == 4) return u_invH4;
    if (index == 5) return u_invH5;
    if (index == 6) return u_invH6;
    if (index == 7) return u_invH7;
    if (index == 8) return u_invH8;
    if (index == 9) return u_invH9;
    if (index == 10) return u_invH10;
    if (index == 11) return u_invH11;
    if (index == 12) return u_invH12;
    if (index == 13) return u_invH13;
    return mat4(1.0);  // Default to identity
}

// Transform for texture sampling - CORRECT VERSION
// The matrix passed can be either homography or inverseHomography (controlled by JS toggle)
// When toggle is FALSE (default): receives homography, inverts it to map display->source
// When toggle is TRUE: receives inverseHomography, applies directly
vec2 transformForSampling(vec2 displayPoint, mat4 H) {
    // Invert the matrix to map backwards from display to source
    mat4 invH = mat4(1.0);
    float a = H[0][0], b = H[0][1], c = H[0][3];
    float d = H[1][0], e = H[1][1], f = H[1][3];
    float g = H[3][0], h = H[3][1], i = H[3][3];
    
    float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    
    if (abs(det) > 0.0001) {
        float invDet = 1.0 / det;
        invH[0][0] = (e * i - f * h) * invDet;
        invH[0][1] = (c * h - b * i) * invDet;
        invH[0][3] = (b * f - c * e) * invDet;
        invH[1][0] = (f * g - d * i) * invDet;
        invH[1][1] = (a * i - c * g) * invDet;
        invH[1][3] = (c * d - a * f) * invDet;
        invH[3][0] = (d * h - e * g) * invDet;
        invH[3][1] = (g * b - a * h) * invDet;
        invH[3][3] = (a * e - b * d) * invDet;
    }
    
    vec4 p = vec4(displayPoint.x, displayPoint.y, 0.0, 1.0);
    vec4 result = invH * p;
    
    if (abs(result.w) < 0.0001) {
        result.w = 0.0001;
    }
    
    return vec2(result.x / result.w, result.y / result.w);
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    
    // Apply input scaling
    fragCoord *= u_inputScale;
    
    // Apply coordinate flips
    if (u_flipY) {
        fragCoord.y = u_resolution.y - fragCoord.y;
    }
    if (u_flipX) {
        fragCoord.x = u_resolution.x - fragCoord.x;
    }
    
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);  // Default to transparent
    
    // Use the uniform for the current shard index
    int s = u_currentShardIndex;
    
    // Make sure we have a valid shard index
    if (s >= 0 && s < u_numShards) {
        // Get the matrix (either homography or inverseHomography based on toggle)
        mat4 H = getInvH(s);
        
        // Transform to get source texture coordinates
        vec2 sourceCoord = transformForSampling(fragCoord, H);
        
        // Apply output scaling
        sourceCoord *= u_outputScale;
        
        // Convert to normalized texture coordinates
        vec2 normalizedTexCoord = sourceCoord / u_resolution;
        
        // Ensure coordinates are in valid range for texture sampling
        normalizedTexCoord = clamp(normalizedTexCoord, 0.0, 1.0);
        
        color = texture(u_sourceTex, normalizedTexCoord);
        
        // If debug view is enabled, tint the shards with color
        if (u_debugView) {
            color = mix(color, debugColor(s), 0.5);
        }
    }
    
    // Output color - background color is already set via clear, so we can output transparent
    // for areas outside texture bounds, but stencil test should prevent those from rendering anyway
    outColor = color;
}
`;

// Shader compilation utility
function createShader(gl, type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        const info = gl.getShaderInfoLog(shader);
        console.error('Shader compilation error:', info);
        gl.deleteShader(shader);
        return null;
    }
    
    return shader;
}

// Program creation utility
function createProgram(gl, vertexShader, fragmentShader) {
    const program = gl.createProgram();
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        const info = gl.getProgramInfoLog(program);
        console.error('Program linking error:', info);
        gl.deleteProgram(program);
        return null;
    }
    
    return program;
}

// Create shader program
function initShaderProgram(gl) {
    const vertexShader = createShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
    const fragmentShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
    
    if (!vertexShader || !fragmentShader) {
        return null;
    }
    
    const program = createProgram(gl, vertexShader, fragmentShader);
    
    if (!program) {
        return null;
    }
    
    // Get attribute and uniform locations
    return {
        program: program,
        attribLocations: {
            position: gl.getAttribLocation(program, 'a_position'),
            texCoord: gl.getAttribLocation(program, 'a_texCoord'),
        },
        uniformLocations: {
            sourceTex: gl.getUniformLocation(program, 'u_sourceTex'),
            resolution: gl.getUniformLocation(program, 'u_resolution'),
            numShards: gl.getUniformLocation(program, 'u_numShards'),
            currentShardIndex: gl.getUniformLocation(program, 'u_currentShardIndex'),
            debugView: gl.getUniformLocation(program, 'u_debugView'),
            bgColor: gl.getUniformLocation(program, 'u_bgColor'),
            invH0: gl.getUniformLocation(program, 'u_invH0'),
            invH1: gl.getUniformLocation(program, 'u_invH1'),
            invH2: gl.getUniformLocation(program, 'u_invH2'),
            invH3: gl.getUniformLocation(program, 'u_invH3'),
            invH4: gl.getUniformLocation(program, 'u_invH4'),
            invH5: gl.getUniformLocation(program, 'u_invH5'),
            invH6: gl.getUniformLocation(program, 'u_invH6'),
            invH7: gl.getUniformLocation(program, 'u_invH7'),
            invH8: gl.getUniformLocation(program, 'u_invH8'),
            invH9: gl.getUniformLocation(program, 'u_invH9'),
            invH10: gl.getUniformLocation(program, 'u_invH10'),
            invH11: gl.getUniformLocation(program, 'u_invH11'),
            invH12: gl.getUniformLocation(program, 'u_invH12'),
            invH13: gl.getUniformLocation(program, 'u_invH13'),
        },
    };
}

