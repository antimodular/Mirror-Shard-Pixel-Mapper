# Mirror Shard Pixel Mapper - Technical Summary

**Developed with AI assistance for Rafael Lozano-Hemmer — *Broken Mirror Poets* artwork**

## Overview

This is a WebGL2-based visualization tool that demonstrates the mathematical calibration and shader warping pipeline used in the physical installation of *Broken Mirror Poets*. The application pre-warps a 4K source image so that when viewed through broken mirror shards, the reflected fragments reassemble into a coherent image from a specific viewpoint.

## Physical Installation Context

The artwork consists of:
- **Four 4K portrait displays** arranged as walls of a box
- **Broken mirror shards** positioned to reflect portions of the displays
- **A camera** positioned at a specific viewpoint looking at the mirror shards
- When correctly calibrated, the camera sees a coherent reconstructed image in the mirror reflections

## Core Technical Concept

### The Calibration Problem

For each mirror shard:
1. We know 4+ **calibration points** in source image space (where content should come from)
2. We know 4+ **display points** where those pixels should appear in the camera view
3. We compute a **homography transformation** (3×3 matrix) that maps between these spaces
4. We use this to pre-warp the source image so reflections align correctly

### The Rendering Pipeline

```
Source Image (4K: 3840×2160)
         ↓
   Apply Scale/Offset (pan/zoom controls)
         ↓
   Homography Transform (per-shard)
         ↓
   Stencil Masking (per-shard polygon)
         ↓
   Output Canvas (3840×2160)
```

## Application Architecture

### File Structure

#### Core Application Files
- **`index.html`** - Main HTML with 2D canvas and controls UI
- **`app.js`** - WebGL2 setup, homography computation, render loop
- **`shader.js`** - GLSL vertex/fragment shaders with homography transforms
- **`box3d.html`** - 3D visualization page (alternative view)
- **`box3d.js`** - Three.js 3D scene with walls and floor reconstruction
- **`style.css`** - UI styling

#### Data Files
- **`data/shardData.js`** - Embedded calibration data for all 14 shards
- **`data/shards/shardN_points.txt`** - Per-shard calibration point pairs (source ↔ display)
- **`data/shards/shardN_mask.txt`** - Per-shard polygon mask vertices
- **`data/images/`** - Source images (3840×2160 resolution)

### Key Components

## 1. Homography Mathematics (`app.js`)

### Computing Homography

The application implements a **Direct Linear Transform (DLT)** algorithm to compute homography matrices from point correspondences:

```javascript
function findHomographyMatrix(srcPoints, dstPoints)
```

**Input:** 4 point pairs (source ↔ destination)  
**Output:** 4×4 matrix in OpenGL column-major format  
**Method:** Gaussian elimination on 8×9 matrix system

The homography H maps display coordinates to source coordinates:
```
[x_source]     [x_display]
[y_source]  = H [y_display]
[   1    ]     [    1     ]
```

### Matrix Inversion

```javascript
function invertMatrix4(m)
```

Inverts the 3×3 homography embedded in a 4×4 matrix to get the reverse mapping (source → display).

### Point Transformation

```javascript
function transformPoint2D(point, matrix)
```

Applies homography to a 2D point, handling homogeneous coordinates and division by w-component.

## 2. WebGL Shader Pipeline (`shader.js`)

### Vertex Shader

Simple pass-through that provides:
- `v_texCoord` - normalized texture coordinates [0,1]
- `v_screenCoord` - normalized device coordinates [-1,1]

### Fragment Shader

The core rendering logic:

1. **Coordinate Transform**
   ```glsl
   vec2 fragCoord = gl_FragCoord.xy;
   fragCoord.y = u_resolution.y - fragCoord.y;  // Flip Y (WebGL vs calibration)
   ```

2. **Select Homography Matrix**
   ```glsl
   mat4 H = getInvH(u_currentShardIndex);  // Gets one of 14 shard matrices
   ```

3. **Transform to Source Space**
   ```glsl
   vec2 sourceCoord = transformForSampling(fragCoord, H);
   ```
   
   This function:
   - Inverts the inverse homography to get display→source mapping
   - Applies matrix transformation
   - Handles homogeneous coordinate division

