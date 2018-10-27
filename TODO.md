* Lexer
    - Use `wstring_view` for token text (it's kept alive by the `source_file`)
    - Better handling of e.g. number literals..
    - Missing stuff?
* Parser
    - `for in` statements
    - Create `parse_test` (and move `test_semicolon_insertion` from interpreter test)
    - Test `source_extend` logic (could probably be more precise for expressions/statements)
* Better GC
    - Add more leak checks
    - Test current GC with live objects (!!!)
    - Make thread-safe (probably need to pass around a context variable)
    - Proper GC! (NaN-tagging / Something like [this](https://github.com/CppCon/CppCon2016/blob/master/Presentations/Lifetime%20Safety%20By%20Default%20-%20Making%20Code%20Leak-Free%20by%20Construction/Lifetime%20Safety%20By%20Default%20-%20Making%20Code%20Leak-Free%20by%20Construction%20-%20Herb%20Sutter%20-%20CppCon%202016.pdf))?
* Finish global object
    - Date
        - mutators
        - time zone adjustments
        - to/from string
    - Function
        - `toString`
        - etc.
    - Probably other missing stuff and non-compliant implementations...
* Optimize `NumberToString()`
* Create example(s)
    - Embedding mjs (I.e. adding user-defined classes)
* Make it easy to evaluate Javascript expressions (e.g. String('12') + 34)
* Optimize nested functions (they only need to be processed once (?))
* AST optimizations (constant propagation etc.)
* Optimize Array(), etc.
* Compile under Linux
* Use `char16_t` instead of `wchar_t` (and update `wstring`/`wistringstream` etc. etc.)
* Avoid duplicating `duration_cast`/`typeid()` logic with `expression_type`/`statement_type`. Consider using std::variant.
* General refactoring, implement ES3, ES5, ES...., JIT, etc. :)
