#pragma once

#ifdef UNICODE
#undef UNICODE
#endif

#include <Windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

class HandleIH {
private:
	HANDLE MainHandle;
	unsigned status = 0;

	HandleIH & operator=(HandleIH) {};
	HandleIH(HandleIH & ) {};
public:
	~HandleIH();
	HandleIH();

	HandleIH(HANDLE CustomHandle);
	HandleIH(DWORD ProcessID);
	HandleIH & operator=(DWORD ProcessID);
	HandleIH & operator=(HANDLE CustomHandle);
	const unsigned GetStatus() const ;
	operator HANDLE() const;
	explicit operator bool() const;
};

class PointerIH {
private:
	void* BaseAddr;
	std::vector<std::size_t> Offsets;
	std::size_t Addend=0;
public:

	PointerIH() : BaseAddr(0), Addend(0) {};
	template<typename ... Off>
	PointerIH(void* Address, Off... args) : BaseAddr(Address), Offsets{ static_cast<std::size_t>(args)... }, Addend(0) {}
	template<typename ... Off>
	PointerIH(std::size_t Address, Off... args) : BaseAddr(reinterpret_cast<void*>(Address)), Offsets{ static_cast<std::size_t>(args)... }, Addend(0) {}

	PointerIH(const PointerIH &rhs);
	PointerIH(PointerIH && rhs);

	const auto & GetOffsets() const { return Offsets; }
	const auto & GetBase() const { return BaseAddr; }
	const auto & GetAddend() const { return Addend; }

	
	PointerIH & operator <<(std::size_t Off);

	const std::size_t operator [](int x)const { return x<Offsets.size()?Offsets[x]:0; }
	const std::size_t size() const { return Offsets.size(); }

	friend void swap(PointerIH & First, PointerIH & Second);
	PointerIH & operator=(PointerIH rhs);
	PointerIH & operator=(PointerIH && rhs);
	PointerIH & operator+=(const size_t & rhs);
	PointerIH & operator-=(const size_t & rhs);
	


};




class HackIH {
private:
	HandleIH ProcHandle;	//Will be opened for the process
	DWORD ProcID;
	std::vector<std::tuple<unsigned /*ProcessID*/, std::string /*ProcName*/>> Processes;
	std::vector<std::tuple<void* /*Address*/, std::size_t /*Size*/, std::string /*ProcName*/>> Modules;
	std::ostream * LogStream=0;
	bool isBound=0;
	void WriteLog(const std::string & Output) const;
	
public:

	void* BaseAddress = 0;
	~HackIH();
	HackIH();
	void WriteProcesses(std::ostream & out) const;
	void WriteModules(std::ostream & out) const;
	const std::string GetProcessName(const DWORD PID) const;
	const DWORD GetProcessPID(const std::string ProcessName) const;


	void SetDebugOutput(std::ostream & OutputStream);
	void DisableLog();

	void GetProcessesInfo();
	void GetModulesInfo(DWORD PID);

	bool bind(DWORD PID);
	bool bind(std::string ProcessName);
	const bool IsBound() const { return isBound; }

	void* GetModuleAddress(const std::string & ModuleName) const;

	void* GetPointerAddress(const PointerIH & Pointer) const;
	std::string GetPointerOffset(const PointerIH & Pointer) const;


	bool WriteRaw(const PointerIH & Pointer,const void* Buffer,std::size_t BufSize) const;
	bool ReadRaw(const PointerIH & Pointer, void* OutBuffer, std::size_t ReadSize) const;
	void* AllocateRaw(std::size_t Size,DWORD Protection = PAGE_EXECUTE_READWRITE) const ;
	HANDLE CreateThread(const PointerIH & Pointer,void* Parameter,bool Suspended=0) const;

	template <std::size_t S>
	bool WriteBytes(const PointerIH & Pointer, const char(&str)[S]) const { return WriteRaw(Pointer, str, S-1); }
	bool WriteBytes(const PointerIH & Pointer, const std::string & str) const ;
	bool WriteBytes(const PointerIH & Pointer, const std::vector<unsigned char> & Vec) const ;
	template <typename T>
	bool Write(const PointerIH &Pointer,const T & Value) const { return WriteRaw(Pointer, static_cast<const void*>(&Value), sizeof(T)); }

	template <typename T>
	T Read(const PointerIH & Pointer) const {
		T Value=0;
		ReadRaw(Pointer, &Value, sizeof(T));
		return Value;
	}
	std::vector<unsigned char> ReadBytes(const PointerIH & Pointer, std::size_t ReadSize) const;

	void* AllocateString(const std::string & str) const;
	bool DllInject(const std::string & FileName,bool UnloadAfterInjection=0) ;
	bool DllEject(const std::string & FileName);

	const std::vector<std::tuple<unsigned /*ProcessID*/, std::string /*ProcName*/>> & GetProcesses() const { return Processes; }
	const std::vector<std::tuple<void* /*Address*/, std::size_t /*Size*/, std::string /*ProcName*/>> & GetModules() const { return Modules; }
};

unsigned DJBHash(const std::vector<unsigned char> & vec);

inline PointerIH operator+(PointerIH lhs, const size_t & rhs) {
	lhs += rhs;
	return lhs;
}

inline PointerIH operator-(PointerIH lhs, const size_t & rhs) {
	lhs -= rhs;
	return lhs;
}
