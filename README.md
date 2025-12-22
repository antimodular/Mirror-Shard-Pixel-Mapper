# Mirror Shard Pixel Mapper (WebGL)

**Note: This project was developed with AI assistance.**

This repo is a web-based (JS/WebGL2) visualization of the calibration + shader warping pipeline used for **[Rafael Lozano-Hemmer — *Broken Mirror Poets*](https://www.lozano-hemmer.com/broken_mirror_poets.php)**.

The core idea is to **pre-warp** a 4K source image so that, when viewed indirectly through **broken mirror shards**, the reflected fragments reassemble into a coherent image from a specific viewpoint.

## Live Demo

**[View Live Demo on GitHub Pages](https://stephanschulz.ca/light-network/)**

## What this visualization represents

- **Physical installation model**:
  - A **4-wall “box”** made from **four displays/monitor quadrants** (arranged as walls).
  - A **camera** looks at the broken mirrors and the monitors’ reflections.
  - Each mirror shard has a **mask polygon** (its outline in the camera/mirror view) and a **homography calibration** (point correspondences).
- **What we compute**:
  - A per-shard homography from **display/content space** ↔ **camera/mirror space**.
  - A shader that maps from the output pixel position back into the correct source texture location.
- **What we render**:
  - The output is rendered into **masked regions** (one per shard) using the stencil buffer.
  - Each shard shows a different warped part of the same 4K source image.

## Screenshots

### Source (top) + masked/transformed output (bottom)

![Source and transformed result](./Screenshot.jpg)

## Finished artwork photos

From `artwork/`:

![Broken Mirror Poets — photo 1](./artwork/broken_mirror_poets_montreal_RLH_001.jpg)

![Broken Mirror Poets — photo 2](./artwork/broken_mirror_poets_montreal_RLH_002.jpg)

![Broken Mirror Poets — photo 3](./artwork/broken_mirror_poets_montreal_RLH_003.jpg)

## Running

Because the app loads shard data via `fetch()`, you must serve it via a local web server (opening the file directly will hit CORS restrictions).

Example:

```bash
cd /Applications/of_v0.12-2.0_osx_release/apps/brokenMirror/shader_pixelMapper-web
python3 -m http.server 8000
```

Then open `http://localhost:8000/index.html`.

## Repo layout (high level)

- **`index.html`**: UI + script includes
- **`style.css`**: styles
- **`shader.js`**: GLSL (vertex/fragment) shaders
- **`app.js`**: WebGL setup, shard loading, homography compute, stencil masking, render loop
- **`bin/data/shards-4k/`**: per-shard calibration + mask files
  - **`shardN_points.txt`**: calibration point pairs
  - **`shardN_mask.txt`**: shard outline polygon
- **`bin/data/images-4k/`**: 4K source images (3840×2160)
  - Example: `grid.jpg` is 3840×2160


