# The "My Game Engine" Engine C++/OpenGL (26 Oct 2019 -> Now)

#### Features
- Actor & Component hierarchy
- Transformations system (local & world)
- Shader loading
- Model and Phong material loading
- Basic material system
- Phong lighting system + normal maps
- PBR Shading (with light probe generation for IBL)
- Skeletal animation system
- Shadow mapping for multiple lights (directional, spot or point)
- Parallax Occlusion Mapping
- Ugly SSAO
- FPS camera system
- Postprocessing - HDR, Gamma Correction and efficient gaussian blur
- SMAAT2x, velocity buffer could be useful for implementing per-pixel motion blur
- Basic PhysX integration (with built-in debug system)
- Primitive audio engine using OpenAL Soft; you can only load .wav files for now
- Interpolation system for simple animation
- Some utility classes for future development

**TODO:**
1. .ogg loading
2. Fix calculating the velocity buffer by taking bone matrices from previous frame into account (applies to animated models). SMAA T2x is shaky because of this.
3. Possibly add about some audio engine that is usable and affordable
4. Think of a more catchy name...


#### Made possible thanks to
* Mirosław Zelent https://www.youtube.com/user/MiroslawZelent https://miroslawzelent.pl/
* Joey De Vries https://learnopengl.com/ https://joeydevries.com/#home

#### by Michał Andryskowski