4. **Apply Scale/Offset (Pan & Zoom)**
   ```glsl
   normalizedTexCoord = (normalizedTexCoord - 0.5) * u_sourceScale + 0.5;
   normalizedTexCoord += u_sourceOffset;
   ```

5. **Sample Texture**
   ```glsl
   color = texture(u_sourceTex, normalizedTexCoord);
   ```

6. **Debug Visualization** (optional)
   ```glsl
   if (u_debugView) {
       color = mix(color, debugColor(s), 0.5);
   }
   ```

### Shader Uniforms

- **Per-Frame:** `u_sourceTex`, `u_resolution`, `u_numShards`, `u_debugView`, `u_bgColor`
- **Transform Controls:** `u_sourceOffset`, `u_sourceScale`
- **Homographies:** `u_invH0` through `u_invH13` (14 shard matrices)
- **Per-Draw:** `u_currentShardIndex` (which shard is being rendered)

## 3. Stencil Buffer Masking (`app.js`)

Each shard has an irregular polygon mask defining its visible region. Rendering uses **two-pass stencil masking**:

### Pass 1: Write Stencil Masks
```javascript
gl.colorMask(false, false, false, false);  // Don't write color
for each visible shard:
    drawPolygonToStencil(shard.transformedMaskPoints, stencilValue);
```

This renders each shard's polygon to the stencil buffer with a unique value (1-14).

### Pass 2: Render Shards
```javascript
gl.colorMask(true, true, true, true);  // Enable color writes
for each visible shard:
    gl.stencilFunc(gl.EQUAL, stencilValue, 0xFF);  // Only render where stencil matches
    gl.uniform1i(currentShardLoc, shardIndex);  // Set which homography to use
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);  // Draw fullscreen quad
```

This renders a fullscreen quad for each shard, but the stencil test limits rendering to the mask polygon.

## 4. Shard Data Structure

Each shard object contains:

```javascript
{
    name: "shard0",
    index: 0,
    
    // Calibration point pairs
    livePoints: [          // Source image coordinates (where content comes from)
        {x, y}, {x, y}, {x, y}, {x, y}
    ],
    displayPoints: [       // Display/camera coordinates (where content appears)
        {x, y}, {x, y}, {x, y}, {x, y}
    ],
    
    // Polygon mask
    maskPoints: [          // Original mask polygon in camera space
        {x, y}, ...        // Variable number of vertices
    ],
    transformedMaskPoints: [ // Mask transformed to display space
        {x, y}, ...
    ],
    
    // Computed transforms
    homography: Float32Array(16),         // 4×4 matrix: display → source
    inverseHomography: Float32Array(16),  // 4×4 matrix: source → display
    homographyReady: true
}
```

### Data Loading

The shard data is **embedded** in `data/shardData.js` as a JavaScript array, avoiding fetch() requests and CORS issues. The raw data was originally in text files:

- `shardN_points.txt`: Space-separated coordinate pairs (4 lines of "srcX srcY dstX dstY")
- `shardN_mask.txt`: Space-separated vertices (N lines of "x y")

## 5. Two Viewing Modes

### Mode 1: 2D Shader View (`index.html`)

**Purpose:** Shows the actual warped output that would be displayed on the physical monitors

**Features:**
- Dropdown to select source images
- Toggle debug view (colors each shard for visibility)
- Show all shards or individual shard selection
- Background color and alpha controls
- Pan (offset X/Y) and zoom (scale) controls for source image
- FPS counter and shard count display

**Technical:** 
- Single WebGL canvas (3840×2160)
- Renders all visible shards using stencil masking
- Real-time shader warping

### Mode 2: 3D Box View (`box3d.html` / `box3d.js`)

**Purpose:** Visualizes the physical installation setup in 3D

**Features:**

#### Four Portrait Walls (16×9 Landscape Geometry)
- Each wall is a 16-unit-wide × 9-unit-tall plane
- Walls form a square box, connected along their 16-unit edges
- Each wall displays one quadrant of the 4K output:
  - **Front wall (Red):** Top-left quadrant, rotated 90° CCW
  - **Right wall (Green):** Top-right quadrant, rotated 90° CCW
  - **Back wall (Blue):** Bottom-left quadrant, rotated 90° CCW
  - **Left wall (Yellow):** Bottom-right quadrant, rotated 90° CCW

