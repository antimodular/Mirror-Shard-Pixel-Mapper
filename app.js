// Constants
const DISPLAY_WIDTH = 3840;
const DISPLAY_HEIGHT = 2160;
const VERSION = '5.1 - Pan/Zoom';
const IMAGE_PATH = 'data/images/';  // New image path

// Global state
let gl;
let shaderInfo;
let sourceTexture;
let shards = [];
window.shards = shards;  // Expose globally for cross-frame access

// Function to load custom image from data URL (called from box3d.html)
window.loadCustomImage = async function(dataUrl) {
    try {
        sourceTexture = await loadImageTexture(dataUrl);
        console.log('Custom image loaded via loadCustomImage');
        return true;
    } catch (e) {
        console.error('Failed to load custom image:', e);
        return false;
    }
};

// Functions to control source transform from box3d.html
window.setSourceScale = function(scale) {
    sourceScale = scale;
};
window.setSourceOffset = function(x, y) {
    sourceOffsetX = x;
    sourceOffsetY = y;
};
window.getSourceTransform = function() {
    return { scale: sourceScale, offsetX: sourceOffsetX, offsetY: sourceOffsetY };
};

let currentImage = null;
let currentImagePath = IMAGE_PATH + 'a-thing-1.jpg';

// Pan and zoom controls
let sourceOffsetX = 0.0;
let sourceOffsetY = 0.0;
let sourceScale = 1.0;
let debugView = false;
let showAllShards = true;
let currentShardIndex = 0;

// Correct transformation settings (hardcoded)
const useInverseHomography = true;  // Use inverseHomography
const flipY = true;                  // Flip Y coordinate (WebGL Y=0 at bottom, calibration Y=0 at top)
const flipX = false;                 // Don't flip X
const inputResolutionScale = 1.0;   // Scale input coordinates
const outputResolutionScale = 1.0;  // Scale output texture sampling
let bgColor = { r: 0, g: 0, b: 0, a: 0 };
let shardVisibility = {}; // Object to track visibility of each shard
let gui;

// Homography computation (ported from ofxHomography.h)
function gaussianElimination(input, n) {
    const A = input;
    let i = 0;
    let j = 0;
    const m = n - 1;
    
    while (i < m && j < n) {
        // Find pivot in column j, starting in row i
        let maxi = i;
        for (let k = i + 1; k < m; k++) {
            if (Math.abs(A[k * n + j]) > Math.abs(A[maxi * n + j])) {
                maxi = k;
            }
        }
        
        if (A[maxi * n + j] !== 0) {
            // Swap rows i and maxi
            if (i !== maxi) {
                for (let k = 0; k < n; k++) {
                    const aux = A[i * n + k];
                    A[i * n + k] = A[maxi * n + k];
                    A[maxi * n + k] = aux;
                }
            }
            
            // Divide each entry in row i by A[i,j]
            const A_ij = A[i * n + j];
            for (let k = 0; k < n; k++) {
                A[i * n + k] /= A_ij;
            }
            
            // Subtract A[u,j] * row i from row u
            for (let u = i + 1; u < m; u++) {
                const A_uj = A[u * n + j];
                for (let k = 0; k < n; k++) {
                    A[u * n + k] -= A_uj * A[i * n + k];
                }
            }
            
            i++;
        }
        j++;
    }
    
    // Back substitution
    for (let i = m - 2; i >= 0; i--) {
        for (let j = i + 1; j < n - 1; j++) {
            A[i * n + m] -= A[i * n + j] * A[j * n + m];
        }
    }
}

