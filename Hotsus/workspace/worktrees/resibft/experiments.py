# Imports
from datetime import datetime
from enum import Enum
from pathlib import Path
from subprocess import Popen
import argparse
import glob
import json
import math
import multiprocessing
import os
import subprocess
import time

# Parameters of experiments
NumViews = 30  # Number of views in each run
NumFaults = [1]  # List of numbers of faults
NumClients = 1  # Number of clients
NumClientTransactions = 1  # Number of transactions sent by each client
NumExperiments = 100  # Number of times to repeat each experiment
NumTransactions = 400  # Number of transactions
PayloadSize = 0  # Total size of a transaction
CutOffBound = 100  # Stop experiment after some time
NetworkLatency = 0  # Network latency in ms
NetworkVariation = 0  # Variation of the network latency
SleepTime = 0  # Time of clients sleep between 2 sends (in microseconds)
LeaderChangeTime = 30  # Timeout before changing leader (in seconds)
HotsusFactor = 1  # Const factor of trusted replicas in Hotsus
GroupMembers = 100

# Parameters of making files
NumCores = multiprocessing.cpu_count()  # Number of cores to use to make

# Parameters of Docker
Docker = "docker"
DockerBase = "hotsus"  # Name of the docker container
DockerMemory = 0  # Memory used by docker containers (0 means no constraints)
DockerCpu = 0  # Cpus used by containers (0 means no constraints)

# Parameters of Aliyun
LocalPath = "/root/Hotsus/"
Aliyun = "aliyun"
InstanceType = "ecs.e-c1m2.large"
InstanceInternetChargeType = "PayByBandwidth"
InstanceSystemDiskCategory = "cloud_essd"
InstancePassword = "ilove1533WAIBIBABU"
InstanceCores = 2

# Parameters of choosing protocols
RunHotstuff = False  # True
RunDamysus = False  # True
RunHotsus = False  # True

# Configuration of running mode
RunLocal = False
RunInstance = False

# Parameters of SGX modes
SgxMode = "SIM"
SgxSource = "source /opt/intel/sgxsdk/environment"  # this is where the sdk is supposed to be installed

# Global variables
CompleteRuns = 0  # Number of runs that successfully completed
AbortedRuns = 0  # Number of runs that got aborted
AbortedRunsList = []  # List of aborted runs
AllLocalPorts = []  # List of all port numbers used in local experiments
TimestampStr = datetime.now().strftime("%d-%b-%Y-%H:%M:%S.%f")
IpAddresses = {}  # Dictionary mapping node ids to IPs

# Statistics directory
StatisticsFile = "results"  # Statistics directory
ParametersFile = "App/parameters.h"  # Parameters directory
ConfigFile = "config"  # Config directory
PointsFile = StatisticsFile + "/points-" + TimestampStr
AbortedFile = StatisticsFile + "/aborted-" + TimestampStr
MainFile = "Hotsus"

# Names of execute files
HotstuffServerFile = "HotstuffServer"
HotstuffClientFile = "HotstuffClient"
DamysusServerFile = "DamysusServer"
DamysusClientFile = "DamysusClient"
DamysusKeysFile = "DamysusKeys"
HotsusServerFile = "HotsusServer"
HotsusClientFile = "HotsusClient"
HotsusKeysFile = "HotsusKeys"
EnclaveSignedFile = "enclave.signed.so"

# Aliyun ECS
RegionId1 = "cn-hongkong"
ImageId1 = "ubuntu_20_04_x64_20G_alibase_20250113.vhd"
SecurityGroupId1 = "sg-j6c7wqjkv4l5ce4ej5sy"
VSwitchId1 = "vsw-j6cod840au18vxxxsile1"

RegionId2 = "ap-southeast-1"
ImageId2 = "ubuntu_20_04_x64_20G_alibase_20250113.vhd"
SecurityGroupId2 = "sg-t4nddoks5tm9afcg7ciw"
VSwitchId2 = "vsw-t4nkla71ohji3e3geq1s1"

RegionId3 = "ap-southeast-5"
ImageId3 = "ubuntu_20_04_x64_20G_alibase_20250113.vhd"
SecurityGroupId3 = "sg-k1a2907rjludhj3z87pf"
VSwitchId3 = "vsw-k1ar7q0fbn5vqmcv3byoz"

RegionId4 = "ap-northeast-2"
ImageId4 = "ubuntu_20_04_x64_20G_alibase_20250113.vhd"
SecurityGroupId4 = "sg-mj73m2mxtn44qg0kwvu7"
VSwitchId4 = "vsw-mj7til587ziwj25fiecxl"

RegionIds = [RegionId1, RegionId2, RegionId3, RegionId4]
RegionInformations = [(RegionId1, ImageId1, SecurityGroupId1, VSwitchId1),
                      (RegionId2, ImageId2, SecurityGroupId2, VSwitchId2),
                      (RegionId3, ImageId3, SecurityGroupId3, VSwitchId3),
                      (RegionId4, ImageId4, SecurityGroupId4, VSwitchId4)]
RegionDictionary = {"cn-hongkong": "Hong Kong",
                    "ap-southeast-1": "Singapore",
                    "ap-southeast-5": "Jakarta",
                    "ap-northeast-2": "Seoul"}


class Protocol(Enum):
    HOTSTUFF = "BASIC_HOTSTUFF"
    DAMYSUS = "BASIC_DAMYSUS"
    HOTSUS = "BASIC_HOTSUS"


def makeParameters(protocolName, numReplicas, numMaxSignatures, numTransactions, payloadSize):
    print("----Making parameters file")
    f = open(ParametersFile, 'w')
    f.write("#ifndef PARAMETERS_H\n")
    f.write("#define PARAMETERS_H\n")
    f.write("\n")
    f.write("#define " + protocolName.value + "\n")
    f.write("#define MAX_NUM_NODES " + str(numReplicas) + "\n")
    f.write("#define MAX_NUM_SIGNATURES " + str(numMaxSignatures) + "\n")
    f.write("#define MAX_NUM_TRANSACTIONS " + str(numTransactions) + "\n")
    f.write("#define MAX_NUM_GROUPMEMBERS " + str(GroupMembers) + "\n")
    f.write("#define PAYLOAD_SIZE " + str(payloadSize) + "\n")
    f.write("\n")
    f.write("#endif\n")
    f.close()


def clearStatisticsPath():
    # Remove all temporary files in statistics path
    file0 = glob.glob(StatisticsFile + "/values*")
    file1 = glob.glob(StatisticsFile + "/done*")
    file2 = glob.glob(StatisticsFile + "/clientvalues*")
    allFile = file0 + file1 + file2
    for file in allFile:
        os.remove(file)


def printNodeParameters():
    f = open(PointsFile, 'a')
    f.write("##Parameters")
    f.write(" NumViews = " + str(NumViews))
    f.write(" NumExperiments = " + str(NumExperiments))
    f.write(" PayloadSize = " + str(PayloadSize))
    f.write(" NetworkLatency = " + str(NetworkLatency))
    f.write(" DockerMemory = " + str(DockerMemory))
    f.write(" DockerCpu = " + str(DockerCpu))
    f.write("\n")
    f.close()