#### Floor Reconstruction
- Shows what the camera sees when looking at the mirror shards
- Uses homography transforms to map floor coordinates to source texture
- Each shard renders with its calibrated transform
- Demonstrates how the warped content appears coherent in reflection

#### Quadrant Extraction Process
```javascript
function update3DTextures() {
    const quadrantWidth = 1920;  // Half of 3840
    const quadrantHeight = 1080; // Half of 2160
    
    for each wall:
        // Extract quadrant from main canvas
        sx = (quadrant % 2) * 1920;       // 0 or 1920
        sy = floor(quadrant / 2) * 1080;  // 0 or 1080
        
        // Create texture from quadrant
        qCtx.drawImage(mainCanvas, sx, sy, 1920, 1080, 0, 0, 1920, 1080);
        texture = new THREE.CanvasTexture(qCanvas);
        
        // Apply to wall material
        wall.material.map = texture;
}
```

#### Camera Controls
- **Orbit controls:** Drag to rotate view around the box
- **Camera sway:** Optional automatic rotation animation
- **Adjustable controls:** Sway amount, grid visibility, line widths
- **Keyboard shortcut:** Press '3' to toggle between 2D and 3D views

#### Lighting
- Ambient light (0.8 intensity)
- Directional light from top-right
- Total illumination designed to show textures clearly

## 6. Source Image Transform System

### Purpose

Allows dynamic adjustment of source image content without reloading different image files. This simulates the effect of loading images with different sized content (like A-1.png, A-2.png, A-3.png).

### Controls

- **Scale (0.5 - 20.0):** Zoom in/out from center
  - Scale > 1.0 = zoom in (content appears larger)
  - Scale < 1.0 = zoom out (content appears smaller)
  
- **Offset X (-1.0 to 1.0):** Pan horizontally
  - Negative = pan left, Positive = pan right
  
- **Offset Y (-1.0 to 1.0):** Pan vertically
  - Negative = pan down, Positive = pan up

### Implementation

The transform is applied **in the shader** before homography transformation:

```glsl
vec2 normalizedTexCoord = sourceCoord / u_resolution;

// Scale around center point (0.5, 0.5)
normalizedTexCoord = (normalizedTexCoord - 0.5) * u_sourceScale + 0.5;

// Apply pan offset
normalizedTexCoord += u_sourceOffset;
```

**Critical:** The transform happens at the shader input stage, ensuring:
1. All shards sample coherently from the transformed source
2. Homography calculations operate on the adjusted content
3. Floor reconstruction in 3D view remains aligned

### Cross-Frame Communication

The 3D view (box3d.html) can control the 2D canvas transform via exposed global functions:

```javascript
window.setSourceScale(scale);
window.setSourceOffset(x, y);
window.getSourceTransform();
```

## 7. User Interface

### dat.GUI Controls

Dynamic GUI with folders:

1. **Main Controls**
   - Debug View toggle
   - Show All Shards toggle

2. **View Options**
   - 3D Box View toggle

3. **Source Transform**
   - Pan X/Y sliders
   - Zoom slider
   - Reset button

4. **Shard Visibility** (populated after data loads)
   - Individual toggle for each of 14 shards

### HTML Controls

Traditional HTML form controls for:
- Source image selection dropdown
- Debug view checkbox
- Show all shards vs. single shard selector
- Background color picker
- Background alpha slider

### Performance Display

- **FPS counter:** Updates every second
- **Shard count:** Shows number of loaded shards

## 8. WebGL Configuration

### Context Options
```javascript
gl = canvas.getContext('webgl2', {
    stencil: true,    // Required for mask rendering
    antialias: false, // Disabled for performance
    depth: false,     // Not needed (2D rendering)
    alpha: true       // Transparent background support
});
```

### Verification
- Checks for WebGL2 support
- Verifies stencil buffer availability (required for masking)
- Logs stencil buffer bit depth

### Blending
```javascript
gl.enable(gl.BLEND);
gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
```

Standard alpha blending for transparent backgrounds.

## 9. Performance Optimization

