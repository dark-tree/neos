import requests

# Don't run this script manually!
# It will be automatically invoked by make after it is modified!

syscalls = requests.get('https://syscalls32.paolostivanin.com/syscalls-x86.js').json().get('aaData')
output = ""

# Add all syscalls implemented in C here
spec_impl = [
	"sys_write"
]

# Add all syscalls implemented in ASM at the interrupt level here
spec_nude = [
	"sys_exit"
]

output += "\n"
output += "#ifndef SYSCALL_IMPL\n"
output += "#	error systable.h Should only ever be included from syscall.c!\n"
output += "#endif\n"
output += "\n"
output += "#define SYS_LINUX_SIZE " + str(len(syscalls)) + "\n"
output += "#define SYS_NAKED NULL\n"
output += "\n"
output += "/*\n"
output += " * Warning!\n"
output += " * This file was automatically generated with a python script.\n"
output += " * Do not modify this file by hand! Learn more in ./util/sysgen.py\n"
output += " */\n"
output += "\n"
output += "SyscallEntry sys_linux_table[SYS_LINUX_SIZE] = {\n\n"

def unwrap(thing):
	if isinstance(thing, str):
		return thing

	return thing.get('type')

for syscall in syscalls:

	name = syscall[1]
	id = syscall[3]   # eax

	ebx = unwrap(syscall[4])
	ecx = unwrap(syscall[5])
	edx = unwrap(syscall[6])
	esi = unwrap(syscall[7])
	edi = unwrap(syscall[8])

	args = 0

	if ebx != "":
		args += 1
	if ecx != "":
		args += 1
	if edx != "":
		args += 1
	if esi != "":
		args += 1
	if edi != "":
		args += 1

#	print(name + " (" + id + ") " + str(args) + " args")
#	print(" ebx: " + ebx)
#	print(" ecx: " + ecx)
#	print(" edx: " + edx)
#	print(" esi: " + esi)
#	print(" edi: " + edi)
#	print()

	value = "NULL"

	if name in spec_impl:
		value = name

	if name in spec_nude:
		value = "SYS_NAKED"

	output += "\t/* " + name + " (" + id + ") */\n"
	output += "\t/* args: " + syscall[2] + " */\n"
	output += "\tSYSCALL_ENTRY(" + str(args) + ", " + value + "),\n\n"

output += "};\n"

print (output)