function findHomographyMatrix(srcPoints, dstPoints) {
    // Create the equation system
    const P = new Array(8);
    for (let i = 0; i < 8; i++) {
        P[i] = new Array(9);
    }
    
    // Fill the P matrix
    P[0] = [-srcPoints[0].x, -srcPoints[0].y, -1, 0, 0, 0, srcPoints[0].x * dstPoints[0].x, srcPoints[0].y * dstPoints[0].x, -dstPoints[0].x];
    P[1] = [0, 0, 0, -srcPoints[0].x, -srcPoints[0].y, -1, srcPoints[0].x * dstPoints[0].y, srcPoints[0].y * dstPoints[0].y, -dstPoints[0].y];
    P[2] = [-srcPoints[1].x, -srcPoints[1].y, -1, 0, 0, 0, srcPoints[1].x * dstPoints[1].x, srcPoints[1].y * dstPoints[1].x, -dstPoints[1].x];
    P[3] = [0, 0, 0, -srcPoints[1].x, -srcPoints[1].y, -1, srcPoints[1].x * dstPoints[1].y, srcPoints[1].y * dstPoints[1].y, -dstPoints[1].y];
    P[4] = [-srcPoints[2].x, -srcPoints[2].y, -1, 0, 0, 0, srcPoints[2].x * dstPoints[2].x, srcPoints[2].y * dstPoints[2].x, -dstPoints[2].x];
    P[5] = [0, 0, 0, -srcPoints[2].x, -srcPoints[2].y, -1, srcPoints[2].x * dstPoints[2].y, srcPoints[2].y * dstPoints[2].y, -dstPoints[2].y];
    P[6] = [-srcPoints[3].x, -srcPoints[3].y, -1, 0, 0, 0, srcPoints[3].x * dstPoints[3].x, srcPoints[3].y * dstPoints[3].x, -dstPoints[3].x];
    P[7] = [0, 0, 0, -srcPoints[3].x, -srcPoints[3].y, -1, srcPoints[3].x * dstPoints[3].y, srcPoints[3].y * dstPoints[3].y, -dstPoints[3].y];
    
    // Flatten for gaussian elimination
    const flatP = new Array(72);
    for (let i = 0; i < 8; i++) {
        for (let j = 0; j < 9; j++) {
            flatP[i * 9 + j] = P[i][j];
        }
    }
    
    gaussianElimination(flatP, 9);
    
    // Extract results and build 4x4 matrix (OpenGL format - column major)
    const aux_H = [
        flatP[0 * 9 + 8], flatP[3 * 9 + 8], 0, flatP[6 * 9 + 8],  // h11 h21 0 h31
        flatP[1 * 9 + 8], flatP[4 * 9 + 8], 0, flatP[7 * 9 + 8],  // h12 h22 0 h32
        0, 0, 1, 0,                                                 // 0   0   1 0
        flatP[2 * 9 + 8], flatP[5 * 9 + 8], 0, 1                   // h13 h23 0 h33
    ];
    
    return new Float32Array(aux_H);
}

// Simple 4x4 matrix inversion for homography matrices (special structure)
// For homography matrices, only the 2x2 top-left, 2x1 top-right, 2x1 bottom-left, and bottom-right are used
function invertMatrix4(m) {
    // Extract the 3x3 homography components (from column-major 4x4 matrix)
    // Matrix structure:
    // [m[0] m[4] m[12]]   [h11 h12 h13]
    // [m[1] m[5] m[13]] = [h21 h22 h23]
    // [m[3] m[7] m[15]]   [h31 h32 h33]
    
    const a = m[0], b = m[4], c = m[12];   // Row 0
    const d = m[1], e = m[5], f = m[13];   // Row 1
    const g = m[3], h = m[7], i = m[15];   // Row 3 (homogeneous coordinate row)
    
    // Determinant of the 3x3 matrix
    const det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    
    if (Math.abs(det) < 0.0001) {
        // Return identity if matrix is singular
        return new Float32Array([
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        ]);
    }
    
    // Inverse of 3x3 matrix
    const inv00 = (e * i - f * h) / det;
    const inv01 = (c * h - b * i) / det;
    const inv02 = (b * f - c * e) / det;
    const inv10 = (f * g - d * i) / det;
    const inv11 = (a * i - c * g) / det;
    const inv12 = (c * d - a * f) / det;
    const inv20 = (d * h - e * g) / det;
    const inv21 = (g * b - a * h) / det;
    const inv22 = (a * e - b * d) / det;
    
    // Build 4x4 matrix in column-major format
    const inv = new Float32Array([
        inv00, inv10, 0, inv20,   // Column 0
        inv01, inv11, 0, inv21,   // Column 1
        0,     0,     1, 0,       // Column 2
        inv02, inv12, 0, inv22    // Column 3
    ]);
    
    return inv;
}