def startContainers(numGeneralReplicas, numTrustedReplicas, numClients):
    global IpAddresses

    print("----Starting", numGeneralReplicas, "general container(s),", numTrustedReplicas, "trusted container(s) and", numClients, "client(s)")
    generalReplicas = list(map(lambda x: (True, "g", str(x)), list(range(0, numGeneralReplicas))))
    trustedReplicas = list(map(lambda x: (True, "t", str(x)), list(range(numGeneralReplicas, numGeneralReplicas + numTrustedReplicas))))
    clients = list(map(lambda x: (False, "c", str(x)), list(range(numClients))))
    allNodes = generalReplicas + trustedReplicas + clients

    for (isReplica, type, i) in allNodes:
        dockerInstance = DockerBase + type + i

        # Stop and remove the container if it still exists
        stopCmd = [Docker + " stop " + dockerInstance]
        subprocess.run(stopCmd, shell=True)
        removeCmd = [Docker + " rm " + dockerInstance]
        subprocess.run(removeCmd, shell=True)

        # Make sure to cover all the ports
        option1 = "--expose=8000-9999"
        option2 = "--network=\"bridge\""
        option3 = "--cap-add=NET_ADMIN"
        option4 = "--name=\"" + dockerInstance + "\""
        if DockerMemory > 0:
            optionMemory = "--memory=" + str(DockerMemory) + "m"
        else:
            optionMemory = ""
        if DockerCpu > 0:
            optionCpu = "--cpus=\"" + str(DockerCpu) + "\""
        else:
            optionCpu = ""

        # Start the Docker instances
        optionsCmd = [Docker + " run -td " + option1 + " " + option2 + " " + option3 + " " + option4 + " " + optionMemory + " " + optionCpu + " " + DockerBase]
        subprocess.run(optionsCmd, shell=True, check=True)

        # Start Intel-SGX
        sgxCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "\""]
        subprocess.run(sgxCmd, shell=True, check=True)

        # Make result folder
        makeCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "mkdir " + StatisticsFile + "\""]
        subprocess.run(makeCmd, shell=True, check=True)

        # Set the network latency
        if NetworkLatency > 0:
            print("----Changing network latency to " + str(NetworkLatency) + "ms")
            latencyCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "tc qdisc add dev eth0 root netem delay " + str(NetworkLatency) + "ms " + str(NetworkVariation) +
                          "ms distribution normal" + "\""]
            subprocess.run(latencyCmd, shell=True, check=True)

        # Extract the IP address of the container
        ipCmd = [Docker + " inspect " + dockerInstance]
        runText = json.loads(subprocess.run(ipCmd, shell=True, capture_output=True, text=True).stdout)[0]
        ipAddress = runText["NetworkSettings"]["Networks"]["bridge"]["IPAddress"]
        if ipAddress:
            print("----Container's address: " + ipAddress)
            if isReplica:
                IpAddresses.update({int(i): ipAddress})
        else:
            print("----Container's address: UNKNOWN")


def makeContainersFiles(protocolName, numGeneralReplicas, numTrustedReplicas, numClients):
    print("----Making files using", str(NumCores), "core(s)")
    generalReplicas = list(map(lambda x: (True, "g", str(x)), list(range(0, numGeneralReplicas))))
    trustedReplicas = list(map(lambda x: (True, "t", str(x)), list(range(numGeneralReplicas, numGeneralReplicas + numTrustedReplicas))))
    clients = list(map(lambda x: (False, "c", str(x)), list(range(numClients))))

    for (isReplica, type, i) in generalReplicas:
        dockerInstance = DockerBase + type + i
        print("----Making general container", dockerInstance)

        # Copy files
        makeFilePath = dockerInstance + ":/app/"
        appPath = dockerInstance + ":/app/App/"
        enclavePath = dockerInstance + ":/app/Enclave/"

        makeFileCmd = [Docker + " cp Makefile " + makeFilePath]
        subprocess.run(makeFileCmd, shell=True, check=True)

        copyAppCmd = [Docker + " cp App/. " + appPath]
        subprocess.run(copyAppCmd, shell=True, check=True)

        copyEnclaveCmd = [Docker + " cp Enclave/. " + enclavePath]
        subprocess.run(copyEnclaveCmd, shell=True, check=True)

        # Make files
        makeCleanCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.HOTSTUFF:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"make -j " + str(NumCores) + " " + HotstuffServerFile + " " + HotstuffClientFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; make -j " + str(NumCores) + " SGX_MODE=" + SgxMode + " " + HotsusServerFile +
                          " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)

    for (isReplica, type, i) in trustedReplicas:
        dockerInstance = DockerBase + type + i
        print("----Making trusted container", dockerInstance)

        # Copy files
        makeFilePath = dockerInstance + ":/app/"
        appPath = dockerInstance + ":/app/App/"
        enclavePath = dockerInstance + ":/app/Enclave/"

        makeFileCmd = [Docker + " cp Makefile " + makeFilePath]
        subprocess.run(makeFileCmd, shell=True, check=True)

        copyAppCmd = [Docker + " cp App/. " + appPath]
        subprocess.run(copyAppCmd, shell=True, check=True)

        copyEnclaveCmd = [Docker + " cp Enclave/. " + enclavePath]
        subprocess.run(copyEnclaveCmd, shell=True, check=True)

        # Make files
        makeCleanCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.DAMYSUS:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; make -j " + str(NumCores) + " SGX_MODE=" + SgxMode + " " + DamysusServerFile +
                          " " + DamysusClientFile + " " + DamysusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; make -j " + str(NumCores) + " SGX_MODE=" + SgxMode + " " + HotsusServerFile +
                          " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)

    for (isReplica, type, i) in clients:
        dockerInstance = DockerBase + type + i
        print("----Making container", dockerInstance)

        # Copy files
        makeFilePath = dockerInstance + ":/app/"
        appPath = dockerInstance + ":/app/App/"
        enclavePath = dockerInstance + ":/app/Enclave/"

        makeFileCmd = [Docker + " cp Makefile " + makeFilePath]
        subprocess.run(makeFileCmd, shell=True, check=True)

        copyAppCmd = [Docker + " cp App/. " + appPath]
        subprocess.run(copyAppCmd, shell=True, check=True)

        copyEnclaveCmd = [Docker + " cp Enclave/. " + enclavePath]
        subprocess.run(copyEnclaveCmd, shell=True, check=True)

        # Make files
        makeCleanCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.HOTSTUFF:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"make -j " + str(NumCores) + " " + HotstuffServerFile + " " + HotstuffClientFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        elif protocolName == Protocol.DAMYSUS:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; make -j " + str(NumCores) + " SGX_MODE=" + SgxMode + " " + DamysusServerFile +
                          " " + DamysusClientFile + " " + DamysusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; make -j " + str(NumCores) + " SGX_MODE=" + SgxMode + " " + HotsusServerFile +
                          " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)


def generateLocalConfig(fileName, numReplicas):
    open(fileName, 'w').close()
    host = "127.0.0.1"

    global AllLocalPorts

    print("----IP Addresses:", IpAddresses)

    f = open(fileName, 'a')
    for i in range(numReplicas):
        host = IpAddresses.get(i, host)
        replicaPort = 8760 + i
        clientPort = 9760 + i
        AllLocalPorts.append(replicaPort)
        AllLocalPorts.append(clientPort)
        f.write("id:" + str(i) + " host:" + host + " port:" + str(replicaPort) + " port:" + str(clientPort) + "\n")
    f.close()


