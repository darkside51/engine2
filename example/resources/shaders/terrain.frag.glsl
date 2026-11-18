#version 450

#include "common"

layout(location = 0) in vec2 v_tileUv;
layout(location = 1) in vec2 v_globalUv;
layout(location = 2) in vec3 v_vertexPosition;
layout(location = 3) in vec3 v_worldPosition;
layout(location = 4) in vec3 v_normalUnscale;

layout(location = 0) out float out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_albedo;
layout(location = 3) out vec4 out_emissive;
layout(location = 4) out vec2 out_metalicRoughness;

layout(set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(set = 0, binding = 3) uniform sampler2D u_normals;
layout(set = 0, binding = 4) uniform sampler2D u_colors;

/////
// Простая и быстрая функция генерации псевдослучайного шума
float terrainNoise(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Улучшенная функция генерации цвета террейна
// height: 0.0 - 1.0 (высота)
// moisture: 0.0 - 1.0 (влажность)
// normal: vec3 (нормаль поверхности в мировых координатах или пространстве вида)
// worldPos: vec2 (координаты XZ вершины/пикселя для наложения микро-шума)
vec3 getAdvancedTerrainColor(float height, float moisture, vec3 normal, vec2 worldPos) {
    // 1. Палитра биомов (Vec3 от 0.0 до 1.0)
    vec3 deepWater  = vec3(0.05, 0.15, 0.4);
    vec3 shallow    = vec3(0.2, 0.5, 0.8);
    vec3 sand       = vec3(0.85, 0.75, 0.55);
    vec3 grass      = vec3(0.3, 0.55, 0.25);
    vec3 forest     = vec3(0.15, 0.35, 0.18);
    vec3 rock       = vec3(0.45, 0.45, 0.48);
    vec3 snow       = vec3(0.95, 0.95, 0.98);

    // 2. Рассчитываем крутизну склона (0.0 — плоскость, 1.0 — вертикальная стена)
    // Предполагается, что ось Y направлена вверх
    float steepness = 1.0 - clamp(normal.z, 0.0, 1.0);

    // 3. Базовое процедурное смешивание по высоте и влажности
    vec3 biomeColor = vec3(0.0);

    if (height < 0.15) {
        // Вода
        float t = smoothstep(0.0, 0.15, height);
        biomeColor = mix(deepWater, shallow, t);
    } 
    else if (height < 0.4) {
        // Пляж
        float t = smoothstep(0.15, 0.4, height);
        biomeColor = mix(shallow, sand, t);
    } 
    else {
        // Суша (Равнины, леса, пустыни)
        // Определяем цвет растительности по влажности
        vec3 vegetation = mix(grass, forest, smoothstep(0.4, 0.6, moisture));
        // Если влажность низкая — уходим в пустынный песок
        vec3 landBase = mix(sand, vegetation, smoothstep(0.15, 0.6, moisture));
        
        // Плавный переход от пляжа к суше
        float landT = smoothstep(0.4, 0.625, height);
        biomeColor = mix(sand, landBase, landT);

        // По мере подъема плавно подмешиваем камень к суше перед полноценными скалами
        if (height > 1.25) {
            float rockT = smoothstep(0.6, 0.82, height * 0.45);
            biomeColor = mix(biomeColor, rock, rockT);
        }
        
        // Самые вершины покрываются снегом
        if (height > 2.25) {
            float snowT = smoothstep(0.92, 1.0, height * 0.33);
            biomeColor = mix(biomeColor, snow, snowT);
        }
    }

    // 4. УЛУЧШЕНИЕ 1: Зависимость от наклона (Склоны сбрасывают снег и траву)
    // Если склон крутой, мы принудительно заменяем цвет на голые скалы (rock)
    if (height >= 0.15) { // Применяем только для суши, воду не трогаем
        // Склоны становятся каменными при крутизне выше 0.35 (примерно 20 градусов)
        float slopeFactor = smoothstep(0.45, 0.7, steepness);
        
        // Ослабляем влияние камня на экстремальных высотах (где лежит вечный снег), 
        // но на совсем вертикальных стенах (steepness > 0.8) даже снег срезается.
        float snowProtection = smoothstep(0.85, 0.98, height) * (1.0 - smoothstep(0.8, 0.95, steepness));
        slopeFactor = clamp(slopeFactor - snowProtection, 0.0, 1.0);
        
        biomeColor = mix(biomeColor, rock, slopeFactor);
    }

    // 5. УЛУЧШЕНИЕ 2: Микро-шум для детализации (Избавление от монотонности)
    // Генерируем мелкую текстурную сетку на основе мировых координат
    //float microDetail = terrainNoise(worldPos * 150.0) * 2.0 - 1.0; // Значение от -1.0 до 1.0
    
    // Мягко накладываем шум, слегка изменяя яркость (в пределах 5%)
    // Воду шумим меньше, сушу чуть сильнее
    //float noiseStrength = (height < 0.15) ? 0.02 : 0.05;
    //biomeColor += microDetail * noiseStrength;

    // Гарантируем, что при операциях сложения цвета не вышли за рамки [0, 1]
    return clamp(biomeColor, 0.0, 1.0);
}
/////

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	const vec2 gtexSize = vec2(textureSize(u_normals, 0));
	const vec3 n = -texture(u_normals, v_globalUv / gtexSize).xyz * v_normalUnscale;
	const vec3 normal = normalize(u_viewParams.viewTransposeInverse * n);

	/////
	float baseMoisture = 1.0;//terrainNoise(v_vertexPosition.xy) * 0.5 + 0.5;

	// 2. Корректируем по высоте: 
	// Чем ближе к уровню воды (например, 0.3), тем влажнее. Чем выше в горы, тем суше.
	float heightFactor = 1.0 - smoothstep(0.5, 0.9, v_vertexPosition.z);
	// 3. Соединяем
	float moisture = clamp(baseMoisture * 0.7 + heightFactor * 0.3, 0.0, 1.0);

	vec3 color = getAdvancedTerrainColor(v_vertexPosition.z, moisture, n, v_vertexPosition.xy);
	
	out_normal = vec4(normal * 0.5f + 0.5f, 0.0f);
	//out_albedo = texture(u_colors, v_tileUv);
    out_albedo = vec4(color, 1.0f);
	//out_albedo.xyz = mix(color * vec3(1.0, 0.0, 0.0), color, smoothstep(0.0f, 4.0f, length2(vec2(-64.0, 0.0) - v_worldPosition.xy)));
    //out_albedo.xyz = mix(color * vec3(1.0, 0.0, 0.0), color, smoothstep(0.0f, 0.005f, length2(vec2(0.5, 0.5) - v_globalUv / gtexSize)));
	out_emissive = vec4(0.0f, 0.0f, 0.0f, 0.f);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}