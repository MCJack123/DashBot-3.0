#include "HAPIH.h"



HandleIH::~HandleIH() {
	if(MainHandle) CloseHandle(MainHandle);
}

HandleIH::HandleIH() {
	MainHandle = 0;
}

HandleIH::HandleIH(DWORD ProcessID) {

	MainHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD, FALSE, ProcessID);

	if (!MainHandle) status = GetLastError();
	return;
}

HandleIH::HandleIH(HANDLE CustomHandle) {
	if (CustomHandle) MainHandle = CustomHandle;
	return;
}

HandleIH & HandleIH::operator=(DWORD ProcessID) {
	if (MainHandle) CloseHandle(MainHandle);
	MainHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD, FALSE, ProcessID);
	if (!MainHandle) status = GetLastError();
	return *this;
}

HandleIH & HandleIH::operator=(HANDLE CustomHandle) {
	if (MainHandle == CustomHandle) return *this;
	if (MainHandle) CloseHandle(MainHandle);
	MainHandle = CustomHandle;
	return *this;
}

const unsigned HandleIH::GetStatus() const {
	return status;
}

HandleIH::operator HANDLE() const {
	if (status) return 0;
	else return MainHandle;
}

HandleIH::operator bool() const {
	return static_cast<bool>(operator HANDLE());
}



void swap(PointerIH & First, PointerIH & Second) {
	using std::swap;
	swap(First.BaseAddr, Second.BaseAddr);
	swap(First.Offsets, Second.Offsets);
	swap(First.Addend, Second.Addend);

}



unsigned DJBHash(const std::vector<unsigned char> & vec){
	unsigned Hash = 5381;
	for (auto & c : vec) {
		Hash = ((Hash << 5) + Hash) + c;
	}
	return Hash;
}

PointerIH & PointerIH::operator<<(std::size_t Off){
	Offsets.push_back(static_cast<std::size_t>(Off));
	return *this;
}

PointerIH & PointerIH::operator=(PointerIH rhs) {
	swap(*this, rhs);
	return *this;
}

PointerIH & PointerIH::operator=(PointerIH && rhs)
{
	swap(*this, rhs);
	return *this;
}

PointerIH::PointerIH(const PointerIH & rhs){
	BaseAddr = rhs.BaseAddr;
	Addend = rhs.Addend;
	Offsets = rhs.Offsets;
}

PointerIH::PointerIH(PointerIH && rhs) {
	PointerIH();
	swap(*this, rhs);
}

PointerIH & PointerIH::operator+=(const size_t & rhs){
	Addend += rhs;
	return *this;
}

PointerIH & PointerIH::operator-=(const size_t & rhs){
	Addend -= rhs;
	return *this;
}





void HackIH::WriteLog(const std::string & Output) const{    
    if (LogStream) {
        std::time_t time = std::time(0);
        std::tm TimeStruct = { 0 };
        if (localtime_s(&TimeStruct, &time) == 0) *LogStream << '[' << std::setw(2) << std::setfill('0') << std::to_string(TimeStruct.tm_hour) << ':' << std::setw(2) << std::setfill('0') << std::to_string(TimeStruct.tm_min) << ':' << std::setw(2) << std::setfill('0') << std::to_string(TimeStruct.tm_sec) << ']' << ' ' << Output << '\n';
        else *LogStream << Output << '\n';
    }
}

void HackIH::GetProcessesInfo() {
	Processes.clear();
	PROCESSENTRY32 pe32;
	HandleIH hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		WriteLog("Failed to get process info, CreateToolhelp32Snapshot returned invalid handle with error: " + std::to_string(GetLastError()));
		return;
	}
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32)) {
		WriteLog("Failed to get process info, Process32First returned invalid handle with error: " + std::to_string(GetLastError()));
		return;
	}
	do {
		Processes.push_back(std::make_tuple(pe32.th32ProcessID, pe32.szExeFile));
		//std::cout << pe32.th32ProcessID <<"\t\t"<< pe32.szExeFile << std::endl;
	} while (Process32Next(hProcessSnap, &pe32));

	std::sort(Processes.begin(), Processes.end(), [](
		std::tuple<unsigned /*ProcessID*/, std::string /*ProcName*/> a,
		std::tuple<unsigned /*ProcessID*/, std::string /*ProcName*/> b
		) {
		return std::get<0>(a) < std::get<0>(b);
	});
}

