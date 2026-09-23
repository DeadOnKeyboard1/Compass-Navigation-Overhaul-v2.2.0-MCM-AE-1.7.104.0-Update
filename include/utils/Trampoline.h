#pragma once

#if defined(SKSE_SUPPORT_XBYAK)

#include "SKSE/Trampoline.h"
#include "REX/W32.h"

#include <Psapi.h>
#include <xbyak/xbyak.h>

namespace hooks
{
	template <std::size_t SrcSize>
	class Hook
	{
		static_assert(SrcSize == 5 || SrcSize == 6);

		static constexpr std::size_t getSizeForSrc()
		{
			// Reference: write_5branch() and write_6branch() of Trampoline.h
			if constexpr (SrcSize == 5)
			{
#pragma pack(push, 1)
				// FF /4
				// JMP r/m64
				struct TrampolineAssembly
				{
					// jmp [rip]
					std::uint8_t jmp;	 // 0 - 0xFF
					std::uint8_t modrm;	 // 1 - 0x25
					std::int32_t disp;	 // 2 - 0x00000000
					std::uint64_t addr;	 // 6 - [rip]
				};
#pragma pack(pop)

				return sizeof(TrampolineAssembly);
			}
			else
			{
				return sizeof(std::uintptr_t);
			}
		}

		static constexpr std::size_t getSizeFor(const Xbyak::CodeGenerator& a_dst)
		{
			return getSizeForSrc() + a_dst.getSize();
		}

	public:

		Hook(std::uintptr_t a_src, const Xbyak::CodeGenerator& a_codeGen) :
			size{ getSizeFor(a_codeGen) }, src{ a_src },
			asmCode{ a_codeGen.getCode(), a_codeGen.getCode() + a_codeGen.getSize() }
		{ }

		Hook(std::uintptr_t a_src, std::uintptr_t a_dst) : 
			size{ getSizeForSrc() }, src{ a_src }, dst{ a_dst }
		{ }

		std::size_t getSize() const { return size; }

		std::uintptr_t getSrc() const { return src; }

		std::uintptr_t getDst() const { return dst; }

		void createDst(SKSE::Trampoline* a_allocator)
		{
			if (!asmCode.empty())
			{
				void* buffer = a_allocator->allocate(asmCode.size());
				std::memcpy(buffer, asmCode.data(), asmCode.size());
				dst = reinterpret_cast<std::uintptr_t>(buffer);
			}
		}

	private:

		std::size_t size;
		std::uintptr_t src;
		std::uintptr_t dst = reinterpret_cast<std::uintptr_t>(nullptr);
		std::vector<std::uint8_t> asmCode;
	};

	class Trampoline
	{
	public:

		template <std::size_t SrcSize>
		std::uintptr_t write_branch(Hook<SrcSize>& a_hook)
		{
			if (!a_hook.getDst()) {
				a_hook.createDst(inst);
			}

			return inst->write_branch<SrcSize>(a_hook.getSrc(), a_hook.getDst());
		}

		template <std::size_t SrcSize>
		std::uintptr_t write_call(Hook<SrcSize>& a_hook)
		{
			if (!a_hook.getDst()) {
				a_hook.createDst(inst);
			}

			return inst->write_call<SrcSize>(a_hook.getSrc(), a_hook.getDst());
		}

	protected:

		Trampoline(SKSE::Trampoline& a_trampoline) :
			inst{ &a_trampoline }
		{ }

		Trampoline(const std::string_view& a_name) :
			inst{ new SKSE::Trampoline{ a_name } }, deleteOnDestruct{ true }
		{ }

		~Trampoline() { if (deleteOnDestruct) delete inst; }

		SKSE::Trampoline* inst;

	private:

		bool deleteOnDestruct = false;
	};

	class DefaultTrampoline : public Trampoline
	{
	public:

		DefaultTrampoline(std::size_t a_size, bool a_trySKSEReserve = true) :
			Trampoline{ SKSE::GetTrampoline() }
		{
			SKSE::AllocTrampoline(a_size, a_trySKSEReserve);
		}
	};

	class CustomTrampoline : public Trampoline
	{
	public:

