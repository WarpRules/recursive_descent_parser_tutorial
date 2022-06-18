/*===============================================================================================
   Recursive descent parser tutorial
  ===============================================================================================
This tutorial, in the form of a C program, explains in detail how to implement a simple recursive
descent parser to parse and evaluate a string containing an arithmetic expression that consists
of integers, the operators +, -, *, /, ^ (exponentiation), unary -, and parenthesized
expressions. For example:

  "1 + 5 * (8-(3+5*(10+20))) - 2^5^2"

(which results in -33555156.)

This tutorial demonstrates how to implement:
 - Binary operators  (eg. "2 + 3", "5 * 7".)
 - Unary operators  (eg. "-5", "-(10+20)".)
 - Parenthesized expressions  (which can be nested without limit.)
 - Operator precedence  (eg. '*' having a higher precedence than '+',
     ie. "1+2*3" is equivalent to "1+(2*3)" instead of "(1+2)*3".)
   Multiple operators can have the same precedence.
 - Left-to-right and right-to-left precedence
     (eg. "1-2+3" is equivalent to "(1-2)+3" = 2, rather than "1-(2+3)" = -4.
      "2^3^2" is equivalent to "2^(3^2)" = 512, rather than "(2^3)^2" = 64.)
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

Note: There are some exercises for the reader at the end of this file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*-----------------------------------------------------------------------------------------------
  Some types and utility functions used in the parser
-----------------------------------------------------------------------------------------------*/
typedef long long ValueType;
enum ParseErrorCode { ParseError_None, ParseError_Syntax, ParseError_Div0, ParseError_NoClosingParenthesis };

struct ParseData
{
    const char *currentPosition;
    enum ParseErrorCode errorCode;
};

static const char* skipWhitespace(const char *str)
{
    while(isspace(*str)) ++str;
    return str;
}


/*-----------------------------------------------------------------------------------------------
  We have to create one function for each precedence level.
  Here the functions are declared in order from lowest to highest precedence.
-----------------------------------------------------------------------------------------------*/
static ValueType parseAddSubtract(struct ParseData*);
static ValueType parseMulDiv(struct ParseData*);
static ValueType parseExponent(struct ParseData*);
static ValueType parseUnaryMinus(struct ParseData*);
static ValueType parseParentheses(struct ParseData*);
static ValueType parseValue(struct ParseData*);


/*-----------------------------------------------------------------------------------------------
  Parse the binary operators '+' and '-' (lowest precedence in this syntax).
-----------------------------------------------------------------------------------------------*/
static ValueType parseAddSubtract(struct ParseData *data)
{
    /* First parse the value before the operator, by calling the next
       higher-precedence parsing function */
    ValueType result = parseMulDiv(data);
    if(data->errorCode) return result;

    /* Since there can be multiple operators in sequence (eg. "1+2+3-4-5") we do this in a loop */
    while(1)
    {
        data->currentPosition = skipWhitespace(data->currentPosition);

        /* If the next character after having parsed the previous value is not '+' nor '-' we
           have reached the end of this precedence level, so we just return. (The calling code
           will check if the next character is valid syntax.) */
        const char c = *data->currentPosition;
        if(c != '+' && c != '-') return result;

        /* Parse the value after the operator, by again calling the next
           higher-precedence parsing function */
        ++data->currentPosition; /* Remember to skip the operator character */
        const ValueType result2 = parseMulDiv(data);
        if(data->errorCode) return result;

        /* Perform the operation in question, and continue the loop */
        if(c == '+') result += result2;
        else result -= result2;
    }
}

/*-----------------------------------------------------------------------------------------------
  Parse the binary operators '*' and '/'.
-----------------------------------------------------------------------------------------------*/
static ValueType parseMulDiv(struct ParseData *data)
{
    /* Since these operators are similar to '+' and '-', this parsing function implementation
       is very similar to the previous one, with one main difference: We have to check for
       division by 0 (and return an error code if that would happen). */

    /* First parse the value before the operator, by calling the next
       higher-precedence parsing function */
    ValueType result = parseExponent(data);
    if(data->errorCode) return 0;

    while(1)
    {
        data->currentPosition = skipWhitespace(data->currentPosition);
        const char c = *data->currentPosition;
        if(c != '*' && c != '/') return result; /* We have reached the end of this precedence level */

        /* Parse the value after the operator */
        ++data->currentPosition; /* Remember to skip the operator character */
        const ValueType result2 = parseExponent(data);
        if(data->errorCode) return 0;

        /* Perform the operation, but in the case of division, check that we aren't dividing by 0. */
        if(c == '*') result *= result2;
        else if(result2 == 0) { data->errorCode = ParseError_Div0; return 0; }
        else result /= result2;
    }
}

