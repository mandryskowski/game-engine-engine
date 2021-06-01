# The "My Game Engine" Engine C++/OpenGL (26 Oct 2019 -> Now)!

![alt text](https://user-images.githubusercontent.com/19364312/120382376-db924500-c323-11eb-8ba4-eaeccd562eef.png)


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

**TO-DO:**
1. .ogg loading
2. Fix calculating the velocity buffer by taking bone matrices from previous frame into account (applies to animated models). SMAA T2x is shaky because of this.
3. Add a normal audio engine that is actually useful in any game making process.
4. Think of a more catchy name...  
Full TO-DO list: https://trello.com/b/TgOo5G0b/game-engine-engine-roadmap


#### Made possible thanks to
* Mirosław Zelent https://www.youtube.com/user/MiroslawZelent or https://miroslawzelent.pl/
* Joey De Vries https://learnopengl.com/ or https://joeydevries.com/#home

#### by Michał Andryskowski
