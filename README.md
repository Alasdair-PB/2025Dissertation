# Dissertation Terraforming with Voxel rendering- Alasdair PB

 This dissertation implements a voxel-based rendering system for dynamic terrain in the Unreal Engine. The project focuses on destruction in the worst case scenario, where terrain is modified each frame. This project first implemented the marching cubes algorithm using compute passes on the GPU as seen in the branch- MarchingCubes_Base. From
 this base, this project investigated different methods for passing the voxel data to shaders in Unreal. The marching cubes implementation was then expanded to utilize an Octree Level of detail (LOD) system with terra forming capabilities. Finally this project investigated implementing Transition cells in a compute pass for the final code seen in main. 
 
 [▶️ Watch the showcase](https://www.youtube.com/watch?v=CKNiDJPdLQk)

 ## Exploring the code:

 ###  Octrees & LOD
 * Dissertation>Plugins/VoxelRendering>Source>Octree>Private>Octree.cpp- Defines the base Octree and initialises buffers for base IsoValues with util functions for applying deformation.
 * Dissertation>Plugins/VoxelRendering>Source>Octree>Private>OctreeNode.h/cpp- Initialises isovalue/type buffers and the Vertex factory for this node with util functions for neighbours & ray intersect checks

 ### Factories & MeshRendering
 * Dissertation>Plugins/VoxelRendering>Source>VoxelRendering\Private\VoxelMeshComponent.cpp- A derived mesh component class that dispatches the compute shaders and initialises it's associated Octree and Sceneproxy
 * Dissertation>Plugins/VoxelRendering>Source>VoxelRenderingUtils\Private\FVoxelVertexFactory.cpp- The vertex factory that defines the vertex stream to shader variants and sets supported shader types.
 * Dissertation>Plugins/VoxelRendering>Source>VoxelRenderingUtils\Private\VoxelRenderBuffers.cpp- Defines the different Render Buffers used by the GPU
  
 ### Compute Passes (.Usf)
 Dissertation>Plugins/VoxelRendering>Shaders>Private>ComputePasses>
   * MarchingCubes.usf: The base marching cubes pass- assigns vertices using March tables and IsoValues. 
   * PlanetNoiseGenerator.usf: A voronoi based noise pass that sets in the IsoValues for terrains to create planets with multi-layered terrain inspired by Outer Wilds and the Wind waker
   * Deformation.usf: Assigns the IsoValues and Type properties of an Octree node based on depth and differences from base values. 







 
