# Supporting ECMAScript 5th Edition

Annex D, E and F contain lists of the necessary changes.

## TODO
- Annex C Strict Mode
- Annex E: 15: New `Object`, `Array` and `Date` functions
- Annex E: 15.3.4.3,4: Change in handling of `null`/`undefined` when passed to `apply/call` (Note: most e.g. Object.prototype functions have been changed to call `ToObject` on the this argument)
- Annex E: 15.9.4.2: `Date.parse` should attempt to parse as ISO format string
- Annex E: 15.12: `JSON` object
- Object.getPrototypeOf
- Object.getOwnPropertyDescriptor
- Object.getOwnPropertyNames
- Object.create
- Object.defineProperty
- Object.defineProperties
- Object.seal
- Object.freeze
- Object.preventExtensions
- Object.isSealed
- Object.isFrozen
- Object.isExtensible
- Object.keys
- Function.prototype.bind
- Array.prototype.indexOf
- Array.prototype.lastIndexOf
- Array.prototype.every
- Array.prototype.some
- Array.prototype.forEach
- Array.prototype.map
- Array.prototype.filter
- Array.prototype.reduce
- Array.prototype.reduceRight
- Date.now
- Date.prototype.toISOString
- Date.prototype.toJSON

## Done:
- 11.1.5: Object Initialiser: Allow trailing comma and reserved words as keys
- Annex E: 15.1.1 `NaN`, `Infinity` and `undefined` are now read-only properties of the global object
- 12.15: `debugger` statement
- Annex E: 15.1.2.2: `parseInt` must not treat a leading 0 as an octal constant
- Annex D 11.2.3: Order of evaluation of member vs. arguments in function call
- Annex D: 15.10.6: `RegExp.prototype` should be a `RegExp` object
- Annex D 11.8.2, 11.8.3, 11.8.5: Order of calls to `ToPrimitive` in comparisons
- Annex D 12.4: Exception object
- Annex D: 13: Scope rules for FunctionExpression
- Annex E: 15.3.5.2: Make the `prototype` property of function objects non-enumerable
- Annex E: 7.1: Handling of format control characters
- Annex E: 7.2: `<BOM>` is white space
- Annex E: 10.4.2: Changes to the environment of `eval`
- Annex E: 10.6: Changes to the arguments array
- Annex E: 15.10.4.1/15.10.6.4: `RegExp.source/toString` now has specified format
- Annex E: 15.11.2.1/15.11.4.3: `Error.message` now has specified format
- Annex E: 15.11.15.11.4.4: `Error.toString` now has specified format
- Annex E: 12.6.4: `for..in` on `null`/`undefined` now allowed
- Annex E: 15.3.4.3: Relax restrictions on the `arguments` argument to `Function.prototype.apply`
- Annex E: 15.5.5.2: Array like access to `String` objects
- Annex E: 15.4.4: `Array.prototype.to(Locale)String()` are now generic
- Annex E: 7.8.5: Changes to handling of RegExp literals
- String.prototype.trim

## Notes:
- 7.1 Form Control Characters changed from ES1 -> ES3 -> ES5
- 7.2 Handling of `<BOM>`
- `IdentifierName` allowed in various places (i.e. `ReservedWord`s can be used as property names)
- Annex D: Can't reproduce expected standard behavior for (12.4: Exception object and 15.10.6)
- 10.4.2 Direct calls to `eval` behave differently than indirect calls!