void HackIH::GetModulesInfo(DWORD PID) {
	Modules.clear();
	MODULEENTRY32 me32;
	HandleIH hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	if (hModuleSnap == INVALID_HANDLE_VALUE) {
		WriteLog("Failed to get modules info, CreateToolHelp32Snapshot returned invalid handle with error: " + std::to_string(GetLastError()));
		return;
	}
	me32.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hModuleSnap, &me32)) {
		WriteLog("Failed to get modules info, Module32First returned invalid handle with error: " + std::to_string(GetLastError()));
		return;
	}
	do {
		Modules.push_back(std::make_tuple(me32.modBaseAddr, me32.modBaseSize, me32.szExePath));

	} while (Module32Next(hModuleSnap, &me32));

	std::sort(Modules.begin(), Modules.end(), [](
		std::tuple<void* /*Address*/, std::size_t /*Size*/, std::string /*ProcName*/> a,
		std::tuple<void* /*Address*/, std::size_t /*Size*/, std::string /*ProcName*/> b
		) {
		return std::get<0>(a) < std::get<0>(b);
	});

}

HackIH::HackIH() {
	GetProcessesInfo();

}

HackIH::~HackIH() {
	
}

void HackIH::WriteProcesses(std::ostream & out) const {
	for (auto c : Processes) {
		out << std::get<0>(c) << "\t\t" << std::get<1>(c) << std::endl;
	}
}

void HackIH::WriteModules(std::ostream & out) const {
	if (isBound) for (auto c : Modules) {
		out << std::get<0>(c) << "\t" << std::get<1>(c) << "\t" << std::get<2>(c) << std::endl;
	}
}

const std::string HackIH::GetProcessName(DWORD PID) const {
	auto it = std::find_if(Processes.begin(), Processes.end(), [PID](
		std::tuple<unsigned /*ProcessID*/,
		std::string /*ProcName*/> x) {
		return std::get<0>(x) == PID;
	});
	if (it == Processes.end()) {
		WriteLog("Couldn't find process with process ID: " + std::to_string(PID));
		return "";
	}
	else return std::get<1>(*it);
}

const DWORD HackIH::GetProcessPID(std::string ProcessName) const {
	auto it = std::find_if(Processes.begin(), Processes.end(), [ProcessName](
		std::tuple<unsigned /*ProcessID*/,
		std::string /*ProcName*/> x) {
		return std::get<1>(x) == ProcessName;
	});
	if (it == Processes.end()) {
		WriteLog("Couldn't find process with process name: " + ProcessName);
		return 0;
	}
	return std::get<0>(*it);
}

void HackIH::SetDebugOutput(std::ostream & OutputStream){
	LogStream = &OutputStream;
}

void HackIH::DisableLog(){
	LogStream = nullptr;
}

bool HackIH::bind(DWORD PID) {
	
	GetModulesInfo(PID);
	BaseAddress = GetModuleAddress(GetProcessName(PID));
	if (!BaseAddress) {
		isBound = 0;
		WriteLog("Couldn't bind to process. (" + GetProcessName(PID) + ' ' + std::to_string(PID)+')');
		GetProcessesInfo();
		return 0;
	}
	ProcHandle = PID;
	if (!ProcHandle) WriteLog("OpenProcess failed with error " + std::to_string(ProcHandle.GetStatus()));
	else WriteLog("Bound successfully to process. (" + GetProcessName(PID) + ' ' + std::to_string(PID) + ')');
	isBound = static_cast<bool>(ProcHandle);
	if (isBound) ProcID = PID;
	return isBound;
}

bool HackIH::bind(std::string ProcessName) {
	return bind(GetProcessPID(ProcessName));
}

