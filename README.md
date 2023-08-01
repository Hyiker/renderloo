# RenderLoo

This is my OpenGL realtime rendering lab based on [loo](https://github.com/Hyiker/loo), in which I can test my ideas.

## Features

- [x] Rendering pipeline
  - [x] Deferred shading
  - [ ] Forward+(under construction)
- [x] Physically based rendering
  - [x] Metallic-roughness workflow(GGX)
  - [ ] Kulla-Conty energy compensation
  - [x] IBL
- [x] Camera
  - [x] Perspective
    - [x] Arcball
    - [x] FPS
- [x] Transparency(2-pass method, alpha test+alpha blend)
- [x] Shadow
  - [x] Main light PCF(5x5 kernel, tent filter)
  - [ ] Cascaded shadow map
- [x] tone mapping
  - [x] ACES
- [x] Model format support
  - [x] glTF 2.0(strongly recommended, since it enables full PBR support)
  - [x] any other formats supported by Assimp
- [ ] Anti-aliasing
  - [x] SMAA(1x only)
  - [ ] TAA(under construction)

## Gallery

![skull](assets/skull.png)

![gun](assets/gun.png)

![Gun with correct transparency and shadow](assets/gun_transparency_shadow.png)

Gun with correct transparency and shadow
