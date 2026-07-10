#pragma once

#include "stl/algorithm.h"
#include "stl/unordered_map.h"
#include "stl/vector.h"

#include <algorithm>
#include <cstdint>

namespace engine {

    enum class InputEventState : uint8_t {
		eNone = 0u,
		ePress = 1u,
		eRelease = 2u,
        eRepeate = 3u
	};

	enum class PointerButton : uint8_t {
		eButton0 = 0u,
		eButton1 = 1u,
		eButton2 = 2u,
		eButton3 = 3u,
		eButton4 = 4u,
		eButton5 = 5u,
		eButton6 = 6u,
		eButton7 = 7u,
		eButton8 = 8u,
		eButton9 = 9u, 
		eButton10 = 10u,
		eButtonNone = 255u,
	};

	enum class KeyboardKey : uint8_t {
		eSpace = 0u,
		eApostrophe = 2u,
	    eComma = 3u,
		eMinus = 4u, 
		ePeriod = 5u,
		eSlash = 6u,
		e0 = 7u,
		e1 = 8u,
		e2 = 9u,
		e3 = 10u,
		e4 = 11u,
		e5 = 12u,
		e6 = 13u,
		e7 = 14u,
		e8 = 15u,
		e9 = 16u,
		eSemicolon = 17u,
		eEqual = 18u,
		eA = 19u,
		eB = 20u,
		eC = 21u,
		eD = 22u,
		eE = 23u,
		eF = 24u,
		eG = 25u,
		eH = 26u,
		eI = 27u,
		eJ = 28u,
		eK = 29u,
		eL = 30u,
		eM = 31u,
		eN = 32u,
		eO = 33u,
		eP = 34u,
		eQ = 35u,
		eR = 36u,
		eS = 37u,
		eT = 38u,
		eU = 39u,
		eV = 40u,
		eW = 41u,
		eX = 42u,
		eY = 43u,
		eZ = 44u,
		eLeftBracket = 45u,
		eBackSlash = 46u,
		eRightBracket = 47u,
		eGraveAccent = 48u,

		eEscape = 49u,
		eEnter = 50u,
		eTab = 51u,
		eBackSpace = 52u,
		eInsert = 53u,
		eDelete = 54u,
		eRight = 55u,
		eLeft = 56u,
		eDown = 57u,
		eUp = 58u,
		ePageUp = 59u,
		ePageDown = 60u,
		eHome = 61u,
		eEnd = 62u,
		eCapsLock = 63u,
		eScrollLock = 64u,
		eNumLock = 65u,
		ePpintScreen = 66u,
		ePause = 67u,
		eF1 = 68u,
		eF2 = 69u,
		eF3 = 70u,
		eF4 = 71u,
		eF5 = 72u,
		eF6 = 73u,
		eF7 = 74u,
		eF8 = 75u,
		eF9 = 76u,
		eF10 = 77u,
		eF11 = 78u,
		eF12 = 79u,
		eF13 = 80u,
		eF14 = 81u,
		eF15 = 82u,
		eF16 = 83u,
		eF17 = 84u,
		eF18 = 85u,
		eF19 = 86u,
		eF20 = 87u,
		eF21 = 88u,
		eF22 = 89u,
		eF23 = 90u,
		eF24 = 91u,
		eF25 = 92u,
		eKP0 = 93u,
		eKP1 = 94u,
		eKP2 = 95u,
		eKP3 = 96u,
		eKP4 = 97u,
		eKP5 = 98u,
		eKP6 = 99u,
		eKP7 = 100u,
		eKP8 = 101u,
		eKP9 = 102u,
		eKPDecimal = 103u,
		eKPDivide = 104u,
		eKPMultiply = 105u,
		eKPSubstract = 106u,
		eKPAdd = 107u,
		eKPEnter = 108u,
		eKPEqual = 109u,
		eLeftShift = 113u,
		eLeftControl = 114u,
		eLeftAlt = 115u,
		eLeftSuper = 116u,
		eRightShift = 117u,
		eRightControl = 118u,
		eRightAlt = 119u,
		eRightSuper = 120u,
		eMenu = 121u,
		eUnknown = 255u
	};