void* HackIH::GetModuleAddress(const std::string & ModuleName) const {
	auto it = std::find_if(Modules.begin(), Modules.end(), [ModuleName](
		std::tuple<void* /*Address*/, std::size_t /*Size*/, std::string /*ProcName*/> x
		) {
		auto n = std::get<2>(x).rfind('\\');
		return ModuleName == std::get<2>(x).substr(n + 1);

	});
	if (it == Modules.end()) {
		WriteLog("Couldn't find " + ModuleName + " module.");
		return 0;
	}
	return std::get<0>(*it);
}

void * HackIH::GetPointerAddress(const PointerIH & Pointer) const{
	if(!isBound) return nullptr;
	std::size_t Addr = reinterpret_cast<std::size_t>(Pointer.GetBase());
	if (Pointer.size()) Addr += Pointer[0];
	
	for (int i = 1; i < Pointer.size(); i++) {
		std::size_t ReadAddr = Addr;
		if (!ReadProcessMemory(ProcHandle, reinterpret_cast<void*>(Addr), &ReadAddr, sizeof(std::size_t), 0)) {
			std::stringstream stream;
			stream << "0x" << reinterpret_cast<void*>(Addr);
			WriteLog("Failed reading memory (Error: "+std::to_string(GetLastError())+") at address: " + stream.str());
			return nullptr;
		}
		Addr = ReadAddr;
		Addr += Pointer[i];
	}
	Addr += Pointer.GetAddend();
	std::stringstream stream;
	stream << "0x" << reinterpret_cast<void*>(Addr);
	WriteLog("Pointer Read successfully, final address: " + stream.str());
	return reinterpret_cast<void*>(Addr);
}

std::string HackIH::GetPointerOffset(const PointerIH & Pointer) const
{
	if (!isBound) return std::string();
	auto Addr = GetPointerAddress(Pointer);
	if (!Addr) return std::string();
	for (auto & M : Modules) {
		if (Addr >= (std::get<0>(M)) && reinterpret_cast<std::size_t>(Addr) < (reinterpret_cast<std::size_t>(std::get<0>(M)) + std::get<1>(M))) {
			auto n = std::get<2>(M).rfind('\\');
			;
			std::stringstream stream;
			stream << std::get<2>(M).substr(n + 1);
			stream << " + 0x";
			stream << std::setw(8) << std::setfill('0') << std::hex << reinterpret_cast<std::size_t>(std::get<0>(M)) - reinterpret_cast<std::size_t>(Addr);
			return stream.str();
		}
		
	}
	return std::string();
}

bool HackIH::WriteRaw(const PointerIH & Pointer, const void * Buffer, std::size_t BufSize) const{
	auto Addr = GetPointerAddress(Pointer);
	if (!Addr) return false;
	std::stringstream stream;
	stream << "0x" << Addr;
	if (!WriteProcessMemory(ProcHandle, Addr, Buffer, BufSize, 0)) {
		
		WriteLog("Error in writing memory (Error code: " + std::to_string(GetLastError()) + ") on Address: " + stream.str());
		return false;
	}
	WriteLog("Successfull memory writing on Address: " + stream.str());
	return true;
}

bool HackIH::ReadRaw(const PointerIH & Pointer, void * OutBuffer, std::size_t ReadSize) const{
	auto Addr = GetPointerAddress(Pointer);
	if (!Addr) return false;
	std::stringstream stream;
	stream << "0x" << Addr;
	if (!ReadProcessMemory(ProcHandle, Addr, OutBuffer, ReadSize, 0)) {
		WriteLog("Error in reading memory (Error code: " + std::to_string(GetLastError()) + ") on Address: " + stream.str());
		return false;
	}
	WriteLog("Successfull memory reading on Address: " + stream.str());
	return true;
}

void* HackIH::AllocateRaw(std::size_t Size,DWORD Protection) const {
	auto Addr = VirtualAllocEx(ProcHandle, nullptr, Size, MEM_COMMIT | MEM_RESERVE, Protection);
	if (!Addr) WriteLog("Error in allocating new memory, error No: " + std::to_string(GetLastError()));
	return Addr;

}