// Transform a 2D point using a 4x4 homography matrix (column-major)
function transformPoint2D(point, matrix) {
    const x = point.x;
    const y = point.y;
    
    // Matrix multiplication: result = matrix * [x, y, 0, 1]^T
    // For column-major 4x4 matrix, to compute M*v where v=[x,y,z,w]:
    // result[i] = M[i+0*4]*v[0] + M[i+1*4]*v[1] + M[i+2*4]*v[2] + M[i+3*4]*v[3]
    // For v = [x, y, 0, 1]:
    const resultX = matrix[0] * x + matrix[4] * y + matrix[8] * 0 + matrix[12] * 1;
    const resultY = matrix[1] * x + matrix[5] * y + matrix[9] * 0 + matrix[13] * 1;
    const resultZ = matrix[2] * x + matrix[6] * y + matrix[10] * 0 + matrix[14] * 1;
    const resultW = matrix[3] * x + matrix[7] * y + matrix[11] * 0 + matrix[15] * 1;
    
    if (Math.abs(resultW) < 0.0001) {
        return { x: x, y: y };
    }
    
    return { x: resultX / resultW, y: resultY / resultW };
}

// Process shard data from embedded SHARD_DATA
function processShard(shardData) {
    const { index, livePoints, displayPoints, maskPoints } = shardData;

    // Compute homography (need at least 4 points)
    if (livePoints.length >= 4 && displayPoints.length >= 4) {
        // IMPORTANT: Match C++ behavior - homography maps displayPoints -> livePoints
        const homography = findHomographyMatrix(displayPoints.slice(0, 4), livePoints.slice(0, 4));
        const inverseHomography = invertMatrix4(homography);

        // Transform mask points to display space using inverse homography
        const transformedMaskPoints = maskPoints.map(p => {
            return transformPoint2D(p, inverseHomography);
        });

        console.log(`Processed shard${index}: ${maskPoints.length} mask points`);

        return {
            name: `shard${index}`,
            index: index,
            livePoints,
            displayPoints,
            maskPoints,
            transformedMaskPoints,
            homography,
            inverseHomography,
            homographyReady: true
        };
    } else {
        console.warn(`Shard ${index}: Not enough points (need 4, got ${livePoints.length}/${displayPoints.length})`);
        return null;
    }
}

// Load all shards from embedded data (no fetch required)
function loadAllShards() {
    shards = [];

    // Check if SHARD_DATA is available (from shardData.js)
    if (typeof SHARD_DATA === 'undefined') {
        console.error('SHARD_DATA not found! Make sure shardData.js is loaded.');
        return;
    }

    for (const shardData of SHARD_DATA) {
        const shard = processShard(shardData);
        if (shard && shard.homographyReady) {
            shards.push(shard);
            // Initialize visibility for this shard
            if (shardVisibility[`shard${shard.index}`] === undefined) {
                shardVisibility[`shard${shard.index}`] = true;
            }
        } else {
            // Shard not loaded, disable it
            shardVisibility[`shard${shardData.index}`] = false;
        }
    }
    console.log(`Loaded ${shards.length} shards from embedded data`);
    window.shards = shards;  // Update global reference after loading
    document.getElementById('shardCount').textContent = shards.length;

    // Update shard selector max value
    const shardInput = document.getElementById('currentShard');
    shardInput.max = Math.max(0, shards.length - 1);
    if (currentShardIndex >= shards.length) {
        currentShardIndex = 0;
        shardInput.value = 0;
    }
}

