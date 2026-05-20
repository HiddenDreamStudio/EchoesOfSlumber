# Pla d'Implementació de Vulkan i Millores de Rendiment

Aquest document detalla l'arquitectura i els passos per migrar el sistema de renderitzat d'Echoes of Slumber per suportar **Vulkan**, habilitar **shaders avançats (il·luminació difuminada, cuques de llum)** i garantir un rendiment **estable a 60FPS**.

## 1. Estat Actual i Millores Realitzades (Branch: `feat/vulkan-implementation`)
- **60FPS Estables:** S'ha assegurat el *frame capping* a 60 FPS dins de `Engine::FinishUpdate()` utilitzant el paràmetre `targetFrameRate` de `config.xml` i el control de `dt` per evitar desajustos en les animacions i físiques.
- **Pantalla Completa i Borderless:** S'han implementat els mètodes `SetFullscreen` (amb resolució d'escriptori completa) i `SetBorderless` a `Window.cpp`. Això permet canviar de mode de finestra en temps real sense problemes de redimensionament.

## 2. Estratègia d'Implementació per Vulkan i Shaders
Actualment, l'engine utilitza `SDL_Renderer`, que és excel·lent per al dibuix 2D però molt limitat per a shaders de píxels complexos (necessaris per a la "cuca de llum difuminada").
Per implementar Vulkan sense haver d'escriure milers de línies de codi base de l'API de Vulkan (que trencaria l'estructura actual de l'engine), utilitzarem la nova API **SDL3 GPU (`SDL_GPU`)**. Aquesta API de baix nivell de SDL3 utilitza **Vulkan** com a backend principal i permet compilar i executar *fragment shaders* de manera eficient.

### Pas 1: Migració del Render Context a SDL_GPU
S'haurà de modificar `Render.cpp` per substituir la creació de `SDL_Renderer` per `SDL_CreateGPUDevice()` sol·licitant explícitament el backend de Vulkan:

```cpp
// A Render::Awake()
gpuDevice = SDL_CreateGPUDevice(
    SDL_GPU_SHADERFORMAT_SPIRV,
    true, // enable debug mode
    "vulkan" // Forçar backend Vulkan
);
SDL_ClaimWindowForGPUDevice(gpuDevice, Engine::GetInstance().window->window);
```

### Pas 2: Integració del Pipeline Gràfic per a l'Art 2D
L'arquitectura `SDL_GPU` funciona amb "Render Passes" i "Graphics Pipelines". S'haurà de reescriure la funció `DrawTexture()` per:
1. Iniciar un Render Pass (vinculat al *Swapchain* actual).
2. Assignar el Pipeline (configurat per a 2D Sprite Rendering).
3. Assignar els recursos de textura i matrius de transformació al Shader.
4. Executar un `Draw` (per exemple, un *quad* amb instancing per a l'optimització massiva de *draw calls*).

### Pas 3: Shaders d'Il·luminació (Cuca de Llum)
El gran avantatge d'aquest backend és que podrem usar shaders SPIR-V compilats des de GLSL o HLSL.
Per a la il·luminació onírica ("cucas de luz" i fons difuminats), crearem un **Post-Processing Pipeline** i un **Deferred Lighting Pass**:

- **Fons Difuminat (Gaussian Blur):** Un *Compute Shader* o *Fragment Shader* de dues passades que aplica un blur gaussià només a les capes de fons (Parallax).
- **Llum "Cuca de Llum":** El shader base de sprites rebrà un *Uniform Buffer Object (UBO)* amb un array de punts de llum (posició, radi, color). El *fragment shader* calcularà la distància de cada píxel a la llum i sumarà el color amb una funció d'atenuació exponencial (per donar aquest efecte difuminat i atmosfèric oníric).

```glsl
// Exemple simplificat del Fragment Shader d'il·luminació
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

struct PointLight {
    vec2 position;
    vec3 color;
    float radius;
    float intensity;
};

layout(binding = 1) uniform Lights {
    PointLight pointLights[10];
    int count;
} ubo;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 ambient = texColor.rgb * 0.2; // Foscor base
    vec3 finalColor = ambient;

    // Calcular "Cuca de llum"
    for(int i=0; i<ubo.count; ++i) {
        float dist = distance(gl_FragCoord.xy, ubo.pointLights[i].position);
        float att = exp(-dist / ubo.pointLights[i].radius) * ubo.pointLights[i].intensity;
        finalColor += texColor.rgb * ubo.pointLights[i].color * att;
    }

    outColor = vec4(finalColor, texColor.a);
}
```

### Pas 4: Estructura de Treball al Repositori
Com que aquesta és una migració profunda (`#192`), treballarem directament a la branca `feat/vulkan-implementation`. El proper objectiu és afegir els components de inicialització `SDL_GPU` mentre mantenim `SDL_Renderer` temporalment operatiu per a l'UI i facilitar la transició de les textures.
