# (Old Notes on) Supporting ECMAScript 5th Edition

## Done:
- 11.1.5: Object Initialiser: Allow trailing comma and reserved words as keys
- Annex E: 15.1.1 `NaN`, `Infinity` and `undefined` are now read-only properties of the global object
- 12.15: `debugger` statement
- Annex E: 15.1.2.2: `parseInt` must not treat a leading 0 as an octal constant
- Annex D 11.2.3: Order of evaluation of member vs. arguments in function call
- Annex D: 15.10.6: `RegExp.prototype` should be a `RegExp` object
- Annex D 11.8.2, 11.8.3, 11.8.5: Order of calls to `ToPrimitive` in comparisons
- Annex D 12.4: Exception object
- Annex D: 13: Scope rules for `FunctionExpression`
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
- Annex E: 7.8.5: Changes to handling of `RegExp` literals
- `String.prototype.trim`
- `Date.now`
- `Date.prototype.toISOString`
- Annex E: 15.9.4.2: `Date.parse` should attempt to parse as ISO format string
- `Date.prototype.toJSON`
- `Object.getPrototypeOf`
- `Object.getOwnPropertyNames`
- `Object.keys`
- `Object.getOwnPropertyDescriptor`
- `Object.defineProperty`
- `Object.defineProperties`
- `Object.preventExtensions`
- `Object.isExtensible`
- `Object.seal`
- `Object.isSealed`
- `Object.freeze`
- `Object.isFrozen`
- `Object.create`
- `Array.prototype.indexOf`
- `Array.prototype.lastIndexOf`
- `Array.prototype.every`
- `Array.prototype.some`
- `Array.prototype.forEach`
- `Array.prototype.map`
- `Array.prototype.filter`
- `Array.prototype.reduce`
- `Array.prototype.reduceRight`
- `Function.prototype.bind`
- `Array.isArray`
- `JSON.stringify`
- `JSON.parse`
- `get` / `set`
- Strict Mode: `FutureReservedWords`
- Strict Mode: `WithStatement` is a `SyntaxError`
- Strict Mode: No octal literals
- Strict Mode: No octal literal escape sequences
- Strict Mode: function parameters may not have the same name
- Strict Mode: Object literals may only contain one definition of data property
- Strict Mode: `eval` and `arguments` may not be part of object literals
- Strict Mode: `eval` and `arguments` may not be assigned to (or modified) in many contexts
- Strict Mode: `SyntaxError` when trying to delete a variable or function argument/name
- Strict Mode: `ReferenceError` when assigning to an undeclared identifier
- Strict Mode: `caller` and `callee` may not be accessed on functions
- Strict Mode: Arguments don't alias formal parameters
- Strict Mode: eval code gets its own environment
- Strict Mode: `this` is not coerced to an object
- Strict Mode: `arguments` is immutable
- Annex E: 15.3.4.3,4: Change in handling of `null`/`undefined` when passed to `apply/call`

## Notes:
- 7.1 Form Control Characters changed from ES1 -> ES3 -> ES5
- 7.2 Handling of `<BOM>`
- `IdentifierName` allowed in various places (i.e. `ReservedWord`s can be used as property names)
- Annex D: Can't reproduce expected standard behavior for (12.4: Exception object and 15.10.6)
- 10.4.2 Direct calls to `eval` behave differently than indirect calls!
- Change to handling of null/undefined argument to `call`/`apply` creating lots of changes
- Inversion of property attributes from ES3 -> ES5 (`dont_enum` is now not `enumerable`, etc.)
- Many changes listed as new functions on `Object` actually require major changes (e.g. `defineProperty`). Property attributes can now change at runtime.
- Ugly that array properties can also be getter/setters
- Suddenly many more places where garbage collection can happen (user-defined functions are now potentially called everywhere!)
- Reserved words switch back and forth and differing between strict and non-strict mode!
- Another parser hack to properly support the strict mode directive
  (stateful parser already necessary for semicolon insertion and regular expression literals)
