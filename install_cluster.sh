#!/bin/bash

SSH_USER="${SSH_USER:-ubuntu}"

MASTER_SSH=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['cluster']['master_ssh'])")
TARGET_SSH=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['cluster']['target_ssh'])")

MASTER_IP=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['cluster']['master_ip'])")

readarray -t WORKER_SSH < <(python3 -c "import yaml; print('\n'.join(yaml.safe_load(open('config.yaml'))['cluster']['worker_ssh']))")

echo "    Installing K3s on Master ($MASTER_IP)..."
ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${MASTER_SSH} "curl -sfL https://get.k3s.io | sh -"
ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${MASTER_SSH} "/Defigium/install_docker.sh"
TOKEN=$(ssh ${SSH_USER}@${MASTER_SSH} "sudo cat /var/lib/rancher/k3s/server/node-token")

echo "    Joining Target (${TARGET_SSH})"
ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${TARGET_SSH} "curl -sfL https://get.k3s.io | K3S_URL=https://${MASTER_IP}:6443 K3S_TOKEN=${TOKEN} sh -"
ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${TARGET_SSH} "/Defigium/install_docker.sh"

for IP in "${WORKER_SSH[@]}"; do
	echo "    -> Joining Worker Node (${IP})..."
	ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${IP} "curl -sfL https://get.k3s.io | K3S_URL=https://${MASTER_IP}:6443 K3S_TOKEN=${TOKEN} sh -"
	ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${IP} "/Defigium/install_docker.sh"

echo "    Fetching Kuberconfig for local control..."
mkdir -p ~/.kube
ssh -o UserKnownHostsFile=/dev/null/ -o StrictHostKeyChecking=no ${SSH_USER}@${MASTER_SSH} "sudo cat /etc/rancher/k3s/k3s.yaml" > ~/.kube/config
sed -i "s/127.0.0.1/${MASTER_IP}/g" ~/.kube/config

echo "    Installation complete."