def executeContainersProtocol(protocolName, numGeneralReplicas, numTrustedReplicas, numAllReplicas, numClients, numViews, numFaults, numClientTransactions, cutOffBound, sleepTime,
                              experimentIndex):
    print("----Executing protocols")
    replicaProcessList = []
    clientProcessList = []

    generateLocalConfig(fileName=ConfigFile, numReplicas=numAllReplicas)

    generalReplicas = list(map(lambda x: (True, "g", str(x)), list(range(0, numGeneralReplicas))))
    trustedReplicas = list(map(lambda x: (True, "t", str(x)), list(range(numGeneralReplicas, numGeneralReplicas + numTrustedReplicas))))
    clients = list(map(lambda x: (False, "c", str(x)), list(range(numClients))))
    allNodes = generalReplicas + trustedReplicas + clients

    for (isReplica, type, i) in allNodes:
        dockerInstance = DockerBase + type + i
        dockerPath = dockerInstance + ":/app/"
        copyCmd = [Docker + " cp " + ConfigFile + " " + dockerPath]
        subprocess.run(copyCmd, shell=True, check=True)

    if protocolName == Protocol.HOTSTUFF:
        serverPath = str("./" + HotstuffServerFile)
    elif protocolName == Protocol.DAMYSUS:
        serverPath = str("./" + DamysusServerFile)
    else:
        serverPath = str("./" + HotsusServerFile)

    if protocolName == Protocol.HOTSTUFF:
        clientPath = str("./" + HotstuffClientFile)
    elif protocolName == Protocol.DAMYSUS:
        clientPath = str("./" + DamysusClientFile)
    else:
        clientPath = str("./" + HotsusClientFile)
    newLeaderChangeTime = int(math.ceil(LeaderChangeTime + math.log(numFaults, 2)))

    # Start general servers
    for generalReplicaId in range(0, numGeneralReplicas):
        print("----Starting No.", str(generalReplicaId + 1), "general replica")

        # Give some time for the nodes to connect gradually
        if generalReplicaId % 10 == 5:
            time.sleep(2)
        dockerInstance = DockerBase + "g" + str(generalReplicaId)

        if protocolName == Protocol.HOTSTUFF:
            serverCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + serverPath + " " + str(generalReplicaId) + " " + str(numGeneralReplicas) +
                         " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) + " " + str(newLeaderChangeTime) + "\""]
        else:
            serverCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; " + serverPath + " " + str(generalReplicaId) + " " + str(numGeneralReplicas) +
                         " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) + " " + str(newLeaderChangeTime) + "\""]
        serverProcess = Popen(serverCmd, shell=True)

        replicaProcessList.append(("General replica", generalReplicaId, serverProcess))
    print("----Started", str(numGeneralReplicas), "general replicas")

    # Start trusted servers
    for trustedReplicaId in range(numGeneralReplicas, numGeneralReplicas + numTrustedReplicas):
        print("----Starting No.", str(trustedReplicaId + 1), "trusted replica")

        # Give some time for the nodes to connect gradually
        if trustedReplicaId % 10 == 5:
            time.sleep(2)
        dockerInstance = DockerBase + "t" + str(trustedReplicaId)

        serverCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; " + serverPath + " " + str(trustedReplicaId) + " " + str(numGeneralReplicas) +
                     " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) + " " + str(newLeaderChangeTime) + "\""]
        serverProcess = Popen(serverCmd, shell=True)

        replicaProcessList.append(("Trusted replica", trustedReplicaId, serverProcess))
    print("----Started", str(numTrustedReplicas), "trusted replicas")

    # Start client after a few seconds
    print("----Starting No. 1 client")
    waitTime = 5 + int(math.ceil(math.log(numFaults, 2)))
    time.sleep(waitTime)
    for clientId in range(numClients):
        dockerInstance = DockerBase + "c" + str(clientId)
        if protocolName == Protocol.HOTSTUFF:
            clientCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + clientPath + " " + str(clientId) + " " + str(numGeneralReplicas) + " " + str(numTrustedReplicas) +
                         " " + str(numAllReplicas) + " " + str(numFaults) + " " + str(numClientTransactions) + " " + str(sleepTime) + " " + str(experimentIndex) + "\""]
        else:
            clientCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + SgxSource + "; " + clientPath + " " + str(clientId) + " " + str(numGeneralReplicas) +
                         " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numFaults) + " " + str(numClientTransactions) + " " + str(sleepTime) +
                         " " + str(experimentIndex) + "\""]
        clientProcess = Popen(clientCmd, shell=True)
        clientProcessList.append(("Client", clientId, clientProcess))
    print("----Started", len(clientProcessList), "clients")

    totalTime = 0

    # Find done file to remove replica
    replicaProcesses = replicaProcessList.copy()
    while len(replicaProcesses) > 0 and cutOffBound > totalTime:
        print("----Remaining processes:", replicaProcesses)
        allReplicaProcess = replicaProcesses.copy()
        for (types, index, process) in allReplicaProcess:
            if types == "General replica":
                dockerInstance = DockerBase + "g" + str(index)
                testCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "find /app/" + StatisticsFile + " -name done-" + str(index) + "* | wc -l" + "\""]
                runText = subprocess.run(testCmd, shell=True, capture_output=True, text=True).stdout
                runResult = int(runText)
                if runResult > 0:
                    replicaProcesses.remove((types, index, process))
            else:
                dockerInstance = DockerBase + "t" + str(index)
                testCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "find /app/" + StatisticsFile + " -name done-" + str(index) + "* | wc -l" + "\""]
                runText = subprocess.run(testCmd, shell=True, capture_output=True, text=True).stdout
                runResult = int(runText)
                if runResult > 0:
                    replicaProcesses.remove((types, index, process))
        time.sleep(1)
        totalTime += 1

    global CompleteRuns
    global AbortedRuns
    global AbortedRunsList

    if totalTime < cutOffBound:
        CompleteRuns += 1
        print("----All", len(replicaProcessList) + len(clientProcessList), "processes are done")
    else:
        AbortedRuns += 1
        conf = (protocolName, numFaults, experimentIndex)
        AbortedRunsList.append(conf)
        f = open(AbortedFile, 'a')
        f.write(str(conf) + "\n")
        f.close()
        print("----Reached cutoff bound")

    # kill python subprocesses
    allProcessList = replicaProcessList + clientProcessList
    for (types, index, process) in allProcessList:
        # Print the nodes that haven't finished yet
        if process.poll() is None:
            print("----Killing process which still running:", (types, index, process.poll()))
            process.kill()

    allPorts = " ".join(list(map(lambda port: str(port) + "/tcp", AllLocalPorts)))

    # kill processes
    for (isReplica, type, i) in allNodes:
        dockerInstance = DockerBase + type + i
        killCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "killall -q " + HotstuffServerFile + " " + HotstuffClientFile + " " + DamysusServerFile +
                   " " + DamysusClientFile + " " + HotsusServerFile + " " + HotsusClientFile + "; fuser -k " + allPorts + "\""]
        subprocess.run(killCmd, shell=True)
        copyCmd = [Docker + " cp " + dockerInstance + ":/app/" + StatisticsFile + "/." + " " + StatisticsFile + "/"]
        subprocess.run(copyCmd, shell=True, check=True)
        removeCmd = [Docker + " exec -t " + dockerInstance + " bash -c \"" + "rm /app/" + StatisticsFile + "/*" + "\""]
        subprocess.run(removeCmd, shell=True)


def printNodeComments(protocolName, numFaults, experimentIndex, numExperiments):
    f = open(PointsFile, 'a')
    f.write("##Comments")
    f.write(" Protocol = " + protocolName.value)
    f.write(" PayloadSize = " + str(PayloadSize))
    f.write(" NumFaults = " + str(numFaults))
    f.write(" ExperimentIndex = " + str(experimentIndex))
    f.write(" NumExperiments = " + str(numExperiments))
    f.write("\n")
    f.close()


