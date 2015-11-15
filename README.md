#Instruction
cd ~/path/to/proj
make
./vsh

#Features
- Simple shell
- "&" in the last position of command to send it to background. Note: the child command will be zombie and output of the command will uncontrollaby display to interface. If "&" is put at position that is not in the end of command, a error message will be displayed.
- "history" command to list 10 most recent commands. If execute "history" command when this shell has not executed any command before, a error message will be displayed.
- "!!" to execute the most recent command. If this shell has not executed any command before, a error message will be displayed.
- "!<number>" to execute the <number>th most recent command. If the <number>th command is not found, a error message will be displayed.
- "exit" to exit
