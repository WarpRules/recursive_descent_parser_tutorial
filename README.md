# Recursive descent parser tutorial

This tutorial, in the form of a C program, explains in detail how to implement a simple recursive
descent parser to parse and evaluate a string containing an arithmetic expression that consists
of integers, the operators `+`, `-`, `*`, `/`, `^` (exponentiation), unary `-`, and parenthesized
expressions. For example:

  `"1 + 5 * (8-(3+5*(10+20))) - 2^5^2"`

(which results in `-33555156`.)

This tutorial demonstrates how to implement:
- Binary operators  (eg. `"2 + 3"`, `"5 * 7"`.)
- Unary operators  (eg. `"-5"`, `"-(10+20)"`.)
- Parenthesized expressions  (which can be nested without limit.)
- Operator precedence  (eg. `*` having a higher precedence than `+`,
     ie. `"1+2*3"` is equivalent to `"1+(2*3)"` instead of `"(1+2)*3"`.)
   Multiple operators can have the same precedence.
- Left-to-right and right-to-left precedence
     (eg. `"1-2+3"` is equivalent to `"(1-2)+3"` = 2, rather than `"1-(2+3)"` = -4.
      `"2^3^2"` is equivalent to `"2^(3^2)"` = 512, rather than `"(2^3)^2"` = 64.)
- Printing syntax error messages that show the position of the error in the input string.

Some advantages of a recursive descent parser:
- Relatively simple to implement (probably one of the if not the simplest parsing algorithms
   for this type of input format).
- Parses the input string in one single pass, traversing it from beginning to end
   without ever requiring any look-backs or jumping back and forth in the string.
- Requires no dynamic memory allocation (regardless of the order in which operators
   of different precedence appear, or how deeply nested parenthesized expressions are).

Some disadvantages:
- While it's a quite efficient way of parsing an input string (especially thanks to not requiring
   any dynamic memory allocations, and traversing the input string only once), it's not the most
   efficient parsing algorithm in existence. However, its simplicity of implementation often makes
   it the superior alternative, especially for simpler input formats.
- It's not necessarily suitable for parsing some more complicated input formats.
- Uses recursion, which may or may not be a problem depending on the target platform.

In this example implementation the parser also evaluates the result as it is parsing the input.
This is done just for the sake of simplicity. The parser can do other things, such as adding the
tokenized elements of the input into a parsing tree, or generating stack-based bytecode to
evaluate the expression later (eg. if it contains variables, with different variable values).
