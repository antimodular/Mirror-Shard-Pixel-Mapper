// 3D Box visualization using Three.js
// Version 4.3 - 16:9 landscape walls (16 wide × 9 tall) attached on long edges
let scene3d, camera3d, renderer3d, controls3d;
let boxGroup;
let show3DView = false;

console.log('===== box3d.js v4.3 loaded =====');
console.log('16:9 landscape walls (16 wide × 9 tall)');
console.log('Walls attached along their 16-unit long edges');
console.log('Press 3 to toggle view');

function init3DView() {
    const container = document.getElementById('container3d');

    // Scene
    scene3d = new THREE.Scene();
    scene3d.background = new THREE.Color(0x111111);

    // Camera - positioned for landscape walls
    camera3d = new THREE.PerspectiveCamera(
        60,
        window.innerWidth / window.innerHeight,
        0.1,
        1000
    );
    camera3d.position.set(0, 6, 25);  // Further back to see wide walls
    camera3d.lookAt(0, 4.5, 0);  // Look at center height of walls (9/2 = 4.5)

    // Renderer
    renderer3d = new THREE.WebGLRenderer({ antialias: true });
    renderer3d.setSize(window.innerWidth, window.innerHeight);
    renderer3d.setPixelRatio(window.devicePixelRatio);
    container.appendChild(renderer3d.domElement);

    // Controls
    controls3d = new THREE.OrbitControls(camera3d, renderer3d.domElement);
    controls3d.enableDamping = true;
    controls3d.dampingFactor = 0.05;
    controls3d.minDistance = 5;
    controls3d.maxDistance = 60;
    controls3d.target.set(0, 4.5, 0);  // Orbit around center height of walls

    // Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.8);
    scene3d.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.3);
    directionalLight.position.set(10, 10, 10);
    scene3d.add(directionalLight);

    // Create box group
    boxGroup = new THREE.Group();
    scene3d.add(boxGroup);

    // Create the 4-wall box
    create4WallBox();

    // Handle window resize
    window.addEventListener('resize', onWindowResize3D, false);

    console.log('3D view initialized');
}

function create4WallBox() {
    // Wall dimensions: 16:9 landscape aspect ratio (16 wide × 9 tall)
    // Matches 1920×1080 quadrant aspect ratio
    // Walls attach along their long (16-unit) edges to form a square box
    const wallWidth = 16;   // Long side (horizontal)
    const wallHeight = 9;   // Short side (vertical)
    const halfWidth = wallWidth / 2;  // 8 units

    console.log(`Creating box with landscape walls: ${wallWidth}w × ${wallHeight}h`);

    // Create placeholder materials with different colors to see each wall
    const materials = [
        new THREE.MeshBasicMaterial({ color: 0xff0000, side: THREE.DoubleSide }), // Front - Red
        new THREE.MeshBasicMaterial({ color: 0x00ff00, side: THREE.DoubleSide }), // Right - Green
        new THREE.MeshBasicMaterial({ color: 0x0000ff, side: THREE.DoubleSide }), // Back - Blue
        new THREE.MeshBasicMaterial({ color: 0xffff00, side: THREE.DoubleSide })  // Left - Yellow
    ];

    // Create 4 walls as planes with 16:9 landscape ratio
    // Walls are positioned to touch along their long (16-unit) edges at corners
    // Box forms a 16×9×16 enclosure (width × height × depth)

    // Front wall (quadrant 0) - Red - faces toward camera (+Z normal)
    const frontWall = new THREE.Mesh(
        new THREE.PlaneGeometry(wallWidth, wallHeight),
        materials[0]
    );
    frontWall.position.set(0, wallHeight / 2, halfWidth);  // Center height, forward
    frontWall.userData.quadrant = 0;
    frontWall.userData.name = 'Front';
    boxGroup.add(frontWall);
    console.log(`Front wall: ${wallWidth}w × ${wallHeight}h at z=${halfWidth}`);

    // Right wall (quadrant 1) - Green - faces left (-X normal after rotation)
    const rightWall = new THREE.Mesh(
        new THREE.PlaneGeometry(wallWidth, wallHeight),
        materials[1]
    );
    rightWall.rotation.y = -Math.PI / 2;  // Rotate to face inward
    rightWall.position.set(halfWidth, wallHeight / 2, 0);  // Right side
    rightWall.userData.quadrant = 1;
    rightWall.userData.name = 'Right';
    boxGroup.add(rightWall);
    console.log(`Right wall: ${wallWidth}w × ${wallHeight}h at x=${halfWidth}`);

    // Back wall (quadrant 2) - Blue - faces toward camera after rotation (-Z normal)
    const backWall = new THREE.Mesh(
        new THREE.PlaneGeometry(wallWidth, wallHeight),
        materials[2]
    );
    backWall.rotation.y = Math.PI;  // Rotate to face inward
    backWall.position.set(0, wallHeight / 2, -halfWidth);  // Back side
    backWall.userData.quadrant = 2;
    backWall.userData.name = 'Back';
    boxGroup.add(backWall);
    console.log(`Back wall: ${wallWidth}w × ${wallHeight}h at z=${-halfWidth}`);

    // Left wall (quadrant 3) - Yellow - faces right (+X normal after rotation)
    const leftWall = new THREE.Mesh(
        new THREE.PlaneGeometry(wallWidth, wallHeight),
        materials[3]
    );
    leftWall.rotation.y = Math.PI / 2;  // Rotate to face inward
    leftWall.position.set(-halfWidth, wallHeight / 2, 0);  // Left side
    leftWall.userData.quadrant = 3;
    leftWall.userData.name = 'Left';
    boxGroup.add(leftWall);
    console.log(`Left wall: ${wallWidth}w × ${wallHeight}h at x=${-halfWidth}`);

    // Add a wireframe box to visualize the enclosure (16×9×16)
    const wireframeGeometry = new THREE.BoxGeometry(wallWidth, wallHeight, wallWidth);
    const wireframeMaterial = new THREE.MeshBasicMaterial({
        color: 0xffffff,
        wireframe: true,
        transparent: true,
        opacity: 0.3
    });
    const wireframe = new THREE.Mesh(wireframeGeometry, wireframeMaterial);
    wireframe.position.y = wallHeight / 2;  // Lift to match wall positions
    boxGroup.add(wireframe);

    // Add floor grid for reference
    const gridHelper = new THREE.GridHelper(wallWidth, 16, 0x444444, 0x222222);
    boxGroup.add(gridHelper);

    // Store wall references
    boxGroup.userData.walls = {
        front: frontWall,
        right: rightWall,
        back: backWall,
        left: leftWall
    };

    console.log('4-wall box created with 16:9 landscape walls (16 wide × 9 tall)');
    console.log('Box dimensions: 16×9×16 (width × height × depth)');
    console.log('Walls connect along their 16-unit long edges at corners');
}

