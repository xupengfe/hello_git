echo "runtests -l xupengfe:XXX"
runtests -l xupengfe:Qazwsx297
sleep 1
echo "runtests -s"
runtests -s
sleep 1
echo "runtests -k"
runtests -k
sleep 1
echo "runtests hpalm -r"
runtests hpalm -r
sleep 1
echo "runtests hpalm -a"
runtests hpalm -a
echo "Done, pleae check above upload results."
