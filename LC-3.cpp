#include "LC-3.h"

#include <bitset>

namespace LC3
{
	VM* VM::m_Instance{ nullptr };

	VM* VM::Init(uint16_t pc_start)
	{
		if(m_Instance == nullptr)
		{
			m_Instance = new VM{pc_start};
			return m_Instance;
		}else
		{
			return m_Instance;
		}
	}

	void VM::Run(int argc, const char* argv[])
	{
        if(argc < 2)
        {
            std::cout << "Not enough arguments\nChoose image\n.\\LC-3.exe <image name>";
            return;
        }

        std::ifstream file(argv[1], std::ios::binary);
        this->ReadImage(file);


        // Debug info
		std::cout << "VM RUN\n";
		std::cout << "R_PC: " << Reg[R::PC] << "\n";
		std::cout << "R_COND: " << Reg[R::COND] << "\n";

		this->m_Running = true;
		while(m_Running)
		{
			uint16_t instruction{VM::ReadMemory(Reg[R::PC]++)}; // read and increment program counter register
			switch (auto op{instruction >> 12})
            {
            case OP::ADD:
                InstrAdd(instruction);
                break;
            case OP::AND:
                InstrAnd(instruction);
				break;
            case OP::NOT:
                InstrNot(instruction);
                break;
            case OP::BR:
                InstrBr(instruction);
                break;
            case OP::JMP:
                InstrJmp(instruction);
				break;
            case OP::JSR:
                InstrJsr(instruction);
				break;
            case OP::LD:
                InstrLd(instruction);
				break;
            case OP::LDI:
                InstrLdi(instruction);
				break;
            case OP::LDR:
                InstrLdr(instruction);
                break;
            case OP::LEA:
                InstrLea(instruction);
                break;
            case OP::ST:
                InstrSt(instruction);
                break;
            case OP::STI:
                InstrSti(instruction);
                break;
            case OP::STR:
                InstrStr(instruction);
                break;
            case OP::TRAP:
                InstrTrap(instruction);
                break;
            case OP::RES:
            case OP::RTI:
			default:
                //BAD OPCODE
                throw std::exception("Bad opcode");
                break;
            }
		}
	}

	void VM::ReadImage(std::ifstream& file)
	{
        std::cout << "Reading image.. \n";
        if (!file)
            throw std::exception("Error: opening image");

        uint16_t origin{};
        if (!file.read(reinterpret_cast<char*>(&origin), sizeof(uint16_t)))
        	throw std::exception("Error: reading origin");

        origin = Swap16(origin);
        std::cout << "Origin = " << std::hex << origin << "\n";

        size_t max = memory_size - origin;
        uint16_t* p = Memory + origin;
        std::streampos pos = file.tellg();
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(pos, std::ios::beg);
#undef min
        if (!file.read(reinterpret_cast<char*>(p), fileSize - pos)) {
            auto rdstate = file.rdstate();
        	std::cout << "error when reading program;\nrdstate: " << rdstate << "\n";
            if (rdstate & std::ios_base::badbit)
                std::cout << "badbit\n";
            if (rdstate & std::ios_base::eofbit)
                std::cout << "eofbit\n";
            if(rdstate & std::ios_base::failbit)
                std::cout << "failbit\n";
            throw std::exception("Error: reading program");
        }
        size_t read = file.gcount() / sizeof(uint16_t);

        while(read-- > 0)
        {
            *p = Swap16(*p);
            p++;
        }
	}

	uint16_t VM::ReadMemory(uint16_t address)
	{
        if (address == MR::KBSR)
        {
            if (CheckKey())
            {
                Memory[MR::KBSR] = 1 << 15;
                Memory[MR::KBDR] = getchar();
            }
            else
            {
                Memory[MR::KBSR] = 0;
            }
        }
		return Memory[address];
	}

	void VM::WriteMemory(uint16_t address, uint16_t value)
	{
        Memory[address] = value;
	}

	uint16_t VM::SignExtend(uint16_t value, int bitCount)
    {
        if ((value >> (bitCount - 1)) & 1)  // if negative (https://w.wiki/AswV)
            value |= (0xFFFF << bitCount); // filling in 1
        return value;
    }

