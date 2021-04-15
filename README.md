# Cli4stm
Command Line Interpreter for Stm32 microcontrolers.
The project was ported from https://github.com/adamkaliszan/FreeRtosOnAvr project.


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
