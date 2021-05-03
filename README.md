# Cli4stm
Command Line Interpreter for Stm32 microcontrolers.
The project was ported from https://github.com/adamkaliszan/FreeRtosOnAvr project.

Very height memory optimization (shared array for command buffer and history).

Functionality:
- commands completition (tab key)
- history (arrow up/down or history command)
- arguments (argc and argv)
- command groups (normal, enabled and configuration)


## Exemplary user command
```
static CliExRes_t sumFunction(CliState_t *state)
{
	int suma = 0;
	for (int i = 1; i < state->argc; i++)
	{
		suma += atoi(state->argv[i]);
	}
	fprintf(state->myStdInOut, "The sum of %d given numbers is equal to %d\r\n", state->argc-1, suma);


	//printStatus(state->myStdInOut);
    return OK_SILENT;
}
```

## Connectivity
Supported interfaces:
- serial port (via stlinkV3)
- udp (socat /dev/pts/1 UDP4-DATAGRAM:192.168.1.231:55151,broadcast)

In the future:
- Telnet (TCP)
- Usb