    void VM::UpdateFlag(uint16_t valueRegister)
    {
        if (Reg[valueRegister] == 0)
        {
            Reg[R::COND] = FL::ZRO;
        }
        else if ((Reg[valueRegister] >> 15) & 1)
        {
            Reg[R::COND] = FL::NEG;
        }
        else
        {
            Reg[R::COND] = FL::POS;
        }
    }

    void VM::HandleInterrupt(std::string_view message)
    {
#ifdef _WIN32
        SetConsoleMode(m_Stdin, m_OldInputMode);
#endif
        std::cout << "\n" << message << "\n" << std::flush;
        std::exit(-2);
    }

    uint16_t VM::CheckKey()
    {
		return WaitForSingleObject(m_Stdin, 1000) == WAIT_OBJECT_0 && _kbhit();
    }

    // ADD DR, SR1, SR2
    // ADD DR, SR1, imm5
    void VM::InstrAdd(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t sr1 = (instruction >> 6) & 0x7;
        uint16_t immFlag = (instruction >> 5) & 0x1; // immediate mode flag

        if (immFlag)
        {
            uint16_t imm = SignExtend(instruction & 0x1F, 5);
            Reg[dr] = Reg[sr1] + imm;
        }
        else
        {
            uint16_t sr2 = instruction & 0X7;
            Reg[dr] = Reg[sr1] + Reg[sr2];
        }
        UpdateFlag(dr);
    }

    // Bit-wise logical AND
    // AND DR, SR1, SR2
    // AND DR, SR1, imm5
    void VM::InstrAnd(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t sr1 = (instruction >> 6) & 0x7;
        uint16_t immFlag = (instruction >> 5) & 0x1;
        if(immFlag)
        {
            uint16_t imm = SignExtend(instruction & 0x1F, 5);
            Reg[dr] = Reg[sr1] & imm;
        }else
        {
            uint16_t sr2 = (instruction & 0x7);
            Reg[dr] = Reg[sr1] & Reg[sr2];
        }
        UpdateFlag(dr);
    }

    // Conditional branch
    void VM::InstrBr(uint16_t instruction)
    {
        uint16_t f = (instruction >> 9) & 0x7;
        uint16_t pc_offset = SignExtend((instruction & 0x1ff), 9);
        if (f & Reg[R::COND]) {
            Reg[R::PC] += pc_offset;
        }
    }

    // Jump
    void VM::InstrJmp(uint16_t instruction)
    {
        uint16_t baseR = (instruction >> 6) & 0x7;
        Reg[R::PC] = Reg[baseR];
    }

    // Return to subroutine
    void VM::InstrRet(uint16_t instruction)
    {
        VM::InstrJmp(R::R7 << 6); // base register is R7 (0x7)
    }

    // Jump to subroutine
    void VM::InstrJsr(uint16_t instruction)
    {
        Reg[R::R7] = Reg[R::PC];
        uint16_t flag = (instruction >> 11) & 0x1;
        if(flag)
        {
            Reg[R::PC] += SignExtend(instruction & 0x7FF, 11);
        }else
        {
            uint16_t r = (instruction >> 6) & 0x7;
        	Reg[R::PC] = Reg[r];
        }
    }

    // Load
    void VM::InstrLd(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t pcOffset = SignExtend(instruction & 0x1FF, 9);
        //Reg[dr] = Memory[Reg[R::PC] + pcOffset];
        Reg[dr] = ReadMemory(Reg[R::PC] + pcOffset);
        UpdateFlag(dr);
    }

    // Load indirect
    void VM::InstrLdi(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t pcOffset = SignExtend(instruction & 0x1FF, 9);
        uint16_t address = ReadMemory(Reg[R::PC] + pcOffset);
        Reg[dr] = ReadMemory(address);
        UpdateFlag(dr);
    }

    // Load base + offset
    void VM::InstrLdr(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t baseR = (instruction >> 6) & 0x7;
        uint16_t offset = SignExtend(instruction & 0x3F, 6);
        //Reg[dr] = Memory[Reg[baseR] + offset];
        Reg[dr] = ReadMemory(Reg[baseR] + offset);
        UpdateFlag(dr);
    }