// Initialize WebGL
function initWebGL() {
    const canvas = document.getElementById('canvas');
    canvas.width = DISPLAY_WIDTH;
    canvas.height = DISPLAY_HEIGHT;

    // Request WebGL2 context with stencil buffer
    gl = canvas.getContext('webgl2', {
        stencil: true,
        antialias: false,
        depth: false,
        alpha: true,
        preserveDrawingBuffer: true  // Helps with Safari compatibility
    });

    if (!gl) {
        console.error('WebGL 2.0 not available, trying WebGL 1.0...');
        // Note: WebGL1 fallback would require different shaders
        alert('WebGL 2.0 is not available. Please use a modern browser (Chrome, Firefox, Safari 15+, Edge).');
        return false;
    }

    // Check max texture size (Safari may have lower limits)
    const maxTextureSize = gl.getParameter(gl.MAX_TEXTURE_SIZE);
    console.log('Max texture size:', maxTextureSize);
    if (maxTextureSize < DISPLAY_WIDTH || maxTextureSize < DISPLAY_HEIGHT) {
        console.warn(`Canvas size ${DISPLAY_WIDTH}x${DISPLAY_HEIGHT} exceeds max texture size ${maxTextureSize}`);
    }
    
    // Verify stencil buffer is available
    const stencilBits = gl.getParameter(gl.STENCIL_BITS);
    if (stencilBits === 0) {
        console.error('Stencil buffer not available! Stencil bits:', stencilBits);
        alert('Stencil buffer is required but not available');
        return false;
    }
    console.log('Stencil buffer available with', stencilBits, 'bits');
    
    shaderInfo = initShaderProgram(gl);
    if (!shaderInfo) {
        return false;
    }
    
    // Create fullscreen quad
    const positions = new Float32Array([
        -1, -1,
        1, -1,
        -1, 1,
        1, 1,
    ]);
    
    const texCoords = new Float32Array([
        0, 1,
        1, 1,
        0, 0,
        1, 0,
    ]);
    
    const positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, positions, gl.STATIC_DRAW);
    
    const texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, texCoords, gl.STATIC_DRAW);
    
    shaderInfo.positionBuffer = positionBuffer;
    shaderInfo.texCoordBuffer = texCoordBuffer;
    
    // Enable blending
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    
    return true;
}

// Load image as texture
function loadImageTexture(imagePath) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.crossOrigin = 'anonymous';
        img.onload = () => {
            const texture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, texture);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            resolve({ texture, width: img.width, height: img.height });
        };
        img.onerror = reject;
        img.src = imagePath;
    });
}

// Simple vertex shader for drawing stencil masks
const maskVertexShaderSource = `#version 300 es
in vec2 a_position;
uniform vec2 u_resolution;

void main() {
    vec2 ndc = vec2(
        (a_position.x / u_resolution.x * 2.0 - 1.0),
        (1.0 - a_position.y / u_resolution.y * 2.0)
    );
    gl_Position = vec4(ndc, 0.0, 1.0);
}
`;

// Simple fragment shader for stencil mask (no color output needed)
const maskFragmentShaderSource = `#version 300 es
precision highp float;
out vec4 outColor;

void main() {
    outColor = vec4(1.0);
}
`;

let maskShaderProgram = null;
let maskShaderLocations = null;

function initMaskShader() {
    const vs = createShader(gl, gl.VERTEX_SHADER, maskVertexShaderSource);
    const fs = createShader(gl, gl.FRAGMENT_SHADER, maskFragmentShaderSource);
    
    if (!vs || !fs) return false;
    
    maskShaderProgram = createProgram(gl, vs, fs);
    if (!maskShaderProgram) return false;
    
    maskShaderLocations = {
        position: gl.getAttribLocation(maskShaderProgram, 'a_position'),
        resolution: gl.getUniformLocation(maskShaderProgram, 'u_resolution'),
    };
    
    return true;
}

