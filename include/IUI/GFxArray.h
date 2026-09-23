#pragma once

#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxValue.h"

class GFxArray : public RE::GFxValue
{
public:
	GFxArray() = default;

	static GFxArray GetFrom(RE::GFxMovieView* a_movieView, const std::string_view& a_pathToArray)
	{
		RE::GFxValue array;
		if (!a_movieView || !a_movieView->GetVariable(&array, a_pathToArray.data()) || !array.IsArray()) {
			return {};
		}
		return GFxArray{ array, a_movieView };
	}

	explicit GFxArray(RE::GFxMovieView* a_movieView) : _movieView{ a_movieView }
	{
		if (!_movieView) {
			return;
		}
		_movieView->CreateArray(this);
		if (!IsArray()) {
			_movieView = nullptr;
		}
	}

	GFxArray(const RE::GFxValue& a_value, RE::GFxMovieView* a_movieView) : _movieView{ a_movieView }
	{
		if (_movieView && a_value.IsArray()) {
			*static_cast<RE::GFxValue*>(this) = a_value;
		} else {
			_movieView = nullptr;
		}
	}

	[[nodiscard]] RE::GFxMovieView* GetMovieView() const noexcept { return _movieView; }
	[[nodiscard]] bool IsUsable() const noexcept { return _movieView && IsArray(); }

	RE::GFxValue GetElement(std::uint32_t a_index) const
	{
		RE::GFxValue value;
		if (IsUsable() && a_index < GetArraySize()) {
			RE::GFxValue::GetElement(a_index, &value);
		}
		return value;
	}

	bool SetElement(std::uint32_t a_index, const RE::GFxValue& a_value)
	{
		return IsUsable() && RE::GFxValue::SetElement(a_index, a_value);
	}

	std::int32_t FindElement(const RE::GFxValue& a_value)
	{
		if (!IsUsable()) {
			return -1;
		}
		for (std::uint32_t i = 0; i < GetArraySize(); ++i) {
			if (GetElement(i) == a_value) {
				return static_cast<std::int32_t>(i);
			}
		}
		return -1;
	}

private:
	RE::GFxMovieView* _movieView = nullptr;
};
