# Supporting ECMAScript 3rd Edition

https://www.ecma-international.org/publications/standards/Ecma-262-arch.htm

* ES2: ?? Only editorial changes according to Wikipedia
* No ES4 (again according to Wikipedia)

Wikipedia: "Added regular expressions, better string handling, new control statements, try/catch exception handling, tighter definition of errors, formatting for numeric output and other enhancements "

- Keep supporting ES1?
    * Yes, need versioning starting with lexer
- Lexer changes
    - `\u00A0` (`<NBSP>`) and other unicode spaces (`<USP>`) are now considered white space, `\u2028` (`<LS>`) and `\u2029` (`<PS>`) are considered line terminators
    - `===` and `!==`
    - `\v` (`\u000B`) supported as escape sequence
    - New reserved words: `case`, `catch`, `default`, `do`, `finally`, `instanceof`, `switch`, `throw`, and `try`
    - And a bunch of new `FutureReservedWord`s
    - Identifiers can now contain various unicode characters
    - `RegularExpressionLiteral` (`RegExp` object created before evaluation start)
    - Real unicode support is a lot of work
- Parser changes:
    - Semi-colon insertion rules (between continue/break and identfier, between throw and expression - like return)
    - New `PrimaryExpresson`: `ArrayLiteral` (note: `[,,,]` is valid) and `ObjectLiteral`
    - New `MemberExpression`: `FunctionExpression`
    - New `RelationalExpression`: `instanceof` and `in`
    - New `EqualityExpression`: `===` and `!==`
    - `do...while`-loop
    - `break`/`continue` with `identifier`
    - New statements: `LabelledStatement`, `SwitchStatement`, `ThrowStatement` and `TryStatement`
- Other
    - `RegExp` Object
    - Error objects: `Error`, `EvalError`, `RangeError`, `RefererenceError`, `SyntaxError`, `TypeError` and `URIError`
        * `NativeError`
        * Converting normal (C++) exceptions to ECMAScript `Error` objects
    - `TypeError` when accessing missing interal properties (e.g. `[[Call]]`)
    - New interal properties: `[[HasInstance]]`, `[Scope]]` and `[[Match]]`
    - Completion type extended with a "target" (an identifier specifying where control should go)
    - Global object extensions
    - `prototype.name` (?)


Weird stuff/notes:
- `\v` added as escape sequence
- Unicode characters can suddenly appear more in the source text (and escape sequences can be used in identifiers)
- Array literal allows item elision (!) and supports trailing comma
- Object literals don't support trailing comma (until ES5)
- Regular expression literals makes the lexer context sensitive (TOOD: mention [oil shell](https://www.oilshell.org/blog/2017/12/15.html))
- Labelled statements / break/continue with identifier (how often have you seen that used in JS?)
    * A bit tricky to implement, required extending the `completion` type
- `do...while` and `switch` were not in ES1
- `switch`
    * case clauses are full expression that can have side effects!
    * `switch(0){default:}` is legal ECMAScript but not C(++)! (Need an empty statement `;` after before the closing brace)
    * Maybe some (all?) of this is also legal Java - need to check
- Exceptions
    * Need to extend completion type
    * Need to go through basically everything to check what (if any) exceptions needs to be throw
- `in` complication with `for..in` statement (another ugly hack in place...)
- `instanceof` requires support for `[[HasInstance]]`
- Implementation order..
- `delete SyntaxError; x` makes the Node.js REPL exit (at least in
  version 6.10.3)
- Overly complex additions to standard library (e.g. `Array.splice` has 54 steps, have mercy!)