    // Load effective register
    void VM::InstrLea(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t offset = SignExtend(instruction & 0x1FF, 9);
        Reg[dr] = Reg[R::PC] + offset;
        UpdateFlag(dr);
    }

    // Bit-wise complement
	void VM::InstrNot(uint16_t instruction)
    {
        uint16_t dr = (instruction >> 9) & 0x7;
        uint16_t sr = (instruction >> 6) & 0x7;
        Reg[dr] = ~Reg[sr];
        UpdateFlag(dr);
    }

    // Return to interrupt
    // UNUSED
	void VM::InstrRti(uint16_t instruction)
	{
        VM::HandleInterrupt("RTI CALL");
		//throw std::exception("RTI CALL");
	}

    // Store
	void VM::InstrSt(uint16_t instruction)
	{
        uint16_t sr = (instruction >> 9) & 0x7;
        uint16_t offset = SignExtend(instruction & 0x1FF, 9);
        //Memory[Reg[R::PC] + offset] = Reg[sr];
        WriteMemory(Reg[R::PC] + offset, Reg[sr]);
	}

    // Store indirect
	void VM::InstrSti(uint16_t instruction)
	{
        uint16_t sr = (instruction >> 9) & 0x7;
        uint16_t offset = SignExtend(instruction & 0x1FF, 9);
        //Memory[Memory[Reg[R::PC] + offset]] = Reg[sr];
        WriteMemory(ReadMemory(Reg[R::PC] + offset), Reg[sr]);
	}

    // Store base + offset
	void VM::InstrStr(uint16_t instruction)
	{
        uint16_t sr = (instruction >> 9) & 0x7;
        uint16_t baseR = (instruction >> 6) & 0x7;
        uint16_t offset = SignExtend(instruction & 0x3F, 6);
        //Memory[Reg[baseR] + offset] = Reg[sr];
        WriteMemory(Reg[baseR] + offset, Reg[sr]);
	}

	void VM::InstrTrap(uint16_t instruction)
	{
        Reg[R::R7] = Reg[R::PC];
        switch (instruction & 0xFF)
        {
        case TRAP::GETC:
            TrapGetc();
            break;
        case TRAP::OUT:
            TrapOut();
            break;
        case TRAP::PUTS:
            TrapPuts();
            break;
        //case TRAP::IN:
        //    TrapIn();
        //    break;
        case TRAP::PUTSP:
            TrapPutsp();
            break;
        case TRAP::HALT:
            TrapHalt();
            break;
        }
	}

	void VM::TrapPuts()
	{
        std::string output;
        for(std::ptrdiff_t i = Reg[R::R0]; Memory[i] != 0; ++i)
        {
            output.push_back(static_cast<char>(Memory[i]));
        }
        std::cout << output << std::flush;
	}
    
	void VM::TrapGetc()
	{
        Reg[R::R0] = static_cast<uint16_t>(getchar());
		UpdateFlag(R::R0);
	}
    
	void VM::TrapOut()
	{
        std::cout.put(static_cast<char>(Reg[R::R0])).flush();
	}
    
	void VM::TrapIn()
	{
        std::cout << "Enter a character: ";
        Reg[R::R0] = static_cast<uint16_t>(std::cin.get());
        std::cout.put(static_cast<char>(Reg[R::R0])).flush();
        UpdateFlag(R::R0);
	}
    
	void VM::TrapPutsp()
	{
        std::string output;
        for (std::ptrdiff_t i = Reg[R::R0]; Memory[i] != 0; ++i)
        {
            output.push_back(static_cast<char>(Memory[i] & 0xFF)); // first char
            if(Memory[i] >> 8)
        		output.push_back(static_cast<char>(Memory[i] >> 8));    // second char
        }
        std::cout << output << std::flush;
	}
	void VM::TrapHalt()
	{
        std::cout << "HALT" << std::flush;
        this->m_Running = false;
	}

    // Convert to Big-Endian
	uint16_t VM::Swap16(uint16_t value)
	{
        return (value << 8) | (value >> 8);
	}
}