    struct PointerEvent {
		PointerButton button;
		InputEventState state;
		float x;
		float y;

		PointerEvent(PointerButton btn, InputEventState st, float px, float py) : button(btn), state(st), x(px), y(py) {}
	};

	struct KeyEvent {
		KeyboardKey key;
		InputEventState state;

		KeyEvent(KeyboardKey k, InputEventState st) : key(k), state(st) {}
	};

	struct InputKeyMod {
		inline static constexpr uint8_t ALT_PRESSED_BIT = 1u;
		inline static constexpr uint8_t CTRL_PRESSED_BIT = 2u;
		inline static constexpr uint8_t SHIFT_PRESSED_BIT = 4u;
		inline static constexpr uint8_t SUPER_PRESSED_BIT = 8u;

		inline bool isAltPressed()	const noexcept { return (value & (ALT_PRESSED_BIT))		!= 0u; }
		inline bool isCtrlPressed()	const noexcept { return (value & (CTRL_PRESSED_BIT))	!= 0u; }
		inline bool isShiftPressed() const noexcept { return (value & (SHIFT_PRESSED_BIT))	!= 0u; }
		inline bool isSuperPressed() const noexcept { return (value & (SUPER_PRESSED_BIT))	!= 0u; }

		uint8_t value = 0u;
	};

    class InputObserver {
	public:
		virtual ~InputObserver() = default;
		virtual bool onInputPointerEvent(const PointerEvent & event) = 0;
		virtual bool onInputWheelEvent(float dx, const float dy) = 0;
		virtual bool onInpuKeyEvent(const KeyEvent & event, const InputKeyMod & mod) = 0;
		virtual bool onInpuCharEvent(uint16_t code) = 0;

		inline void setPriority(uint16_t p) noexcept { _priority = p; }
		inline uint16_t getPriority() const noexcept { return _priority; }

	private:
		uint16_t _priority = 0u;
	};

    class InputHandler {
	public:

		InputKeyMod getInputKeyMod() const noexcept { return _mask; }

		void addInputObserver(InputObserver* o) {
			_observers.push_back(o);
			if (_observers.size() > 1u) {
				stl::sort(_observers.begin(), _observers.end(), 
                    [](const InputObserver* o1, const InputObserver* o2) { 
                        return o1->getPriority() > o2->getPriority();
                    }
                );
			}
		}
		
		void removeInputObserver(InputObserver* o) noexcept {
			_observers.erase(std::remove(_observers.begin(), _observers.end(), o), _observers.end());
		}

		template <typename E = PointerEvent&&>
		inline void onPointerEvent(E && evt) const {
			for (auto && o : _observers) {
				if (o->onInputPointerEvent(std::forward<E>(evt))) {
					break;
				}
			}
		}

		template <typename E = KeyEvent&&>
		inline void onKeyEvent(E && evt, uint8_t mask) {
			if (mask) {
				changeMask(mask, evt.state);
			}
			for (auto && o : _observers) {
				if (o->onInpuKeyEvent(std::forward<E>(evt), _mask)) {
					break;
				}
			}
		}

		inline void onWheelEvent (float dx, float dy) const {
			for (auto && o : _observers) {
				if (o->onInputWheelEvent(dx, dy)) {
					break;
				}
			}
		}

		inline void onCharEvent(uint16_t code) const {
			for (auto && o : _observers) {
				if (o->onInpuCharEvent(code)) {
					break;
				}
			}
		}

		inline void changeMask(uint8_t mask, InputEventState s) noexcept {
			switch (s) {
				case InputEventState::ePress:
					_mask.value |= mask;
					break;
				case InputEventState::eRelease:
					_mask.value &= ~mask;
					break;
				default:
					break;
			}
		}

	private:
		InputKeyMod _mask;
		stl::vector<InputObserver*> _observers;
	};
}