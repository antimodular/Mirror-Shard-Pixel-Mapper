#version 120

uniform sampler2D sourceTex;  // Changed from sampler2DRect to sampler2D
uniform vec2 resolution;
uniform int numShards;
uniform int currentShardIndex;  // Add this line after other uniforms

// Individual matrices to avoid array indexing issues - change to 4x4 matrices
uniform mat4 invH0;
uniform mat4 invH1;
uniform mat4 invH2;
uniform mat4 invH3;
uniform mat4 invH4;
uniform mat4 invH5;
uniform mat4 invH6;
uniform mat4 invH7;
uniform mat4 invH8;
uniform mat4 invH9;
uniform mat4 invH10;
uniform mat4 invH11;
uniform mat4 invH12;
uniform mat4 invH13;

uniform bool debugView;           // Flag to show debug visualization

varying vec2 texCoordVarying;
varying vec2 screenCoordVarying;

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
    if (shardIndex == 12) return vec4(0.0, 0.4, 0.2, 1.0); // Dark Green
    if (shardIndex == 13) return vec4(0.0, 0.4, 0.3, 1.0); // Dark Green
    return vec4(0.5, 0.5, 0.5, 1.0); // Gray
}

// Transform a point using inverse homography (from camera space to display space)
vec2 transformCameraToDisplay(vec2 cameraPoint, mat4 invH) {
    // Convert to homogeneous coordinates
    vec4 p = vec4(cameraPoint.x, cameraPoint.y, 0.0, 1.0);
    
    // Apply inverse homography
    vec4 result = invH * p;
    
    // Convert back to Cartesian coordinates
    if (abs(result.w) < 0.0001) {
        result.w = 0.0001;
    }
    
    return vec2(result.x / result.w, result.y / result.w);
}

// Transform a point from display space to camera space
vec2 transformDisplayToCamera(vec2 displayPoint, mat4 invH) {
    // We need to transform from display to camera, which is the inverse of our invH matrix
    // Since invH transforms from camera to display, to go back we need the original homography matrix
    
    // Convert display point to homogeneous coordinates
    vec4 p = vec4(displayPoint.x, displayPoint.y, 0.0, 1.0);
    
    // For inverting the invH matrix, we'll use a simplified approach
    // Create a matrix that works in the opposite direction to invH
    mat4 H = mat4(1.0);
    
    // This is a direct approach - normally you would compute proper matrix inverse
    // Extract the core 3x3 transform components
    float a = invH[0][0], b = invH[0][1], c = invH[0][3];
    float d = invH[1][0], e = invH[1][1], f = invH[1][3];
    float g = invH[3][0], h = invH[3][1], i = invH[3][3];
    
    // Determinant of the 3x3 matrix
    float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    
    if (abs(det) > 0.0001) {
        // Build the inverse matrix
        H[0][0] = (e * i - f * h) / det;
        H[0][1] = (c * h - b * i) / det;
        H[0][3] = (b * f - c * e) / det;
        H[1][0] = (f * g - d * i) / det;
        H[1][1] = (a * i - c * g) / det;
        H[1][3] = (c * d - a * f) / det;
        H[3][0] = (d * h - e * g) / det;
        H[3][1] = (g * b - a * h) / det;
        H[3][3] = (a * e - b * d) / det;
    }
    
    // Apply the original homography
    vec4 result = H * p;
    
    // Convert back to Cartesian coordinates
    if (abs(result.w) < 0.0001) {
        result.w = 0.0001;
    }
    
    return vec2(result.x / result.w, result.y / result.w);
}

// Helper function to get the correct matrix for the shard index
mat4 getInvH(int index) {
    if (index == 0) return invH0;
    if (index == 1) return invH1;
    if (index == 2) return invH2;
    if (index == 3) return invH3;
    if (index == 4) return invH4;
    if (index == 5) return invH5;
    if (index == 6) return invH6;
    if (index == 7) return invH7;
    if (index == 8) return invH8;
    if (index == 9) return invH9;
    if (index == 10) return invH10;
    if (index == 11) return invH11;
    if (index == 12) return invH12;
    if (index == 13) return invH13;
    return mat4(1.0);  // Default to identity
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);  // Default to transparent
    
    // Use the uniform for the current shard index
    int s = currentShardIndex;
    
    // Make sure we have a valid shard index
    if (s >= 0 && s < numShards) {
        // Get the appropriate homography matrix for this shard
        mat4 invH = getInvH(s);
        
        // Map this display space coordinate back to camera space for texturing
        vec2 cameraCoord = transformDisplayToCamera(fragCoord, invH);
        
        // Convert camera coordinates to normalized texture coordinates
        vec2 normalizedTexCoord = cameraCoord / resolution;
        
        // Ensure coordinates are in valid range for texture sampling
        normalizedTexCoord = clamp(normalizedTexCoord, 0.0, 1.0);
        
        color = texture2D(sourceTex, normalizedTexCoord);
        
        // If debug view is enabled, tint the shards with color
        if (debugView) {
            color = mix(color, debugColor(s), 0.5);
        }
    }
    
    gl_FragColor = color;
}
