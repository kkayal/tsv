auto grammar = R"(
table    <- _ head body EOF
head     <- row LF
body     <- row (LF row)* _

row      <- !( LF / EOF ) cell ( '\t' cell )*
cell     <- empty / number / phrase

empty    <- &'\t' / &LF / EOF

phrase <- < char+ >   # A sequence of chars. Allows space characters!
char <- !['\t''\n''\r'] . # Anything, except a tab or line feed or carriage return

# TODO: extend this to allow floating point numbers
number  <- < sign? uint ( '.' uint ( [eE] sign? uint)? )? > &('\t' / LF / EOF)
sign    <- '+' / '-'
uint    <- < [0-9]+ >

~LF <- '\r\n' / '\n' / '\r'

# If reading any character at all fails, we reached End Of File
~EOF <- !.

# Allow "some" whilespace before and after the table, except tabs,
# because the first cell of a table might be an empty cell
# Additionally, not that a space char is valid cell content.
# Therefore, a line starting with a space will be correctly
# recognised as the first cell of a new row with one space char in it
~_    <- [ '\r''\n']*

)";