function drawPolygonToStencil(points, stencilValue) {
    if (points.length < 3) return;
    
    // Convert points to pixel coordinates
    const vertices = new Float32Array(points.length * 2);
    for (let i = 0; i < points.length; i++) {
        vertices[i * 2] = points[i].x;
        vertices[i * 2 + 1] = points[i].y;
    }
    
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
    
    gl.useProgram(maskShaderProgram);
    gl.uniform2f(maskShaderLocations.resolution, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    gl.enableVertexAttribArray(maskShaderLocations.position);
    gl.vertexAttribPointer(maskShaderLocations.position, 2, gl.FLOAT, false, 0, 0);
    
    // Set stencil function: ALWAYS pass, write stencilValue when drawing
    // The reference value in stencilFunc is written when stencilOp is REPLACE
    gl.stencilFunc(gl.ALWAYS, stencilValue, 0xFF);
    gl.stencilOp(gl.KEEP, gl.KEEP, gl.REPLACE);
    
    // Draw as triangle fan (triangulate the polygon)
    const indices = new Uint16Array((points.length - 2) * 3);
    for (let i = 0; i < points.length - 2; i++) {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }
    
    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.DYNAMIC_DRAW);
    
    // Draw the triangles - this writes stencilValue to the stencil buffer
    gl.drawElements(gl.TRIANGLES, indices.length, gl.UNSIGNED_SHORT, 0);
    
    // Clean up buffers
    gl.deleteBuffer(buffer);
    gl.deleteBuffer(indexBuffer);
    
    // Unbind to avoid issues
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
}

