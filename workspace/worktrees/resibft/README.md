# PT-BFT: A BFT Consensus Protocol Based on Partial-TEE

This is the code corresponding to the paper "PT-BFT: A BFT Consensus Protocol Based on Partial-TEE" which was accepted to XXX.

We offer two modes, the **Local Mode** and the **Server Mode** in Ubuntu 20.04. The **Local Mode** simulates real-world nodes through a container, while the **Server Mode** accomplishes the same through multiple servers.

We use the [Salticidae](https://github.com/Determinant/salticidae) library, which is prepared in the `Dockerfile`.

## Local Mode

### Preparing

First we prepare the packages:

`apt-get update`

`apt-get install -y git; apt-get install -y docker; apt-get install -y docker.io`

### Running

We need to prepare Docker containers:

`git clone https://github.com/ZhaoHaotian5/Hotsus; cd Hotsus`

`docker build -t hotsus .`

We provided various options in `experiment.py`. Several default running commands in the Local Mode of Hotsus is as following:

`python3 experiments.py --RunLocal --RunHotstuff --NumExperiments 1 --NumFaults 2`

`python3 experiments.py --RunLocal --RunDamysus --NumExperiments 10 --NumFaults 5 --NumViews 50`

`python3 experiments.py --RunLocal --RunHotsus --NumExperiments 50 --NumFaults 5 --HotsusFactor 1`

## Server Mode

### Preparing

For the same, we prepare the packages:

`apt-get update`

`apt-get install -y git; apt-get install -y sshpass`

### Making Aliyun

And we prepare the configuration of Aliyun:

`mkdir -p /root/aliyun; cd aliyun`

`curl -O https://aliyuncli.alicdn.com/aliyun-cli-linux-latest-amd64.tgz; tar -xzvf aliyun-cli-linux-latest-amd64.tgz; cp aliyun /usr/local/bin`

`aliyun configure`

### Running

We need to prepare Docker containers:

`git clone https://github.com/ZhaoHaotian5/Hotsus; cd Hotsus`

`docker build -t hotsus .`

Several default running commands in the Server Mode of Hotsus is as following:

`python3 experiments.py --RunInstance --RunHotstuff --NumExperiments 1 --NumFaults 2`

`python3 experiments.py --RunInstance --RunDamysus --NumExperiments 10 --NumFaults 5 --NumViews 50`

`python3 experiments.py --RunInstance --RunHotsus --NumExperiments 50 --NumFaults 10 --HotsusFactor 1`

## Options

A part of optional parameters in `experiment.py` are explained as following:

- `--NumViews` set the number of views to run per experiments
- `--NumFaults` set the number of faults to test
- `--NumExperiments` set the number of experiments
- `--PayloadSize` set the size of payloads in Bytes
- `--RunHotstuff` run Hotstuff protocol
- `--RunDamysus` run Damysus protocol
- `--RunHotsus` run Hotsus protocol
- `--HotsusFactor` set the const factor of trusted replicas in Hotsus
