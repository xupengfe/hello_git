#!/bin/bash

FTRACE_SYSFS=/sys/kernel/debug/tracing

# Freeze ftrace
echo 0 > $FTRACE_SYSFS/tracing_on
# Clear buffer
echo > $FTRACE_SYSFS/trace

# Set filters
echo $BASHPID > $FTRACE_SYSFS/set_ftrace_pid
# Set tracer: function_graph
echo function_graph > $FTRACE_SYSFS/current_tracer

# Fire ftrace
echo 1 > $FTRACE_SYSFS/tracing_on

# Use shell builtin function here to issue some syscalls: open, write, etc
echo "begin echo hello" > $FTRACE_SYSFS/trace_marker
echo "hello" > test1.txt
#echo "hello"
#cat test1.txt
echo "end echo hello" > $FTRACE_SYSFS/trace_marker

# Stop ftrace and then collect trace
echo 0 > $FTRACE_SYSFS/tracing_on
echo "cat $FTRACE_SYSFS/trace > syscall.ftrace.log"
cat $FTRACE_SYSFS/trace > syscall.ftrace.log
echo "cat $FTRACE_SYSFS/trace_marker in syscall.ftrace.log"