// Render function
function render() {
    if (!gl || !shaderInfo || !sourceTexture) {
        requestAnimationFrame(render);
        return;
    }
    
    if (shards.length === 0) {
        // Just clear with background color if no shards loaded
        const bg = bgColor;
        gl.clearColor(bg.r, bg.g, bg.b, bg.a);
        gl.clear(gl.COLOR_BUFFER_BIT);
        requestAnimationFrame(render);
        return;
    }
    
    gl.viewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    // Enable stencil testing BEFORE clearing
    gl.enable(gl.STENCIL_TEST);
    
    // Set stencil clear value to 0
    gl.clearStencil(0);
    
    // Clear with background color and stencil buffer
    const bg = bgColor;
    gl.clearColor(bg.r, bg.g, bg.b, bg.a);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);
    
    // Calculate how many shards to potentially render (before visibility filtering)
    const numShardsToRender = showAllShards ? Math.min(14, shards.length) : 1;
    
    // First pass: Draw masks to stencil buffer
    // Disable color writes, enable stencil writes
    gl.colorMask(false, false, false, false);
    gl.stencilOp(gl.KEEP, gl.KEEP, gl.REPLACE);
    
    // Draw each shard's mask to stencil
    // Important: Track which shards are actually rendered for visibility control
    const visibleShards = [];
    let visibleIndex = 0;
    
    for (let s = 0; s < numShardsToRender; s++) {
        const shardIndex = showAllShards ? s : currentShardIndex;
        if (shardIndex >= shards.length) continue;
        
        // Check visibility
        const shardKey = `shard${shardIndex}`;
        if (shardVisibility[shardKey] === false) {
            continue; // Skip this shard if it's disabled
        }
        
        const shard = shards[shardIndex];
        if (shard.transformedMaskPoints && shard.transformedMaskPoints.length >= 3) {
            // Use visibleIndex+1 as stencil value (0 is reserved for background)
            // This matches the shader rendering loop where we check for gl.EQUAL
            drawPolygonToStencil(shard.transformedMaskPoints, visibleIndex + 1);
            visibleShards.push({ shardIndex: shardIndex, stencilValue: visibleIndex + 1, arrayIndex: s });
            visibleIndex++;
        } else {
            console.warn(`Shard ${shardIndex} has invalid mask points:`, shard.transformedMaskPoints ? shard.transformedMaskPoints.length : 'null');
        }
    }
    
    gl.colorMask(true, true, true, true);
    
    // Re-enable color writes and prepare for rendering
    // Blending is already enabled in initWebGL
    
    // Second pass: Render with shader
    gl.useProgram(shaderInfo.program);
    
    // Update numShards uniform based on visible shards
    gl.uniform1i(shaderInfo.uniformLocations.numShards, visibleShards.length);
    
    // Set up attributes
    gl.bindBuffer(gl.ARRAY_BUFFER, shaderInfo.positionBuffer);
    gl.enableVertexAttribArray(shaderInfo.attribLocations.position);
    gl.vertexAttribPointer(shaderInfo.attribLocations.position, 2, gl.FLOAT, false, 0, 0);
    
    gl.bindBuffer(gl.ARRAY_BUFFER, shaderInfo.texCoordBuffer);
    gl.enableVertexAttribArray(shaderInfo.attribLocations.texCoord);
    gl.vertexAttribPointer(shaderInfo.attribLocations.texCoord, 2, gl.FLOAT, false, 0, 0);
    
    // Bind source texture
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, sourceTexture.texture);
    gl.uniform1i(shaderInfo.uniformLocations.sourceTex, 0);
    
    // Set uniforms
    gl.uniform2f(shaderInfo.uniformLocations.resolution, sourceTexture.width, sourceTexture.height);
    gl.uniform1i(shaderInfo.uniformLocations.debugView, debugView ? 1 : 0);

    // Pan and zoom uniforms
    gl.uniform2f(shaderInfo.uniformLocations.sourceOffset, sourceOffsetX, sourceOffsetY);
    gl.uniform1f(shaderInfo.uniformLocations.sourceScale, sourceScale);

    gl.uniform4f(shaderInfo.uniformLocations.bgColor, bg.r, bg.g, bg.b, bg.a);
    
    // Update numShards uniform based on visible shards
    gl.uniform1i(shaderInfo.uniformLocations.numShards, visibleShards.length);
    
    // Set homography matrices - use inverseHomography (correct setting)
    const matrixNames = ['invH0', 'invH1', 'invH2', 'invH3', 'invH4', 'invH5', 'invH6', 
                         'invH7', 'invH8', 'invH9', 'invH10', 'invH11', 'invH12', 'invH13'];
    
    for (let i = 0; i < visibleShards.length; i++) {
        const visibleShard = visibleShards[i];
        const shard = shards[visibleShard.shardIndex];
        const matrixLoc = shaderInfo.uniformLocations[matrixNames[i]];
        if (matrixLoc && matrixLoc !== -1) {
            // Use inverseHomography (correct setting with Y-flip)
            gl.uniformMatrix4fv(matrixLoc, false, shard.inverseHomography);
        } else {
            console.error(`Matrix location for ${matrixNames[i]} not found!`);
        }
    }
    
    // Render each visible shard
    const currentShardLoc = shaderInfo.uniformLocations.currentShardIndex;
    if (currentShardLoc === -1) {
        console.error('currentShardIndex uniform location not found!');
    } else {
        for (let i = 0; i < visibleShards.length; i++) {
            const visibleShard = visibleShards[i];
            const shard = shards[visibleShard.shardIndex];
            
            // Set which shard matrix to use (this maps to invH0, invH1, etc. in shader)
            gl.uniform1i(currentShardLoc, i);
            
            // Only render pixels where stencil equals the stencil value we set in mask pass
            gl.stencilFunc(gl.EQUAL, visibleShard.stencilValue, 0xFF);
            gl.stencilOp(gl.KEEP, gl.KEEP, gl.KEEP);
            
            // Draw fullscreen quad - stencil test will limit rendering to mask area
            // The shader will use matrix slot 'i' which contains this shard's transformation
            gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        }
    }
    
    // Reset stencil function (though we disable it next anyway)
    gl.stencilFunc(gl.ALWAYS, 0, 0xFF);
    
    gl.disable(gl.STENCIL_TEST);
    
    requestAnimationFrame(render);
}

