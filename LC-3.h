#pragma once

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string_view>
#include <cstdlib> // for std::exit

#ifdef _WIN32 // WINDOWS
#include <Windows.h>
#include <conio.h> // for _kbhit
#else // LINUX
// TODO
#endif

namespace LC3
{
	constexpr auto memory_size{ (1 << 16) }; // 65536

	namespace R
	{
		enum R : uint8_t
		{
			R0 = 0, // R0=R7 is general purpose registers
			R1,
			R2,
			R3,
			R4,
			R5,
			R6,
			R7,
			PC, // program counter
			COND, // condition flag
			COUNT // count of registers
		};
	}
	namespace OP
	{
		enum OP : uint8_t
		{
			BR = 0, // branch
			ADD,    // add
			LD,     // load
			ST,     // store
			JSR,    // jump register
			AND,    // bitwise and
			LDR,    // load register
			STR,    // store register
			RTI,    // unused
			NOT,    // bitwise not
			LDI,    // load indirect
			STI,    // store indirect
			JMP,    // jump
			RES,    // reserved (unused)
			LEA,    // load effective address
			TRAP    // execute trap
		};
	}
	namespace FL
	{
		// Condition flags
		enum FL : uint8_t
		{
			POS = 1 << 0, // P
			ZRO = 1 << 1, // Z
			NEG = 1 << 2  // N
		};
	}
	namespace TRAP
	{
#ifdef _WIN32 // I hope this didn't break anything
#undef OUT
#undef IN
#endif
		enum TRAP : uint8_t
		{
			GETC = 0x20,  // get character from keyboard, not echoed onto the terminal
			OUT = 0x21,   // output a character
			PUTS = 0x22,  // output a word string
			IN = 0x23,    // get character from keyboard, echoed onto the terminal
			PUTSP = 0x24, // output a byte string
			HALT = 0x25   // halt the program
		};
	}
	namespace MR
	{
		enum MR : uint16_t
		{
			KBSR = 0xFE00, // keyboard status
			KBDR = 0xFE02  // keyboard data
		};
	}

	class VM {
	public:
		/**
		 * @param pc_start Default starting position
		 * @return VM instance
		 */
		static VM* Init(uint16_t pc_start = 0x3000);
		void Run(int argc, const char* argv[]);

		void ReadImage(std::ifstream& file);

		uint16_t ReadMemory(uint16_t address);
		void WriteMemory(uint16_t address, uint16_t value);
	private:
		bool m_Running{false};

		static VM* m_Instance;

		// Main memory (128KB) 0x0000 - 0xFFFF
		uint16_t Memory[memory_size]{};
		// Registers
		uint16_t Reg[R::COUNT]{};

#ifdef _WIN32
		HANDLE m_Stdin{GetStdHandle(STD_INPUT_HANDLE)};
		DWORD m_InputMode, m_OldInputMode;
#endif

		VM(uint16_t pc_start)
		{
			std::cout << "VM CTOR\n";

#ifdef _WIN32
			GetConsoleMode(m_Stdin, &m_OldInputMode);
			m_InputMode = m_OldInputMode ^ ENABLE_ECHO_INPUT ^ ENABLE_LINE_INPUT;
			SetConsoleMode(m_Stdin, m_InputMode);
			FlushConsoleInputBuffer(m_Stdin);
#endif

			Reg[R::PC] = pc_start;
			Reg[R::COND] = FL::ZRO;
		}

		~VM()
		{
#ifdef _WIN32
			SetConsoleMode(m_Stdin, m_OldInputMode);
#endif
			delete m_Instance;
		}


		void InstrAdd(uint16_t instruction);
		void InstrAnd(uint16_t instruction);
		void InstrBr(uint16_t instruction);
		void InstrJmp(uint16_t instruction);
		void InstrRet(uint16_t instruction);
		void InstrJsr(uint16_t instruction);
		void InstrLd(uint16_t instruction);
		void InstrLdi(uint16_t instruction);
		void InstrLdr(uint16_t instruction);
		void InstrLea(uint16_t instruction);
		void InstrNot(uint16_t instruction);
		void InstrRti(uint16_t instruction);
		void InstrSt(uint16_t instruction);
		void InstrSti(uint16_t instruction);
		void InstrStr(uint16_t instruction);
		void InstrTrap(uint16_t instruction);

		void TrapPuts();
		void TrapGetc();
		void TrapOut();
		void TrapIn();
		void TrapPutsp();
		void TrapHalt();

		uint16_t Swap16(uint16_t value);

		uint16_t SignExtend(uint16_t value, int bitCount);

		void UpdateFlag(uint16_t valueRegister);

		void HandleInterrupt(std::string_view message);

#ifdef _WIN32
		uint16_t CheckKey();
#endif
	};
}