def computeStatistics(protocolName, numFaults, experimentIndex, numExperiments):
    # Compute throughput and latency
    throughputValue = 0.0
    throughputNum = 0
    latencyValue = 0.0
    latencyNum = 0
    handleValue = 0.0
    handleNum = 0
    printNodeComments(protocolName=protocolName, numFaults=numFaults, experimentIndex=experimentIndex, numExperiments=numExperiments)

    allFile = glob.glob(StatisticsFile + "/*")
    for file in allFile:
        if file.startswith(StatisticsFile + "/values"):
            f = open(file, "r")
            text = f.read()
            [throughput, latency, handle] = text.split(" ")

            valueThroughput = float(throughput)
            throughputNum += 1
            throughputValue += valueThroughput

            valueLatency = float(latency)
            latencyNum += 1
            latencyValue += valueLatency

            valueHandle = float(handle)
            handleNum += 1
            handleValue += valueHandle

    if throughputNum > 0:
        throughputView = throughputValue / throughputNum
    else:
        throughputView = 0.0
    if latencyNum > 0:
        latencyView = latencyValue / latencyNum
    else:
        latencyView = 0.0
    if handleNum > 0:
        handle = handleValue / handleNum
    else:
        handle = 0.0

    print("Throughput:", throughputView, "out of", throughputNum)
    print("Latency:", latencyView, "out of", latencyNum)
    print("Handle:", handle, "out of", handleNum)

    return throughputView, latencyView, handle


def terminateContainers(numGeneralReplicas, numTrustedReplicas, numClients):
    print("----Terminating docker containers")
    generalReplicas = list(map(lambda x: (True, "g", str(x)), list(range(0, numGeneralReplicas))))
    trustedReplicas = list(map(lambda x: (True, "t", str(x)), list(range(numGeneralReplicas, numGeneralReplicas + numTrustedReplicas))))
    clients = list(map(lambda x: (False, "c", str(x)), list(range(numClients))))
    allNodes = generalReplicas + trustedReplicas + clients

    for (isReplica, type, i) in allNodes:
        dockerInstance = DockerBase + type + i

        # Stop and remove the container
        stopCmd = [Docker + " stop " + dockerInstance]
        subprocess.run(stopCmd, shell=True)

        removeCmd = [Docker + " rm " + dockerInstance]
        subprocess.run(removeCmd, shell=True)


def runLocalExperiment(protocolName, constFactor, numViews, numFaults, numClients, numClientTransactions, cutOffBound, numExperiments, sleepTime, hotsusFactor):
    # Initialize the parameters
    throughputViews = []
    latencyViews = []
    handles = []
    numExperimentsWork = 0

    if protocolName == Protocol.HOTSTUFF:
        numAllReplicas = (constFactor * numFaults) + 1
        numGeneralReplicas = numAllReplicas
        numTrustedReplicas = 0
        numMaxSignatures = numAllReplicas - numFaults
    elif protocolName == Protocol.DAMYSUS:
        numAllReplicas = (constFactor * numFaults) + 1
        numGeneralReplicas = 0
        numTrustedReplicas = numAllReplicas
        numMaxSignatures = numAllReplicas - numFaults
    else:
        numAllReplicas = (constFactor * numFaults) + 1
        numTrustedReplicas = int(hotsusFactor * numFaults)
        numGeneralReplicas = numAllReplicas - numTrustedReplicas
        numMaxSignatures = numAllReplicas - numFaults
        if numTrustedReplicas < 5:
            print("----The number of trusted replicas is too small")
            exit(0)

    # Start the containers
    startContainers(numGeneralReplicas=numGeneralReplicas, numTrustedReplicas=numTrustedReplicas, numClients=numClients)

    # Make parameters files
    makeParameters(protocolName=protocolName, numReplicas=numAllReplicas, numMaxSignatures=numMaxSignatures, numTransactions=NumTransactions, payloadSize=PayloadSize)

    # Make files with correct parameters
    makeContainersFiles(protocolName=protocolName, numGeneralReplicas=numGeneralReplicas, numTrustedReplicas=numTrustedReplicas, numClients=numClients)

    # Run protocols
    for i in range(numExperiments):
        print("----Aborted runs so far:", AbortedRunsList)
        clearStatisticsPath()
        executeContainersProtocol(protocolName=protocolName, numGeneralReplicas=numGeneralReplicas, numTrustedReplicas=numTrustedReplicas, numAllReplicas=numAllReplicas,
                                  numClients=numClients, numViews=numViews, numFaults=numFaults, numClientTransactions=numClientTransactions, cutOffBound=cutOffBound,
                                  sleepTime=sleepTime, experimentIndex=i)
        (throughputView, latencyView, handle) = computeStatistics(protocolName=protocolName, numFaults=numFaults, experimentIndex=i, numExperiments=numExperiments)

        if throughputView > 0 and latencyView > 0 and handle > 0:
            throughputViews.append(throughputView)
            latencyViews.append(latencyView)
            handles.append(handle)
            numExperimentsWork += 1

    # Stop the containers
    terminateContainers(numGeneralReplicas=numGeneralReplicas, numTrustedReplicas=numTrustedReplicas, numClients=numClients)

    if numExperimentsWork > 0:
        throughputView = sum(throughputViews) / numExperimentsWork
        latencyView = sum(latencyViews) / numExperimentsWork
        handle = sum(handles) / numExperimentsWork
    else:
        throughputView = 0.0
        latencyView = 0.0
        handle = 0.0
    
    print("All throughput:", throughputViews)
    print("All latency:", latencyViews)
    print("All handle:", handles)
    print("Average throughput:", throughputView)
    print("Average latency:", latencyView)
    print("Average handle:", handle)


def runLocalExperiments():
    # Create statistics directory
    Path(StatisticsFile).mkdir(parents=True, exist_ok=True)
    printNodeParameters()
    for numFaults in NumFaults:
        if RunHotstuff:
            runLocalExperiment(protocolName=Protocol.HOTSTUFF, constFactor=3, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                               numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                               hotsusFactor=HotsusFactor)
        if RunDamysus:
            runLocalExperiment(protocolName=Protocol.DAMYSUS, constFactor=2, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                               numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                               hotsusFactor=HotsusFactor)
        if RunHotsus:
            runLocalExperiment(protocolName=Protocol.HOTSUS, constFactor=3, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                               numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                               hotsusFactor=HotsusFactor)
    print("Number of complete runs:", CompleteRuns)
    print("Number of aborted runs:", AbortedRuns)
    print("Aborted runs:", AbortedRunsList)


def getInstancesIpAddress(region, instanceId):
    option1 = "--RegionId " + region
    option2 = "--InstanceIds " + "'[" + "\"" + instanceId + "\"" + "]'"

    instanceCmd = [Aliyun + " ecs DescribeInstances " + option1 + " " + option2]
    instanceText = json.loads(subprocess.run(instanceCmd, shell=True, capture_output=True, text=True).stdout)

    instance = instanceText["Instances"]["Instance"][0]
    status = instance["Status"]
    while status != "Running":
        instanceCmd = [Aliyun + " ecs DescribeInstances " + option1 + " " + option2]
        instanceText = json.loads(subprocess.run(instanceCmd, shell=True, capture_output=True, text=True).stdout)
        instance = instanceText["Instances"]["Instance"][0]
        status = instance["Status"]

    privateIpAddress = instance["VpcAttributes"]["PrivateIpAddress"]["IpAddress"][0]
    publicIpAddress = instance["PublicIpAddress"]["IpAddress"][0]
    return privateIpAddress, publicIpAddress