// Setup controls
function setupControls() {
    // Update version display
    document.getElementById('version').textContent = `v${VERSION}`;
    
    const imageSelect = document.getElementById('imageSelect');
    imageSelect.addEventListener('change', async (e) => {
        currentImagePath = IMAGE_PATH + e.target.value;
        sourceTexture = await loadImageTexture(currentImagePath);

        // If the 3D view is enabled, the box walls need a refresh because they
        // snapshot the current canvas into quadrant textures.
        if (typeof window.update3DTextures === 'function') {
            window.update3DTextures();
        }
    });
    
    document.getElementById('debugView').addEventListener('change', (e) => {
        debugView = e.target.checked;
    });
    
    document.getElementById('showAllShards').addEventListener('change', (e) => {
        showAllShards = e.target.checked;
    });
    
    document.getElementById('currentShard').addEventListener('change', (e) => {
        currentShardIndex = parseInt(e.target.value);
    });
    
    document.getElementById('bgColor').addEventListener('change', (e) => {
        const hex = e.target.value;
        bgColor.r = parseInt(hex.substr(1, 2), 16) / 255;
        bgColor.g = parseInt(hex.substr(3, 2), 16) / 255;
        bgColor.b = parseInt(hex.substr(5, 2), 16) / 255;
    });
    
    document.getElementById('bgAlpha').addEventListener('input', (e) => {
        bgColor.a = parseFloat(e.target.value);
        document.getElementById('bgAlphaValue').textContent = bgColor.a.toFixed(2);
    });
}

// Setup dat.GUI controls
function setupDatGUI() {
    if (typeof dat === 'undefined') {
        console.warn('dat.GUI not available, skipping GUI setup');
        return;
    }
    
    try {
        gui = new dat.GUI({ autoPlace: false });
        
        // Make sure it's added to the DOM
        if (!gui.domElement.parentElement) {
            document.body.appendChild(gui.domElement);
        }
        
        gui.domElement.style.position = 'fixed';
        gui.domElement.style.top = '50px';
        gui.domElement.style.right = '10px';
        gui.domElement.style.zIndex = '10000';
        
        console.log('dat.GUI initialized successfully');
        
        // Add other controls to GUI
        const mainFolder = gui.addFolder('Main Controls');
        const guiControls = {
            debugView: debugView,
            showAllShards: showAllShards
        };
        
        mainFolder.add(guiControls, 'debugView').name('Debug View').onChange((val) => {
            debugView = val;
            const checkbox = document.getElementById('debugView');
            if (checkbox) checkbox.checked = val;
        });
        mainFolder.add(guiControls, 'showAllShards').name('Show All Shards').onChange((val) => {
            showAllShards = val;
            const checkbox = document.getElementById('showAllShards');
            if (checkbox) checkbox.checked = val;
        });
        mainFolder.open();
        
        // Add 3D view toggle
        const viewFolder = gui.addFolder('View Options');
        const viewControls = {
            show3D: false
        };
        viewFolder.add(viewControls, 'show3D').name('3D Box View (or press 3)').onChange((val) => {
            if (typeof toggle3DView === 'function') {
                if (val !== show3DView) {
                    toggle3DView();
                }
            }
        });
        viewFolder.open();

        // Add Source Transform folder for pan and zoom
        const transformFolder = gui.addFolder('Source Transform');
        const transformControls = {
            offsetX: sourceOffsetX,
            offsetY: sourceOffsetY,
            scale: sourceScale,
            reset: function() {
                sourceOffsetX = 0.0;
                sourceOffsetY = 0.0;
                sourceScale = 1.0;
                this.offsetX = 0.0;
                this.offsetY = 0.0;
                this.scale = 1.0;
                // Update GUI controllers
                for (let c of transformFolder.__controllers) {
                    c.updateDisplay();
                }
            }
        };
        transformFolder.add(transformControls, 'offsetX', -1.0, 1.0, 0.01).name('Pan X').onChange((val) => {
            sourceOffsetX = val;
        });
        transformFolder.add(transformControls, 'offsetY', -1.0, 1.0, 0.01).name('Pan Y').onChange((val) => {
            sourceOffsetY = val;
        });
        transformFolder.add(transformControls, 'scale', 0.5, 20.0, 0.1).name('Zoom').onChange((val) => {
            sourceScale = val;
        });
        transformFolder.add(transformControls, 'reset').name('Reset Transform');
        transformFolder.open();
    } catch (e) {
        console.error('Error setting up dat.GUI:', e);
    }
}

