# Sphere Orbital Simulation - DirectX 11

A real-time 3D orbital simulation demonstrating dynamic lighting and parametric sphere generation, built with C++ and DirectX 11.

![Orbitals](https://github.com/user-attachments/assets/74efb59b-1b8a-4e5e-a8fe-f72337cc5a6a)

## Features

- **Parametric Sphere Generation** - Mesh created using spherical coordinate equations
- **Orbital Mechanics** - Smaller sphere orbiting larger central sphere
- **Dynamic Lighting** - Real-time lighting with adjustable parameters
- **Orbital Controls** - Variable orbital velocity by controlling theta change rate
- **Interactive Parameters** - Live adjustment of lighting and orbital properties

## Technical Implementation

### Sphere Generation
- **Parametric Equations**: 
  - `x = r * sin(φ) * cos(θ)`
  - `y = r * cos(φ)` 
  - `z = r * sin(φ) * sin(θ)`
- **Custom Mesh Generation** - Vertices and indices computed mathematically
- **Theta-only Rotation** - Orbital motion controlled by varying theta over time

### Orbital System
- **Central Sphere** - Large stationary sphere as orbital center
- **Orbiting Sphere** - Smaller sphere following circular orbit
- **Theta Animation** - `θ(t) = ω * t` where ω is adjustable angular velocity
- **Real-time Controls** - Modify orbital speed and direction

## Controls & Parameters

### Real-time Adjustable:
- **Orbital Velocity** - Change rate of theta (ω) for orbital speed
- **Light Direction** - Dynamic light source positioning
- **Light Intensity** - Adjustable diffuse and specular components
- **Light Colors** - Customizable RGB lighting

### Interactive:
- **UI Sliders** - Fine-tune all simulation parameters