def startInstances(numGeneralReplicas, numTrustedReplicas, numClients):
    print("----Starting", str(numGeneralReplicas), "general instance(s),", str(numTrustedReplicas), "trusted instance(s) and", str(numClients), "client instance(s)")
    numAllRegions = len(RegionIds)
    numAllInstances = numGeneralReplicas + numTrustedReplicas + numClients
    groupInstances, extraRegions = divmod(numAllInstances, numAllRegions)
    allInstances = []

    for i in range(numAllRegions):
        (regionId, imageId, securityGroupId, vswitchId) = RegionInformations[i]
        if i >= numAllRegions - extraRegions:
            amount = groupInstances + 1
        else:
            amount = groupInstances
        print("----Starting", str(amount), "instance(s) in", RegionDictionary[regionId])

        option1 = "--RegionId " + regionId
        option2 = "--ImageId " + imageId
        option3 = "--Amount " + str(amount)
        option4 = "--InstanceType " + InstanceType
        option5 = "--SecurityGroupId " + securityGroupId
        option6 = "--InternetChargeType " + InstanceInternetChargeType
        option7 = "--VSwitchId " + vswitchId
        option8 = "--SystemDisk.Category " + InstanceSystemDiskCategory
        option9 = "--InternetMaxBandwidthOut " + str(20)
        option10 = "--Password " + InstancePassword

        instanceCmd = [Aliyun + " ecs RunInstances " + option1 + " " + option2 + " " + option3 + " " + option4 + " " + option5 + " " + option6 + " " + option7 + " " + option8 +
                       " " + option9 + " " + option10]
        instanceText = json.loads(subprocess.run(instanceCmd, shell=True, capture_output=True, text=True).stdout)
        allInstances.append((regionId, instanceText))

    # Erase the content of the config file
    allFile = glob.glob(ConfigFile)
    for file in allFile:
        os.remove(file)

    # Get information of instances
    replicasInstances = []
    clientInstances = []

    numStartedInstances = 0  # total number of instances
    numStartedReplicas = 0  # number of replicas
    numStartedClients = 0  # number of clients
    for (regionId, instanceText) in allInstances:
        instanceSets = instanceText["InstanceIdSets"]["InstanceIdSet"]
        numRegionInstances = len(instanceSets)
        for i in range(numRegionInstances):
            instanceId = instanceSets[i]
            (privateIpAddress, publicIpAddress) = getInstancesIpAddress(regionId, instanceId)
            if numStartedInstances < numGeneralReplicas + numTrustedReplicas:
                replicasInstances.append((numStartedReplicas, instanceId, privateIpAddress, publicIpAddress, regionId))
                f = open(ConfigFile, 'a')
                replicaPort = 8760 + numStartedReplicas
                clientPort = 9760 + numStartedReplicas
                f.write("id:" + str(numStartedReplicas) + " host:" + str(publicIpAddress) + " port:" + str(replicaPort) + " port:" + str(clientPort) + "\n")
                f.close()
                numStartedReplicas += 1
            else:
                clientInstances.append((numStartedClients, instanceId, privateIpAddress, publicIpAddress, regionId))
                numStartedClients += 1
            numStartedInstances += 1

    generalReplicasInstances = replicasInstances[0: numGeneralReplicas]
    trustedReplicasInstances = replicasInstances[numGeneralReplicas: numGeneralReplicas + numTrustedReplicas]
    return generalReplicasInstances, trustedReplicasInstances, clientInstances


def copyInstanceFiles(sshAddress):
    # Make direction in the instance
    direction = "/root/" + MainFile
    passwordCmd = ["until sshpass -p " + InstancePassword + " ssh -o StrictHostKeyChecking=no " + sshAddress + " \"" + "mkdir -p " + direction + "\"" + "; do sleep 5; done"]
    subprocess.run(passwordCmd, shell=True, check=True)

    # Copy files from source to destination
    source = LocalPath + "*"
    destination = sshAddress + ":/root/" + MainFile
    copyCmd = ["until sshpass -p " + InstancePassword + " scp -r " + source + " " + destination + "; do sleep 1; done"]
    subprocess.run(copyCmd, shell=True, check=True)


def prepareInstanceFiles(sshAddress):
    devNull = open(os.devnull, 'w')

    # Prepare requirements
    preparing = "DEBIAN_FRONTEND=noninteractive; sudo apt-get update; sudo apt-get install -y apt-utils git wget iptables libdevmapper1.02.1; " \
                "sudo apt-get install -y build-essential ocaml ocamlbuild automake autoconf libtool python-is-python3 libssl-dev git cmake perl; " \
                "sudo apt-get install -y libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper reprepro unzip build-essential python; " \
                "sudo apt-get install -y lsb-release software-properties-common; sudo apt-get install -y pkg-config libuv1-dev python3-matplotlib; " \
                "sudo apt-get install -y emacs psmisc jq iproute2;"
    preparingCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + preparing + "\""]
    subprocess.run(preparingCmd, shell=True, check=True, stdout=devNull)

    # Install OpenSSL
    installing = "wget https://www.openssl.org/source/openssl-1.1.1i.tar.gz; tar -xvzf openssl-1.1.1i.tar.gz; cd openssl-1.1.1i; ./config --prefix=/usr no-mdc2 no-idea; " \
                 "make; make install;"
    installingCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + installing + "\""]
    subprocess.run(installingCmd, shell=True, check=True, stdout=devNull)


def installSgxsdkInstanceFiles(sshAddress):
    devNull = open(os.devnull, 'w')

    # Build Intel-SGX SDK
    building1 = "mkdir /opt/intel; cd /opt/intel; git clone https://github.com/intel/linux-sgx; cd linux-sgx; git checkout -b sgx2.13 73b8b57aea306d1633cc44a2efc40de6f4217364;"
    building1Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + building1 + "\""]
    subprocess.run(building1Cmd, shell=True, check=True, stdout=devNull)
    
    building2 = "cd /opt/intel/linux-sgx; make preparation;"
    building2Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + building2 + "\""]
    subprocess.run(building2Cmd, shell=True, check=True, stdout=devNull)

    building3 = "cd /opt/intel/linux-sgx/external/toolset/ubuntu20.04; cp as /usr/local/bin; cp ld /usr/local/bin; cp ld.gold /usr/local/bin; cp objdump /usr/local/bin;"
    building3Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + building3 + "\""]
    subprocess.run(building3Cmd, shell=True, check=True, stdout=devNull)

    building4 = "cd /opt/intel/linux-sgx; make sdk;"
    building4Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + building4 + "\""]
    subprocess.run(building4Cmd, shell=True, check=True, stdout=devNull)
    
    building5 = "cd /opt/intel/linux-sgx; make sdk_install_pkg;"
    building5Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + building5 + "\""]
    subprocess.run(building5Cmd, shell=True, check=True, stdout=devNull)

    # Install Intel-SGX SDK
    installing = "cd /opt/intel/linux-sgx/linux/installer/bin; echo -e 'no\n/opt/intel\n' | ./sgx_linux_x64_sdk_2.13.100.4.bin; . /opt/intel/sgxsdk/environment;"
    installingCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + installing + "\""]
    subprocess.run(installingCmd, shell=True, check=True, stdout=devNull)


def installSgxpswInstanceFiles(sshAddress):
    devNull = open(os.devnull, 'w')

    # Prepare Intel-SGX PSW
    preparing1 = "chmod +x /opt/intel/sgxsdk/environment; mkdir /etc/init; . /opt/intel/sgxsdk/environment; cd /opt/intel/linux-sgx; make psw; . /opt/intel/sgxsdk/environment; " \
                 "cd /opt/intel/linux-sgx; make deb_psw_pkg; . /opt/intel/sgxsdk/environment; cd /opt/intel/linux-sgx; make deb_local_repo;"
    preparing1Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + preparing1 + "\""]
    subprocess.run(preparing1Cmd, shell=True, check=True, stdout=devNull)

    preparing2 = "echo 'deb [trusted=yes arch=amd64] file:/opt/intel/linux-sgx/linux/installer/deb/sgx_debian_local_repo focal main' >> /etc/apt/sources.list; " \
                 "echo '# deb-src [trusted=yes arch=amd64] file:/opt/intel/linux-sgx/linux/installer/deb/sgx_debian_local_repo focal main' >> /etc/apt/sources.list; " \
                 "sudo apt-get update;"
    preparing2Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + preparing2 + "\""]
    subprocess.run(preparing2Cmd, shell=True, check=True, stdout=devNull)

    # Install Intel-SGX PSW
    installing1 = "sudo apt-get install -y libsgx-launch libsgx-urts libsgx-epid libsgx-quote-ex libsgx-dcap-ql;"
    installing1Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + installing1 + "\""]
    subprocess.run(installing1Cmd, shell=True, check=True, stdout=devNull)

    installing2 = "cd /opt/intel/linux-sgx/external/dcap_source/QuoteVerification/sgxssl/Linux; ./build_openssl.sh; make all; make install;"
    installing2Cmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + installing2 + "\""]
    subprocess.run(installing2Cmd, shell=True, check=True, stdout=devNull)