/*-----------------------------------------------------------------------------------------------
  Parse the binary operator '^'.
-----------------------------------------------------------------------------------------------*/
static ValueType parseExponent(struct ParseData *data)
{
    /* This operator is different from the previous ones in that we want it to have
       right-to-left precedence instead of left-to-right. In other words, for example
       "2^3^2" should be parsed as if it were "2^(3^2)". This requires a slight trick.
    */

    /* First parse the value before the operator, by calling the next
       higher-precedence parsing function */
    ValueType result = parseUnaryMinus(data);
    if(data->errorCode) return result;

    /* If the next character isn't '^', we have reached the end of this precedence level */
    data->currentPosition = skipWhitespace(data->currentPosition);
    if(*data->currentPosition != '^') return result;

    /* We achieve right-to-left precedence by using this clever trick: To parse the value after
       the '^' character we call *this* function rather than the next-higher-precedence one.
       (Note that we don't need a while loop here because this recursion is the loop.) */
    ++data->currentPosition; /* Remember to skip the operator character */
    ValueType result2 = parseExponent(data);
    if(data->errorCode) return result;

    /* Perform the operation. */
    if(result2 == 0) return 1;
    if(result2 < 0 && result == 0) { data->errorCode = ParseError_Div0; return 0; }
    if(result2 < 0) return 0;

    const ValueType factor = result;
    while(--result2) result *= factor;
    return result;
}

/*-----------------------------------------------------------------------------------------------
  Parse the unary operator '-'.
-----------------------------------------------------------------------------------------------*/
static ValueType parseUnaryMinus(struct ParseData *data)
{
    /* A unary operator is much simpler to parse than a binary operator because we just
       need to check for the existence of the operator and call the next-higher-precedence
       parsing function once. The code should be quite self-explanatory. */
    data->currentPosition = skipWhitespace(data->currentPosition);
    const char c = *data->currentPosition;
    if(c == '-') ++data->currentPosition;
    ValueType result = parseParentheses(data);
    if(c == '-') result = -result;
    return result;

    /* Note: Parsing a postfix unary operator is very similar to the above, but in this
       case we would call the next-higher-precedence parsing function first, and only then
       do we check if the operator appears. */
}

/*-----------------------------------------------------------------------------------------------
  Parse parentheses
-----------------------------------------------------------------------------------------------*/
static ValueType parseParentheses(struct ParseData *data)
{
    /* Parsing parenthesized expressions is actually very similar to parsing a unary operator
       (because the opening parenthesis acts effectively as if it were a unary operator).
       The two major difference are:
        - If there is an opening parenthesis we call the lowest-precedence parsing function
          (else we call the next-higher-precedence one).
        - If there was an opening parenthesis we need to check that the next character after
          is the closing parenthesis (else return an error).
    */
    data->currentPosition = skipWhitespace(data->currentPosition);

    /* If the next character is not '(', then simply call the next-higher-precedence function */
    if(*data->currentPosition != '(')
        return parseValue(data);

    /* If there was an opening parenthesis, we call the *lowest* precedence parsing function */
    ++data->currentPosition; /* Remember to skip the '(' character */
    const ValueType result = parseAddSubtract(data);

    /* After the call we have to check that the next character is the closing parenthesis */
    data->currentPosition = skipWhitespace(data->currentPosition);
    if(*data->currentPosition != ')')
    {
        data->errorCode = ParseError_NoClosingParenthesis;
        return 0;
    }

    ++data->currentPosition; /* Remember to skip the ')' character */
    return result;
}

/*-----------------------------------------------------------------------------------------------
  Highest-precedence parsing function (parse a value, variable, or other singular syntactic unit)
-----------------------------------------------------------------------------------------------*/
static ValueType parseValue(struct ParseData *data)
{
    /* Here we can parse any singular syntactic unit, such as a variable, a function name
       (which would then require more parsing for the parenthesized parameters, but this
       is a topic for a more advanced tutorial), or a literal value.
       Here we just parse an integer literal value. */
    char *endPtr;
    data->currentPosition = skipWhitespace(data->currentPosition);
    const ValueType result = strtoll(data->currentPosition, &endPtr, 10);

    if(endPtr == data->currentPosition) /* There was no valid integer */
        data->errorCode = ParseError_Syntax;

    data->currentPosition = endPtr; /* Remember to jump to the end of the integer */
    return result;
}

