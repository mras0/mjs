# Supporting ECMAScript 5th Edition

Annex D, E and F contain lists of the necessary changes.

## TODO
- 12.15: `debugger` statement
- Annex C Strict Mode
- Annex D 11.8.2, 11.8.3, 11.8.5: Check order in ES1/ES3 vs. ES5
- Annex D 11.2.3: Check order
- Annex D 12.4: Exception object
- Annex D: 13: Scope rules for FunctionExpression
- Annex D: 15.10.6: `RegExp.prototype` should be a `RegExp` object
- Annex E: 7.1: Handling of format control characters
- Annex E: 7.2: `<BOM>` is white space
- Annex E: 7.8.5: Changes to handling of RegExp literals
- Annex E: 10.4.2: Changes to the environment of `eval`
- Annex E: 15.4.4: `Array.prototype.to(Locale)String()` is now generic
- Annex E: 10.6: Changes to the arguments array
- Annex E: 12.6.4: `for..in` on `null`/`undefined` now allowed
- Annex E: 15: New `Object`, `Array` and `Date` functions
- Annex E: 15.1.2.2: `parseInt` must not treat a leading 0 as an octal constant
- Annex E: 15.3.4.3: Relax restrictions on the `arguments` argument to `Function.prototype.apply`
- Annex E: 15.3.4.3,4: Change in handling of `null`/`undefined` when passed to `apply/call`
- Annex E: 15.3.5.2: Make the `prototype` property of function objects non-enumerable
- Annex E: 15.5.5.2: Array like access to `String` objects
- Annex E: 15.9.4.2: `Date.parse` should attempt to parse as ISO format string
- Annex E: 15.10.4.1/15.10.6.4: `RegExp.source/toString` now has specified format
- Annex E: 15.11.2.1/15.11.4.3: `Error.message` now has specified format
- Annex E: 15.11.15.11.4.4: `Error.toString` now has specified format
- Annex E: 15.12: `JSON` object

## Done:
- 11.1.5: Object Initialiser: Allow trailing comma and reserved words as keys
- Annex E: 15.1.1 `NaN`, `Infinity` and `undefined` are now read-only properties of the global object

## Notes:
- 7.1 Form Control Characters changed from ES1 -> ES3 -> ES5
- 7.2 Handling of `<BOM>`
- `IdentifierName` allowed in various places (i.e. `ReservedWord`s can be used as property names)
