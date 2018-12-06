https://www.ecma-international.org/publications/standards/Ecma-262-arch.htm

- Keep supporting ES1?

- `RegExp` Object
- Error objects: `Error`, `EvalError`, `RangeError`, `RefererenceError`, `SyntaxError`, `TypeError` and `URIError`
- Lexer: `\u00A0` (`<NBSP>`) and other unicode spaces (`<USP>`) are now considered white space, `\u2028` (`<LS>`) and `\u2029` (`<PS>`) are considered line terminators
- Lexer `===` and `!==`
- New reserved words: `case`, `catch`, `default`, `do`, `finally`, `instanceof`, `switch`, `throw`, and `try`
- And a bunch of new `FutureReservedrWord`s
- Identifiers can now contain various unicode characters
- `\v` (`\u000B`) supported as escape sequence
- `RegularExpressionLiteral` (`RegExp` object created before evaluation start)
- Semi-colon insertion rules (between continue/break and identfier, between throw and expression - like return)
- `TypeError` when accessing missing interal properties (e.g. `[[Call]]`)
- New interal properties: `[[HasInstance]]`, `[Scope]]` and `[[Match]]`
- New `PrimaryExpresson`: `ArrayLiteral` (note: `[,,,]` is valid) and `ObjectLiteral`
- New `MemberExpression`: `FunctionExpression`
- New `RelationalExpression`: `instanceof` and `in`
- New `EqualityExpression`: `===` and `!==`
- `...NoIn` expressions (?)
- Completion type extended with a "target" (an identifier specifying where control should go)
- New statements: `LabelledStatement`, `SwitchStatement`, `ThrowStatement` and `TryStatement`
- `do...while`-loop
- `break`/`continue` with `identifier`
- Global object extensions


ES2:
 ?? Only editorial changes according to Wikipedia
No ES4 (again according to Wikipedia)

Wikipedai: "Added regular expressions, better string handling, new control statements, try/catch exception handling, tighter definition of errors, formatting for numeric output and other enhancements "