def installSalticidaeInstanceFiles(sshAddress):
    devNull = open(os.devnull, 'w')

    # Install Salticidae
    installing = "cd /root/Hotsus; git clone https://github.com/Determinant/salticidae.git; cd salticidae; cmake . -DCMAKE_INSTALL_PREFIX=.; make; make install;"
    installingCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"" + installing + "\""]
    subprocess.run(installingCmd, shell=True, check=True, stdout=devNull)


def initializeInstancesFiles(allInstances):
    copyProcesses = []
    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        print("----Copying files to the instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress
        process = multiprocessing.Process(target=copyInstanceFiles, args=(sshAddress,))
        process.start()
        copyProcesses.append(process)
    for process in copyProcesses:
        process.join()
    print("----Copied all files to all instances")

    prepareProcesses = []
    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        print("----Preparing files in the instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress
        process = multiprocessing.Process(target=prepareInstanceFiles, args=(sshAddress,))
        process.start()
        prepareProcesses.append(process)
    for process in prepareProcesses:
        process.join()
    print("----Prepared all files in all instances")

    installSgxsdkProcesses = []
    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        print("----Installing Intel-SGX SDK in the instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress
        process = multiprocessing.Process(target=installSgxsdkInstanceFiles, args=(sshAddress,))
        process.start()
        installSgxsdkProcesses.append(process)
    for process in installSgxsdkProcesses:
        process.join()
    print("----Installed Intel-SGX SDK in all instances")

    installSgxpswProcesses = []
    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        print("----Installing Intel-SGX PSW in the instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress
        process = multiprocessing.Process(target=installSgxpswInstanceFiles, args=(sshAddress,))
        process.start()
        installSgxpswProcesses.append(process)
    for process in installSgxpswProcesses:
        process.join()
    print("----Installed Intel-SGX PSW in all instances")

    installSalticidaeProcesses = []
    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        print("----Installing Salticidae in the instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress
        process = multiprocessing.Process(target=installSalticidaeInstanceFiles, args=(sshAddress,))
        process.start()
        installSalticidaeProcesses.append(process)
    for process in installSalticidaeProcesses:
        process.join()
    print("----Installed Salticidae in all instances")


def makeInstancesFiles(protocolName, generalReplicasInstances, trustedReplicasInstances, clientInstances):
    print("----Making files using", str(NumCores), "core(s)")

    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in generalReplicasInstances:
        print("----Making general instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress

        makeCleanCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.HOTSTUFF:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; make -j " + str(InstanceCores) + " " + HotstuffServerFile +
                          " " + HotstuffClientFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; make -j " + str(InstanceCores) + " SGX_MODE=" + SgxMode +
                          " " + HotsusServerFile + " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)

    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in trustedReplicasInstances:
        print("----Making trusted instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress

        makeCleanCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.DAMYSUS:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; make -j " + str(InstanceCores) + " SGX_MODE=" + SgxMode +
                          " " + DamysusServerFile + " " + DamysusClientFile + " " + DamysusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; make -j " + str(InstanceCores) + " SGX_MODE=" + SgxMode +
                          " " + HotsusServerFile + " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)

    for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in clientInstances:
        print("----Making client instance:", publicIpAddress)
        sshAddress = "root@" + publicIpAddress

        makeCleanCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; make clean\""]
        subprocess.run(makeCleanCmd, shell=True, check=True)

        if protocolName == Protocol.HOTSTUFF:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; make -j " + str(InstanceCores) + " " + HotstuffServerFile +
                          " " + HotstuffClientFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        elif protocolName == Protocol.DAMYSUS:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; make -j " + str(InstanceCores) + " SGX_MODE=" + SgxMode +
                          " " + DamysusServerFile + " " + DamysusClientFile + " " + DamysusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)
        else:
            compileCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; make -j " + str(InstanceCores) + " SGX_MODE=" + SgxMode +
                          " " + HotsusServerFile + " " + HotsusClientFile + " " + HotsusKeysFile + " " + EnclaveSignedFile + "\""]
            subprocess.run(compileCmd, shell=True, check=True)


def executeInstancesProtocol(protocolName, generalReplicasInstances, trustedReplicasInstances, clientInstances, numAllReplicas, numViews, numFaults, numClientTransactions, 
                             cutOffBound, sleepTime, experimentIndex):
    numGeneralReplicas = len(generalReplicasInstances)
    numTrustedReplicas = len(trustedReplicasInstances)
    print("----Connecting to", str(numGeneralReplicas), "general replica instance(s)")
    print("----Connecting to", str(numTrustedReplicas), "trusted replica instance(s)")
    print("----Connecting to", str(len(clientInstances)), "client instance(s)")

    replicaProcessList = []
    clientProcessList = []

    if protocolName == Protocol.HOTSTUFF:
        serverPath = str("./" + HotstuffServerFile)
    elif protocolName == Protocol.DAMYSUS:
        serverPath = str("./" + DamysusServerFile)
    else:
        serverPath = str("./" + HotsusServerFile)

    if protocolName == Protocol.HOTSTUFF:
        clientPath = str("./" + HotstuffClientFile)
    elif protocolName == Protocol.DAMYSUS:
        clientPath = str("./" + DamysusClientFile)
    else:
        clientPath = str("./" + HotsusClientFile)
    newLeaderChangeTime = int(math.ceil(LeaderChangeTime + math.log(numFaults, 2)))

    # Start general servers
    for (generalReplicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in generalReplicasInstances:
        print("----Starting No.", str(generalReplicaId + 1), "general replica")
        sshAddress = "root@" + publicIpAddress

        # Give some time for the nodes to connect gradually
        if generalReplicaId % 10 == 5:
            time.sleep(2)

        if protocolName == Protocol.HOTSTUFF:
            serverCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + serverPath + " " + str(generalReplicaId) +
                         " " + str(numGeneralReplicas) + " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) +
                         " " + str(newLeaderChangeTime) + "\""]
        else:
            serverCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; " + serverPath + " " + str(generalReplicaId) +
                         " " + str(numGeneralReplicas) + " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) +
                         " " + str(newLeaderChangeTime) + "\""]
        serverProcess = Popen(serverCmd, shell=True)

        replicaProcessList.append(("General replica", generalReplicaId, instanceId, privateIpAddress, publicIpAddress, regionId, serverProcess))
    print("----Started", len(generalReplicasInstances), "general replicas")

    # Start trusted servers
    for (trustedReplicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in trustedReplicasInstances:
        print("----Starting No.", str(trustedReplicaId + 1), "trusted replica")
        sshAddress = "root@" + publicIpAddress

        # Give some time for the nodes to connect gradually
        if trustedReplicaId % 10 == 5:
            time.sleep(2)

        serverCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; " + serverPath + " " + str(trustedReplicaId) + " " +
                     str(numGeneralReplicas) + " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numViews) + " " + str(numFaults) + " " +
                     str(newLeaderChangeTime) + "\""]
        serverProcess = Popen(serverCmd, shell=True)

        replicaProcessList.append(("Trusted replica", trustedReplicaId, instanceId, privateIpAddress, publicIpAddress, regionId, serverProcess))
    print("----Started", len(trustedReplicasInstances), "trusted replicas")

    # Start client after a few seconds
    print("----Starting No. 1 client")
    waitTime = 5 + int(math.ceil(math.log(numFaults, 2)))
    time.sleep(waitTime)
    for (clientId, instanceId, privateIpAddress, publicIpAddress, regionId) in clientInstances:
        sshAddress = "root@" + publicIpAddress

        if protocolName == Protocol.HOTSTUFF:
            clientCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + clientPath + " " + str(clientId) + " " + str(numGeneralReplicas) +
                         " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numFaults) + " " + str(numClientTransactions) + " " + str(sleepTime) +
                         " " + str(experimentIndex) + "\""]
        else:
            clientCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + SgxSource + "; " + clientPath + " " + str(clientId) +
                         " " + str(numGeneralReplicas) + " " + str(numTrustedReplicas) + " " + str(numAllReplicas) + " " + str(numFaults) + " " + str(numClientTransactions) +
                         " " + str(sleepTime) + " " + str(experimentIndex) + "\""]
        clientProcess = Popen(clientCmd, shell=True)
        clientProcessList.append(("Client", clientId, instanceId, privateIpAddress, publicIpAddress, regionId, clientProcess))
    print("----Started", len(clientProcessList), "clients")

    totalTime = 0

    # Find done file to remove replica
    replicaProcesses = replicaProcessList.copy()
    while len(replicaProcesses) > 0 and cutOffBound > totalTime:
        print("----Remaining processes:", replicaProcesses)
        allReplicaProcess = replicaProcesses.copy()
        for (types, index, instanceId, privateIpAddress, publicIpAddress, regionId, process) in allReplicaProcess:
            sshAddress = "root@" + publicIpAddress
            if types == "General replica":
                testCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + "find " + StatisticsFile + " -name done-" + str(index) + "* | wc -l" + "\""]
                runText = subprocess.run(testCmd, shell=True, capture_output=True, text=True).stdout
                runResult = int(runText)
                if runResult > 0:
                    replicaProcesses.remove((types, index, instanceId, privateIpAddress, publicIpAddress, regionId, process))
            else:
                testCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + "find " + StatisticsFile + " -name done-" + str(index) + "* | wc -l" + "\""]
                runText = subprocess.run(testCmd, shell=True, capture_output=True, text=True).stdout
                runResult = int(runText)
                if runResult > 0:
                    replicaProcesses.remove((types, index, instanceId, privateIpAddress, publicIpAddress, regionId, process))
        time.sleep(1)
        totalTime += 1

    global CompleteRuns
    global AbortedRuns
    global AbortedRunsList

    if totalTime < cutOffBound:
        CompleteRuns += 1
        print("----All", len(replicaProcessList) + len(clientProcessList), "processes are done")
    else:
        AbortedRuns += 1
        conf = (protocolName, numFaults, experimentIndex)
        AbortedRunsList.append(conf)
        f = open(AbortedFile, 'a')
        f.write(str(conf) + "\n")
        f.close()
        print("----Reached cutoff bound")

    # kill python subprocesses
    allProcessList = replicaProcessList + clientProcessList
    for (types, index, instanceId, privateIpAddress, publicIpAddress, regionId, process) in allProcessList:
        # Print the nodes that haven't finished yet
        if process.poll() is None:
            print("----Killing process which still running:", (types, index, process.poll()))
            process.kill()
    
	# kill processes
    allInstances = generalReplicasInstances + trustedReplicasInstances + clientInstances
    for (clientId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
        sshAddress = "root@" + publicIpAddress
        killCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + MainFile + "; " + "killall -q " + HotstuffServerFile + " " + HotstuffClientFile + " " + DamysusServerFile +
                   " " + DamysusClientFile + " " + HotsusServerFile + " " + HotsusClientFile + "\""]
        subprocess.run(killCmd, shell=True)


