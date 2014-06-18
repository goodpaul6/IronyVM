IronyVM
=======

A simple virtual machine I am creating as a learning project. It also features it's own assembler for simplified compilation.
The syntax of the assembly is quite simple, there are only 24 instructions at the moment:

hlt (register) 											-> stop the program, returning the value in the (register) as the result
mov (register) (unsigned int)							-> move an unsigned value into a register
add/sub/mul/div (register) (register 2) (register 3)	-> add/sub/mul/div registers 2 and 3 up and place the result in the first register


TO BE CONTINUED...
