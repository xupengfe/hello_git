#!/bin/bash

file="/home/p-cflh06/cet_0912.raw"
remote_path="simics@bach.sh.intel.com:/home/simics/"
remote="simics@bach.sh.intel.com"

echo "scp -o StrictHostKeyChecking=no $file $remote_path "
scp -o StrictHostKeyChecking=no $file $remote_path
sleep 2

echo "ssh $remote bash /home/simics/1.sh"
ssh $remote "bash /home/simics/1.sh"