def terminateInstances():
    for (regionId, imageId, securityGroupId1, vswitchId) in RegionInformations:
        option1 = "--RegionId " + regionId
        option2 = "--ImageId " + imageId
        option3 = "--InstanceType " + InstanceType
        option4 = "--PageSize " + str(100)

        descriptionCmd = [Aliyun + " ecs DescribeInstances " + option1 + " " + option2 + " " + option3 + " " + option4]
        runText = json.loads(subprocess.run(descriptionCmd, shell=True, capture_output=True, text=True).stdout)
        allInstances = runText["Instances"]["Instance"]
        numInstances = len(allInstances)
        print("----Terminating", str(numInstances), "instances in", RegionDictionary[regionId])
        for i in range(numInstances):
            instance = allInstances[i]
            instanceId = instance["InstanceId"]

            option1 = "--RegionId " + regionId
            option2 = "--InstanceId " + instanceId
            option3 = "--Force true"

            terminationCmd = [Aliyun + " ecs DeleteInstance " + option1 + " " + option2 + " " + option3]
            subprocess.run(terminationCmd, shell=True)


def runInstanceExperiment(protocolName, constFactor, numViews, numFaults, numClients, numClientTransactions, cutOffBound, numExperiments, sleepTime, hotsusFactor):
    # Initialize the parameters
    throughputViews = []
    latencyViews = []
    handles = []
    numExperimentsWork = 0

    if protocolName == Protocol.HOTSTUFF:
        numAllReplicas = (constFactor * numFaults) + 1
        numGeneralReplicas = numAllReplicas
        numTrustedReplicas = 0
        numMaxSignatures = numAllReplicas - numFaults
    elif protocolName == Protocol.DAMYSUS:
        numAllReplicas = (constFactor * numFaults) + 1
        numGeneralReplicas = 0
        numTrustedReplicas = numAllReplicas
        numMaxSignatures = numAllReplicas - numFaults
    else:
        numAllReplicas = (constFactor * numFaults) + 1
        numTrustedReplicas = int(hotsusFactor * numFaults)
        numGeneralReplicas = numAllReplicas - numTrustedReplicas
        numMaxSignatures = numAllReplicas - numFaults
        if numTrustedReplicas < 5:
            print("----The number of trusted replicas is too small")
            exit(0)

    # Start the instances
    (generalReplicasInstances, trustedReplicasInstances, clientInstances) = startInstances(numGeneralReplicas=numGeneralReplicas, numTrustedReplicas=numTrustedReplicas,
                                                                                           numClients=numClients)
    allInstances = generalReplicasInstances + trustedReplicasInstances + clientInstances

    # Make parameters files
    makeParameters(protocolName=protocolName, numReplicas=numAllReplicas, numMaxSignatures=numMaxSignatures, numTransactions=NumTransactions, payloadSize=PayloadSize)

    # Copy code to instances
    initializeInstancesFiles(allInstances)

    # Make files with correct parameters
    makeInstancesFiles(protocolName=protocolName, generalReplicasInstances=generalReplicasInstances, trustedReplicasInstances=trustedReplicasInstances,
                       clientInstances=clientInstances)

    for i in range(numExperiments):
        print("----Aborted runs so far:", AbortedRunsList)
        clearStatisticsPath()
        executeInstancesProtocol(protocolName=protocolName, generalReplicasInstances=generalReplicasInstances, trustedReplicasInstances=trustedReplicasInstances,
                                 clientInstances=clientInstances, numAllReplicas=numAllReplicas, numViews=numViews, numFaults=numFaults, numClientTransactions=numClientTransactions, 
                                 cutOffBound=cutOffBound, sleepTime=sleepTime, experimentIndex=i)

        # copy the statistics
        for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
            sshAddress = "root@" + publicIpAddress
            source = sshAddress + ":/root/" + MainFile + "/" + StatisticsFile + "/*"
            destination = LocalPath + StatisticsFile
            copyCmd = ["until sshpass -p " + InstancePassword + " scp -r " + source + " " + destination + "; do sleep 1; done"]
            subprocess.run(copyCmd, shell=True, check=True)
        
		# remove the statistics
        for (replicaId, instanceId, privateIpAddress, publicIpAddress, regionId) in allInstances:
            sshAddress = "root@" + publicIpAddress     
            source = MainFile + "/" + StatisticsFile
            removeCmd = ["sshpass -p " + InstancePassword + " ssh " + sshAddress + " \"cd " + source + "; " + "rm -f *\""]
            subprocess.run(removeCmd, shell=True, check=True)

        (throughputView, latencyView, handle) = computeStatistics(protocolName=protocolName, numFaults=numFaults, experimentIndex=i, numExperiments=numExperiments)
        
        print("This throughput:", throughputView)
        print("This latency:", latencyView)
        print("This handle:", handle)

        if throughputView > 0 and latencyView > 0 and handle > 0:
            throughputViews.append(throughputView)
            latencyViews.append(latencyView)
            handles.append(handle)
            numExperimentsWork += 1

    # Terminate all instances
    terminateInstances()

    if numExperimentsWork > 0:
        throughputView = sum(throughputViews) / numExperimentsWork
        latencyView = sum(latencyViews) / numExperimentsWork
        handle = sum(handles) / numExperimentsWork
    else:
        throughputView = 0.0
        latencyView = 0.0
        handle = 0.0

    print("All throughput:", throughputViews)
    print("All latency:", latencyViews)
    print("All handle:", handles)
    print("Average throughput:", throughputView)
    print("Average latency:", latencyView)
    print("Average handle:", handle)