### Techniques Used

1. **Embedded Data:** Shard data embedded in JavaScript to avoid fetch() latency
2. **Reusable Buffers:** Fullscreen quad buffers created once, reused per frame
3. **Stencil Masking:** Hardware-accelerated polygon clipping instead of shader conditionals
4. **Efficient Rendering:** Only renders visible shards (respects visibility toggles)
5. **Texture Caching:** Source textures cached until image changes
6. **No Antialiasing:** Disabled for faster rendering (3840×2160 is high enough resolution)

### Rendering Complexity

Per frame:
- 1 clear operation
- N stencil mask draws (N = number of visible shards, max 14)
- N fullscreen quad draws with different uniforms
- Total complexity: O(visible_shards)

Typical performance: 60 FPS at 4K resolution on modern GPUs.

## 10. Coordinate System Transformations

### Multiple Coordinate Spaces

1. **Source Image Space**
   - Range: (0,0) to (3840, 2160)
   - Origin: Top-left
   - Used for: texture sampling, calibration source points

2. **Display/Camera Space**
   - Range: (0,0) to (3840, 2160)
   - Origin: Top-left (after Y-flip)
   - Used for: calibration display points, mask polygons

3. **WebGL Framebuffer Space**
   - Range: (0,0) to (3840, 2160)
   - Origin: Bottom-left (WebGL convention)
   - Used for: `gl_FragCoord.xy`

4. **Normalized Texture Space**
   - Range: (0,0) to (1,1)
   - Origin: Bottom-left
   - Used for: texture sampling in GLSL

5. **Normalized Device Coordinates (NDC)**
   - Range: (-1,-1) to (1,1)
   - Origin: Center
   - Used for: vertex shader output

### Key Transformations

**WebGL to Calibration Y-Flip:**
```glsl
fragCoord.y = u_resolution.y - fragCoord.y;
```

**Pixel to Normalized Texture Coords:**
```glsl
vec2 normalizedTexCoord = pixelCoord / u_resolution;
```

**NDC to Pixel Coords (for stencil masking):**
```glsl
vec2 ndc = vec2(
    (pixelCoord.x / u_resolution.x * 2.0 - 1.0),
    (1.0 - pixelCoord.y / u_resolution.y * 2.0)
);
```

## 11. Image Assets

### Provided Test Images

Located in `data/images/`:
- **`source_oath.png`** - Default source image
- **`source_gab.jpg`** - Alternative source
- **`grid.jpg`** - Grid pattern for calibration verification
- **`test_source_4k_large_text.png`** - Text-based test image
- **`A-1.png`, `A-2.png`, `A-3.png`** - Pre-scaled content examples

### Expected Format
- Resolution: 3840×2160 (4K)
- Format: JPG or PNG
- Color space: RGB
- Aspect ratio: 16:9

### Loading Custom Images

Users can select from dropdown or (in 3D view) load custom images via file picker. The application supports:
- CORS-compliant image loading
- Dynamic texture upload to GPU
- Automatic 3D view texture refresh

## 12. Build and Deployment

### Development Server

