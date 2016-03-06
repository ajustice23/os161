set auto-load safe-path /
set can-use-hw-watchpoints 0
define asst2
dir ~/os161/kern/compile/ASST2
target remote unix:.sockets/gdb
b panic
end
