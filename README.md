# Mirror Shard Image Transform - Web Version

This is a web-based JavaScript/WebGL version of the C++ OpenFrameworks application that uses shaders to transform 4K source images so that when viewed through mirror segments, they create a coherent image.

## Features

- WebGL 2.0 shader-based image transformation
- Support for multiple mirror shards (up to 14)
- Homography-based perspective transformation
- Real-time rendering with configurable controls
- Debug view mode to visualize shard regions

## Files

- `index.html` - Main HTML file with UI controls
- `style.css` - Styling for the interface
- `shader.js` - WebGL shader code (vertex and fragment shaders)
- `app.js` - Main application logic, shard loading, and rendering

## Usage

1. Serve the files from a web server (required for loading shard data files due to CORS restrictions)
2. Open `index.html` in a modern web browser that supports WebGL 2.0
3. The application will automatically load shard data from `bin/data/shards-4k/`
4. Select different source images from the dropdown
5. Toggle debug view to see shard regions colored
6. Adjust background color and opacity
7. Switch between showing all shards or a single shard

## Requirements

- Modern web browser with WebGL 2.0 support (Chrome, Firefox, Safari, Edge)
- Web server to serve files (to avoid CORS issues when loading shard data)

## Data Structure

Shard data files should be in the format:
- `shardN_points.txt` - Contains point pairs for homography computation
- `shardN_mask.txt` - Contains mask polygon points

Source images should be placed in `bin/data/images-4k/`

## Technical Details

The application:
1. Loads shard point data and computes homography matrices
2. Uses WebGL stencil buffers to mask shard regions
3. Applies inverse homography transformations in the fragment shader
4. Samples the source texture at transformed coordinates to create the warped image

