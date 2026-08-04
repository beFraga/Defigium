#!/bin/bash
set -e

MODE="${1:-full}"

JOB_TEMPLATE="k8s/jobs/${MODE}.yaml.template"

if [[ ! "$MODE" =~ ^(generator|executor|full)$ ]]; then
	echo "Error: Invalid mode '$MODE'. Usage: ./run.sh [generator|executor|full]"
	exit 1
fi

export TYPE_BENCH=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['target']['type'])")
export PARALLELISM=$(python3 -c "import yaml; print(len(yaml.safe_load(open('config.yaml'))['cluster']['worker_ips']))")

echo "=============================="
echo "Mode    : ${MODE}"
echo "Type    : ${TYPE_BENCH}"
echo "Workers : ${PARALLELISM} Nodes"
echo "=============================="

echo "    Creating K8s ConfigMap..."
kubectl create configmap app-config \
	--from-file=config.yaml=config.yaml \
	--dry-run=client -o yaml | kubectl apply -f -

echo "    Building Docker Image..."
docker build -t defigium-python:latest -f Dockerfile .
docker build -t cpp-executor:latest -f src/executors/Dockerfile src/executors/

docker save defigium-python:latest cpp-executor:latest -o /tmp/images.tar
sudo k3s image import /tmp/images.tar
rm /tmp/images.tar

TARGET_MANIFEST="k8s/targets/${TYPE_BENCH}.yaml"
if [ -f "$TARGET_MANIFEST" ]; then
	kubectl apply -f "$TARGET_MANIFEST"
fi

run_generator() {
	echo "    Submitting Generator Job (1 Node)..."
	kubectl delete job benchmark-generator-job 2>/dev/null || true
	envsubst < k8s/jobs/generator.yaml.template | kubectl apply -f -

	kubectl logs -f job/benchmark-generator-job -c trace-generator

	kubectl wait --for=condition=complete job/benchmark-generator-job --timeout=300s
	echo "    -> Trace Generation Finished Successfully."
}

run_executor() {
	echo "    Submitting Executor Job (${PARALLELISM} Nodes)..."
	kubectl delete job benchmark-executor-job 2>/dev/null || true
	envsubst < k8s/jobs/executor.yaml.template | kubectl apply -f -

	kubectl logs -f -l app=benchmark-runner -c cpp-executor --all-containers=true
}

if [ "$MODE" == "generator" ]; then
	run_generator
elif [ "$MODE" == "executor" ]; then
	run_executor
else
	run_generator
	run_executor
fi
