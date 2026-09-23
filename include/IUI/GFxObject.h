#pragma once

#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxValue.h"

namespace IUI
{
	class GFxObject : public RE::GFxValue
	{
	protected:
		GFxObject() = default;

		template <typename... Args>
		GFxObject(RE::GFxMovieView* a_movieView, const char* a_className, Args&&... args) :
			_movieView{ a_movieView }
		{
			if (!_movieView) {
				return;
			}

			std::array<RE::GFxValue, sizeof...(Args)> gfxArgs{ std::forward<Args>(args)... };
			_movieView->CreateObject(this, a_className, sizeof...(Args) ? gfxArgs.data() : nullptr,
				static_cast<std::uint32_t>(sizeof...(Args)));

			if (!IsObject()) {
				_movieView = nullptr;
			}
		}

	public:
		static GFxObject GetFrom(RE::GFxMovieView* a_movieView, const std::string_view& a_pathToObject)
		{
			RE::GFxValue object;
			if (!a_movieView || !a_movieView->GetVariable(&object, a_pathToObject.data()) || !object.IsObject()) {
				return {};
			}
			return GFxObject{ object, a_movieView };
		}

		template <typename... Args>
		explicit GFxObject(RE::GFxMovieView* a_movieView, Args&&... args) :
			_movieView{ a_movieView }
		{
			if (!_movieView) {
				return;
			}

			std::array<RE::GFxValue, sizeof...(Args)> gfxArgs{ std::forward<Args>(args)... };
			_movieView->CreateObject(this, nullptr, sizeof...(Args) ? gfxArgs.data() : nullptr,
				static_cast<std::uint32_t>(sizeof...(Args)));

			if (!IsObject()) {
				_movieView = nullptr;
			}
		}

		GFxObject(const RE::GFxValue& a_value, RE::GFxMovieView* a_movieView) :
			_movieView{ a_movieView }
		{
			if (_movieView && a_value.IsObject()) {
				*static_cast<RE::GFxValue*>(this) = a_value;
			} else {
				_movieView = nullptr;
			}
		}

		void Rebind(const RE::GFxValue& a_value, RE::GFxMovieView* a_movieView)
		{
			Invalidate();
			if (a_movieView && a_value.IsObject()) {
				*static_cast<RE::GFxValue*>(this) = a_value;
				_movieView = a_movieView;
			}
		}

		void Invalidate()
		{
			// Release the managed Scaleform value while the HUD movie is still alive.
			// Callers invalidate on InfinityUI's StartLoadInstances before teardown.
			*static_cast<RE::GFxValue*>(this) = nullptr;
			_movieView = nullptr;
		}

		[[nodiscard]] RE::GFxMovieView* GetMovieView() const noexcept { return _movieView; }
		[[nodiscard]] bool IsUsable() const noexcept { return _movieView && IsObject(); }

		RE::GFxValue GetMember(const std::string_view& a_memberName) const
		{
			RE::GFxValue value;
			if (IsUsable()) {
				RE::GFxValue::GetMember(a_memberName.data(), &value);
			}
			return value;
		}
	 
		bool SetMember(const std::string_view& a_memberName, const RE::GFxValue& a_value)
		{
			return IsUsable() && RE::GFxValue::SetMember(a_memberName.data(), a_value);
		}

		template <typename... Args>
		RE::GFxValue Invoke(const std::string_view& a_functionName, Args&&... args)
		{
			RE::GFxValue result;
			Invoke(a_functionName, &result, std::forward<Args>(args)...);
			return result;
		}

		template <typename... Args>
		bool Invoke(const std::string_view& a_functionName, RE::GFxValue* a_result, Args&&... args)
		{
			if (!IsUsable()) {
				return false;
			}

			std::array<RE::GFxValue, sizeof...(Args)> gfxArgs{ std::forward<Args>(args)... };
			return RE::GFxValue::Invoke(a_functionName.data(), a_result,
				sizeof...(Args) ? gfxArgs.data() : nullptr, static_cast<std::uint32_t>(sizeof...(Args)));
		}

	private:
		RE::GFxMovieView* _movieView = nullptr;
	};
}