HANDLE HackIH::CreateThread(const PointerIH & Pointer,void* Parameter,bool Suspended) const {
	auto Addr = GetPointerAddress(Pointer);
	if (!Addr) return nullptr;
	HANDLE Handle = CreateRemoteThread(ProcHandle, NULL, NULL, static_cast<LPTHREAD_START_ROUTINE>(Addr), Parameter, Suspended?CREATE_SUSPENDED:0, NULL);
	if(!Handle) WriteLog("Error in creating new thread, error No: "+std::to_string(GetLastError()));
	std::stringstream stream;
	stream << "0x" << Addr;
	WriteLog("Thread Started at address: " + stream.str());
	return Handle;
}

bool HackIH::WriteBytes(const PointerIH & Pointer, const std::string & str) const {
	return WriteRaw(Pointer, str.c_str(), str.size());
}

bool HackIH::WriteBytes(const PointerIH & Pointer, const std::vector<unsigned char> & Vec) const {
	return WriteRaw(Pointer, Vec.data(), Vec.size());
}

std::vector<unsigned char> HackIH::ReadBytes(const PointerIH & Pointer, std::size_t ReadSize) const {
	std::vector<unsigned char> Vec(ReadSize);
	if (!ReadRaw(Pointer, Vec.data(), ReadSize)) return std::vector<unsigned char>();
	else return Vec;
}

void * HackIH::AllocateString(const std::string & str) const {
	auto Addr = AllocateRaw(str.size(),PAGE_READWRITE);
	if (!Addr) return nullptr;
	if (!WriteBytes(Addr, str)) return nullptr;
	return Addr;

}

bool HackIH::DllInject(const std::string & FileName,bool UnloadAfterInjection) {
	if (!isBound) return false;
	auto FilePtr = AllocateString(FileName);
	if (!FilePtr) return false;
	HandleIH Thread = CreateThread(reinterpret_cast<void*>(LoadLibraryA), FilePtr,1);
	if (!Thread) return false;
	if (ResumeThread(Thread) == -1) {
		WriteLog("Couldn't resume thread, Error: "+ std::to_string(GetLastError()));
		return false;
	}
	GetModulesInfo(ProcID);

	auto n = FileName.rfind('\\');
	auto InjectedAddr = GetModuleAddress(FileName.substr(n + 1));
	if (!InjectedAddr) {
		WriteLog("DLL isn't loaded inside process (It could have unloaded itself, unlikely)");
		return false;
	}

	if (!UnloadAfterInjection) return true;

	auto WaitCode = WaitForSingleObject(Thread, INFINITE);	
	bool Finished = 0;
	DWORD ThreadStatus = 0;
	switch (WaitCode) {
	case 0x80:
		WriteLog("Wait Abandoned for thread.");
		return false;
	case 0x0:
		WriteLog("Thread finished Injector routine.");
		if (!GetExitCodeThread(Thread, &ThreadStatus)) WriteLog("Couldn't get status of the thread, improbable error, possibly the injection has succedeed?");
		WriteLog("DLL Injection exited with code: " + std::to_string(ThreadStatus));
		Finished = 1;
		break;
	case 0x102:
		WriteLog("Wait routine has timed out, still running, can't unload DLL.");
		break;
	case 0xFFFFFFFF:
		WriteLog("The function has failed, Error: "+std::to_string(GetLastError()));
		return false;
	}
	
	if (!Finished) return true;
	

	
	return DllEject(FileName);
}

bool HackIH::DllEject(const std::string & FileName){
	GetModulesInfo(ProcID);
	auto n = FileName.rfind('\\');
	auto InjectedAddr = GetModuleAddress(FileName.substr(n + 1));
	HandleIH Thread = CreateThread(reinterpret_cast<void*>(FreeLibrary), InjectedAddr, 1);
	if (!Thread) return false;
	if (ResumeThread(Thread) == -1) {
		WriteLog("Couldn't resume thread, Error: " + std::to_string(GetLastError()));
		return false;
	}
	WriteLog("DLL Unloaded");
	return true;
}
