#pragma once

#include "utils/Logger.h"

#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxValue.h"

constexpr const char* const GFxValueTypeToString(RE::GFxValue::ValueType a_valueType)
{
	switch (a_valueType)
	{
	case RE::GFxValue::ValueType::kNull: return "Null";
	case RE::GFxValue::ValueType::kBoolean: return "Boolean";
	case RE::GFxValue::ValueType::kNumber: return "Number";
	case RE::GFxValue::ValueType::kString: return "String";
	case RE::GFxValue::ValueType::kStringW: return "StringW";
	case RE::GFxValue::ValueType::kArray: return "Array";
	case RE::GFxValue::ValueType::kObject: return "Object";
	case RE::GFxValue::ValueType::kDisplayObject: return "DisplayObject";
	default: return "Undefined";
	}
};

template <logger::level logLevel = logger::level::debug>
class GFxMemberLogger : public RE::GFxValue::ObjectVisitor
{
public:

	void LogMembersOf(const RE::GFxValue& a_value)
	{
		logger::at_level(logLevel, "GFxValue type: {}", GFxValueTypeToString(a_value.GetType()));
		if (a_value.IsObject())
		{
			logger::at_level(logLevel, "{}", "{");
			a_value.VisitMembers(this);
			logger::at_level(logLevel, "{}", "}");
		}
		logger::flush();
	}

	void LogMembersOf(RE::GFxMovieView* a_view, const char* a_pathToMember)
	{
		if (a_view) 
		{
			RE::GFxValue value;
			if (a_view->GetVariable(&value, a_pathToMember)) 
			{
				LogMembersOf(value);
			}
		}
	}

protected:

	using ValueType = RE::GFxValue::ValueType;

	void Visit(const char* a_name, const RE::GFxValue& a_value) override
	{
		std::string_view name = a_name;
		if (name != "PlayReverse" && name != "PlayForward")
		{
			logger::at_level(logLevel, "\tvar {}: {}", a_name, GFxValueTypeToString(a_value.GetType()));
		}
	}
};

template <logger::level logLevel = logger::level::debug>
class GFxArrayLogger
{
public:
	void LogElementsOf(const RE::GFxValue& a_value)
	{
		logger::at_level(logLevel, "GFxValue type: {}", GFxValueTypeToString(a_value.GetType()));
		if (a_value.IsArray())
		{
			logger::at_level(logLevel, "{}", "{");
			const auto count = a_value.GetArraySize();
			for (std::uint32_t i = 0; i < count; ++i) {
				RE::GFxValue element;
				if (a_value.GetElement(i, &element)) {
					logger::at_level(logLevel, "\t[{}]: {}", i, GFxValueTypeToString(element.GetType()));
				}
			}
			logger::at_level(logLevel, "{}", "}");
		}
		logger::flush();
	}

	void LogElementsOf(RE::GFxMovieView* a_view, const char* a_pathToMember)
	{
		if (a_view)
		{
			RE::GFxValue value;
			if (a_view->GetVariable(&value, a_pathToMember))
			{
				LogElementsOf(value);
			}
		}
	}
};
