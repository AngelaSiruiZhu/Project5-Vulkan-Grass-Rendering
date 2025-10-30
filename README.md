Vulkan Grass Rendering
==================================

**University of Pennsylvania, CIS 565: GPU Programming and Architecture, Project 5**

* Sirui Zhu
* Tested on: Windows 11, i7-13620H, RTX 4060 (Personal)

![](img/grass1.gif)

## Overview
Real-time grass in Vulkan: blades are Bezier curves simulated in a compute pass, then tessellated and shaded. Includes wind animation, distance-based LOD, and an optional interactive sphere that pushes grass aside.

## Implemented Features
- Basic Render without physics: Baseline pipeline to render the plane and blades without applying forces, validating geometry, tessellation, and shading paths.
 -- Representing Grass as Bezier Curves: Each blade uses v0/v1/v2 + up; orientation/height/width/stiffness are packed in the .w components per the project spec.
- Simulating forces: Implemented in the compute shader to update v2 and keep blades stable.
 -- Gravity: Environmental gravity plus a front-gravity term derived from blade orientation; contributes to bending at the tip (v2).
 --Recovery: Hooke’s law recovery pulls v2 toward its initial upright position; stiffness controls how strongly it snaps back.
 -- Wind: Time-varying, noise-modulated wind direction and gusts; aligned with blade facing to amplify believable sway.
 -- Total force: Forces integrated to update v2, projected above ground, then v1 is recomputed to preserve blade length/curvature.
- Culling: Optional compile-time toggles to reduce overdraw and cost.
 --Oriental: Orientation culling removes blades nearly edge-on to the camera to avoid sub-pixel aliasing.
 --View-frustum: Clip-space tests for v0, v2, and an approximate midpoint with tolerance to conservatively cull off-screen blades.
 --Distance: Bucketed distance culling reduces blade density with range using NUM_BUCKETS and MAX_DISTANCE.
- Extra credit: LOD: Tessellation levels adapt by camera distance (near/mid/far ≈ 10/5/2) to save work while keeping nearby detail.
- Extra credit: interactive sphere.: A movable sphere repels nearby blades in compute; sphere position/radius are passed as a uniform.
--Set `ADD_INTERACTIVE_SPHERE` in `src/main.cpp` (1 on, 0 off).