def runInstanceExperiments():
    # Create statistics directory
    Path(StatisticsFile).mkdir(parents=True, exist_ok=True)
    printNodeParameters()

    # Terminate all instances
    terminateInstances()

    for numFaults in NumFaults:
        if RunHotstuff:
            runInstanceExperiment(protocolName=Protocol.HOTSTUFF, constFactor=3, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                                  numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                                  hotsusFactor=HotsusFactor)
        if RunDamysus:
            runInstanceExperiment(protocolName=Protocol.DAMYSUS, constFactor=2, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                                  numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                                  hotsusFactor=HotsusFactor)
        if RunHotsus:
            runInstanceExperiment(protocolName=Protocol.HOTSUS, constFactor=3, numViews=NumViews, numFaults=numFaults, numClients=NumClients,
                                  numClientTransactions=NumClientTransactions, cutOffBound=CutOffBound, numExperiments=NumExperiments, sleepTime=SleepTime,
                                  hotsusFactor=HotsusFactor)

    print("Number of complete runs:", CompleteRuns)
    print("Number of aborted runs:", AbortedRuns)
    print("Aborted runs:", AbortedRunsList)


parser = argparse.ArgumentParser(description='Evaluation')
# Configuration of protocol
parser.add_argument("--NumViews", type=int, default=0, help="Number of views to run per experiments")
parser.add_argument("--NumFaults", type=str, default="", help="Number of faults to test, separated by commas: 1,2,3,etc.")
parser.add_argument("--NumClientTransactions", type=int, default=0, help="Number of transactions sent by each client")
parser.add_argument("--NumExperiments", type=int, default=0, help="Number of experiments")
parser.add_argument("--NumTransactions", type=int, default=0, help="Number of transactions per experiment")
parser.add_argument("--PayloadSize", type=int, default=0, help="Size of payloads in Bytes")
parser.add_argument("--CutOffBound", type=int, default=0, help="Time after which the experiments are stopped")
parser.add_argument("--NetworkLatency", type=int, default=0, help="Network latency in ms")
parser.add_argument("--NetworkVariation", type=int, default=0, help="Variation of the network latency in ms")

# Configuration of docker
parser.add_argument("--DockerMemory", type=int, default=0, help="Memory used by docker containers")
parser.add_argument("--DockerCpu", type=float, default=0, help="Cpus used by docker containers")

# Configuration of experiments
parser.add_argument("--RunHotstuff", action="store_true", help="Run Hotstuff protocol")
parser.add_argument("--RunDamysus", action="store_true", help="Run Damysus protocol")
parser.add_argument("--RunHotsus", action="store_true", help="Run Hotsus protocol")
parser.add_argument("--HotsusFactor", type=float, default=0, help="Const factor of trusted replicas in Hotsus")

# Configuration of running mode
parser.add_argument("--RunLocal", action="store_true", help="Run experiments in local")
parser.add_argument("--RunInstance", action="store_true", help="Run experiments in aliyun")

args = parser.parse_args()

# Configuration of protocol
if args.NumViews > 0:
    NumViews = args.NumViews
    print("SUCCESSFULLY PARSED ARGUMENT - The number of views is:", NumViews)
if args.NumFaults:
    NumFaults = list(map(lambda x: int(x), args.NumFaults.split(",")))
    print("SUCCESSFULLY PARSED ARGUMENT - The number of fault replicas is:", NumFaults)
if args.NumClientTransactions > 0:
    NumClientTransactions = args.NumClientTransactions
    print("SUCCESSFULLY PARSED ARGUMENT - The number of transactions sent by each client is:", NumClientTransactions)
if args.NumExperiments > 0:
    NumExperiments = args.NumExperiments
    print("SUCCESSFULLY PARSED ARGUMENT - The number of experiments is:", NumExperiments)
if args.NumTransactions > 0:
    NumTransactions = args.NumTransactions
    print("SUCCESSFULLY PARSED ARGUMENT - The number of transactions is:", NumTransactions)
if args.PayloadSize >= 0:
    PayloadSize = args.PayloadSize
    print("SUCCESSFULLY PARSED ARGUMENT - The payload size is:", PayloadSize)
if args.CutOffBound > 0:
    CutOffBound = args.CutOffBound
    print("SUCCESSFULLY PARSED ARGUMENT - The cutoff bound is:", CutOffBound)
if args.NetworkLatency >= 0:
    NetworkLatency = args.NetworkLatency
    print("SUCCESSFULLY PARSED ARGUMENT - The network latency (in ms) is:", NetworkLatency)
if args.NetworkVariation >= 0:
    NetworkVariation = args.NetworkVariation
    print("SUCCESSFULLY PARSED ARGUMENT - The variation of the network latency (in ms) is:", NetworkVariation)

# Configuration of docker
if args.DockerMemory > 0:
    DockerMemory = args.DockerMemory
    print("SUCCESSFULLY PARSED ARGUMENT - The memory (in MB) used by docker containers is:", DockerMemory)
if args.DockerCpu > 0:
    DockerCpu = args.DockerCpu
    print("SUCCESSFULLY PARSED ARGUMENT - The cpus used by docker containers is:", DockerCpu)

# Configuration of experiments
if args.RunHotstuff:
    RunHotstuff = True
    print("SUCCESSFULLY PARSED ARGUMENT - Test Hotstuff protocol")
if args.RunDamysus:
    RunDamysus = True
    print("SUCCESSFULLY PARSED ARGUMENT - Test Damysus protocol")
if args.RunHotsus:
    RunHotsus = True
    print("SUCCESSFULLY PARSED ARGUMENT - Test Hotsus protocol")
if args.HotsusFactor > 0:
    HotsusFactor = args.HotsusFactor
    print("SUCCESSFULLY PARSED ARGUMENT - The const factor of trusted replicas in Hotsus is:", HotsusFactor)

# Configuration of running mode
if args.RunLocal:
    RunLocal = True
    runLocalExperiments()
if args.RunInstance:
    RunInstance = True
    runInstanceExperiments()
