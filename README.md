Vulkan Grass Rendering
==================================

**University of Pennsylvania, CIS 565: GPU Programming and Architecture, Project 5**

* Sirui Zhu
* Tested on: Windows 11, i7-13620H, RTX 4060 (Personal)

<p align="center">
	<img src="img/grass3.gif" alt="Grass rendering demo" width="640">
</p>

## Overview
Real-time grass in Vulkan: blades are Bezier curves simulated in a compute pass, then tessellated and shaded. Includes wind/gravity/recovery forces, distance-based LOD, and a controllable sphere that pushes grass aside.

## Implemented Features
- Basic Render without physics: 
	<p align="center">
		<img src="img/Basic.png" alt="Basic render without physics" width="520">
	</p>
	- Baseline pipeline to render the plane and blades without applying forces, validating geometry, tessellation, and shading paths.
	- Representing Grass as Bezier Curves: Each blade uses v0/v1/v2 + up; orientation/height/width/stiffness are packed in the .w components per the project spec.
	- Blade shape (tessellated quad ribbon): Quad patch forms a ribbon; width tapers as **w(v) = width * (1 - v^2)** so the tip appears triangular. Vertical tessellation uses LOD 10/5/2 (near/mid/far); horizontal kept at 1 for thin blades.
	<p align="center">
		<img src="img/blade_model.jpg" alt="Blade model diagram" width="520">
	</p>

- Simulating forces: Implemented in the compute shader to update v2 and keep blades stable.
  - Gravity: Environmental gravity plus a front-gravity term derived from blade orientation; contributes to bending at the tip (v2).
  - Recovery: Hooke’s law recovery pulls v2 toward its initial upright position; stiffness controls how strongly it snaps back.
  - Wind: Time-varying, noise-modulated wind direction and gusts; aligned with blade facing to amplify believable sway.
    - Noise source: position-based simplex noise sampled on v0.xz and animated over total time to produce spatially coherent, non-repeating motion across the field.
    - Wave-like motion: a low-frequency sine wave is blended with the noise (phase and amplitude modulated by noise), so nearby blades sway together while still having subtle per-blade variation.
    - Direction + strength: the noise/wave set a wind angle (cos/sin) and a time-varying gust factor; the final wind force scales with gusts and is reduced near the root using a height ratio.
  - Total force: Forces integrated to update v2, projected above ground, then v1 is recomputed to preserve blade length/curvature.
- Culling:
	- Orientation:
		<p align="center">
			<img src="img/Orientation_Culling.png" alt="Orientation culling" width="520">
		</p>
		- Removes blades nearly edge-on to the camera to avoid sub-pixel aliasing. Cull if **abs(dot(viewDir, f)) < 0.9**.
		- Toggle **ORIENTATION_CULLING** (1 on, 0 off) in compute.comp

	- View-frustum:
		<p align="center">
			<img src="img/View_Frustum_Culling.gif" alt="View frustum culling" width="520">
		</p>
		- Clip-space tests for v0, v2, and an approximate midpoint with tolerance to conservatively cull off-screen blades. Cull if all of v0, v2, and m are outside x/y bounds **[-(w+TOLERANCE), +(w+TOLERANCE)]** in clip space.
		- Toggle **VIEW_FRUSTUM_CULLING** (1 on, 0 off) in compute.comp

	- Distance:
		<p align="center">
			<img src="img/Distance_Culling.gif" alt="Distance culling" width="520">
		</p>
		- Bucketed distance culling reduces blade density with range using **NUM_BUCKETS** and **MAX_DISTANCE**. Cull when **index % NUM_BUCKETS < NUM_BUCKETS * (d_proj / MAX_DISTANCE)**, where **d_proj = length((v0 - camPos) - up * dot(v0 - camPos, up))**.
		- Toggle **DISTANCE_CULLING** (1 on, 0 off) in compute.comp
    
- Extra credit: LOD 
	<p align="center">
		<img src="img/LOD.gif" alt="LOD demonstration" width="520">
	</p>
	- Tessellation levels adapt by camera distance (near/mid/far ≈ 10/5/2) to save work while keeping nearby detail.
	- Along blade height (vertical): high subdivision uses 10/5/2 to capture curvature where it matters visually.
	- Across blade width (horizontal): kept at 1.0 (no subdivision) since blades are thin; prioritizes performance over negligible width detail.

- Extra credit: Controllable sphere
	<p align="center">
		<img src="img/Sphere.gif" alt="Interactive sphere pushing grass" width="520">
	</p>
	- A movable sphere repels nearby blades in compute; sphere position/radius are passed as a uniform.  
    - Set ADD_INTERACTIVE_SPHERE (1 on, 0 off) in main.cpp

## Performance Analysis

### Impact of Blade Count on FPS

<p align="center">
	<img src="img/NumBlades.png" alt="FPS vs Number of Blades" width="640">
</p>

Summary (2^8 → 2^18 blades):

- 2^8–2^12: FPS stays very high and almost flat. The GPU spends most of its time on fixed per-frame work rather than per-blade work. Since there are relatively few blades, compute and tessellation units are not fully used, and reading or writing blade data in SSBOs adds almost no cost.

- 2^13–2^15: FPS starts to drop. At this range, the GPU spends more time running blade simulation (forces, wind noise， culling) and tessellation. The workload per frame increases steadily, but memory access is still efficient, so performance falls at a moderate rate.

- 2^16–2^18: The GPU becomes limited by compute and memory bandwidth. Reading and writing all blade data saturates the SSBO traffic, and tessellation patches fill GPU capacity. Frame time grows roughly in proportion to the number of blades — doubling blades roughly doubles render time.

### Impact of Culling on FPS

<p align="center">
	<img src="img/Culling.png" alt="Impact of culling on FPS" width="640">
</p>

- Without culling: The GPU simulates, tessellates, and shades all blades in view space. Cost scales with total N, so FPS drops quickly as density grows.

- Orientation culling: Discards blades whose normals are nearly parallel to the camera’s view direction. The benefit is minor in most cases but noticeable at grazing angles, where many ribbons would otherwise overlap and alias.

- View‑frustum culling: Rejects blades entirely outside the camera frustum early in the compute pass. This gives a strong FPS boost when large areas lie outside the FOV, avoiding unnecessary tessellation and rasterization.

- Distance culling: Reduces far‑field density with bucketed thresholds. This typically provides the largest, most consistent improvement in open scenes, since distant blades contribute least but are most numerous.