function update3DTextures() {
    if (!show3DView || !boxGroup) return;
    
    // Get the main canvas
    const canvas = document.getElementById('canvas');
    if (!canvas) return;
    
    // Split canvas into 4 quadrants and map to walls
    const quadrantWidth = canvas.width / 2;  // 1920
    const quadrantHeight = canvas.height / 2; // 1080
    
    const walls = boxGroup.userData.walls;
    const wallArray = [walls.front, walls.right, walls.back, walls.left];
    
    wallArray.forEach((wall, index) => {
        // Dispose previous material/texture to avoid leaking GPU memory when swapping images
        if (wall.material) {
            const oldMap = wall.material.map;
            if (oldMap && typeof oldMap.dispose === 'function') oldMap.dispose();
            if (typeof wall.material.dispose === 'function') wall.material.dispose();
        }

        // Create a canvas for this quadrant
        const qCanvas = document.createElement('canvas');
        qCanvas.width = quadrantWidth;
        qCanvas.height = quadrantHeight;
        const qCtx = qCanvas.getContext('2d');
        
        // Calculate source position for this quadrant
        // Layout: [0: TL] [1: TR]
        //         [2: BL] [3: BR]
        const sx = (index % 2) * quadrantWidth;
        const sy = Math.floor(index / 2) * quadrantHeight;
        
        // Draw the quadrant
        qCtx.drawImage(
            canvas,
            sx, sy, quadrantWidth, quadrantHeight,
            0, 0, quadrantWidth, quadrantHeight
        );
        
        // Create texture from quadrant
        const texture = new THREE.CanvasTexture(qCanvas);
        texture.needsUpdate = true;
        texture.minFilter = THREE.LinearFilter;
        texture.magFilter = THREE.LinearFilter;
        
        // Update wall material
        wall.material = new THREE.MeshBasicMaterial({ 
            map: texture, 
            side: THREE.DoubleSide 
        });
        wall.material.needsUpdate = true;
    });
    
    console.log('3D textures updated');
}

function animate3D() {
    if (!show3DView) return;
    
    requestAnimationFrame(animate3D);
    controls3d.update();
    renderer3d.render(scene3d, camera3d);
}

function onWindowResize3D() {
    if (!camera3d || !renderer3d) return;
    
    camera3d.aspect = window.innerWidth / window.innerHeight;
    camera3d.updateProjectionMatrix();
    renderer3d.setSize(window.innerWidth, window.innerHeight);
}

function toggle3DView() {
    show3DView = !show3DView;
    
    const canvas2d = document.getElementById('canvas');
    const container3d = document.getElementById('container3d');
    const controls = document.getElementById('controls');
    const version2d = document.getElementById('version');
    const version3d = document.getElementById('version3d');
    
    if (show3DView) {
        // Switch to 3D view
        canvas2d.style.display = 'none';
        container3d.style.display = 'block';
        if (controls) controls.style.display = 'none';
        if (version2d) version2d.style.display = 'none';
        if (version3d) version3d.style.display = 'block';
        
        // Initialize 3D if not already done
        if (!scene3d) {
            init3DView();
        }
        
        // Update textures and start animation
        update3DTextures();
        animate3D();
        
        console.log('Switched to 3D view - v4.3 with 16:9 landscape walls (16w × 9h)');
    } else {
        // Switch to 2D view
        canvas2d.style.display = 'block';
        container3d.style.display = 'none';
        if (controls) controls.style.display = 'block';
        if (version2d) version2d.style.display = 'block';
        if (version3d) version3d.style.display = 'none';
        
        console.log('Switched to 2D view');
    }
}

// Add keyboard shortcut to toggle (press '3' key)
document.addEventListener('keydown', (e) => {
    if (e.key === '3') {
        toggle3DView();
    }
});

