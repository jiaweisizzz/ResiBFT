# My Settings
SALTICIDAE = $(realpath ./salticidae)
Salticidae_Include_Paths = -I$(SALTICIDAE)/include
Salticidae_Lib_Paths = -L$(SALTICIDAE)/lib

CFLAGS = `pkg-config --cflags libcrypto openssl libuv` # gio-2.0 openssl
LDLIBS = `pkg-config --libs   libcrypto openssl libuv` #-lgmp # gio-2.0 openssl

# SGX SSL Settings
SGXSSL_UNTRUSTED_LIB_PATH := /opt/intel/sgxssl/lib64/
SGXSSL_TRUSTED_LIB_PATH := /opt/intel/sgxssl/lib64/
SGXSSL_INCLUDE_PATH := /opt/intel/sgxssl/include/

# SGX SDK Settings
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE := SIM
SGX_ARCH := x64
SGX_DEBUG := 0
SGX_PRERELEASE := 1
SGX_RELEASE := 0

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

SGX_COMMON_CFLAGS += -O2

# App Settings
Urts_Library_Name := sgx_urts_sim

# Hotstuff Files
Hotstuff_App_Cpp_Files := $(wildcard App/*.cpp)
Hotstuff_App_Cpp_Files := $(filter-out App/Client.cpp App/Damysus.cpp App/Keys.cpp App/Hotsus.cpp App/HotsusBasic.cpp App/Server.cpp, $(Hotstuff_App_Cpp_Files))
Hotstuff_App_Cpp_Objects := $(Hotstuff_App_Cpp_Files:.cpp=.o)

# Damysus Files
Damysus_App_Cpp_Files := $(wildcard App/*.cpp)
Damysus_App_Cpp_Files := $(filter-out App/Client.cpp App/Hotstuff.cpp App/HotstuffBasic.cpp App/Keys.cpp App/Hotsus.cpp App/HotsusBasic.cpp  App/Server.cpp, $(Damysus_App_Cpp_Files))
Damysus_App_Cpp_Files :=  $(Damysus_App_Cpp_Files) App/sgx_utils/sgx_utils.cpp
Damysus_App_Cpp_Objects := $(Damysus_App_Cpp_Files:.cpp=.o)

# Hotsus Files
Hotsus_App_Cpp_Files := $(wildcard App/*.cpp)
Hotsus_App_Cpp_Files := $(filter-out App/Client.cpp App/Damysus.cpp App/Hotstuff.cpp App/HotstuffBasic.cpp App/Keys.cpp App/Server.cpp, $(Hotsus_App_Cpp_Files))
Hotsus_App_Cpp_Files :=  $(Hotsus_App_Cpp_Files) App/sgx_utils/sgx_utils.cpp
Hotsus_App_Cpp_Objects := $(Hotsus_App_Cpp_Files:.cpp=.o)

# App Files
App_Include_Paths := -IApp -I$(SGX_SDK)/include $(Salticidae_Include_Paths)
App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)
App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
App_Cpp_Flags := $(App_C_Flags) -std=c++14 $(CFLAGS)
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L$(SGXSSL_UNTRUSTED_LIB_PATH) -lsgx_usgxssl $(LDLIBS) $(Salticidae_Lib_Paths) -lsalticidae
App_Link_Flags += -lsgx_uae_service_sim

# Enclave Settings
Trts_Library_Name := sgx_trts_sim
Service_Library_Name := sgx_tservice_sim
Crypto_Library_Name := sgx_tcrypto
Enclave_Cpp_Files := Enclave/EnclaveBasic.cpp Enclave/EnclaveDamysus.cpp Enclave/EnclaveHotsus.cpp
Enclave_Include_Paths := -IEnclave -I$(SGX_SDK)/include -I$(SGX_SDK)/include/libcxx -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport -I$(SGXSSL_INCLUDE_PATH)
Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++11 -nostdinc++
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles \
	-L$(SGXSSL_TRUSTED_LIB_PATH) -Wl,--whole-archive -lsgx_tsgxssl -Wl,--no-whole-archive -lsgx_tsgxssl_crypto \
	-L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_pthread -lsgx_tcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0
Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)
Enclave_Unsigned_Name := enclave.so
Enclave_Signed_Name := enclave.signed.so
Enclave_Config_File := Enclave/Enclave.config.xml

# Hotstuff Objects
HotstuffServer: App/Server.o $(Hotstuff_App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(LDLIBS) $(Salticidae_Lib_Paths) -lsalticidae $(Salticidae_Include_Paths)
	@echo "LINK <=  $@"

HotstuffClient: App/Client.o App/KeysFunctions.o App/Node.o App/Nodes.o App/Sign.o App/Signs.o App/Statistics.o App/Transaction.o
	@$(CXX) $^ -o $@ $(LDLIBS) $(Salticidae_Lib_Paths) -lsalticidae $(Salticidae_Include_Paths)
	@echo "LINK <=  $@"

HotstuffKeys: App/Keys.o App/KeysFunctions.o
	@$(CXX) $^ -o $@ $(LDLIBS) $(Salticidae_Lib_Paths) -lsalticidae $(Salticidae_Include_Paths)
	@echo "LINK <=  $@"

# App Objects
App/Enclave_u.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd App && $(SGX_EDGER8R) --untrusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include --search-path $(SGXSSL_INCLUDE_PATH)
	@echo "GEN  =>  $@"

App/Enclave_u.o: App/Enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

App/%.o: App/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

# Damysus Objects
DamysusServer: App/Server.cpp App/Enclave_u.o $(Damysus_App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK =>  $@"

DamysusClient: App/Client.cpp App/KeysFunctions.o App/Node.o App/Nodes.o App/Sign.o App/Signs.o App/Statistics.o App/Transaction.o
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK <=  $@"

DamysusKeys: App/Keys.cpp App/KeysFunctions.o
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK <=  $@"

# Hotsus Objects
HotsusServer: App/Server.cpp App/Enclave_u.o $(Hotsus_App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK =>  $@"

HotsusClient: App/Client.cpp App/KeysFunctions.o App/Node.o App/Nodes.o App/Sign.o App/Signs.o App/Statistics.o App/Transaction.o
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK <=  $@"

HotsusKeys: App/Keys.cpp App/KeysFunctions.o
	@$(CXX) $^ -o $@ $(App_Link_Flags) $(App_Include_Paths)
	@echo "LINK <=  $@"

# Enclave Objects
Enclave/Enclave_t.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd Enclave && $(SGX_EDGER8R) --trusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include --search-path $(SGXSSL_INCLUDE_PATH)
	@echo "GEN  =>  $@"

Enclave/Enclave_t.o: Enclave/Enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

Enclave/%.o: Enclave/%.cpp Enclave/Enclave_t.c
	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

Enclave/%.o: Enclave/%.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC  <=   $<"

$(Enclave_Unsigned_Name): Enclave/Enclave_t.o $(Enclave_Cpp_Objects)
	@echo $(Enclave_Link_Flags)
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Enclave_Signed_Name): $(Enclave_Unsigned_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key Enclave/Enclave_private.pem -enclave $(Enclave_Unsigned_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

.PHONY: clean

clean:
	@rm -f DamysusServer DamysusClient DamysusKeys $(Enclave_Unsigned_Name) $(Enclave_Signed_Name) $(Damysus_App_Cpp_Objects) App/Keys.o App/Client.o App/Server.o App/Enclave_u.* $(Enclave_Cpp_Objects) Enclave/Enclave_t.*
