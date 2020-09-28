#!/bin/sh

dir=/sys/kernel/debug/tracing
mmio_debug="/tmp/mmio_debug.txt"
result=""

sysctl kernel.ftrace_enabled=1
echo mmiotrace > ${dir}/current_tracer
echo 1 > ${dir}/tracing_on
sleep 5
echo 0 > ${dir}/tracing_on
#less ${dir}/trace
cat "$dir"/trace > "$mmio_debug"
result=$(cat "$mmio_debug" | grep -v "^#" | head -n 10)
if [[ -n "$result" ]]; then
  echo "Get mmio info:$result"
else
  echo "Could not get mmio info:$result and do you connect one tbt device"
  exit 1
fi