		// Reference: BranchTrampoline::Create() of https://github.com/ianpatt/skse64/blob/master/skse64_common/BranchTrampoline.cpp
		CustomTrampoline(const std::string_view& a_name, void* a_module, std::size_t a_size) :
			Trampoline{ a_name }
		{
			// search backwards from module base
			auto moduleBase = reinterpret_cast<std::uintptr_t>(a_module);
			std::uintptr_t addr = moduleBase;
			std::uintptr_t maxDisplacement = 0x80000000 - (1024 * 1024 * 128);	// largest 32-bit displacement with 128MB scratch space
			std::uintptr_t lowestOKAddress = (moduleBase >= maxDisplacement) ? moduleBase - maxDisplacement : 0;
			addr--;

			void* base = nullptr;

			while (!base)
			{
				MEMORY_BASIC_INFORMATION info;

				if (!VirtualQuery((void*)addr, &info, sizeof(info)))
				{
					break;
				}

				if (info.State == MEM_FREE)
				{
					// free block, big enough?
					if (info.RegionSize >= a_size)
					{
						// try to allocate it
						addr = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize - a_size;

						base = VirtualAlloc(reinterpret_cast<void*>(addr), a_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
					}
				}

				// move back and try again
				if (!base)
				{
					addr = ((uintptr_t)info.BaseAddress) - 1;
				}

				if (addr < lowestOKAddress)
				{
					break;
				}
			}

			if (base) {
				inst->set_trampoline(base, a_size,
					[](void* a_mem, std::size_t)
					{
						REX::W32::VirtualFree(a_mem, 0, 0x00008000u);  // MEM_RELEASE; avoid Windows macro collision
					});
				valid = true;
			} else {
				SKSE::log::error("{}: failed to allocate {} bytes of trampoline memory near module", a_name, a_size);
			}
		}

		bool IsValid() const noexcept { return valid; }

	private:
		bool valid = false;
	};

	class SigScanner
	{
	public:

		template <SKSE::stl::nttp::string str>
		static std::uintptr_t FindPattern(REX::W32::HMODULE a_moduleHandle)
		{
			if (!a_moduleHandle) {
				return 0;
			}

			MODULEINFO moduleInfo{};
			if (!GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(a_moduleHandle), &moduleInfo, sizeof(MODULEINFO)) ||
				!moduleInfo.lpBaseOfDll || moduleInfo.SizeOfImage == 0) {
				SKSE::log::warn("Signature scan: GetModuleInformation failed (error {})", GetLastError());
				return 0;
			}

			const auto moduleBase = reinterpret_cast<std::uintptr_t>(moduleInfo.lpBaseOfDll);
			const auto moduleEnd = moduleBase + static_cast<std::uintptr_t>(moduleInfo.SizeOfImage);
			const std::size_t patternLength = (str.length() + 1) / 3;
			if (patternLength == 0 || moduleInfo.SizeOfImage < patternLength) {
				return 0;
			}
			auto pattern = REL::make_pattern<str>();

			// Scan only committed, readable pages. Directly walking SizeOfImage can cross
			// PAGE_NOACCESS/guard regions in third-party DLLs and raise an access violation.
			std::uintptr_t cursor = moduleBase;
			while (cursor < moduleEnd) {
				MEMORY_BASIC_INFORMATION mbi{};
				if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &mbi, sizeof(mbi))) {
					SKSE::log::warn("Signature scan: VirtualQuery failed at {:X} (error {})", cursor, GetLastError());
					return 0;
				}

				const auto regionBase = (std::max)(cursor, reinterpret_cast<std::uintptr_t>(mbi.BaseAddress));
				const auto rawRegionEnd = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
				const auto regionEnd = (std::min)(moduleEnd, rawRegionEnd);
				const DWORD protection = mbi.Protect & 0xFF;
				const bool readable = mbi.State == MEM_COMMIT &&
					(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 &&
					(protection == PAGE_READONLY || protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
					 protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY);

				if (readable && regionEnd > regionBase && regionEnd - regionBase >= patternLength) {
					for (std::uintptr_t addr = regionBase; addr <= regionEnd - patternLength; ++addr) {
						if (pattern.match(addr)) {
							return addr;
						}
					}
				}

				if (regionEnd <= cursor) {
					break;
				}
				cursor = regionEnd;
			}

			return 0;
		}
	};
}

#else
#error "Xbyak support needed for Trampoline"
#endif