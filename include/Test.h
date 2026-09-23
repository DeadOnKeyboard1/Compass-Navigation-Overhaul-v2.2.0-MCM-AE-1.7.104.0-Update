#pragma once

class Test : public IUI::GFxDisplayObject
{
public:
	static constexpr inline std::string_view path = "_level0.Test";

	static void InitSingleton(const IUI::GFxDisplayObject& a_test)
	{
		if (!singleton) {
			static Test singletonInstance{ a_test };
			singleton = &singletonInstance;
		} else {
			*static_cast<IUI::GFxDisplayObject*>(singleton) = a_test;
		}
	}

	static void InvalidateSingleton()
	{
		if (singleton) {
			singleton->Invalidate();
		}
	}

private:
	explicit Test(const IUI::GFxDisplayObject& a_test) : IUI::GFxDisplayObject{ a_test } {}
	static inline Test* singleton = nullptr;
};
