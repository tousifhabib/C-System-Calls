	.data

message:
	.ascii "Hello, world!\n"

	# The .text section contains instructions. The name is "text" for...
	# historical reasons.
	.text

	# Here's how we define a function in assembly! This function doesn't
	# take any parameters, so we just start executing instructions.
	.global	do_stuff

do_stuff:
	mov	$message, %eax
	int	$0x80

	ret