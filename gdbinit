set pagination off
target remote :3333

mon reset halt
flushregs

b app_main
commands
# mon esp sysview start file:///tmp/sysview_example.svdat
# For dual-core mode uncomment the line below and comment the line above
mon esp sysview start file:///tmp/sysview_example0.svdat file:///tmp/sysview_example1.svdat
c
end

c