// Update dat.GUI with shard controls after shards are loaded
function updateDatGUIShards() {
    if (!gui) {
        console.warn('GUI not initialized, cannot update shard controls');
        return;
    }
    
    try {
        // Remove existing shard folder if it exists
        const folders = gui.__folders || {};
        const existingFolder = folders['Shard Visibility'];
        if (existingFolder) {
            gui.removeFolder(existingFolder);
        }
        
        // Initialize shard visibility - all enabled by default for loaded shards
        for (let i = 0; i < shards.length; i++) {
            if (shardVisibility[`shard${i}`] === undefined) {
                shardVisibility[`shard${i}`] = true;
            }
        }
        
        // Add shard visibility toggles for loaded shards only
        const shardFolder = gui.addFolder('Shard Visibility');
        for (let i = 0; i < shards.length; i++) {
            shardFolder.add(shardVisibility, `shard${i}`).name(`Shard ${i}`);
        }
        shardFolder.open();
        console.log(`Added ${shards.length} shard visibility controls to dat.GUI`);
    } catch (e) {
        console.error('Error updating dat.GUI shards:', e);
    }
}

// FPS counter
let lastTime = performance.now();
let frameCount = 0;
function updateFPS() {
    frameCount++;
    const now = performance.now();
    if (now - lastTime >= 1000) {
        document.getElementById('fps').textContent = frameCount;
        frameCount = 0;
        lastTime = now;
    }
    requestAnimationFrame(updateFPS);
}

// Initialize
async function init() {
    console.log(`Initializing... ${VERSION} - standalone client-side`);

    // Wait for dat.GUI to be available
    let retries = 0;
    while (typeof dat === 'undefined' && retries < 50) {
        await new Promise(resolve => setTimeout(resolve, 100));
        retries++;
    }

    if (typeof dat === 'undefined') {
        console.warn('dat.GUI not loaded after waiting, continuing without GUI');
    } else {
        console.log('dat.GUI is available');
    }

    if (!initWebGL()) {
        alert('Failed to initialize WebGL');
        return;
    }
    console.log('WebGL initialized');

    if (!initMaskShader()) {
        console.warn('Failed to initialize mask shader - continuing without stencil masking');
    } else {
        console.log('Mask shader initialized');
    }

    setupControls();
    setupDatGUI();

    // Load initial image
    try {
        sourceTexture = await loadImageTexture(currentImagePath);
        console.log('Source texture loaded:', sourceTexture.width, 'x', sourceTexture.height);
    } catch (e) {
        console.error('Failed to load source texture:', e);
        alert('Failed to load source texture. Make sure the image file exists.');
        return;
    }

    // Load shards from embedded data (synchronous, no fetch needed)
    loadAllShards();
    console.log(`Total shards loaded: ${shards.length}`);
    
    // Update dat.GUI after shards are loaded
    updateDatGUIShards();
    
    // Start render loop
    console.log('Starting render loop...');
    render();
    updateFPS();
}

// Start when page loads
window.addEventListener('load', init);