/*-----------------------------------------------------------------------------------------------
  Main parsing function
-----------------------------------------------------------------------------------------------*/
ValueType parseInputString(struct ParseData *data)
{
    /* Call the lowest-precedence parsing function */
    const ValueType result = parseAddSubtract(data);
    if(data->errorCode) return 0;

    /* If we have not reached the end of the string, that means that there's an invalid
       syntax at the lowest ("outermost") precedence level. We have to check for that. */
    data->currentPosition = skipWhitespace(data->currentPosition);
    if(*data->currentPosition) /* There's an invalid non-null non-whitespace character */
        data->errorCode = ParseError_Syntax;

    return result;
}

/*-----------------------------------------------------------------------------------------------
  The parser code is done. Below we just call it with strings given in the command line,
  and print the result or error message.
-----------------------------------------------------------------------------------------------*/
static int printErrorMsg(const char *str, const struct ParseData *data)
{
    const char *const errorMessages[] =
    {
        "Syntax error", "Division by 0", "Expecting )"
    };

    printf("%s\n", str);
    for(const char *strPos = str; strPos != data->currentPosition; ++strPos)
        putchar(' ');
    printf("^\n%s\n", errorMessages[data->errorCode - 1]);
    return 1;
}

int main(int argc, char **argv)
{
    for(int argInd = 1; argInd < argc; ++argInd)
    {
        struct ParseData data = { argv[argInd], ParseError_None };
        const ValueType result = parseInputString(&data);
        if(data.errorCode) return printErrorMsg(argv[argInd], &data);
        printf("%lld\n", result);
    }
    return 0;
}



/*-----------------------------------------------------------------------------------------------
  Exercises for the reader
  -----------------------------------------------------------------------------------------------

1) Try changing ValueType to 'double' instead of 'long long' in order to support floating point
   values in the input string syntax. (Hint: You should use strtod() instead of strtoll() in the
   parseValue() function.)

2) After having successfully done that, implement support for some mathematical functions, such
   as the trigonometric functions and the square root function, so that the input string can be
   for example:

     "sin(1.5) + 2*cos(2.2)"

   (Hint: This is easier than it might sound at first: The name of the function should be
   parsed in the parseValue() function, and if a supported function name appears, check
   that the next (non-whitespace) character is an opening parenthesis, and then parse an
   expression in parentheses by calling that parsing function.)

   You could also implement support for named constants, such as "pi" (so that the input
   string can be for example ("sin(pi) + cos(2*pi) - 3*pi").

3) Implement support for variables. For example the variables 'x' and 'y'. Change the main()
   function so that the values of these variables can be specified in the command like, for
   example like:

     ./thisprogram 'x*x+y*y-10' 5 7

   (where the second parameter specifies the value of 'x' and the third the value of 'y'.
   Or any other command line syntax you prefer.)

4) Note that due to how the parser is implemented above, the following input strings are
   considered valid: "2--5", "2---5", "--5"
   but these will not: "2----5", "---5", "--(5)"

   4a) Why is this happening?
       (Hint: For example in the case of "2---5" this is because the first '-' is parsed as
       the binary operator '-', the second '-' will be parsed as a unary '-', and the third
       '-' is parsed by strtoll(). A fourth '-' will be too much and will be considered a
       syntax error because there's nothing accepting it as valid.)

   4b) This isn't something that's usually desirable in this kind of arithmetic formats.
       Try changing the implementation so that multiple consecutive minus signs are handled
       appropriately. (It's a matter of preference whether "1 - -2" should be considered valid
       or invalid syntax. However, "1 - - -2" should definitely be considered invalid.)
       (Note that this is not absolutely trivial and may make the code a bit more complicated.)

5) Note that due to the precedence order used in this example implementation, an input string
   like "-2^4" will be parsed as if it were "(-2)^4" (resulting in 16). Most often this kind of
   expression should be interpreted as if it were "-(2^4)" (resulting in -16) instead.
   Try modifying the code to make this happen.
   Note, however, that an input string like "-2^-(1+3)" should still be valid (and evaluate
   to 0 if using integers, or -0.0625 if using floating point). For this reason this cannot
   be achieved by merely switching the precedences of the ^ operator and the unary minus.
   (Simply switching the precedences would make "2^-(1+3)" issue a syntax error.)
*/