Due to security restrictions (CORS, file:// protocol), a web server is required:

```bash
cd /path/to/shader_pixelMapper-web
python3 -m http.server 8000
```

Then access:
- `http://localhost:8000/index.html` - 2D view
- `http://localhost:8000/box3d.html` - 3D view

### Dependencies (CDN)

- **Three.js r128** - 3D rendering
- **OrbitControls** - Camera controls
- **dat.GUI 0.7.9** - UI controls

No build step or package manager required.

### Browser Requirements

- WebGL 2.0 support
- ES6 JavaScript support
- Stencil buffer support
- Tested on: Chrome, Firefox, Safari (recent versions)

## 13. Debugging Features

### Debug View Mode

Enabled via checkbox or dat.GUI. When active:
- Each shard is tinted with a unique color
- Helps visualize shard boundaries and overlaps
- Colors are semi-transparent (50% mix with source image)

**Color assignments:**
```
Shard 0: Red      Shard 1: Green    Shard 2: Blue     Shard 3: Yellow
Shard 4: Magenta  Shard 5: Cyan     Shard 6: Orange   Shard 7: Purple
Shard 8: Teal     Shard 9: Olive    Shard 10: Maroon  Shard 11: Dark Green
Shard 12/13: Teal variants
```

### Console Logging

Extensive console output tracks:
- WebGL initialization status
- Shader compilation/linking
- Shard loading progress
- Homography computation
- Texture updates
- View mode switches

### Visual Debugging

- **3D Wireframe:** Shows box enclosure boundaries
- **Grid Helper:** Floor grid for spatial reference
- **Wall Colors:** Distinct colors per wall for identification
- **Shard Outlines:** Optional outlines on walls and floor

## 14. Known Limitations

1. **Fixed Shard Count:** Maximum 14 shards (shader uniform limit)
   - Can be extended by using uniform arrays or texture lookups
   
2. **No Runtime Calibration:** Calibration data is pre-computed
   - Adding calibration UI would require point selection interface
   
3. **No Mobile Support:** Designed for desktop/large displays
   - Touch controls and responsive layout not implemented
   
4. **4K Output Only:** Canvas is fixed at 3840×2160
   - Could be made responsive with aspect ratio preservation
   
5. **Browser-Dependent Performance:** Relies on WebGL2 hardware acceleration
   - Older GPUs may struggle with 4K real-time rendering

## 15. Future Enhancement Possibilities

### Technical Improvements
- Dynamic shard loading from server/API
- Runtime calibration point editing
- Texture array for unlimited shards
- Compute shader optimization
- Multi-resolution support

### Features
- Recording/playback of camera paths
- Video source support
- Real-time camera input
- Animation timeline
- Preset management
- Export to video/image sequences

### UI/UX
- Mobile-responsive layout
- Touch gesture controls
- Keyboard shortcuts
- Preset save/load system
- Calibration workflow UI

## 16. Mathematical Background

### Homography (Projective Transform)

A homography is a transformation that maps points between two planes. Represented as a 3×3 matrix:

```
[x']   [h11 h12 h13] [x]
[y'] = [h21 h22 h23] [y]
[w']   [h31 h32 h33] [1]
```

Where final coordinates are: `x' = x'/w'`, `y' = y'/w'`

**Properties:**
- Preserves straight lines
- Maps parallel lines to (potentially) non-parallel lines
- Requires 4 point correspondences to solve
- Has 8 degrees of freedom (9 parameters - 1 for scale)

### Direct Linear Transform (DLT)

Algorithm to compute H from point correspondences:

1. **Build equation system** (2 equations per point pair):
   ```
   -xi  -yi  -1   0    0    0   xi*xi'  yi*xi'  -xi'
   0    0    0   -xi  -yi  -1   xi*yi'  yi*yi'  -yi'
   ```

2. **Stack 4 point pairs** → 8×9 matrix

3. **Gaussian elimination** → find homography parameters

4. **Reconstruct 3×3 matrix** from solution

### Mirror Reflection Geometry

In the physical installation, each mirror shard reflects a portion of the display. The calibration process establishes:

1. **Where on the source** content should originate (livePoints)
2. **Where in the camera view** it should appear after reflection (displayPoints)
3. **Homography** encodes the mirror's position, angle, and projective distortion

This allows pre-warping the display content so the reflected image appears correct from the camera viewpoint.

## 17. Credits and Context

### Artwork
**Rafael Lozano-Hemmer — *Broken Mirror Poets***  
Website: [lozano-hemmer.com/broken_mirror_poets.php](https://www.lozano-hemmer.com/broken_mirror_poets.php)

### Development
- **Original Installation:** Production team at Studio lozano-hemmer
- **Web Visualization:** Developed with AI assistance
- **Purpose:** Educational tool and calibration visualization

### License
Refer to repository license file for usage terms.

## 18. Conclusion

This application demonstrates sophisticated real-time computer graphics techniques:
- **Projective geometry** for mirror reflection calibration
- **Stencil buffer masking** for complex polygon clipping
- **Shader-based warping** for efficient per-pixel transformation
- **Cross-frame communication** between 2D and 3D views
- **Interactive controls** for exploring the parameter space

It serves as both a practical calibration tool and an educational resource for understanding the mathematics behind anamorphic projection and mirror-based installations in digital art.

