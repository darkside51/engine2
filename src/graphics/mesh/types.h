#pragma once

#include <cstdint>
#include <utility>

namespace engine::graphics::models {

    inline constexpr uint16_t kInvalidValue = 0xff'ffu;

    enum class AccessorComponentType : uint8_t {
		eByte = 0u,
		eUnsigned_Byte = 1u,
		eShort = 2u,
		eUnsigned_Short = 3u,
		eUnsigned_int = 4u,
		eFloat = 5u
	};

    enum class AccessorType : uint8_t {
		eScalar = 0u,
		eVec2 = 1u,
		eVec3 = 2u,
		eVec4 = 3u,
		eMat2 = 4u,
		eMat3 = 5u,
		eMat4 = 6u
	};

    enum class AttributesSemantic : uint16_t {
        ePosition = 1u << 0u,
        eNormal = 1u << 1u,
        eTangent = 1u << 2u,
        eColor0 = 1u << 3u,
        eColor1 = 1u << 4u,
        eColor2 = 1u << 5u,
        eColor3 = 1u << 6u,
        eJoints = 1u << 7u,
        eWeight = 1u << 8u,
        eUv0 = 1u << 9u,
        eUv1 = 1u << 10u,
        eUv2 = 1u << 11u,
        eUv3 = 1u << 12u,
        eUv4 = 1u << 13u,
        eSemanticsCount = 14u
    };
    inline AttributesSemantic operator | (AttributesSemantic a, AttributesSemantic b) {
        return static_cast<AttributesSemantic>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline AttributesSemantic operator & (AttributesSemantic a, AttributesSemantic b) {
        return static_cast<AttributesSemantic>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class BufferTarget : uint8_t {
		eArray_Buffer = 0u,
		eElement_Array_Buffer = 1u,
		eAny_Data = 3u
	};

    enum class PrimitiveMode : uint8_t {
        ePoints = 0u,
        eLines = 1u,
        eLine_Loop = 2u,
        eLine_Strip = 3u,
        eTriangles = 4u,
        eTriangle_Strip = 5u,
        eTriangle_Fan = 6u
    };

    enum class AnimationChannelPath : uint8_t {
        eTranslation = 0u,
        eRotation = 1u,
        eScale = 2u,
        eWeights = 3u
    };

    enum class Interpolation : uint8_t { 
        eLinear = 0u,
        eStep = 1u,
        eCubicSpline = 2u
    };

    enum class MimeType : uint8_t {
		eJpeg = 0u,
		ePng = 1u,
        eOther = 2u
	};

	enum class AlphaMode : uint8_t {
		eOpaque = 0u,
		eMask = 1u,
		eBlend = 2u
	};

	enum class SamplerFilter : uint8_t {
		eNearest = 0u,
		eLinear = 1u,
		eNearest_Mipmap_Nearest = 2u,
		eLinear_Mipmap_Nearest = 3u,
		eNearest_Mipmap_Linear = 4u,
		eLinear_Mipmap_Linear = 5u
	};

    enum class SamplerWrap : uint8_t {
		eClamp_To_Edge = 0u,
		eMirrored_Repeat = 1u,
		eRepeat = 2u
	};

    constexpr inline uint8_t getTypeSize(AccessorComponentType type) noexcept {
        switch (type) {
        case AccessorComponentType::eByte : return 1u;
        case AccessorComponentType::eUnsigned_Byte : return 1u;
        case AccessorComponentType::eShort : return 2u;
        case AccessorComponentType::eUnsigned_Short : return 2u;
        case AccessorComponentType::eUnsigned_int : return 4u;
        case AccessorComponentType::eFloat : return 4u;
        default: return 1u;
        }
    }

    constexpr inline uint8_t getComponentsCount(AccessorType type) noexcept {
        switch (type) {
        case AccessorType::eScalar : return 1u;
        case AccessorType::eVec2 : return 2u;
        case AccessorType::eVec3 : return 3u;
        case AccessorType::eVec4 : return 4u;
        case AccessorType::eMat2 : return 4u;
        case AccessorType::eMat3 : return 9u;
        case AccessorType::eMat4 : return 16u;
        default: return 1u;
        }
    }
}